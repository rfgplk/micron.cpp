#!/usr/bin/env python3
import re
import pathlib

# Matches: throw TYPE("any string");
# - TYPE can include :: and >
# - arbitrary spaces around throw, type, and (
# - string content can include anything except unescaped quotes
pattern = re.compile(
    r'throw\s+'                  # throw + spaces
    r'([a-zA-Z0-9_:<>]+)\s*'     # capture full type (namespaces, template <>, etc.)
    r'\(\s*"'                    # open paren + optional spaces + opening quote
    r'((?:[^"\\]|\\.)*)'         # string content (handles escaped quotes)
    r'"\s*\)\s*;'                # closing quote + optional spaces + closing paren + semicolon
)

def replacer(match):
    type_name = match.group(1)
    string_content = match.group(2)
    return f'throw except::exc<{type_name}>("{string_content}");'

root = pathlib.Path("src")

for file_path in root.rglob("*"):
    if file_path.is_file():
        text = file_path.read_text(encoding="utf-8")
        new_text = pattern.sub(replacer, text)
        if new_text != text:
            file_path.write_text(new_text, encoding="utf-8")
