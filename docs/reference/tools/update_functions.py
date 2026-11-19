#!/usr/bin/env python3
"""Read callable definitions, declarations and their source evidence.

Only this inventory refresh needs the pinned parser packages in requirements.txt.
Rendering and reading the checked-in reference do not need them.
"""
from __future__ import annotations

import hashlib
import json
import re
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.dont_write_bytecode = True
from tree_sitter import Language, Parser
import tree_sitter_cpp
import tree_sitter_javascript
from update_inventory import branches, namespace_lines, repair_scope
from macro_functions import expand_macros

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "src"
OUT = ROOT / "docs/reference/content/functions.json"
PARSER = Parser(Language(tree_sitter_cpp.language()))
JS_PARSER = Parser(Language(tree_sitter_javascript.language()))


def walk(node, prune=()):
    yield node
    if node.type not in prune:
        for child in node.named_children:
            yield from walk(child, prune)


def ancestors(node):
    current = node.parent
    while current:
        yield current
        current = current.parent


def text(raw, node):
    return raw[node.start_byte:node.end_byte].decode("utf-8", errors="replace") if node else ""


def function_declarator(node):
    if node is None:
        return None
    if node.type in {"function_declarator", "operator_cast"}:
        return node
    for child in node.named_children:
        if child.type not in {"parameter_list", "template_argument_list", "compound_statement"}:
            result = function_declarator(child)
            if result:
                return result
    return None


def simple_name(name):
    name = re.sub(r"^operator\s+(?=[^\w])", "operator", name.strip())
    # A template specialization belongs to the same overload family.
    if not name.startswith("operator"):
        name = re.sub(r"<.*>", "", name)
    return name


def scope_of(raw, node, lexical):
    scopes = []
    access = ""
    current = node.parent
    local = False
    while current:
        if current.type in {"function_definition", "lambda_expression"}:
            local = True
        if current.type in {"namespace_definition", "class_specifier", "struct_specifier", "union_specifier"}:
            name = current.child_by_field_name("name")
            value = simple_name(text(raw, name)) if name else f"anonymous_{current.start_point.row + 1}"
            scopes.append((current.type, value))
            if not access and current.type != "namespace_definition":
                access = "private" if current.type == "class_specifier" else "public"
                body = current.child_by_field_name("body")
                if body:
                    for child in body.named_children:
                        if child.start_byte > node.start_byte:
                            break
                        if child.type == "access_specifier":
                            access = text(raw, child).rstrip(":")
        current = current.parent
    scopes.reverse()
    namespaces = [value for kind, value in scopes if kind == "namespace_definition"]
    classes = [value for kind, value in scopes if kind != "namespace_definition"]
    namespace = lexical if lexical is not None else "::".join(namespaces)
    return "::".join(filter(None, [namespace, *classes])), access, local


def public_name(name, access="", local=False):
    return not local and access not in {"private", "protected"} and not any(
        part.startswith(("__", "anonymous_", "anon")) for part in name.split("::")
    )


def enclosing_guards(raw, node, body):
    guards = []
    current = node
    while current.parent and current.parent != body:
        parent = current.parent
        if parent.type in {"if_statement", "while_statement", "for_statement", "for_range_loop", "switch_statement"}:
            condition = parent.child_by_field_name("condition")
            if condition:
                prefix = "else of " if current == parent.child_by_field_name("alternative") else ""
                original = text(raw, parent).lstrip()
                consteval = re.match(r"if\s+!?\s*consteval\b", original)
                guards.append(prefix + (consteval[0] if consteval else text(raw, condition)))
        current = parent
    return list(reversed(guards))


