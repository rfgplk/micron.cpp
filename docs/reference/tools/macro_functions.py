"""Expand source macros that declare functions, using the C preprocessor."""
from __future__ import annotations

import re
import subprocess
from collections import defaultdict
from update_inventory import branches


def definitions(raw):
    result = []
    lines = raw.decode("utf-8", errors="replace").splitlines(keepends=True)
    index = 0
    while index < len(lines):
        match = re.match(r"\s*#\s*define\s+(\w+)\(([^\n]*?)\)(.*)", lines[index])
        if not match:
            index += 1
            continue
        start = index
        definition = lines[index]
        while lines[index].rstrip().endswith("\\") and index + 1 < len(lines):
            index += 1
            definition += lines[index]
        body = definition[definition.find(")") + 1:]
        body = re.sub(r"\\\r?\n", " ", body)
        result.append({"name": match[1], "line": start + 1, "end": index + 1,
                       "definition": definition.rstrip(), "body": body})
        index += 1
    return result


def invocations(raw, names, excluded):
    source = raw.decode("utf-8", errors="replace")
    # Match tokens outside string literals and comments; preserve byte positions.
    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\b[A-Za-z_]\w*\b', re.S)
    for token in tokens.finditer(source):
        name = token[0]
        if name not in names:
            continue
        line = source.count("\n", 0, token.start()) + 1
        if any(first <= line <= last for first, last in excluded):
            continue
        position = token.end()
        while position < len(source) and source[position].isspace():
            position += 1
        if position == len(source) or source[position] != "(":
            continue
        depth, end = 0, position
        quote = None
        while end < len(source):
            char = source[end]
            if quote:
                if char == "\\":
                    end += 2
                    continue
                if char == quote:
                    quote = None
            elif char in "\"'":
                quote = char
            elif char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    end += 1
                    break
            end += 1
        if depth == 0:
            yield {"name": name, "line": line, "invocation": source[token.start():end],
                   "byte": len(source[:token.start()].encode("utf-8"))}


def expand_macros(raw, path, parser, walk, function_declarator, parser_input):
    macros = definitions(raw)
    conditions = branches(raw.decode('utf-8', errors='replace').splitlines())
    candidates = set()
    for macro in macros:
        body = re.sub(r"\s*##\s*", "", macro["body"]).encode()
        tree = parser.parse(parser_input(body))
        if any(node.type == "function_definition" and function_declarator(node.child_by_field_name("declarator"))
               for node in walk(tree.root_node)):
            candidates.add(macro["name"])
    # Some declaration macros are wrappers around another declaration macro.
    changed = True
    while changed:
        changed = False
        for macro in macros:
            if macro["name"] not in candidates and any(re.search(r"\b" + re.escape(name) + r"\s*\(", macro["body"]) for name in candidates):
                candidates.add(macro["name"])
                changed = True
    if not candidates:
        return [], set()
    uses = list(invocations(raw, candidates, [(macro["line"], macro["end"]) for macro in macros]))
    groups = defaultdict(list)
    for use in uses:
        possible = [macro for macro in macros if macro["name"] == use["name"] and macro["line"] < use["line"]]
        if possible:
            variants = {}
            for selected in possible:
                variants[tuple(conditions.get(selected['line'], []))] = selected
            for selected in variants.values():
                groups[selected["line"]].append((selected, use))
    result = []
    for group in groups.values():
        selected = group[0][0]
        # Supply declaration-macro dependencies from the same lexical source.
        # Other helper names remain as written; their definitions are separately
        # documented instead of being assigned guessed behavior here.
        dependencies = {selected["name"]: selected}
        changed = True
        while changed:
            changed = False
            for macro in list(dependencies.values()):
                identifiers = set(re.findall(r"\b[A-Za-z_]\w*\b", macro["body"]))
                for name in identifiers - dependencies.keys():
                    available = [m for m in macros if m["name"] == name and m["line"] <= selected["line"]]
                    if available:
                        dependencies[name] = max(available, key=lambda m: m["line"])
                        changed = True
        source = '\n'.join(macro["definition"] for macro in dependencies.values()) + '\n'
        for index, (_, use) in enumerate(group):
            source += f'\nMICRON_REFERENCE_BEGIN_{index}\n#line {use["line"]} "{path}"\n{use["invocation"]}\nMICRON_REFERENCE_END_{index}\n'
        completed = subprocess.run(["g++", "-E", "-P", "-undef", "-nostdinc", "-x", "c++", "-"],
                                   input=source, text=True, capture_output=True)
        if completed.returncode:
            raise RuntimeError(f"Cannot expand {path}:{selected['line']}: {completed.stderr}")
        for index, (macro, use) in enumerate(group):
            match = re.search(rf"MICRON_REFERENCE_BEGIN_{index}\s*\n(.*?)\nMICRON_REFERENCE_END_{index}", completed.stdout, re.S)
            if not match:
                raise RuntimeError(f"Missing expansion of {path}:{use['line']}")
            expanded = match[1].strip()
            result.append({**use, "expanded": expanded, "macro_line": macro["line"],
                           "macro_end": macro["end"], "macro_definition": macro["definition"],
                           "macro_conditions": conditions.get(macro['line'], [])})
    return result, candidates
