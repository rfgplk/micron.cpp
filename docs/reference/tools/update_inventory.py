#!/usr/bin/env python3
"""Refresh the source inventory. Requires Universal Ctags with JSON output."""

from __future__ import annotations

import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "docs/reference/content/inventory.json"
SOURCE = ROOT / "src"


def internal(path: str) -> bool:
    parts = Path(path).parts
    return any(part.startswith("__") for part in parts) or any(
        part in {"bits", "arch", "upstream"} for part in parts[:-1]
    ) or parts[0] == "asm"


def branches(lines: list[str]) -> dict[int, list[str]]:
    result: dict[int, list[str]] = {}
    stack: list[tuple[str, str]] = []
    number = 1
    while number <= len(lines):
        line = lines[number - 1]
        end = number
        m = re.match(r"\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)", line)
        if m:
            op, expr = m.group(1), m.group(2).strip()
            while expr.endswith("\\") and end < len(lines):
                end += 1
                expr = expr[:-1].rstrip() + " " + lines[end - 1].strip()
            if op in ("if", "ifdef", "ifndef"):
                stack.append((f"#{op} {expr}".rstrip(), f"#{op} {expr}".rstrip()))
            elif op == "elif" and stack:
                stack[-1] = (stack[-1][0], f"#elif {expr}")
            elif op == "else" and stack:
                stack[-1] = (stack[-1][0], f"#else of {stack[-1][0]}")
            elif op == "endif" and stack:
                stack.pop()
        for current in range(number, end + 1):
            result[current] = [state for _, state in stack]
        number = end + 1
    return result


def namespace_lines(lines: list[str]) -> dict[int, str] | None:
    """Recover namespace nesting from the project's namespace end comments.

    Ctags can lose a namespace when conditional function bodies have different
    braces. Fall back to Ctags for files whose namespace comments do not balance.
    """
    result = {}
    stack: list[str] = []
    mismatch = False
    for number, line in enumerate(lines, 1):
        code = line.split("//", 1)[0]
        code = re.sub(r'\b__micron_diagnostic_(?:push|pop)\b|\b__micron_diagnostic_ignored\s*\(\s*"[^"]*"\s*\)', '', code)
        opening = re.match(r"^\s*(?:inline\s+)?namespace\s+([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*(?:\{|$)", code)
        if opening:
            stack.extend(opening.group(1).split("::"))
        result[number] = "::".join(stack)
        closing = re.search(r"\}\s*;?\s*//\s*namespace\s*([\w:]*)", line)
        if closing:
            for part in reversed(closing.group(1).split("::") if closing.group(1) else [""]):
                if stack and (not part or stack[-1] == part):
                    stack.pop()
                else:
                    mismatch = True
        elif opening and "{" in code and code.count("{") == code.count("}"):
            del stack[-len(opening.group(1).split("::")):]
    return None if stack or mismatch else result


def repair_scope(scope: str, scope_kind: str, lexical: str) -> str:
    if not lexical:
        return scope
    if scope_kind in ("", "namespace"):
        return lexical
    actual = lexical.split("::")
    parsed = scope.split("::") if scope else []
    common = 0
    while common < min(len(actual), len(parsed)) and actual[common] == parsed[common]:
        common += 1
    if common < len(actual):
        return "::".join(actual + parsed[common:])
    return scope


