#!/usr/bin/env python3
"""Regression checks for source evidence and callable extraction."""
import sys
import unittest

sys.dont_write_bytecode = True
from update_functions import PARSER, branches, entry, facts, function_declarator, parser_input, walk
from macro_functions import expand_macros


def extract(source):
    raw = source.encode()
    result = []
    for node in walk(PARSER.parse(parser_input(raw)).root_node):
        if node.type == 'function_definition':
            fd = function_declarator(node.child_by_field_name('declarator'))
            if fd:
                item = entry(raw, node, fd, 'example.hpp', {}, None, {})
                if item:
                    result.append(item)
    return result


class SourceEvidenceTests(unittest.TestCase):
    def test_conversion_qualifiers(self):
        source = 'struct V { explicit operator const char*() const { return p; } operator T&() & { return value; } };'
        entries = extract(source)
        self.assertEqual([item['name'] for item in entries], ['V::operator const char*', 'V::operator T&'])
        self.assertEqual(entries[0]['signature'], '() const')
        self.assertEqual(entries[1]['signature'], '() &')

    def test_template_conversion(self):
        item = extract('struct V { constexpr operator view<F>() const noexcept { return {x}; } };')[0]
        self.assertEqual(item['name'], 'V::operator view<F>')

    def test_out_of_class_definition(self):
        item = extract('namespace micron { void worker::join() { wait(); } }')[0]
        self.assertEqual(item['name'], 'micron::worker::join')
        self.assertEqual(item['leaf'], 'join')

    def test_special_members(self):
        items = extract('struct V { V() = default; V(const V&) = delete; ~V() {} };')
        self.assertEqual([item['kind'] for item in items], ['defaulted', 'deleted', 'definition'])

    def test_qualifier_macros_preserve_offsets(self):
        source = '__inline_neon float32x4_t load(const float *p) noexcept { return vld1q_f32(p); }'
        item = extract(source)[0]
        self.assertEqual(item['name'], 'load')
        self.assertEqual(item['returns'][0]['expression'], 'vld1q_f32(p)')
        self.assertEqual(source[item['body_start']:item['body_end']], '{ return vld1q_f32(p); }')

    def test_return_in_callback_is_not_outer_return(self):
        item = extract('int f(int x) { auto fn = [] { return 10; }; if (x) return fn(); return 0; }')[0]
        self.assertEqual([site['expression'] for site in item['returns']], ['fn()', '0'])
        self.assertEqual(item['returns'][0]['guards'], ['(x)'])

    def test_source_diagnostic_and_guard(self):
        item = extract('void f(int n) { if (n < 0) micron::exc<error>("negative"); }')[0]
        self.assertEqual(item['failures'][0]['expression'], 'micron::exc<error>("negative")')
        self.assertEqual(item['failures'][0]['guards'], ['(n < 0)'])

    def test_consteval_and_type_traits_keep_original_evidence(self):
        source = 'bool f() { if !consteval { if constexpr (__is_same(T, char)) return true; } return false; }'
        item = extract(source)[0]
        self.assertEqual(len(parser_input(source.encode())), len(source.encode()))
        self.assertEqual([site['expression'] for site in item['returns']], ['true', 'false'])
        self.assertIn('if !consteval', item['returns'][0]['guards'])
        self.assertIn('(__is_same(T, char))', item['returns'][0]['guards'])

    def test_multiline_conditional(self):
        source = ['#if defined(A) \\', ' || defined(B)', 'void f();', '#else', 'void g();', '#endif']
        gates = branches(source)
        self.assertEqual(gates[3], ['#if defined(A) || defined(B)'])
        self.assertEqual(gates[5], ['#else of #if defined(A) || defined(B)'])

    def test_macro_generated_overloads(self):
        source = '#define OP(NAME) inline int NAME(int x) { return x + 1; } inline long NAME(long x) { return x - 1; }\nOP(next)\n'
        expansions, names = expand_macros(source.encode(), 'example.hpp', PARSER, walk, function_declarator, parser_input)
        self.assertEqual(names, {'OP'})
        self.assertEqual(len(expansions), 1)
        entries = extract(expansions[0]['expanded'])
        self.assertEqual([item['name'] for item in entries], ['next', 'next'])
        self.assertEqual([item['returns'][0]['expression'] for item in entries], ['x + 1', 'x - 1'])

    def test_both_conditional_macro_definitions(self):
        source = '#if defined(STATS)\n#define GET(N) int N() { return count; }\n#else\n#define GET(N) int N() { return 0; }\n#endif\nGET(value)\n'
        expansions, _ = expand_macros(source.encode(), 'example.hpp', PARSER, walk, function_declarator, parser_input)
        self.assertEqual(len(expansions), 2)
        self.assertEqual([extract(item['expanded'])[0]['returns'][0]['expression'] for item in expansions], ['count', '0'])
        self.assertEqual(expansions[0]['macro_conditions'], ['#if defined(STATS)'])


if __name__ == '__main__':
    unittest.main()
