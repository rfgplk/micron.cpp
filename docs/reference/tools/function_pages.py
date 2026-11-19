"""Render source-backed effects for every callable, without name-based guesses."""
from __future__ import annotations

import hashlib
import html
import re


def esc(value):
    return html.escape(str(value), quote=True)


def function_anchor(item):
    value = f"{item['path']}\0{item['line']}\0{item['column']}\0{item['name']}\0{item.get('macro_line', '')}"
    return "f-" + hashlib.sha1(value.encode()).hexdigest()[:16]


def source_page(path):
    return "source/" + path + ".html"


def source_link(current, item, line, link):
    return link(current, source_page(item["path"]) + f"#L{line}", f"line {line}")


def code(value):
    return "<code>" + esc(value) + "</code>"


def function_doc(current, item, raw, link, row, table, lookup):
    ident = function_anchor(item)
    label = item["name"] + item["signature"]
    result = f'<section class="function-entry" id="{ident}"><h3>{code(label)}</h3>'
    result += '<p>Defined in <code>' + esc(item["path"]) + '</code>, ' + source_link(current, item, item["line"], link) + '.</p>'
    if not item["public"]:
        result += '<p>Implementation function. Its declaration is used by the library implementation.</p>'
    result += '<table class="t-dcl-begin"><tbody><tr class="t-dcl"><td><pre>' + esc(item["declaration"]) + '</pre></td></tr></tbody></table>'
    if item["conditions"]:
        result += '<h4>Availability</h4><pre>' + esc("\n".join(item["conditions"])) + '</pre>'
    parameters = item["parameters"]
    if parameters and parameters != ["void"]:
        result += '<h4>Parameters</h4><p>The parameter declarations, including any default arguments, are:</p><ul>'
        result += ''.join('<li>' + code(parameter) + '</li>' for parameter in parameters) + '</ul>'
    result += '<h4>Effects</h4>'
    if item["kind"] == "deleted":
        result += '<p>This overload is deleted. A program that selects it is ill-formed.</p>'
    elif item["kind"] == "defaulted":
        result += '<p>This special member is explicitly defaulted. Its initialization, assignment, or destruction is performed memberwise as specified by the declaration and the class definition.</p>'
    elif item["body_start"] is not None:
        body = raw[item["body_start"]:item["body_end"]].decode("utf-8", errors="replace")
        # A single-return definition has a complete, direct return contract.
        stripped = re.sub(r"/\*.*?\*/|//[^\n]*", "", body, flags=re.S).strip()
        single_return = re.fullmatch(r"\{\s*return\s+(.+?);\s*\}", stripped, re.S)
        if single_return and len(item["returns"]) == 1:
            result += '<p>Returns ' + code(item["returns"][0]["expression"]) + '.</p>'
        elif re.fullmatch(r"\{\s*\}", stripped):
            result += '<p>The function body performs no operations. Any base and member initialization is specified in the declaration.</p>'
        else:
            result += '<p>Equivalent to the following definition. Expressions, calls, initialization, and conditional branches are evaluated in the order specified here.</p>'
        definition = raw[item["source_start"]:item["source_end"]].decode("utf-8", errors="replace")
        result += '<details class="equivalent-definition"><summary>Equivalent definition</summary><pre class="source-cpp">' + esc(definition) + '</pre></details>'
    else:
        destination = lookup.get(item['name'], ('', ''))[0]
        if destination and destination != current + '#' + ident:
            result += '<p>See ' + link(current, destination, 'the definition and overloads') + ' for the effects. This declaration supplies the parameter types and qualifiers shown above.</p>'
        else:
            result += '<p>This is a declaration. Its body is supplied by the runtime, compiler, or host. The source declaration does not specify additional effects.</p>'
    if item.get("macro"):
        result += '<h4>Macro expansion</h4><p>This overload is produced by ' + code(item["macro"]) + ', defined at ' + source_link(current, item, item["macro_line"], link) + '. The equivalent definition shows the preprocessor expansion at the invocation.</p>'
    if item["returns"]:
        result += '<h4>Return value</h4><p>Evaluates the return expression reached in the equivalent definition. The return sites are listed in source order; preceding exits and enclosing branches determine which site is reached.</p>'
        result += table([row(code(site["expression"]) if site["expression"] else 'No value (<code>return;</code>)',
                             source_link(current, item, site["line"], link)) for site in item["returns"]])
    elif item["body_start"] is not None:
        if item.get('language') == 'JavaScript':
            if item.get('leaf') == 'constructor':
                result += '<h4>Return value</h4><p>Constructs an instance. Base initialization and property assignments are specified by the equivalent definition.</p>'
            else:
                result += '<h4>Return value</h4><p>There is no explicit return expression. Completing the function body produces <code>undefined</code>; an async function resolves its returned promise with that value.</p>'
        else:
            result += '<h4>Return value</h4><p>The body contains no explicit return statement. Constructors and destructors do not produce a return value; a <code>void</code> function returns without a value when its body completes.</p>'
    if item["failures"]:
        result += '<h4>Constraints and explicit failure paths</h4><p>The definition contains the following assertions, throw expressions, or diagnostic calls. Their enclosing tests are shown where present. Calls made by the function can have additional requirements and failure behavior.</p>'
        rows = []
        for site in item["failures"]:
            condition = '<br>Within ' + ', '.join(code(g) for g in site["guards"]) if site["guards"] else ''
            rows.append(row(code(site["expression"]), source_link(current, item, site["line"], link) + condition))
        result += table(rows)
    dependencies = []
    for callee in item["calls"]:
        normalized = re.sub(r"<[^<>]*>", "", callee).strip()
        candidates = [normalized]
        scopes = item["name"].split("::")[:-1]
        while scopes:
            candidates.append("::".join(scopes + [normalized]))
            scopes.pop()
        destination = next((lookup[name][0] for name in candidates if name in lookup), None)
        if destination and destination != current + '#' + ident:
            dependencies.append(link(current, destination, callee))
    if dependencies:
        result += '<h4>Referenced functions</h4><p>' + ', '.join(dict.fromkeys(dependencies)) + '.</p>'
    return result + '</section>'


def source_body(raw, current, path, link):
    lines = raw.decode("utf-8", errors="replace").splitlines()
    result = '<p>Verbatim source snapshot. Copyright and license notices are retained. '
    result += link(current, "headers/" + path + ".html", "Function and declaration reference") + '.</p><pre class="source-listing">'
    result += '\n'.join(f'<span id="L{number}" class="source-line"><a class="line-number" href="#L{number}">{number}</a> {esc(line)}</span>'
                        for number, line in enumerate(lines, 1))
    return result + '</pre>'