def main() -> None:
    files = sorted(p for p in SOURCE.rglob("*") if p.is_file())
    headers = [p for p in files if p.suffix == ".hpp"]
    # Public declarations can be defined in an implementation-named header and
    # exported by an umbrella (for example allocator_traits and GL/Vulkan types).
    paths = [p.relative_to(ROOT).as_posix() for p in headers]
    command = [
        "ctags", "--options=NONE", "--output-format=json", "--language-force=C++",
        "--fields=+enaKS", "--kinds-C++=+pNA", "--extras=+F", "--sort=no", "-f", "-", "-L", "-",
    ]
    output = subprocess.run(command, cwd=ROOT, input="\n".join(paths) + "\n", text=True,
                            check=True, stdout=subprocess.PIPE).stdout
    tags = [json.loads(line) for line in output.splitlines() if line.startswith("{")]
    source_lines = {p: (ROOT / p).read_text(encoding="utf-8", errors="replace").splitlines() for p in paths}
    conditions = {p: branches(lines) for p, lines in source_lines.items()}
    lexical_namespaces = {p: namespace_lines(lines) for p, lines in source_lines.items()}
    namespaces: dict[str, list[tuple[int, int, str]]] = {}
    # Ctags can lose enclosing namespaces at architecture-specific branches.
    # These files have a single enclosing public namespace for their contents.
    scope_overrides = {
        "src/math/blas/level1.hpp": "micron::math::blas::level1",
        "src/math/blas/level2.hpp": "micron::math::blas::level2",
        "src/math/blas/level3.hpp": "micron::math::blas::level3",
        "src/math/quaternions/algebra.hpp": "micron::math::quaternions",
    }
    for tag in tags:
        if tag.get("kind") == "namespace" and tag.get("end"):
            scope = tag.get("scope", "")
            full = "::".join(filter(None, (scope, tag.get("name", ""))))
            namespaces.setdefault(tag["path"], []).append((tag["line"], tag["end"], full))

    accepted = {"class", "struct", "union", "enum", "enumerator", "typedef", "function",
                "prototype", "variable", "member", "macro", "name", "namespaceAlias"}
    symbols = []
    seen = set()
    for tag in tags:
        kind, name, path = tag.get("kind"), tag.get("name", ""), tag.get("path", "")
        if kind not in accepted or path not in source_lines or not name or name.startswith("__"):
            continue
        scope = tag.get("scope", "")
        line = tag.get("line", 0)
        lexical = lexical_namespaces[path]
        if lexical is not None:
            scope = repair_scope(scope, tag.get("scopeKind", ""), lexical.get(line, ""))
        expected = scope_overrides.get(path)
        if expected and (not scope or expected.startswith(scope + "::")):
            scope = expected
        if kind == "macro":
            scope = ""  # Preprocessor names are global, even inside a namespace.
        if tag.get("scopeKind") in ("function", "method") or tag.get("access") in ("private", "protected"):
            continue
        if any(part.startswith("__") or part.startswith("anon") for part in scope.split("::")):
            continue
        if scope and not (scope == "micron" or scope.startswith("micron::") or scope == "abc" or scope.startswith("abc::")):
            continue
        if kind == "macro" and not name.startswith(("MICRON_", "ABC_", "ABCMALLOC_")):
            continue
        if not scope and kind != "macro" and "/" in path.removeprefix("src/"):
            # Public C ABI entry points and the allocator's namespace are exceptions.
            if not path.startswith("src/attach/"):
                continue
        if not line or line > len(source_lines[path]):
            continue
        text = source_lines[path][line - 1].strip()
        if kind == "prototype" and ("concept " in text or text.startswith("requires")):
            continue
        normalized_name = re.sub(r"^operator (?=[^\w])", "operator", name)
        qualified = "::".join(filter(None, (scope, normalized_name)))
        signature = tag.get("signature", "")
        key = (path, line, qualified, signature)
        if key in seen:
            continue
        seen.add(key)
        symbols.append({"name": qualified, "kind": kind, "signature": signature,
                        "path": path.removeprefix("src/"), "line": line,
                        "conditions": conditions[path].get(line, [])})

    # Ctags does not emit C++ concepts. Find their names and recover their namespace
    # from the enclosing namespace ranges reported by Ctags.
    for path, lines in source_lines.items():
        for line, text in enumerate(lines, 1):
            match = re.match(r"\s*concept\s+([A-Za-z][A-Za-z_0-9]*)\b", text)
            if not match:
                continue
            candidates = [n for start, end, n in namespaces.get(path, []) if start <= line <= end]
            scope = max(candidates, key=lambda n: n.count("::"), default="")
            lexical = lexical_namespaces[path]
            if lexical is not None:
                scope = lexical.get(line, "")
            if scope and not (scope == "micron" or scope.startswith("micron::") or scope == "abc" or scope.startswith("abc::")):
                continue
            name = "::".join(filter(None, (scope, match.group(1))))
            if any(part.startswith("__") for part in scope.split("::")):
                continue
            symbols.append({"name": name, "kind": "concept", "signature": "", "path": path.removeprefix("src/"),
                            "line": line, "conditions": conditions[path].get(line, [])})

    # This public variable template is omitted when Ctags interprets its
    # requires-expressions as member declarations of Alloc instead.
    path = 'src/memory/allocation/allocator_types/__scheme.hpp'
    name = 'micron::allocator_traits::has_unsized_deallocate'
    if not any(item['name'] == name for item in symbols):
        for line, text in enumerate(source_lines[path], 1):
            if re.match(r'\s*static constexpr bool has_unsized_deallocate\b', text):
                symbols.append({'name': name, 'kind': 'variable', 'signature': '',
                                'path': path.removeprefix('src/'), 'line': line,
                                'conditions': conditions[path].get(line, [])})

    symbols.sort(key=lambda item: (item["path"], item["line"], item["name"], item["signature"]))
    inventory = {
        "headers": [{"path": p.relative_to(SOURCE).as_posix(), "internal": internal(p.relative_to(SOURCE).as_posix())}
                    for p in headers],
        "symbols": symbols,
    }
    OUT.write_text(json.dumps(inventory, ensure_ascii=False, separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"Recorded {len(headers)} headers and {len(symbols)} declaration entries in {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
