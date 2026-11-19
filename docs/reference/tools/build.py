#!/usr/bin/env python3
"""Render the checked-in offline reference from its content and inventory."""
from __future__ import annotations

import argparse
import hashlib
import html
import importlib.util
import json
import posixpath
import re
import sys
from collections import defaultdict
from pathlib import Path
sys.dont_write_bytecode = True
from function_pages import function_anchor, function_doc, source_page, source_body
from guide_pages import guide_html, load_guides, type_anchor, type_html, validate_guides

SITE = Path(__file__).resolve().parents[1]
CONTENT = SITE / "content"
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("reference_modules", CONTENT / "modules.py")
modules = importlib.util.module_from_spec(spec)
spec.loader.exec_module(modules)
spec = importlib.util.spec_from_file_location("reference_details", CONTENT / "details.py")
details_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(details_module)
DETAILS = details_module.DETAILS
MODULES = modules.MODULES
GROUPS = modules.GROUPS
BY_ID = {module[0]: module for module in MODULES}
INVENTORY = json.loads((CONTENT / "inventory.json").read_text(encoding="utf-8"))
FUNCTIONS = json.loads((CONTENT / "functions.json").read_text(encoding="utf-8"))
GUIDES = load_guides(CONTENT)
validate_guides(GUIDES, MODULES, {source["path"] for source in FUNCTIONS["sources"]})


def esc(value):
    return html.escape(str(value), quote=True)


def prose(value):
    return "".join(f"<code>{esc(part)}</code>" if index % 2 else esc(part)
                   for index, part in enumerate(value.split("`")))


def href(current, target):
    path, sep, anchor = target.partition("#")
    relative = posixpath.relpath(path, posixpath.dirname(current) or ".") if path else ""
    return esc(relative + (sep + anchor if sep else ""))


def link(current, target, label):
    return f'<a href="{href(current, target)}">{esc(label)}</a>'


def anchor(*values):
    return "s-" + hashlib.sha1("\0".join(str(x) for x in values).encode()).hexdigest()[:16]


def module_for(header):
    if header["path"].startswith("bits/wasm/"):
        return "wasm"
    if header["internal"]:
        return "internal"
    path = header["path"]
    matches = [(len(pattern), module[0]) for module in MODULES for pattern in module[3]
               if path == pattern or (pattern.endswith("/") and path.startswith(pattern))]
    return max(matches)[1] if matches else "foundation"


def header_page(path):
    return "headers/" + path + ".html"


def symbol_anchor(item):
    return anchor(item["name"], item["path"], item["line"], item["signature"])


def module_anchor(name):
    return anchor("feature", name)


def initial(name):
    name = name.split("::")[-1].lstrip("~_")
    return name[0].upper() if name and name[0].isascii() and name[0].isalpha() else "other"


def page(current, title, body, breadcrumb=None, search=False):
    root = href(current, "index.html")
    css1 = href(current, "assets/cppreference2/screen.css")
    css2 = href(current, "assets/cppreference2/site_modules.css")
    css3 = href(current, "assets/micron.css")
    action = href(current, "search.html")
    search_scripts = (f'<script defer src="{href(current, "assets/search-index.js")}"></script>'
                      f'<script defer src="{href(current, "assets/search.js")}"></script>') if search else ""
    trail = link(current, "index.html", "micron reference")
    if breadcrumb:
        trail += " › " + breadcrumb
    return f'''<!DOCTYPE html>
<html lang="en" dir="ltr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)} - micron reference</title>
<link rel="stylesheet" href="{css1}"><link rel="stylesheet" href="{css2}"><link rel="stylesheet" href="{css3}">
{search_scripts}</head><body class="mediawiki">
<div id="mw-head" class="noprint"><div id="cpp-head-first-base"><div id="cpp-head-first">
<h5><a href="{root}">micron reference</a></h5>
<div id="cpp-head-search"><form action="{action}" method="get" role="search"><label class="visually-hidden" for="searchInput">Search reference</label><input id="searchInput" type="search" name="q" placeholder="Search" required><button type="submit">Search</button></form></div>
</div></div><div id="cpp-head-second-base"><div id="cpp-head-second"><div id="cpp-head-tools-left"><div class="reference-tabs">{link(current, "index.html", "Reference")} {link(current, "headers/index.html", "Headers")} {link(current, "functions/index.html", "Functions")} {link(current, "symbols/index.html", "Symbols")} {link(current, "source/index.html", "Source")} {link(current, "about.html", "About")}</div></div></div></div></div>
<div id="cpp-content-base"><div id="content" role="main"><a id="top"></a><h1 id="firstHeading" class="firstHeading">{esc(title)}</h1><div id="bodyContent"><div id="siteSub">From micron reference</div><div id="contentSub">{trail}</div>{body}<div class="visualClear"></div></div></div></div>
<div id="cpp-footer-base"><div id="footer"><p>micron reference · {link(current, "about.html", "Attribution and licenses")} · {link(current, "index.html", "Contents")}</p></div></div>
</body></html>\n'''


