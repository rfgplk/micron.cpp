#!/usr/bin/env python3
"""Check generated pages, local links, assets, and source inventory."""
from __future__ import annotations

import json
import hashlib
import re
import subprocess
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

sys.dont_write_bytecode = True
from guide_pages import load_guides

SITE = Path(__file__).resolve().parents[1]
ROOT = SITE.parents[1]


class PageParser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.ids = set()
        self.duplicates = set()
        self.links = []

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        ident = attrs.get("id")
        if ident:
            if ident in self.ids:
                self.duplicates.add(ident)
            self.ids.add(ident)
        if tag == "a" and attrs.get("name"):
            self.ids.add(attrs["name"])
        for attr in ("href", "src", "action"):
            if attrs.get(attr):
                self.links.append((tag, attr, attrs[attr]))


def main():
    subprocess.run(["python3", str(SITE / "tools/build.py"), "--check"], cwd=ROOT, check=True)
    inventory = json.loads((SITE / "content/inventory.json").read_text())
    actual = {p.relative_to(ROOT / "src").as_posix() for p in (ROOT / "src").rglob("*.hpp") if p.is_file()}
    recorded = {item["path"] for item in inventory["headers"]}
    errors = []
    if actual != recorded:
        errors.append(f"header inventory differs: added={sorted(actual - recorded)} removed={sorted(recorded - actual)}")
    names = {item["name"] for item in inventory["symbols"]}
    functions = json.loads((SITE / "content/functions.json").read_text())
    names.update(item['name'] for item in functions['functions'])
    if functions['unmatched']:
        errors.append(f"unreconciled callable definitions: {functions['unmatched'][:10]}")
    source_files = {p.relative_to(ROOT / 'src').as_posix() for p in (ROOT / 'src').rglob('*')
                    if p.is_file() and (p.suffix in {'.hpp', '.c', '.mjs'} or not p.suffix)}
    if source_files != {item['path'] for item in functions['sources']}:
        errors.append('source evidence manifest differs from the source tree')
    source_bytes = {}
    for source in functions['sources']:
        raw = (ROOT / 'src' / source['path']).read_bytes()
        source_bytes[source['path']] = raw
        if hashlib.sha256(raw).hexdigest() != source['sha256']:
            errors.append(f"{source['path']}: function evidence is stale; refresh functions.json")
    identities = set()
    for item in functions['functions']:
        identity = (item['path'], item['line'], item['column'], item['name'], item.get('macro_line'))
        if identity in identities:
            errors.append(f'duplicate function identity: {identity}')
        identities.add(identity)
        raw = item['generated_source'].encode() if item.get('generated_source') else source_bytes[item['path']]
        if not 0 <= item['source_start'] < item['source_end'] <= len(raw):
            errors.append(f"{identity}: invalid source extent")
        if not item['declaration']:
            errors.append(f"{identity}: missing declaration")
        if item['kind'] == 'definition' and item['body_start'] is None:
            errors.append(f"{identity}: definition has no effects evidence")
        if item['body_start'] is not None and not item['source_start'] <= item['body_start'] < item['body_end'] <= item['source_end']:
            errors.append(f"{identity}: invalid effects extent")
    required = {"micron::vector", "micron::vector::push_back", "micron::array", "micron::robin_map::find",
                "micron::find", "micron::lz::filter", "micron::io::read", "micron::mutex::lock",
                "micron::auto_thread", "micron::regex", "micron::math::fft::rfft", "micron::basic_semaphore::flag",
                "micron::allocator_traits", "micron::allocator_traits::has_unsized_deallocate", "micron::exc", "micron::gfx::vk::host_allocation_callbacks",
                "micron::math::blas::level1::axpy", "micron::math::blas::level3::gemm",
                "micron::math::quaternions::normalize"}
    if required - names:
        errors.append(f"representative declarations missing: {sorted(required - names)}")
    guides = load_guides(SITE / "content")
    from importlib.util import module_from_spec, spec_from_file_location
    module_spec = spec_from_file_location("reference_modules", SITE / "content/modules.py")
    module_content = module_from_spec(module_spec)
    module_spec.loader.exec_module(module_content)
    missing_guides = {module[0] for module in module_content.MODULES} - guides.keys()
    if missing_guides:
        errors.append(f"module guides missing: {sorted(missing_guides)}")
    reviewed_sources = {path for guide in guides.values() for path in guide["sources"]}
    if source_files - reviewed_sources:
        errors.append(f"sources absent from guide review inventory: {sorted(source_files - reviewed_sources)}")
    guide_types = [item for guide in guides.values() for item in guide.get("types", [])]
    guide_names = {item["name"] for item in guide_types}
    explained_names = {name for item in guide_types for name in [item["name"], *item.get("covers", [])]}
    internal_headers = {item["path"] for item in inventory["headers"] if item["internal"]}
    class_scopes = {item["name"] for item in inventory["symbols"] if item["kind"] in {"class", "struct", "union"}}
    unexplained = set()
    for item in inventory["symbols"]:
        parts = item["name"].split("::")
        if item["kind"] not in {"class", "struct", "union", "enum", "typedef", "alias"}:
            continue
        if item["path"] in internal_headers or any(part.startswith("_") or part in {"impl", "detail", "details"} for part in parts):
            continue
        if any("::".join(parts[:i]) in class_scopes for i in range(1, len(parts))):
            continue
        if item["name"] not in explained_names:
            unexplained.add((item["path"], item["name"]))
    if unexplained:
        errors.append(f"public type names absent from explanatory articles: {sorted(unexplained)}")
    required_guides = {"micron::vector", "micron::segmented_vector", "micron::parray", "micron::robin_map",
                       "micron::immutable_map", "micron::rb_tree", "micron::lz::filter_view",
                       "usize", "micron::numeric_limits", "micron::weak_pointer"}
    if required_guides - guide_names:
        errors.append(f"representative type prose missing: {sorted(required_guides - guide_names)}")
    for item in guide_types:
        name = item["name"]
        leaf = name.rsplit("::", 1)[-1]
        if leaf.encode() not in source_bytes[item["header"]]:
            errors.append(f"{item['header']}: guide type {name} absent from its defining source")
    source_lines = {}
    for item in inventory["symbols"]:
        path = item["path"]
        if path not in source_lines:
            source_lines[path] = (ROOT / "src" / path).read_text(encoding="utf-8", errors="replace").splitlines()
        line = item["line"]
        leaf = item["name"].split("::")[-1]
        if not 1 <= line <= len(source_lines[path]) or re.sub(r"\s+", "", leaf) not in re.sub(r"\s+", "", source_lines[path][line - 1]):
            errors.append(f"{path}:{line}: indexed name {item['name']} is absent from the source line")
    modules_text = (SITE / "content/modules.py").read_text()
    if 'a null pointer when no element matches' not in modules_text:
        errors.append("the vector find miss result is not documented")

    html_paths = sorted(SITE.rglob("*.html"))
    pages = {}
    for path in html_paths:
        parser = PageParser()
        parser.feed(path.read_text(encoding="utf-8"))
        pages[path.resolve()] = parser
        if parser.duplicates:
            errors.append(f"{path.relative_to(SITE)}: duplicate ids {sorted(parser.duplicates)}")

    site_root = SITE.resolve()
    resolved_paths = {}
    file_status = {}

    def check_url(origin, url, external_allowed):
        parsed = urlsplit(url)
        if parsed.scheme or parsed.netloc:
            if not external_allowed or parsed.scheme not in ("https", "http", "mailto"):
                errors.append(f"{origin.relative_to(SITE)}: external resource {url}")
            return
        if not parsed.path:
            target = origin
        else:
            key = (origin.parent, parsed.path)
            if key not in resolved_paths:
                resolved_paths[key] = (origin.parent / unquote(parsed.path)).resolve()
            target = resolved_paths[key]
        if not target.is_relative_to(site_root):
            errors.append(f"{origin.relative_to(SITE)}: link escapes reference: {url}")
        elif target not in pages and not file_status.setdefault(target, target.is_file()):
            errors.append(f"{origin.relative_to(SITE)}: missing target {url}")
        elif parsed.fragment and target in pages and unquote(parsed.fragment) not in pages[target].ids:
            errors.append(f"{origin.relative_to(SITE)}: missing anchor {url}")

    for origin, parser in pages.items():
        for tag, attr, url in parser.links:
            check_url(origin, url, external_allowed=(tag == "a" and attr == "href"))
    for css in SITE.rglob("*.css"):
        text = css.read_text(encoding="utf-8")
        for url in re.findall(r"url\(\s*([^)]+?)\s*\)", text, re.I):
            url = url.strip(" \t\"'")
            if not url.startswith("data:"):
                check_url(css, url, external_allowed=False)
        if re.search(r"@import\s+(?:url\()?\s*['\"]?https?:", text, re.I):
            errors.append(f"{css.relative_to(SITE)}: external CSS import")
    index_js = (SITE / "assets/search-index.js").read_text(encoding="utf-8")
    entries = json.loads(index_js.removeprefix("window.MICRON_SEARCH_INDEX = ").removesuffix(";\n"))
    for name, target, _ in entries:
        check_url(SITE / "search.html", target, external_allowed=False)
        if name in required_guides and not target.startswith("modules/"):
            errors.append(f"{name}: type search does not lead to handwritten prose")
    type_search = {name: target for name, target, _ in entries}
    if not type_search.get("micron::thread", "").startswith("modules/threads.html#"):
        errors.append("native micron::thread search is hidden by a conditional backend type")
    subprocess.run(["node", "--check", str(SITE / "assets/search.js")], check=True)
    if errors:
        print("Validation errors:")
        for error in errors[:100]:
            print(" ", error)
        print(f"{len(errors)} errors")
        raise SystemExit(1)
    print(f"Validated {len(html_paths)} HTML pages, {len(entries)} search entries, {len(functions['functions'])} callable entries, source evidence, local assets, and {len(recorded)} headers")


if __name__ == "__main__":
    main()
