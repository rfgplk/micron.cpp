"""Serialization and terminal-control representation contracts."""


def section(title, *paragraphs, rows=None, columns=None):
    result = {"title": title, "paragraphs": list(paragraphs)}
    if rows is not None:
        result["table"] = {"columns": columns or ["Member", "Description"], "rows": rows}
    return result


GUIDES = {
    "serialization": {
        "summary": "Serialization provides recursive native-representation MFR1 frames, raw byte/object dumps, and a separate bounded canonical little-endian scalar/byte interface. These encodings have different portability and reconstruction requirements.",
        "sections": [
            section("Encoding selection", columns=["Interface", "Representation", "Use"], rows=[
                ["`framed_size`, `frame_into`, `unframe_from`", "Recursive MFR1 with native-endian counts and raw object representations.", "Reconstructing a known compatible container/type layout on the same representation model."],
                ["`frame(file, value)`, `unframe(file, out)`", "The same MFR1 encoding through an allocated intermediate buffer and a file object.", "File-backed framing when whole-remainder buffering is acceptable."],
                ["`serialize`, `deserialize`, `serialize_bytes`", "Raw bytes with no schema, count, or endian metadata.", "Explicitly controlled representation dumps or an independently specified byte protocol."],
                ["`canonical::writer`, `reader`", "Bounds-checked little-endian scalar fields and borrowed byte spans.", "Constructing a caller-defined portable field layout in existing storage."],
                ["`canonical::frame_bytes`, `unframe_bytes`", "Canonical MFR1 byte sequence with fixed header and little-endian length.", "Framing one byte span without serializing C++ container metadata."],
            ]),
            section("Native MFR1 grammar",
                "The outer header is eight bytes: magic MFR1, an endian flag, a kind byte, and two reserved zero bytes. The body is packed recursively. Strings store a native-endian `u64` byte length followed by character bytes. Contiguous containers of trivial leaf values store a `u64` element count and one representation block. Node chains and other iterable containers store a count followed by encoded elements; maps store a pair count followed by key/value encodings.",
                "Trivially copyable nonstring leaves store exactly `sizeof(T)` bytes, including representation/padding bytes. There is no field schema, element type id, numeric conversion, pointer relocation, authentication, or endian conversion. Both ends must agree on the type and representation. The writer emits kind 1 for detected maps and 0 otherwise, including sets; the decoder validates magic and endian truth value but ignores kind, reserved bytes, and other flag bits.",
                "A node-chain interface is detected through `ibegin()->next/data`. Maps are detected through key/value or pair a/b iterators plus insertion support. Key/value iterators with false-valued keys are skipped while the header count still comes from `size()`; a generic map permitting such a key can therefore produce an inconsistent stream. Hash-tagged map reconstruction uses `insert_unhash` where available. Iterable and map reconstruction requires default-constructible elements and a supported push/insert interface."),
            section("Framing and reconstruction",
                "`framed_size(value)` traverses the value to compute header plus body bytes without writing. `frame_into(dst, capacity, value)` writes the header then body and returns bytes produced or negative invalid-argument on null/insufficient storage. A failed frame can leave a partially written prefix. Size and capacity arithmetic in this generic path is not uniformly overflow-checked; counts and total encoded size must fit `usize` and the returned signed `max_t`.",
                "`unframe_from(src, length, out)` returns consumed bytes rather than requiring the complete input span to be consumed; trailing frames or unrelated bytes can remain. It checks source bounds at individual reads and rejects mismatched endian, but modifies `out` incrementally with no rollback. Different destination categories reserve/set length, clear and push, overwrite existing indexed elements, or reconstruct a fresh value. Fixed-size contiguous destinations copy only their available prefix and skip excess encoded elements.",
                "For multibyte character strings, a byte length not divisible by the character width reconstructs whole character units and skips the trailing partial unit. It does not validate a text encoding. Recursive template depth is limited to 64; exceeding it is a compile-time assertion on the nested type, not a runtime graph-depth check. Cyclic node chains are not detected.",
                "Reconstruction of raw leaves and contiguous leaf elements requires the internal `mfr_pod_safe` rule: arithmetic types other than bool, or an explicit `mfr_pod_safe_tag`. Trivially copyable alone is insufficient because pointer, bool, enum, or opaque representations may be invalid. Adding the tag asserts that every bit pattern is a valid value; the decoder does not verify that assertion. Container allocation and element operations run inside noexcept generic helpers and cannot propagate an escaping exception through them."),
            section("File interfaces and raw dumps",
                "`frame(file, value)` measures, allocates a `buffer`, encodes, and forwards one complete span to `file::write` at its current position. The return is that file operation's result; check it against the encoded size when completeness matters. `unframe(file, out)` obtains the current offset, refreshes cached metadata, allocates the entire reported remainder, reads it, and decodes the bytes obtained. It does not perform an incremental length-delimited stream parse or return unread remainder bytes to the descriptor position.",
                "The value-returning `unframe<C>(file)` default-constructs `C` and returns `option<C, error_t>`, with data in the first alternative and a positive-code error object in the second. Allocation and metadata/position requirements make this interface unsuitable for an arbitrary unbounded pipe stream.",
                "`serialize_bytes(file, ptr, n)` counts bytes even for its nonbyte pointer overload. The `Chunk` template parameter is unused; it does not implement chunked output. `serialize(file, object)` writes `sizeof(T)` raw representation bytes. `deserialize` reads directly into the destination and applies the same POD-safe assertion; a short read can leave only a prefix updated, so success requires the complete expected byte count. `serialize_page` aligns the object's address downward and writes the entire default-size containing page, including bytes outside the object; that complete page must be readable and intentionally selected for output."),
            section("Complexity and lifetime",
                "Generic encoding/decoding performs linear byte-copy work plus traversal, element construction, and insertion costs. Linked generic count passes may traverse a container again; file framing also measures before encoding. Map reconstruction inherits its map's insertion complexity rather than guaranteeing linear total work. Intermediate file buffers require space proportional to encoded/remainder bytes; buffer forms use caller-provided storage.",
                "Source containers cannot be mutated during measuring/encoding. Borrowed buffers must remain valid for the call, and output storage must not overlap source state in ways that encoding or reconstruction can overwrite unread input. The canonical byte-view decoder is the explicit zero-copy path; generic container decoding ordinarily copies or constructs owned values. No interface supplies synchronization or rollback after a partially modified output."),
        ],
        "types": [
            {
                "name": "micron::io::serialize::canonical::writer", "header": "io/canonical.hpp",
                "summary": "A non-owning bounded cursor that appends explicitly encoded fields to a mutable byte span.",
                "sections": [
                    section("Storage and operations", "Construction retains a pointer and capacity; a null pointer with nonzero capacity is normalized to zero usable capacity. `position()` counts bytes successfully written and `remaining()` is capacity minus position. The writer allocates nothing and does not own or extend buffer lifetime. Copying copies cursor position and the borrowed pointer, not bytes.", rows=[
                        ["`put_u8`, `put_u16`, `put_u32`, `put_u64`", "Write unsigned values in little-endian order. The constrained generic `put_unsigned` accepts u16/u32/u64 only."],
                        ["`put_i64`", "Converts to u64 and writes the resulting 64-bit representation."],
                        ["`put_f64`", "Bit-casts f64 to u64 and writes its representation; no decimal or range conversion occurs."],
                        ["`put_bytes(source, count)`", "Copies exactly count bytes without a length prefix. Rejects a null nonempty source or insufficient remaining space."],
                    ]),
                    section("Failure and complexity", "Each put returns bool. Insufficient space returns false without advancing position or writing that field; prior fields remain. Scalar puts take constant work, bytes take linear work in count. Constant evaluation uses byte loops; runtime uses representation copies and byte swaps on a big-endian host. Source/destination byte spans must satisfy the underlying non-overlapping copy contract. Concurrent copied writers sharing one buffer require external coordination."),
                ],
            },
            {
                "name": "micron::io::serialize::canonical::reader", "header": "io/canonical.hpp",
                "summary": "A non-owning bounded cursor decoding little-endian fields from a constant byte span.",
                "sections": [
                    section("Reads and views", "Construction normalizes a null nonempty span to zero size. `get_u8/u16/u32/u64` decode scalar fields, `get_i64` bit-casts decoded u64 bits, and `get_f64` bit-casts them to f64. A failed scalar read returns false without advancing the cursor or changing the destination value. All complete representations are accepted; no numeric/schema validation is added.", "`get_bytes(count, view)` advances and returns a borrowed `byte_view` rather than copying bytes or reading an encoded count. On insufficient bytes it leaves the cursor and view unchanged. A zero-byte view can have null data for an empty/null source. Copying the reader copies its current position; copies can independently traverse the same immutable bytes."),
                    section("Complexity and lifetime", "Scalar operations and forming a byte view take constant work; no allocation occurs. Position/remaining count bytes. Views and readers must not outlive their source span or access it during conflicting mutation. The cursor provides no rewind, variable-integer decoding, string conversion, or automatic field schema."),
                ],
            },
            {
                "name": "micron::io::serialize::canonical::byte_view", "header": "io/canonical.hpp",
                "summary": "A borrowed constant byte span described by `data` and byte `size`.",
                "sections": [
                    section("Canonical byte frames", "`byte_frame_overhead` is 16: an exact eight-byte MFR1 header followed by a little-endian u64 byte count. `byte_frame_size(count)` validates signed-result/size bounds and returns total bytes or negative invalid-argument. `frame_bytes` validates the complete destination/source spans before writing and returns that total; it allocates nothing.", "`unframe_bytes(source, count, view, maximum_bytes)` requires the exact canonical header, checks encoded count against both the supplied maximum and available bytes, and returns consumed frame bytes while assigning a view into source + 16. Trailing bytes are allowed. Failure leaves the caller's view unchanged. Unlike generic MFR1 decoding, this interface is explicitly little-endian on every host and copies no payload during decoding."),
                    section("Lifetime", "A view owns no bytes. Copying it copies the pointer/count. Mutating, reallocating, unmapping, or destroying its source affects all views; the caller retains the source until the last use. Forming/decoding a view takes constant work; encoding copies a linear number of payload bytes."),
                ],
            },
        ],
        "sources": ["io/__serial_core.hpp", "io/serial.hpp", "io/canonical.hpp"],
    },
    "terminal": {
        "summary": "Terminal support provides indexed escape-sequence templates and Linux terminal-attribute records. Selecting a constant does not emit output, negotiate terminal capabilities, or restore changed terminal state.",
        "entry_overrides": {"micron::ansi": "Contains indexed constant strings for terminal controls, attributes, reports, character sets, and mouse modes. Parameterized strings retain literal formatting placeholders until formatted by the caller."},
        "sections": [
            section("Sequence tables", "Each unscoped code enum is an index into its matching constant string array, rather than the terminal's numeric command value. For example `sgr_code::fg_red` indexes the stored ESC[31m string; it is not itself SGR parameter 31. Every family has a count sentinel that is not an array entry. Table strings have static lifetime and are not allocated.", columns=["Index enum / table", "Control family and parameters"], rows=[
                ["`c0_code` / `C0`", "Single C0 controls and DEL. The nul entry contains a zero byte, which is empty to null-terminated-string length functions."],
                ["`c1_code` / `C1`", "Selected C1 controls represented by seven-bit ESC sequences, including CSI/OSC/DCS openers and string termination."],
                ["`fe_code` / `FE`", "Transmission/conformance selection, DEC cursor save/restore, keypad, character shifts, and index operations."],
                ["`csi_code` / `CSI`", "Cursor movement, tabs, erase/insert/delete, scrolling, regions/rectangles, status/device reports, modes, cursor style, and window operations."],
                ["`sgr_code` / `SGR`", "Text style/reset controls; basic/bright colors; indexed color and RGB templates for foreground, background, and underline."],
                ["`osc_code` / `OSC`", "Window/icon title, palette/default colors, fonts, clipboard, hyperlinks, notifications, and shell integration; templates use BEL termination."],
                ["`charset_code` / `CHARSET`", "G0/G1/G2/G3 character-set selection, including ASCII and line-drawing sets."],
                ["`dcs_code` / `DCS`", "DCS open/close, sixel, settings requests, programmable keys/fonts, and tmux passthrough templates."],
                ["`pm_code` / `PM`, `apc_code` / `APC`", "Privacy/application-control open/close strings and a kitty-graphics payload template."],
                ["`mouse_code` / `MOUSE`", "Paired enable/disable strings for X10, normal/button/any motion, focus, UTF-8, SGR, URXVT, and pixel modes."],
                ["`misc_code` / `MISC`", "Cursor visibility/keys, width, scrolling/video/origin/wrap/repeat, keypad/backarrow, local echo, alternate screen, paste, synchronized update, grapheme, and resize modes."],
            ]),
            section("Parameters, output, and validity", "Parameterized sequences contain literal `{}` placeholders. Select and format the complete template with an appropriate formatter before writing; plain printing of that string does not substitute its parameters. Cursor/region coordinates in the documented templates use the terminal protocol's one-based convention. RGB and indexed colors are templates, not range-checked color objects.", "The tables include ECMA/DEC controls and terminal-specific extensions. Presence in a table does not establish support by a given terminal. Report-request strings only request a response; this header supplies no reply parser. OSC/DCS/APC text payloads are not escaped, base64-encoded, or sanitized by the constants. Operations such as clipboard_set expect the already encoded payload indicated by their template.", "Indexing a valid enum into its table takes constant work. Formatting/output costs depend on the selected formatter and byte length. Concurrent output can interleave escape sequences unless the output path supplies an enclosing synchronization protocol. The constants themselves are immutable and require no runtime initialization."),
            section("Linux attributes", "`terminal_getattr<Fd>` and `terminal_setattr<Fd>` call Linux TCGETS/TCSETS ioctl on a compile-time descriptor, default 1 (stdout); select 0 explicitly for stdin. They return 0 on success or the negative ioctl error unchanged. Set always uses TCSETS: the TCSANOW/TCSADRAIN/TCSAFLUSH constants do not select a waiting mode for this function.", "The caller owns saving/restoring terminal state. These helpers neither close the descriptor nor make a scoped restoration guard. File descriptors must represent an appropriate terminal, and ordinary stream buffering remains independent of the terminal attribute record."),
        ],
        "types": [
            {
                "name": "micron::ansi::group", "header": "io/term/ansi.hpp",
                "summary": "A scoped classification enum naming the terminal-control table families.",
                "covers": ["micron::ansi::c0_code", "micron::ansi::c1_code", "micron::ansi::fe_code", "micron::ansi::csi_code", "micron::ansi::sgr_code", "micron::ansi::osc_code", "micron::ansi::dcs_code", "micron::ansi::pm_code", "micron::ansi::apc_code", "micron::ansi::charset_code", "micron::ansi::mouse_code", "micron::ansi::misc_code"],
                "sections": [section("Values and indexed code enums", "Values are c0, c1, fe, csi, sgr, osc, dcs, pm, apc, charset, mouse, and misc, followed by internal `__end`. The header provides no function mapping `(group, index)` to an erased sequence; select the corresponding typed index enum and array directly. Each related code enum and table is described in the module table above.", "All code enums use `usize` as their underlying type. They are table selectors, not bit flags. Do not use their count sentinels as indices, combine unrelated families, or assume enum ordinals equal the numeric escape parameter.")],
            },
            {
                "name": "micron::posix::termios_t", "header": "io/os/term.hpp",
                "summary": "A Linux terminal-attribute value record used by the terminal ioctl wrappers.",
                "covers": ["micron::posix::tcflag_t", "micron::posix::cc_t", "micron::posix::speed_t"],
                "sections": [section("Fields and constants", rows=[
                    ["`c_iflag`, `c_oflag`, `c_cflag`, `c_lflag`", "Input, output, control, and local mode bitfields of `tcflag_t` (unsigned int)."],
                    ["`c_line`", "Line-discipline byte of `cc_t` (unsigned char)."],
                    ["`c_cc[32]`", "Control-character array indexed by VINTR/VQUIT/VERASE/etc., including VTIME and VMIN."],
                    ["`c_ispeed`, `c_ospeed`", "Input/output speed values of `speed_t` (unsigned int)."],
                ]), section("Use and lifetime", "The header supplies Linux numeric constants for character slots, input/output/local/control modes, baud values through B38400, and attribute-action values. The record performs no validation, synchronization, initialization syscall, or automatic application. Value-initialize it or populate it through `terminal_getattr` before changing fields and applying it. Copying copies the record only; it does not change terminal state. Each get/set performs one ioctl with constant wrapper storage/work.")],
            },
        ],
        "sources": ["io/term/ansi.hpp", "io/os/term.hpp"],
    },
}
