// regex_corpus.hpp
// Shared (pattern, input) corpus for differential testing the micron ERE engine
// against glibc's POSIX regexec. Plain C data so both the C oracle and the
// micron driver can include it. Patterns stick to portable POSIX ERE (no GNU
// backslash extensions like \w, which glibc would interpret but micron treats
// as a literal).
#pragma once

typedef struct {
  const char *pat;
  const char *in;
} regex_case;

static const regex_case REGEX_CORPUS[] = {
  // --- literals + search ---
  { "abc", "abc" },
  { "abc", "xabcy" },
  { "abc", "ab" },
  { "abc", "" },
  { "abc", "abcabc" },
  { "hello", "well hello there" },
  // --- dot ---
  { "a.c", "abc" },
  { "a.c", "a.c" },
  { "a.c", "ac" },
  { ".", "" },
  { ".", "a" },
  { "...", "ab" },
  { "a.c", "axcyc" },
  // --- star / plus / quest ---
  { "a*", "aaa" },
  { "a*", "" },
  { "a*", "baaa" },
  { "a+", "aaa" },
  { "a+", "" },
  { "a+", "baaa" },
  { "ab?c", "ac" },
  { "ab?c", "abc" },
  { "a*b*", "aabb" },
  { "a*b*", "bbb" },
  { "a*b*", "" },
  { "colou?r", "color" },
  { "colou?r", "colour" },
  // --- greedy ---
  { "a.*b", "axbxb" },
  { "a.*", "abcdef" },
  { ".*", "hello" },
  { "a.*c", "abcxc" },
  { "<.*>", "<a><b><c>" },
  // --- alternation (incl. length-ambiguous) ---
  { "a|b", "b" },
  { "a|b", "c" },
  { "cat|dog", "the dog" },
  { "a|ab", "ab" },
  { "ab|a", "ab" },
  { "a|ab|abc", "abc" },
  { "(a|b)*", "abba" },
  { "x(a|b)y", "xay" },
  { "(foo|bar|baz)", "say baz now" },
  // --- groups & captures ---
  { "(ab)(cd)", "abcd" },
  { "(a+)(b+)", "aaabb" },
  { "((a)(b))", "ab" },
  { "(a)(b)(c)", "abc" },
  { "(ab)+", "ababab" },
  { "(a|b)+", "abab" },
  { "(abc)?", "abc" },
  { "(abc)?", "xyz" },
  { "(a*)(b*)", "aabb" },
  { "(a*)(a*)", "aaa" },
  { "(a+)(a+)", "aa" },
  { "x(y(z))w", "xyzw" },
  // --- classes ---
  { "[abc]+", "cabbage" },
  { "[a-z]+", "Hello" },
  { "[A-Za-z]+", "Hello" },
  { "[0-9]+", "x123y" },
  { "[^0-9]+", "abc1" },
  { "[^a-z]+", "ABCdef" },
  { "[-a]+", "-a-a" },
  { "[a-]+", "a-a-" },
  { "[]a]+", "]a]a" },
  { "[[:digit:]]+", "ab12cd" },
  { "[[:alpha:]]+", "ab12cd" },
  { "[[:space:]]+", "a  \tb" },
  { "[[:upper:]]+", "ABCabc" },
  { "[x[:digit:]y]+", "x1y2z" },
  { "[a-fA-F0-9]+", "DeadBeef99zz" },
  // --- anchors ---
  { "^abc", "abcd" },
  { "^abc", "xabc" },
  { "abc$", "xabc" },
  { "abc$", "abcx" },
  { "^abc$", "abc" },
  { "^abc$", "abcd" },
  { "^$", "" },
  { "^$", "a" },
  { "^a*$", "aaa" },
  { "^a*$", "aab" },
  { "^[a-z]+$", "hello" },
  { "^[a-z]+$", "hello1" },
  // --- intervals ---
  { "a{3}", "aaa" },
  { "a{3}", "aa" },
  { "a{2,4}", "aaaaa" },
  { "a{2,}", "aaaa" },
  { "a{0,2}", "aaa" },
  { "a{0}", "aaa" },
  { "(ab){2}", "abab" },
  { "x{2,3}y", "xxxy" },
  { "[0-9]{3}", "ab1234" },
  // --- escapes ---
  { "a\\.b", "a.b" },
  { "a\\.b", "axb" },
  { "\\(x\\)", "(x)" },
  { "a\\*b", "a*b" },
  { "a\\+", "a+" },
  { "\\[", "[" },
  // --- empty / edge ---
  { "", "abc" },
  { "a|", "b" },
  { "(a|)b", "b" },
  { "(a|)b", "ab" },
  { "()", "x" },
  // --- adversarial (must stay linear, no hang) ---
  { "(a+)+$", "aaaaaaaaaaaaaaaaaaaa!" },
  { "(a*)*b", "aaaaaaaaaaaaaaaaaaaac" },
  { "(a|a)*c", "aaaaaaaaaaaaaaaab" },
  { "(.*)*x", "aaaaaaaaaaaaaaaaaaaa" },
  // --- realistic ---
  { "[a-z]+@[a-z]+", "send to user@host now" },
  { "(foo|bar)+", "foobarfoo" },
  { "a.c.e", "abcde" },
  { "[ -~]+", "Hello, World!" },
  { "[0-9]+\\.[0-9]+", "pi=3.14159" },
  { "(0x)?[0-9a-f]+", "0xdeadbeef" },
  // --- discriminating: POSIX longest-of-subexpression vs greedy ---
  { "(a|ab)(c|bcd)", "abcd" },
  { "(a|ab)(b|)", "ab" },
  { "(ab|a)(b|bc)", "abc" },
  { "(foo|foobar)(bar|baz)", "foobarbaz" },
  { "(a)?(b)", "b" },
  { "(a)(b)?", "a" },
  { "(a|(b))", "b" },
  { "(a|(b))", "a" },
  { "((a)|b)", "b" },
  { "(a+)(a+)", "aaaa" },
  { "(a*)(a*)(a*)", "aaa" },
  { "(.?)(.?)(.?)", "ab" },
  { "x(a*)(a*)y", "xaaay" },
  { "(a*)*", "aaa" },
  { "(ab)*", "ababab" },
  { "(a.)*", "axbycz" },
};

static const int REGEX_CORPUS_N = (int)(sizeof(REGEX_CORPUS) / sizeof(REGEX_CORPUS[0]));
