"""Value-oriented bit operations in the public bits umbrella."""

GUIDES = {"bitops": {
    "summary": "Value-oriented masks, rotations, bit counts, byte operations, alignment arithmetic, and extraction/deposition helpers in micron.",
    "sections": [
        {"title": "Operation families", "table": {"columns": ["Family", "Behavior and selection"], "rows": [
            ["`mask_low`, `mask_high`, `bits_range`, `deposit_range`", "Construct or extract contiguous bit fields; deposit preserves destination bits outside the selected range."],
            ["`set_bit`, `clear_bit`, `toggle_bit`, `assign_bit`, `copy_bit`, `swap_bits`", "Return an edited value. Template-position and runtime-position forms do not mutate an argument by reference."],
            ["`rotl`, `rotr`, width-suffixed rotations", "Rotate a value; the generic form reduces its shift modulo the type's bit width and handles zero explicitly."],
            ["`bitcount` / `popcount`, `countl_*`, `countr_*`, `bit_width`", "Count set bits or leading/trailing zeros/ones. Zero returns the full type width for zero counts and width zero for bit_width."],
            ["`bit_floor`, `bit_ceil`, `log2_floor`, `log2_ceil`", "Power-of-two bounds and exponents. bit_ceil returns zero if its computed exponent cannot fit; log2_floor(0) is -1."],
            ["`lowest_set_bit`, `highest_set_bit`, `nth_set_bit`, fill/smear helpers", "Select a set-bit mask or fill below one. nth_set_bit uses a zero-based ordinal and repeatedly clears lower set bits."],
            ["`reverse_bits`, `reverse_bits_per_byte`, `bswap`, `swap_bytes`", "Reverse the whole representation's bit order, reverse bits independently within bytes, reverse byte order, or exchange selected bytes."],
            ["`spread_bits_16`, `compact_bits_16`, `interleave_bits`, `deinterleave_bits`", "Convert two 16-bit coordinates to/from alternating positions in a 32-bit Morton-style code."],
            ["`pext`, `pdep`", "Pack bits selected by a mask into low positions or deposit low source bits into the mask positions, using scalar loops."],
        ]}},
        {"title": "Types and preconditions", "paragraphs": [
            "These templates mostly constrain is_arithmetic_v rather than the narrower unsigned-integral domain required by their operators. An admitted floating type can still fail on bitwise instantiation. Use suitable unsigned integer types for masks, shifts, rotations, and overflow helpers; the implementations do not generally establish defined signed-overflow behavior.",
            "Bit and byte indices must select existing positions, ranges must fit, and shift counts must be valid. Negative counts are not normalized by mask/range helpers. Generic rotations mask their counts, but 32/64-bit fallback formulas and the 8/16-bit variants have different zero/full-width shift boundaries. Builtin availability can therefore change those edge cases; use generic rotl/rotr for their explicit zero handling.",
            "align_up/align_down require a nonzero power-of-two alignment, with representable addition for align_up. aligned_256, aligned_64, aligned_32, and aligned_16 use bit counts in their names: they test 32-, 8-, 4-, and 2-byte alignment. aligned(ptr) tests the pointed-to type's alignment. None accesses the memory.",
            "reverse_bits uses reinterpret_cast byte access and a 256-entry table; declaring it constexpr does not make that cast available in ordinary C++ constant evaluation. bswap handles widths 1/2/4/8 only. The endian conversion helpers assume the library's little-endian targets: big-endian conversion swaps, little-endian conversion returns the original value."
        ]},
        {"title": "Related arithmetic and byte predicates", "paragraphs": [
            "repeat_bytes repeats a byte pattern across word. has_zero reports whether any byte is zero, and byte_eq applies that test to XOR, reporting whether any corresponding byte matches. It does not compare entire values for equality. get_byte shifts by the selected byte position but does not mask away higher bytes.",
            "flip XORs only the low eight bits; invert complements the full value. saturate_bits masks to the low n bits rather than clamping a numerical magnitude. saturating_add/sub and overflow predicates use unsigned-style formulas. Narrow integer promotion and signed arithmetic still need suitable operand/range choices; for explicit checked arithmetic, consult the math::arith functions.",
            "merge_bits selects b where mask is set and a elsewhere; select chooses between complete values. average/average_ceil use bitwise rounded-average identities. sign_extend takes the zero-based sign-bit position b, not a bit count, and assumes bits above the selected representation have been prepared appropriately."
        ]},
        {"title": "Complexity", "paragraphs": [
            "Fixed-width scalar masks, edits, rotations, counts, and swaps use a bounded number of local operations/builtins and allocate no memory. Actual generated instructions depend on target flags and compiler lowering. reverse_bits and reverse_bits_per_byte perform O(sizeof(T)) byte steps; nth_set_bit performs O(n) clears, and pext/pdep perform O(popcount(mask)) iterations. The latter are scalar implementations even when a target ISA has similarly named hardware instructions."
        ]},
    ],
    "types": [],
    "sources": ["bits.hpp"],
}}