def facts(raw, body):
    returns, failures, calls = [], [], []
    if body is None:
        return returns, failures, calls
    for node in walk(body, {"lambda_expression", "class_specifier", "struct_specifier", "function_definition",
                            "function_declaration", "function_expression", "arrow_function", "method_definition"}):
        if node.type == "return_statement":
            value = text(raw, node).strip().removeprefix("return").removesuffix(";").strip()
            returns.append({"expression": value, "line": node.start_point.row + 1,
                            "guards": enclosing_guards(raw, node, body)})
        if node.type in {"throw_statement", "static_assert_declaration"}:
            failures.append({"expression": text(raw, node).strip(), "line": node.start_point.row + 1,
                             "guards": enclosing_guards(raw, node, body)})
        if node.type == "call_expression":
            callee = text(raw, node.child_by_field_name("function"))
            if callee:
                calls.append(callee)
            if re.search(r"(?:^|::)(?:exc|panic|abort|terminate|halt|assert)(?:\s*<|$)", callee):
                failures.append({"expression": text(raw, node).strip(), "line": node.start_point.row + 1,
                                 "guards": enclosing_guards(raw, node, body)})
    return returns, failures, list(dict.fromkeys(calls))


def entry(raw, node, fd, path, conditions, lexical, tags_at_line):
    conversion = fd.type == "operator_cast"
    name_node = fd.child_by_field_name("type" if conversion else "declarator")
    if name_node is None:
        return None
    parameter_declarator = fd
    if conversion:
        parameter_declarator = next((child for child in walk(fd) if child.type == "abstract_function_declarator"), None)
        if parameter_declarator is None:
            return None
        leaf = re.sub(r"\s+", " ", raw[fd.start_byte:parameter_declarator.start_byte].decode()).strip()
        name_node = fd
    elif text(raw, node.child_by_field_name('type')) == 'operator':
        leaf = 'operator ' + text(raw, name_node)
    else:
        leaf = simple_name(text(raw, name_node))
    spelling = leaf
    qualified_spelling = not leaf.startswith('operator') and '::' in leaf
    if qualified_spelling:
        leaf = leaf.rsplit('::', 1)[-1]
    if not leaf or any(x in leaf for x in ("\n", "{", ";")):
        return None
    line = name_node.start_point.row + 1
    lexical_scope = lexical.get(line) if lexical is not None else None
    scope, access, local = scope_of(raw, node, lexical_scope)
    candidates = [tag for tag in tags_at_line.get(line, [])
                  if re.sub(r'\s+', '', simple_name(tag["name"])) == re.sub(r'\s+', '', leaf)
                  and tag["kind"] in {"function", "prototype"}]
    if candidates:
        tag = candidates[0]
        # Ctags also sees access labels inside conditional sections.
        access = tag.get("access", access)
        parsed_scope = tag.get("scope", "")
        if lexical_scope is not None:
            parsed_scope = repair_scope(parsed_scope, tag.get("scopeKind", ""), lexical_scope)
        if parsed_scope and (not scope or scope == lexical_scope or qualified_spelling):
            scope = parsed_scope
    name = "::".join(filter(None, (scope, leaf if candidates or not qualified_spelling else spelling)))
    body = node.child_by_field_name("body")
    if body is not None and body.type != "compound_statement":
        body = None
    start = node
    while start.parent and start.parent.type in {"template_declaration", "friend_declaration"}:
        start = start.parent
    declaration_end = body.start_byte if body else node.end_byte
    declaration = raw[start.start_byte:declaration_end].decode("utf-8", errors="replace").strip()
    # Initializer lists are part of the effects, as well as the declaration.
    returns, failures, calls = facts(raw, body)
    parameters = parameter_declarator.child_by_field_name("parameters") if parameter_declarator else None
    parameter_list = [text(raw, p).strip() for p in parameters.named_children
                      if p.type not in {"comment"}] if parameters else []
    kind = "definition" if body else "declaration"
    if any(c.type == "default_method_clause" for c in node.named_children):
        kind = "defaulted"
    if any(c.type == "delete_method_clause" for c in node.named_children):
        kind = "deleted"
    source_end = node.end_byte
    if kind == "declaration" and not declaration.rstrip().endswith(";"):
        if not candidates:
            return None
        semicolon = raw.find(b";", source_end, source_end + 2048)
        if semicolon != -1:
            source_end = semicolon + 1
            declaration = raw[start.start_byte:source_end].decode("utf-8", errors="replace").strip()
    return {"name": name, "leaf": leaf, "path": path, "line": line, "start": start.start_point.row + 1,
            "end": node.end_point.row + 1, "column": name_node.start_point.column,
            "signature": text(raw, parameter_declarator) if conversion else text(raw, fd)[len(text(raw, name_node)):].strip(),
            "declaration": declaration, "parameters": parameter_list,
            "kind": kind, "access": access, "public": public_name(name, access, local),
            "conditions": conditions.get(line, []), "returns": returns,
            "failures": failures, "calls": calls, "parser": "syntax",
            "source_start": start.start_byte, "source_end": source_end,
            "body_start": body.start_byte if body else None,
            "body_end": body.end_byte if body else None}