def table(rows, classes="t-dsc-begin"):
    return f'<table class="{classes}"><tbody>' + "".join(rows) + "</tbody></table>"


def row(left, right):
    return f'<tr class="t-dsc"><td>{left}</td><td>{right}</td></tr>'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="report stale generated files without writing")
    args = parser.parse_args()
    result = {}
    headers = list(INVENTORY["headers"])
    header_paths = {header["path"] for header in headers}
    for source in FUNCTIONS["sources"]:
        if source["path"] not in header_paths:
            headers.append({"path": source["path"], "internal": True})
    headers.sort(key=lambda header: header["path"])
    # The syntax-based callable catalog replaces Ctags' function guesses,
    # including false declarations inside requires-expressions.
    symbols = [item for item in INVENTORY["symbols"] if item["kind"] not in {"function", "prototype"}]
    functions = FUNCTIONS["functions"]
    function_map = defaultdict(list)
    source_data = {item["path"]: (SITE.parents[1] / "src" / item["path"]).read_bytes() for item in FUNCTIONS["sources"]}
    for item in functions:
        function_map[item["path"]].append(item)
        if item["public"]:
            symbols.append({**item, "kind": "function", "function_anchor": function_anchor(item)})
    header_map = defaultdict(list)
    symbols_map = defaultdict(list)
    for item in headers:
        header_map[module_for(item)].append(item)
    for item in symbols:
        symbols_map[item["path"]].append(item)

    # A preferred destination for each symbol; detailed entries are added below.
    lookup = {}
    is_internal = {header["path"]: header["internal"] for header in headers}
    for item in sorted(symbols, key=lambda entry: (entry["kind"] == "function" and entry.get("body_start") is None,
                                                  is_internal[entry["path"]], entry["path"], entry["line"])):
        destination = item.get("function_anchor", symbol_anchor(item))
        lookup.setdefault(item["name"], (header_page(item["path"]) + "#" + destination, item["path"]))
    for item in functions:
        lookup.setdefault(item["name"], (header_page(item["path"]) + "#" + function_anchor(item), item["path"]))
    type_guides = defaultdict(list)
    type_references = {}
    type_review_sources = {}
    preferred_types = {}
    for ident, guide in GUIDES.items():
        reviewed_sources = frozenset(guide["sources"])
        for item in guide.get("types", []):
            type_guides[item["header"]].append((ident, item))
            target = "modules/" + ident + ".html#" + type_anchor(ident, item, anchor)
            for name in [item["name"], *item.get("covers", [])]:
                type_references[(item["header"], name)] = (target, item["summary"])
                type_review_sources[(item["header"], name)] = reviewed_sources
                preferred_path = lookup.get(name, (None, None))[1]
                rank = (item["header"] != preferred_path,
                        is_internal.get(item["header"], True),
                        item["header"], ident)
                candidate = (rank, target, item["header"])
                if name not in preferred_types or rank < preferred_types[name][0]:
                    preferred_types[name] = candidate
    for name, (_, target, path) in preferred_types.items():
        lookup[name] = (target, path)

    current = "index.html"
    sections = []
    for group, title in GROUPS:
        rows = []
        for module in MODULES:
            if module[2] == group:
                rows.append(row(link(current, "modules/" + module[0] + ".html", module[1]),
                                prose(GUIDES.get(module[0], {}).get("summary", module[4]))))
        sections.append(f'<h2><span class="mw-headline">{esc(title)}</span></h2>' + table(rows))
    intro = ('<p>The micron reference describes the library’s facilities, function overloads, and headers. '
             'Choose a section below, browse the function, header, and symbol indexes, or search by name.</p>'
             '<p>This reference is self-contained and can be opened from a local filesystem.</p>')
    result[current] = page(current, "micron reference", intro + "".join(sections))

    for module in MODULES:
        ident, title, group, _, intro, entries, example = module
        current = "modules/" + ident + ".html"
        guide = GUIDES.get(ident, {})
        body = f'<p>{prose(guide.get("summary", intro))}</p>'
        body += guide_html(current, ident, guide, anchor, esc, prose, link, header_page, row, table)
        if entries:
            rows = []
            detail = []
            for name, description in entries:
                description = guide.get("entry_overrides", {}).get(name, description)
                a = module_anchor(name)
                rows.append(row(f'<a href="#{a}"><code>{esc(name)}</code></a>', prose(description)))
                contract = DETAILS.get((ident, name), {})
                extra = ""
                if contract.get("header"):
                    header = contract["header"]
                    extra += '<p>Defined in header ' + link(current, header_page(header), '<micron/' + header + '>') + '.</p>'
                for declaration in contract.get("declarations", []):
                    extra += '<table class="t-dcl-begin"><tbody><tr class="t-dcl"><td><pre>' + esc(declaration) + '</pre></td></tr></tbody></table>'
                if contract.get("parameters"):
                    extra += '<h4>Parameters</h4>' + table([row('<code>' + esc(key) + '</code>', prose(value))
                                                            for key, value in contract["parameters"]], "t-par-begin")
                if contract.get("returns"):
                    extra += '<h4>Return value</h4><p>' + prose(contract["returns"]) + '</p>'
                if contract.get("notes"):
                    extra += '<h4>Notes</h4><p>' + prose(contract["notes"]) + '</p>'
                detail.append(f'<h3 id="{a}"><code>{esc(name)}</code></h3><p>{prose(description)}</p>{extra}')
            body += ('<h2>Selected interfaces</h2>' if guide else '<h2>Overview</h2>') + table(rows) + '<h2>Interface notes</h2>' + "".join(detail)
        if example:
            source = (SITE / "examples" / example).read_text(encoding="utf-8")
            body += '<h2>Example</h2><pre class="source-cpp">' + esc(source) + '</pre>'
            body += '<p>' + link(current, "examples/" + example, "Download this example") + '</p>'
        hs = header_map[ident]
        module_functions = [item for header in hs for item in function_map[header["path"]] if item["public"]]
        if module_functions:
            body += '<h2>Functions</h2><p>Each entry describes one overload. Select an entry for its declaration, effects, return expressions, and source evidence.</p>'
            body += table([row(link(current, header_page(item["path"]) + '#' + function_anchor(item), item["name"] + item["signature"]),
                               '<code>' + esc(item["path"]) + '</code>') for item in module_functions])
        if hs:
            public = hs
            private = []
            body += '<h2>Headers</h2><p>Select a header to browse its declaration index.</p><ul class="header-list">'
            body += "".join('<li>' + link(current, header_page(h["path"]), "<micron/" + h["path"] + ">") + '</li>' for h in public)
            body += "".join('<li><code>' + esc("<micron/" + h["path"] + ">") + '</code></li>' for h in private)
            body += '</ul>'
        group_title = dict(GROUPS)[group]
        result[current] = page(current, title, body, esc(group_title))

    current = "headers/index.html"
    body = ('<p>Headers are grouped by module. Some implementation headers contain declarations '
            'exposed by public headers; use the appropriate public include for those facilities.</p>')
    for module in MODULES:
        items = header_map[module[0]]
        if not items:
            continue
        body += f'<h2 id="{esc(module[0])}">{link(current, "modules/" + module[0] + ".html", module[1])}</h2><ul class="header-list">'
        for item in items:
            label = "<micron/" + item["path"] + ">"
            body += '<li>' + link(current, header_page(item["path"]), label) + '</li>'
        body += '</ul>'
    result[current] = page(current, "Header index", body)

    for header in headers:
        path = header["path"]
        current = header_page(path)
        owner = BY_ID[module_for(header)]
        items = symbols_map[path]
        body = f'<p>Defined in header <code>&lt;micron/{esc(path)}&gt;</code>.</p>'
        if header["internal"]:
            body += ('<p>This is an implementation header. Some named declarations are exposed through public '
                     'headers; include the public interface that provides the facility.</p>')
        body += ('<p>Function overloads are documented below with their declarations, source-equivalent effects, return sites, and explicit failure paths. '
                 + link(current, source_page(path), 'Read the complete source snapshot') + ' for the surrounding type definitions and macros.</p>')
        if type_guides[path]:
            body += '<h2>Type reference</h2>'
            for ident, item in type_guides[path]:
                body += type_html(current, ident, item, anchor, esc, prose, link, header_page)
                body += '<p>' + link(current, 'modules/' + ident + '.html', 'Related types and selection guide') + '.</p>'
        if items:
            rows = []
            for item in items:
                sig = esc(item["name"] + item["signature"])
                gate = item["conditions"]
                conditions = '<br><small>' + '<br>'.join('<code>' + esc(x) + '</code>' for x in gate) + '</small>' if gate else ''
                label = (link(current, '#' + item['function_anchor'], item['name'] + item['signature'])
                         if 'function_anchor' in item else f'<code id="{symbol_anchor(item)}">{sig}</code>')
                description = f'{esc(item["kind"])}; line {item["line"]}{conditions}'
                reference = type_references.get((path, item["name"]))
                if reference is None and item["kind"] in {"class", "struct", "union", "enum", "typedef", "alias", "concept"} and item["name"] in preferred_types:
                    _, _, defining_path = preferred_types[item["name"]]
                    if path in type_review_sources[(defining_path, item["name"])]:
                        reference = type_references[(defining_path, item["name"])]
                if reference is not None:
                    target, summary = reference
                    description += '<br>' + prose(summary) + ' ' + link(current, target, 'Type reference') + '.'
                rows.append(row(label,
                                description))
            body += table(rows)
        else:
            body += '<p>This file provides implementation definitions, macros, or included facilities shown in its source snapshot.</p>'
        public_functions = [item for item in function_map[path] if item["public"]]
        implementation_functions = [item for item in function_map[path] if not item["public"]]
        for heading, functions_here, section_id in [('Functions', public_functions, 'functions'),
                                                      ('Implementation functions', implementation_functions, 'implementation-functions')]:
            if functions_here:
                body += f'<h2 id="{section_id}">{heading}</h2>'
                for item in functions_here:
                    raw = item["generated_source"].encode() if item.get("generated_source") else source_data[path]
                    body += function_doc(current, item, raw, link, row, table, lookup)
        result[current] = page(current, "<micron/" + path + ">", body,
                               link(current, "modules/" + owner[0] + ".html", owner[1]))

    current = 'functions/index.html'
    body = ('<p>Function overloads are grouped by module and defining file. Each function entry includes its declaration and source-backed effects. '
            'Defaulted and deleted members, generated overloads, and conditional backends are included.</p>')
    for module in MODULES:
        rows = []
        for header in header_map[module[0]]:
            items = function_map[header['path']]
            if items:
                public_count = sum(item['public'] for item in items)
                rows.append(row(link(current, header_page(header['path']), header['path']),
                                f'{public_count} public overloads; {len(items) - public_count} implementation overloads'))
        if rows:
            body += '<h2>' + link(current, 'modules/' + module[0] + '.html', module[1]) + '</h2>' + table(rows)
    result[current] = page(current, 'Function reference', body)

    current = 'source/index.html'
    body = '<p>Complete source snapshots used by this reference. Select a file to inspect declarations, definitions, comments, and compile-time branches.</p><ul class="header-list">'
    body += ''.join('<li>' + link(current, source_page(item['path']), item['path']) + '</li>' for item in FUNCTIONS['sources']) + '</ul>'
    result[current] = page(current, 'Source index', body)
    for source in FUNCTIONS['sources']:
        path = source['path']
        current = source_page(path)
        result[current] = page(current, path + ' source', source_body(source_data[path], current, path, link))

    letters = list("ABCDEFGHIJKLMNOPQRSTUVWXYZ") + ["other"]
    buckets = defaultdict(list)
    for name, (target, path) in lookup.items():
        buckets[initial(name)].append((name, target, path))
    for items in buckets.values():
        items.sort(key=lambda row: (row[0].casefold(), row[0], row[2]))
    current = "symbols/index.html"
    body = '<p>Browse source-derived declarations by name. Select an entry for its header, overloads, and source location.</p><ul class="symbol-letters">'
    body += "".join('<li>' + link(current, "symbols/" + x.lower() + ".html", x if x != "other" else "Other") + '</li>' for x in letters)
    body += '</ul>'
    result[current] = page(current, "Symbol index", body)
    for letter in letters:
        current = "symbols/" + letter.lower() + ".html"
        rows = [row(link(current, target, name), f'<code>&lt;micron/{esc(path)}&gt;</code>')
                for name, target, path in buckets[letter]]
        body = '<p>' + link(current, "symbols/index.html", "All initials") + '</p>' + table(rows)
        result[current] = page(current, "Symbols: " + letter, body)

    search_entries = [[name, target, path] for name, (target, path) in sorted(lookup.items(), key=lambda i: i[0].casefold())]
    result["assets/search-index.js"] = "window.MICRON_SEARCH_INDEX = " + json.dumps(search_entries, ensure_ascii=False, separators=(",", ":")) + ";\n"
    result["content/guide-coverage.json"] = json.dumps({
        ident: {"types": [item["name"] for item in guide.get("types", [])],
                "related_types": sorted({name for item in guide.get("types", [])
                                         for name in item.get("covers", [])}),
                "sources": guide["sources"]}
        for ident, guide in sorted(GUIDES.items())
    }, indent=2) + "\n"
    current = "search.html"
    body = ('<p>Search names and headers in the local reference.</p><form id="search-form" role="search">'
            '<label for="query">Search terms</label> <input id="query" name="q" type="search" required> '
            '<button type="submit">Search</button></form><p id="search-status" role="status"></p>'
            '<ol id="search-results"></ol><noscript><p>Search requires JavaScript. '
            + link(current, "symbols/index.html", "Browse the symbol index") + ' instead.</p></noscript>')
    result[current] = page(current, "Search", body, search=True)

    current = "about.html"
    body = ('<p>This reference describes the micron project. The page layout and visual assets are adapted '
            'from the <a href="https://github.com/PeterFeicht/cppreference-doc">cppreference-doc offline package</a> '
            'and its cppreference/MediaWiki skin. The source checkout used is revision <code>4f773b7</code> '
            '(release v20251201). The site-specific CSS was adjusted to use local images and local fonts.</p>'
            '<p>Reference prose and examples were written for micron. The inherited assets have their own '
            'copyright and license terms; see ' + link(current, "licenses/NOTICE.txt", "the attribution notice") +
            ' and the accompanying license texts. cppreference documentation content is available under '
            '<a href="https://en.cppreference.com/Cppreference%3ACopyrights">its published terms</a>.</p>'
            '<h2>Maintaining this reference</h2><p>See ' + link(current, "README.md", "the maintenance instructions") +
            ' for the generation and inventory commands.</p>'
            '<h2>Module and type descriptions</h2><p>Handwritten guides describe storage, ownership, operations, complexity, invalidation, '
            'and implementation limits for ' + str(sum(len(guide.get("types", [])) for guide in GUIDES.values())) +
            ' type entries in ' + str(len(GUIDES)) + ' modules. Related aliases and supporting types are explained together. '
            'Their source-file coverage is recorded in ' + link(current, 'content/guide-coverage.json', 'the guide inventory') + '.</p>'
            '<h2>Function coverage and source evidence</h2><p>The function reference contains '
            + str(len(functions)) + ' callable entries from ' + str(len(FUNCTIONS['sources'])) +
            ' source files. Definitions specify their effects through source-equivalent code; return expressions and explicit failure sites are shown with source locations. '
            'Constructors, destructors, conversion operators, defaulted and deleted members, macro-generated overloads, and conditional backend variants are included.</p>'
            '<p>Declaration-only compiler and host entry points are marked as declarations. Their bodies are not present in the declaring source. '
            'The complete numbered source snapshots retain surrounding class definitions, macros, branches, and copyright notices.</p>')
    result[current] = page(current, "About this reference", body)

    manifest = sorted(result)
    result["content/generated.json"] = json.dumps(manifest, indent=2) + "\n"
    previous_file = CONTENT / "generated.json"
    previous = json.loads(previous_file.read_text()) if previous_file.exists() else []
    mismatches = []
    for name, text in result.items():
        path = SITE / name
        existing = path.read_text(encoding="utf-8") if path.exists() else None
        if existing != text:
            mismatches.append(name)
            if not args.check:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text, encoding="utf-8")
    stale = sorted(set(previous) - set(manifest))
    if not args.check:
        for name in stale:
            path = SITE / name
            if path.is_file():
                path.unlink()
    if args.check and (mismatches or stale):
        print("Stale generated files:", *(mismatches + stale)[:20], sep="\n  ")
        raise SystemExit(1)
    print(f"{'Checked' if args.check else 'Rendered'} {len(manifest)} files; {len(headers)} source files; {len(functions)} callable entries; {len(symbols)} declaration entries")


if __name__ == "__main__":
    main()
