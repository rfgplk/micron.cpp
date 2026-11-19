# micron reference

Open [index.html](index.html) directly in a browser. Navigation, the header
index, and the symbol index do not require a server, an internet connection,
or JavaScript. Search uses a bundled JavaScript index and also works from a
`file://` URL.

The function reference includes ordinary functions, overloads, constructors,
destructors, conversion operators, special members, generated functions, and
conditional backend definitions. Each definition has a declaration, parameters,
source-equivalent effects, return sites, and explicit assertion or diagnostic
sites where present. The equivalent definitions are taken from the actual
source, including initialization and compile-time branches. They are not inferred
from the function's name. Function-generating macros are expanded with the C++
preprocessor and retain links to their macro definitions and invocation sites.

Module pages provide orientation and additional prose contracts. Function
entries and complete, numbered source snapshots cover the remaining interfaces,
including implementation functions and compiler support headers. Declaration-only
host and compiler entry points are identified as declarations; a source
declaration is not used to invent effects that its body does not specify.

## Editing

* Edit the module descriptions and selected interfaces in `content/modules.py`.
* Edit the more detailed contracts in `content/details.py` and complete example
  programs in `examples/`.
* Run `python3 docs/reference/tools/build.py` from the repository root. Commit
  the changed content and generated pages together. Run the same command with
  `--check` to verify that the output is current.
* When source changes, refresh both inventories before building. Install
  `tools/requirements.txt` in a Python virtual environment, then run
  `python docs/reference/tools/update_inventory.py` and
  `python docs/reference/tools/update_functions.py` in that environment.
  Refreshing requires Universal Ctags with JSON support and a `g++` preprocessor.
  The callable pass reads the syntax tree and reconciles its definitions against
  an independent Ctags pass. Review the resulting evidence against the source,
  especially when changing compiler syntax or declaration-generating macros.
  Run `python docs/reference/tools/test_extraction.py` for extraction regressions.
* Run `python3 docs/reference/tools/check.py` to validate the generated site,
  local resources, links, anchors, source hashes, function evidence, inventory
  reconciliation, and representative symbols.
  This check also uses Node.js to check the search script's syntax.
* With Node.js 22 or later and Chromium installed, run
  `node docs/reference/tools/browser_check.js` for an offline browser smoke
  test of navigation, layout, search, and the script-free symbol index.

The generated reference is a snapshot of the current checkout. Update it when
public declarations or their behavior change. See `licenses/NOTICE.txt` for
asset provenance and license information.
