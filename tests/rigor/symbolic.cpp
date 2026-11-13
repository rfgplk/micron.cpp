//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include <micron/math/symbolic.hpp>
#include <snowball/snowball.hpp>

namespace sym = micron::math::symbolic;

namespace
{

sym::node_id
ok(sym::result value)
{
  sb::require(value.code, sym::error::none);
  sb::require_true(static_cast<bool>(value));
  return value.id;
}

void
require_integer(const sym::arena &owner, sym::node_id id, i64 expected)
{
  const auto *q = owner.coefficient(id);
  sb::require_true(q != nullptr);
  sb::require_true(q->is_integer());
  sb::require_true(q->numerator() == expected);
}

struct output {
  char bytes[4096]{};
  usize length = 0;
  usize capacity = 4096;

  bool
  operator()(const char *source, usize count)
  {
    if ( count > capacity - length ) return false;
    for ( usize i = 0; i < count; ++i ) bytes[length++] = source[i];
    return true;
  }

  template<usize N>
  bool
  equal(const char (&text)[N]) const
  {
    if ( length != N - 1 ) return false;
    for ( usize i = 0; i < length; ++i )
      if ( bytes[i] != text[i] ) return false;
    return true;
  }
};

}      // namespace

int
main()
{
  sb::test_case("immutable DAG interning stable views and independent owned clones");
  {
    sym::arena owner;
    const auto x = ok(owner.symbol("x", sym::assumptions::real));
    const auto y = ok(owner.symbol("y"));
    const auto *view = owner.get(x);
    const auto name = owner.symbol_name(x);
    sb::require(ok(owner.symbol("x", sym::assumptions::real)), x);
    sb::require(owner.symbol("x", sym::assumptions::positive).code, sym::error::assumption_conflict);
    sb::require(ok(owner.add(x, y)), ok(owner.add(y, x)));
    const auto square = ok(owner.multiply(x, x));
    sb::require(owner.get(square)->type, sym::kind::power);
    sb::require(owner.get(square)->exponent, i64{ 2 });
    for ( i64 i = 0; i < 512; ++i ) ok(owner.integer(i));
    sb::require_true(view == owner.get(x));
    sb::require(name.data[0], 'x');
    sym::expression original(micron::move(owner), square), copied;
    sb::require(original.clone_to(copied), sym::error::none);
    sb::require_true(copied.valid());
    sb::require(copied.root(), original.root());
    sb::require_true(copied.owner().get(copied.root()) != original.owner().get(original.root()));
    output text;
    sb::require(copied.write_csexp(text), sym::error::none);
    sb::require_true(text.equal("(math.pow x 2)"));
  }
  sb::end_test_case();

  sb::test_case("simplifications require their mathematical assumptions");
  {
    sym::arena owner;
    const auto x = ok(owner.symbol("x", sym::assumptions::real));
    const auto z = ok(owner.symbol("z"));
    const auto positive = ok(owner.symbol("p", sym::assumptions::positive));
    sb::require(owner.get(ok(owner.divide(x, x)))->type, sym::kind::divide);
    require_integer(owner, ok(owner.divide(positive, positive)), 1);
    const auto root = ok(owner.function(sym::kind::sqrt, ok(owner.power(x, 2))));
    sb::require(owner.get(root)->type, sym::kind::abs);
    sb::require(owner.get(ok(owner.function(sym::kind::sqrt, ok(owner.power(z, 2)))))->type, sym::kind::sqrt);
    sb::require(ok(owner.function(sym::kind::exp, ok(owner.function(sym::kind::log, positive)))), positive);
    sb::require(ok(owner.function(sym::kind::log, ok(owner.function(sym::kind::exp, x)))), x);
    sb::require(owner.get(ok(owner.function(sym::kind::log, ok(owner.function(sym::kind::exp, z)))))->type, sym::kind::log);
    sb::require(owner.function(sym::kind::log, ok(owner.integer(0))).code, sym::error::domain);
    sb::require(owner.function(sym::kind::sqrt, ok(owner.integer(-1))).code, sym::error::domain);
    sb::require(owner.divide(x, ok(owner.integer(0))).code, sym::error::domain);
    sb::require(owner.substitute(positive, positive, ok(owner.integer(0))).code, sym::error::assumption_conflict);
  }
  sb::end_test_case();

  sb::test_case("exact derivative product quotient chain and polynomial antiderivative");
  {
    sym::arena owner;
    const auto x = ok(owner.symbol("x", sym::assumptions::real));
    const auto cube = ok(owner.power(x, 3));
    const auto polynomial = ok(owner.add(cube, ok(owner.multiply(ok(owner.integer(4)), x))));
    const auto derivative = ok(owner.differentiate(polynomial, x));
    for ( i64 value = -10; value <= 10; ++value ) {
      const auto evaluated = ok(owner.substitute(derivative, x, ok(owner.integer(value))));
      require_integer(owner, evaluated, 3 * value * value + 4);
    }
    const auto affine_square = ok(owner.power(ok(owner.add(x, ok(owner.integer(1)))), 2));
    const auto primitive = ok(owner.integrate(affine_square, x));
    const auto recovered = ok(owner.differentiate(primitive, x));
    for ( i64 value = -4; value <= 4; ++value )
      require_integer(owner, ok(owner.substitute(recovered, x, ok(owner.integer(value)))), (value + 1) * (value + 1));
    const auto quotient = ok(owner.divide(cube, ok(owner.add(x, ok(owner.integer(1))))));
    const auto quotient_derivative = ok(owner.differentiate(quotient, x));
    const auto actual = ok(owner.substitute(quotient_derivative, x, ok(owner.integer(2))));
    const auto *q = owner.coefficient(actual);
    sb::require_true(q != nullptr);
    sb::require_true(*q == micron::math::make_rational(28, 9).value);
    const auto sine = ok(owner.function(sym::kind::sin, ok(owner.power(x, 2))));
    const auto chained = ok(owner.differentiate(sine, x));
    require_integer(owner, ok(owner.substitute(chained, x, ok(owner.integer(0)))), 0);
  }
  sb::end_test_case();

  sb::test_case("unsupported calculus is explicit and formatting is bounded");
  {
    sym::arena owner;
    const auto x = ok(owner.symbol("x", sym::assumptions::real));
    const auto absolute = ok(owner.function(sym::kind::abs, x));
    sb::require(owner.get(ok(owner.differentiate(absolute, x)))->type, sym::kind::derivative);
    const auto sine = ok(owner.function(sym::kind::sin, x));
    const auto integral = ok(owner.integrate(sine, x));
    sb::require(owner.get(integral)->type, sym::kind::integral);
    sb::require(ok(owner.differentiate(integral, x)), sine);
    output text;
    sb::require(owner.write_csexp(integral, text), sym::error::none);
    sb::require_true(text.equal("(sym.integral (math.sin x) x)"));
    output short_output;
    short_output.capacity = 3;
    sb::require(owner.write_csexp(integral, short_output), sym::error::output_limit);
    const auto escaped = ok(owner.symbol("a b\"c"));
    output escaped_output;
    sb::require(owner.write_csexp(escaped, escaped_output), sym::error::none);
    sb::require_true(escaped_output.equal("(sym.symbol \"a b\\\"c\")"));
    sb::require(owner.differentiate(x, ok(owner.integer(2))).code, sym::error::invalid_variable);
    sb::require(owner.add(x, sym::invalid_node).code, sym::error::invalid_node);
  }
  sb::end_test_case();

  sb::test_case("node depth symbol text polynomial and rewrite budgets reject work");
  {
    sym::limits configuration;
    configuration.max_nodes = 2;
    sym::arena tiny(configuration);
    const auto x = ok(tiny.symbol("x"));
    const auto one = ok(tiny.integer(1));
    sb::require(tiny.add(x, one).code, sym::error::node_limit);
    sb::require(ok(tiny.integer(1)), one);
    sb::require_true(tiny.get(x) != nullptr);
    sym::arena constants(configuration);
    const auto zero = ok(constants.integer(0));
    ok(constants.integer(1));
    sb::require(ok(constants.integer(0)), zero);
    configuration.max_nodes = 128;
    configuration.max_depth = 3;
    sym::arena shallow(configuration);
    const auto sx = ok(shallow.symbol("x"));
    const auto first = ok(shallow.function(sym::kind::sin, sx));
    const auto second = ok(shallow.function(sym::kind::sin, first));
    sb::require(shallow.function(sym::kind::sin, second).code, sym::error::depth_limit);
    configuration.max_depth = 32;
    configuration.max_rewrites = 1;
    sym::arena bounded(configuration);
    const auto bx = ok(bounded.symbol("x"));
    const auto square = ok(bounded.power(bx, 2));
    sb::require(bounded.differentiate(square, bx).code, sym::error::rewrite_limit);
    configuration.max_rewrites = 1000;
    configuration.max_polynomial_degree = 3;
    sym::arena polynomial(configuration);
    const auto px = ok(polynomial.symbol("x"));
    sb::require(polynomial.integrate(ok(polynomial.power(px, 4)), px).code, sym::error::polynomial_limit);
    configuration.max_symbols = 1;
    configuration.max_text_bytes = 2;
    sym::arena symbols(configuration);
    ok(symbols.symbol("xx"));
    sb::require(symbols.symbol("yy").code, sym::error::text_limit);
    sym::arena texts(configuration);
    sb::require(texts.symbol("xxx").code, sym::error::text_limit);
    configuration.max_text_bytes = 128;
    sym::arena symbol_count(configuration);
    ok(symbol_count.symbol("x"));
    sb::require(symbol_count.symbol("y").code, sym::error::symbol_limit);
    configuration.max_depth = 513;
    sym::arena invalid(configuration);
    sb::require(invalid.status(), sym::error::invalid_limits);
  }
  sb::end_test_case();
  return 1;
}
