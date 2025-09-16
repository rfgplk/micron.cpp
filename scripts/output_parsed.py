import clang.cindex
import sys

def recurse(node, depth=0):
    loc = node.location
    if loc.file:
        print(" " * depth, node.kind, node.spelling, loc.file, loc.line)
    else:
        print(" " * depth, node.kind, node.spelling)
    for c in node.get_children():
        recurse(c, depth + 2)


if __name__ == "__main__":
    clang.cindex.Config.set_library_file("/usr/lib64/libclang.so")

    index = clang.cindex.Index.create()
    tu = index.parse(
        sys.argv[1],
        args=["-x", "c++", "-std=c++23"],
        options=clang.cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
    )

    recurse(tu.cursor)