def parser_input(raw):
    # Expand declaration-only qualifier macros to whitespace, retaining every
    # original byte offset. Their actual spelling remains in the reference.
    pattern = rb"\b(?:__inline(?:__|_g|_neon)?|__restrict__?|restrict|__bb_hot|gconstructor_|gdestructor_|__micron_(?:push_options|pop_options|optimize_no_fast_math|optimize_no_tree_loop_distribute))\b"
    parsed = re.sub(pattern, lambda m: b" " * len(m[0]), raw)
    parsed = re.sub(rb"\bif\s+(!?\s*consteval)\b",
                    lambda m: (b"if (false)" if b"!" in m[1] else b"if (true)").ljust(len(m[0])), parsed)
    # Type-trait builtins take types rather than ordinary call arguments. Mask
    # only these compiler expressions so the grammar can retain surrounding
    # statements. All published expressions still come from the original bytes.
    traits = rb"\b__(?:is_(?:same(?:_as)?|base_of|class|union|enum|trivial|trivially_copyable|standard_layout|polymorphic|abstract|final|empty|aggregate|constructible|assignable|destructible|nothrow_constructible|nothrow_assignable|nothrow_destructible|trivially_constructible|trivially_assignable|trivially_destructible|literal_type|pod|convertible(?:_to)?|layout_compatible|pointer_interconvertible_base_of)|has_(?:unique_object_representations|virtual_destructor))\s*\("
    for match in reversed(list(re.finditer(traits, parsed))):
        depth, end = 1, match.end()
        while end < len(parsed) and depth:
            depth += (parsed[end] == 40) - (parsed[end] == 41)
            end += 1
        if not depth:
            replacement = b"x" + re.sub(rb"[^\n]", b" ", parsed[match.start() + 1:end])
            parsed = parsed[:match.start()] + replacement + parsed[end:]
    return parsed


def javascript_entries(raw, path):
    tree = JS_PARSER.parse(raw)
    functions = []
    callable_types = {"function_declaration", "generator_function_declaration", "method_definition", "arrow_function", "function_expression"}
    for node in walk(tree.root_node):
        if node.type not in callable_types:
            continue
        name_node = node.child_by_field_name("name")
        if name_node is None and node.parent and node.parent.type in {"variable_declarator", "pair"}:
            name_node = node.parent.child_by_field_name("name") or node.parent.child_by_field_name("key")
        leaf = text(raw, name_node) if name_node else f"anonymous_callback_{node.start_point.row + 1}_{node.start_point.column}"
        scopes = [Path(path).name]
        parent_names = []
        parent = node.parent
        exported = False
        while parent:
            if parent.type == "export_statement":
                exported = True
            if parent.type in callable_types | {"class_declaration"}:
                parent_name = parent.child_by_field_name("name")
                if parent_name:
                    parent_names.append(text(raw, parent_name))
            parent = parent.parent
        scopes.extend(reversed(parent_names))
        name = '::'.join(scopes + [leaf])
        body = node.child_by_field_name("body")
        if body is None:
            continue
        params = node.child_by_field_name("parameters")
        one_parameter = node.child_by_field_name("parameter")
        parameters = [text(raw, parameter) for parameter in params.named_children] if params else [text(raw, one_parameter)] if one_parameter else []
        returns, failures, calls = facts(raw, body)
        if node.type == "arrow_function" and body.type != "statement_block":
            returns = [{"expression": text(raw, body), "line": body.start_point.row + 1, "guards": []}]
        line = name_node.start_point.row + 1 if name_node else node.start_point.row + 1
        functions.append({"name": name, "path": path, "line": line, "start": node.start_point.row + 1,
                          "end": node.end_point.row + 1, "column": node.start_point.column,
                          "signature": '(' + ', '.join(parameters) + ')',
                          "declaration": raw[node.start_byte:body.start_byte].decode().strip(),
                          "parameters": parameters, "kind": "definition", "access": "",
                          "public": exported and (not parent_names or node.type == "method_definition") and name_node is not None,
                          "language": "JavaScript", "leaf": leaf, "conditions": [], "returns": returns, "failures": failures,
                          "calls": calls, "parser": "syntax", "source_start": node.start_byte,
                          "source_end": node.end_byte, "body_start": body.start_byte, "body_end": body.end_byte})
    return functions


