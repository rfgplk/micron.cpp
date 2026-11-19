"""Load and render the handwritten module and type reference."""
from __future__ import annotations

import importlib.util
from pathlib import Path


def load_guides(content: Path):
    guides = {}
    for path in sorted((content / "guides").glob("*.py")):
        spec = importlib.util.spec_from_file_location("reference_guide_" + path.stem, path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        for ident, guide in module.GUIDES.items():
            if ident in guides:
                raise ValueError(f"duplicate guide for {ident}: {path}")
            guides[ident] = guide
    return guides


def validate_guides(guides, modules, source_paths):
    """Reject malformed prose and source references before generating pages."""
    module_entries = {module[0]: {entry[0] for entry in module[5]} for module in modules}
    documented = set()

    def text(value, context):
        if not isinstance(value, str) or not value.strip():
            raise ValueError(f"{context}: expected nonempty prose")
        if value.count("`") % 2:
            raise ValueError(f"{context}: unmatched inline-code delimiter")

    def sections(items, context):
        for section in items:
            text(section["title"], context)
            if not any(section.get(key) for key in ("paragraphs", "table", "bullets")):
                raise ValueError(f"{context}: empty section {section['title']}")
            for value in [*section.get("paragraphs", []), *section.get("bullets", [])]:
                text(value, context)
            if section.get("table"):
                table = section["table"]
                if not table["columns"] or not table["rows"]:
                    raise ValueError(f"{context}: empty comparison table")
                for value in table["columns"]:
                    text(value, context)
                for row in table["rows"]:
                    if len(row) != len(table["columns"]):
                        raise ValueError(f"{context}: table row has the wrong number of cells")
                    for value in row:
                        text(value, context)

    for ident, guide in guides.items():
        if ident not in module_entries:
            raise ValueError(f"unknown guide module: {ident}")
        if guide.get("summary"):
            text(guide["summary"], ident)
        sources = guide.get("sources", [])
        if not sources or len(sources) != len(set(sources)):
            raise ValueError(f"{ident}: missing or duplicate reviewed source paths")
        if set(sources) - source_paths:
            raise ValueError(f"{ident}: unknown source paths {sorted(set(sources) - source_paths)}")
        sections(guide.get("sections", []), ident)
        for name, description in guide.get("entry_overrides", {}).items():
            if name not in module_entries[ident]:
                raise ValueError(f"{ident}: unknown selected interface {name}")
            text(description, ident)
        for item in guide.get("types", []):
            text(item["name"], ident)
            text(item["summary"], item["name"])
            covers = item.get("covers", [])
            if not isinstance(covers, list):
                raise ValueError(f"{item['name']}: related types must be a list of names")
            for name in covers:
                text(name, item["name"])
            if len(covers) != len(set(covers)) or item["name"] in covers:
                raise ValueError(f"{item['name']}: duplicate related type name")
            if item["header"] not in sources:
                raise ValueError(f"{item['name']}: defining header is absent from reviewed sources")
            if (ident, item["header"], item["name"]) in documented:
                raise ValueError(f"{ident}: duplicate type {item['name']}")
            documented.add((ident, item["header"], item["name"]))
            sections(item.get("sections", []), item["name"])


def type_anchor(ident, item, anchor):
    return anchor("type", ident, item["header"], item["name"])


def sections_html(sections, prose, esc, level=2):
    result = []
    for section in sections:
        result.append(f'<h{level}>{esc(section["title"])}</h{level}>')
        result.extend("<p>" + prose(paragraph) + "</p>" for paragraph in section.get("paragraphs", []))
        if section.get("table"):
            table = section["table"]
            result.append('<div class="guide-table"><table class="wikitable"><thead><tr>')
            result.extend('<th scope="col">' + prose(column) + '</th>' for column in table["columns"])
            result.append('</tr></thead><tbody>')
            for cells in table["rows"]:
                result.append('<tr>' + ''.join('<td>' + prose(cell) + '</td>' for cell in cells) + '</tr>')
            result.append('</tbody></table></div>')
        if section.get("bullets"):
            result.append('<ul>' + ''.join('<li>' + prose(item) + '</li>' for item in section["bullets"]) + '</ul>')
    return ''.join(result)


def type_html(current, ident, item, anchor, esc, prose, link, header_page, level=3):
    result = f'<section class="type-entry"><h{level} id="{type_anchor(ident, item, anchor)}"><code>{esc(item["name"])}</code></h{level}>'
    result += '<p>Defined in header ' + link(current, header_page(item["header"]), '<micron/' + item["header"] + '>') + '.</p>'
    result += '<p>' + prose(item["summary"]) + '</p>'
    result += sections_html(item.get("sections", []), prose, esc, level + 1)
    return result + '</section>'


def guide_html(current, ident, guide, anchor, esc, prose, link, header_page, row, table):
    result = sections_html(guide.get("sections", []), prose, esc)
    types = guide.get("types", [])
    if types:
        result += '<h2 id="types">Types</h2>'
        result += table([row('<a href="#' + type_anchor(ident, item, anchor) + '"><code>' + esc(item["name"]) + '</code></a>',
                             prose(item["summary"])) for item in types])
        result += ''.join(type_html(current, ident, item, anchor, esc, prose, link, header_page) for item in types)
    return result