def main():
    headers = sorted(p for p in SOURCE.rglob("*") if p.is_file() and
                     (p.suffix in {".hpp", ".c", ".mjs"} or not p.suffix))
    cpp = [p for p in headers if p.suffix != ".mjs"]
    command = ["ctags", "--options=NONE", "--output-format=json", "--language-force=C++",
               "--fields=+enaKS", "--kinds-C++=+pNA", "--extras=+F", "--sort=no", "-f", "-", "-L", "-"]
    output = subprocess.run(command, cwd=ROOT, input="\n".join(p.relative_to(ROOT).as_posix() for p in cpp) + "\n",
                            text=True, check=True, capture_output=True).stdout
    tags = defaultdict(list)
    for line in output.splitlines():
        if line.startswith("{"):
            tag = json.loads(line)
            tags[tag["path"].removeprefix("src/")].append(tag)
    functions, sources, unmatched, exclusions = [], [], [], []
    for path in headers:
        relative = path.relative_to(SOURCE).as_posix()
        raw = path.read_bytes()
        sources.append({"path": relative, "sha256": hashlib.sha256(raw).hexdigest()})
        if path.suffix == ".mjs":
            functions.extend(javascript_entries(raw, relative))
            continue
        lines = raw.decode("utf-8", errors="replace").splitlines()
        conditions = branches(lines)
        lexical = namespace_lines(lines)
        tags_at_line = defaultdict(list)
        for tag in tags[relative]:
            tags_at_line[tag["line"]].append(tag)
        parsed = parser_input(raw)
        tree = PARSER.parse(parsed)
        seen = set()
        local_entries = []
        for node in walk(tree.root_node):
            if node.type == "function_definition":
                fd = function_declarator(node.child_by_field_name("declarator"))
            elif node.type in {"declaration", "field_declaration"}:
                fd = function_declarator(node.child_by_field_name("declarator"))
                # A pointer-to-function member is an object, not a callable declaration.
                if fd and fd.child_by_field_name("declarator") and fd.child_by_field_name("declarator").type in {
                    "parenthesized_declarator", "abstract_parenthesized_declarator"}:
                    fd = None
            else:
                continue
            if fd is None:
                continue
            # Do not mistake calls in requires-expressions or function bodies for declarations.
            ancestor = node.parent
            expression = False
            while ancestor and ancestor.type not in {"namespace_definition", "field_declaration_list", "translation_unit"}:
                if ancestor.type in {"requires_expression", "compound_statement", "lambda_expression"}:
                    expression = True
                    break
                ancestor = ancestor.parent
            if expression:
                continue
            record = entry(raw, node, fd, relative, conditions, lexical, tags_at_line)
            if record is None:
                continue
            key = (record["line"], record["column"], record["name"])
            if key not in seen:
                seen.add(key)
                local_entries.append(record)
        expansions, declaration_macros = expand_macros(raw, relative, PARSER, walk, function_declarator, parser_input)
        local_entries = [item for item in local_entries if item["name"].split("::")[-1] not in declaration_macros]
        expanded_lines = set()
        for expansion in expansions:
            generated = expansion["expanded"].encode()
            expanded_tree = PARSER.parse(parser_input(generated))
            origin = tree.root_node.descendant_for_byte_range(expansion["byte"], expansion["byte"])
            lex_scope = lexical.get(expansion["line"]) if lexical is not None else None
            scope, access, local = scope_of(raw, origin, lex_scope)
            for node in walk(expanded_tree.root_node):
                if node.type not in {"function_definition", "declaration", "field_declaration"}:
                    continue
                ancestor = node.parent
                if any(parent.type in {"compound_statement", "lambda_expression", "requires_expression"}
                       for parent in ancestors(node)):
                    continue
                fd = function_declarator(node.child_by_field_name("declarator"))
                if not fd:
                    continue
                generated_lexical = {number: scope for number in range(1, generated.count(b"\n") + 2)}
                record = entry(generated, node, fd, relative, {}, generated_lexical, {})
                if not record:
                    continue
                record.update({"line": expansion["line"], "start": expansion["line"], "end": expansion["line"],
                               "generated_source": expansion["expanded"], "macro": expansion["name"],
                               "macro_line": expansion["macro_line"], "macro_end": expansion["macro_end"],
                               "macro_definition": expansion["macro_definition"], "access": access,
                               "parser": "preprocessor and syntax",
                               "conditions": list(dict.fromkeys(conditions.get(expansion["line"], []) + expansion['macro_conditions']))})
                record["public"] = public_name(record["name"], access, local)
                for site in record["returns"] + record["failures"]:
                    site["expanded_line"] = site["line"]
                    site["line"] = expansion["line"]
                local_entries.append(record)
                expanded_lines.add(expansion["line"])
        # Independently audit the syntax tree against Ctags. Missing definitions
        # retain their complete source range, rather than guessing their effects.
        covered = {(item["line"], re.sub(r'\s+', '', item['leaf'])) for item in local_entries}
        line_offsets = [0]
        for source_line in raw.splitlines(keepends=True):
            line_offsets.append(line_offsets[-1] + len(source_line))
        macro_names = {tag["name"] for tag in tags[relative] if tag["kind"] == "macro"}
        for tag in tags[relative]:
            tag_key = (tag['line'], re.sub(r'\s+', '', simple_name(tag['name'])))
            if tag["kind"] != "function" or tag_key in covered or tag["name"].startswith("__anon"):
                continue
            if tag["name"] in declaration_macros and tag["line"] in expanded_lines:
                continue
            if lines[tag["line"] - 1].strip().startswith("requires"):
                exclusions.append({"path": relative, "line": tag["line"], "name": tag["name"],
                                   "reason": "call in a requires-clause, not a function definition"})
                continue
            if tag["name"] not in macro_names:
                first = tag["line"] - 1
                while first > 0 and tag["line"] - first < 20:
                    preceding = lines[first - 1].strip()
                    if not preceding or preceding.startswith(("#", "//")) or preceding.endswith((";", "}", "{")):
                        break
                    first -= 1
                start_byte = line_offsets[first]
                end_byte = line_offsets[min(tag.get("end", tag["line"]), len(line_offsets) - 1)]
                prefix = re.sub(rb"[^\n]", b" ", parsed[:start_byte])
                fragment = prefix + parsed[start_byte:end_byte]
                # Some compiler builtins are also valid names in source branches
                # where the corresponding compiler does not reserve them.
                fragment = fragment.replace(b"__is_same(", b"doc_same_(")
                recovered = PARSER.parse(fragment)
                for node in walk(recovered.root_node):
                    if node.type != "function_definition":
                        continue
                    fd = function_declarator(node.child_by_field_name("declarator"))
                    if not fd:
                        continue
                    record = entry(raw, node, fd, relative, conditions, lexical, tags_at_line)
                    if record and record["line"] == tag["line"]:
                        record["parser"] = "isolated syntax"
                        local_entries.append(record)
                        covered.add((record['line'], re.sub(r'\s+', '', record['leaf'])))
                        break
            if tag_key in covered:
                continue
            unmatched.append({"path": relative, "line": tag["line"], "end": tag.get("end"),
                              "name": tag["name"], "scope": tag.get("scope", ""),
                              "access": tag.get("access", ""), "signature": tag.get("signature", "")})
        functions.extend(local_entries)
    functions.sort(key=lambda x: (x["path"], x["line"], x["column"], x["name"]))
    inventory = {"sources": sources, "functions": functions, "unmatched": unmatched, "exclusions": exclusions}
    OUT.write_text(json.dumps(inventory, ensure_ascii=False, separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"Read {len(sources)} source files; {len(functions)} callable entries; {len(unmatched)} Ctags definitions need reconciliation")
    print(dict(Counter(item["kind"] for item in functions)))


if __name__ == "__main__":
    main()
