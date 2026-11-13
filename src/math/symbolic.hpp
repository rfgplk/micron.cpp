//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../vector/reserved_vector.hpp"
#include "rational.hpp"

namespace micron::math::symbolic
{

using node_id = u32;
inline constexpr node_id invalid_node = ~node_id{ 0 };

enum class error : u8 {
  none,
  invalid_limits,
  invalid_node,
  invalid_variable,
  invalid_symbol,
  assumption_conflict,
  node_limit,
  symbol_limit,
  text_limit,
  depth_limit,
  rewrite_limit,
  polynomial_limit,
  limb_limit,
  exponent_limit,
  output_limit,
  domain,
  allocation,
};

enum class kind : u8 { rational, symbol, add, multiply, divide, power, sin, cos, exp, log, sqrt, abs, derivative, integral };

enum class assumptions : u8 { none = 0, real = 1, nonzero = 2, nonnegative = 4, positive = 8 };

constexpr assumptions
operator|(assumptions a, assumptions b) noexcept
{
  return static_cast<assumptions>(static_cast<u8>(a) | static_cast<u8>(b));
}

constexpr bool
has(assumptions value, assumptions flag) noexcept
{
  return (static_cast<u8>(value) & static_cast<u8>(flag)) == static_cast<u8>(flag);
}

struct limits {
  u32 max_nodes = 65536;
  u32 max_symbols = 4096;
  u32 max_text_bytes = 1048576;
  u32 max_depth = 256;
  u64 max_rewrites = 131072;
  u32 max_polynomial_degree = 256;
  rational_limits rational{};
};

struct result {
  node_id id = invalid_node;
  error code = error::none;

  explicit
  operator bool() const noexcept
  {
    return code == error::none && id != invalid_node;
  }
};

struct text_view {
  const char *data = nullptr;
  usize size = 0;
};

struct node {
  kind type = kind::rational;
  node_id left = invalid_node;
  node_id right = invalid_node;
  i64 exponent = 0;
  u32 payload = 0;
  u32 depth = 1;
  assumptions proven = assumptions::none;
};

namespace __impl
{

constexpr assumptions
closure(assumptions value) noexcept
{
  if ( has(value, assumptions::positive) ) return value | assumptions::real | assumptions::nonzero | assumptions::nonnegative;
  if ( has(value, assumptions::nonnegative) ) value = value | assumptions::real;
  if ( has(value, assumptions::nonnegative) && has(value, assumptions::nonzero) ) value = value | assumptions::positive;
  return value;
}

constexpr u64
mix(u64 hash, u64 value) noexcept
{
  hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
  return hash;
}

inline error
convert(rational_error code) noexcept
{
  switch ( code ) {
  case rational_error::none:
    return error::none;
  case rational_error::zero_denominator:
    return error::domain;
  case rational_error::limb_limit:
    return error::limb_limit;
  case rational_error::exponent_limit:
    return error::exponent_limit;
  }
  return error::domain;
}

}      // namespace __impl

// IDs and const node/coefficient/name views remain valid for the arena lifetime
class arena
{
  struct empty_tag {
  };
  friend class expression;

  explicit arena(empty_tag) noexcept : __state(error::invalid_limits) { }

  struct symbol_record {
    u32 offset = 0;
    u32 length = 0;
    node_id id = invalid_node;
    assumptions declared = assumptions::none;
  };

  limits __limits{};
  error __state = error::none;
  micron::reserved_vector<node> __nodes;
  micron::reserved_vector<rational> __constants;
  micron::reserved_vector<symbol_record> __symbols;
  micron::reserved_vector<char> __text;
  micron::reserved_vector<node_id> __table;

  struct context {
    u64 remaining;
    micron::reserved_vector<node_id> memo;
    micron::reserved_vector<i8> dependent;
    error failure = error::none;

    context(usize count, u64 budget) : remaining(budget)
    {
      if ( !memo.initialize(count ? count : 1) || !memo.try_resize(count) || !dependent.initialize(count ? count : 1)
           || !dependent.try_resize(count) ) {
        failure = error::allocation;
        return;
      }
      for ( usize i = 0; i < count; ++i ) {
        memo[i] = invalid_node;
        dependent[i] = -1;
      }
    }

    bool
    step() noexcept
    {
      if ( failure != error::none ) return false;
      if ( !remaining ) {
        failure = error::rewrite_limit;
        return false;
      }
      --remaining;
      return true;
    }
  };

  struct polynomial {
    micron::reserved_vector<rational> terms;
    bool recognized = true;
    error failure = error::none;
  };

  struct __text_buffer {
    char *bytes;
    usize capacity;
    micron::reserved_vector<char> *overflow = nullptr;
    usize maximum = 0;

    bool
    ensure(usize needed)
    {
      if ( needed <= capacity ) return true;
      if ( !overflow || needed > maximum ) return false;
      if ( !overflow->max_size() && !overflow->initialize(maximum) ) return false;
      if ( !overflow->try_resize(needed) ) return false;
      bytes = overflow->data();
      capacity = overflow->size();
      return true;
    }
  };

  template<typename Sink>
  error
  write_node(node_id id, Sink &sink, __text_buffer &scratch, u64 &remaining) const
  {
    if ( !remaining ) return error::rewrite_limit;
    --remaining;
    const auto emit = [&](const char *text, usize length) { return sink(text, length); };
    const auto &value = __nodes[id];
    if ( value.type == kind::rational ) {
      const auto &q = __constants[value.payload];
      const usize numerator_capacity = q.numerator().magnitude().bit_length() + 2;
      const usize denominator_capacity = q.denominator().bit_length() + 1;
      if ( !scratch.ensure(numerator_capacity > denominator_capacity ? numerator_capacity : denominator_capacity) )
        return scratch.overflow ? error::allocation : error::output_limit;
      const bool fraction = !q.is_integer();
      if ( fraction && !emit("(/ ", 3) ) return error::output_limit;
      const usize bytes = micron::math::to_chars(scratch.bytes, scratch.capacity, q.numerator());
      if ( !bytes || !emit(scratch.bytes, bytes) ) return error::output_limit;
      if ( fraction ) {
        if ( !emit(" ", 1) ) return error::output_limit;
        const usize denominator_bytes = micron::math::to_chars(scratch.bytes, scratch.capacity, q.denominator());
        if ( !denominator_bytes || !emit(scratch.bytes, denominator_bytes) || !emit(")", 1) ) return error::output_limit;
      }
      return error::none;
    }
    if ( value.type == kind::symbol ) {
      const auto name = symbol_name(id);
      bool simple = name.data[0] != '#' && (name.data[0] < '0' || name.data[0] > '9');
      if ( name.size > 1 && (name.data[0] == '+' || name.data[0] == '-') && name.data[1] >= '0' && name.data[1] <= '9' ) simple = false;
      for ( usize i = 0; i < name.size; ++i ) {
        const char c = name.data[i];
        if ( static_cast<u8>(c) <= 32 || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '"' || c == '\''
             || c == '`' || c == ',' || c == ';' )
          simple = false;
      }
      if ( simple ) return emit(name.data, name.size) ? error::none : error::output_limit;
      if ( !emit("(sym.symbol \"", 13) ) return error::output_limit;
      for ( usize i = 0; i < name.size; ++i ) {
        const u8 c = static_cast<u8>(name.data[i]);
        if ( c == '"' || c == '\\' ) {
          const char escaped[2] = { '\\', static_cast<char>(c) };
          if ( !emit(escaped, 2) ) return error::output_limit;
        } else if ( c < 32 || c == 127 ) {
          constexpr char hex[] = "0123456789abcdef";
          const char escaped[4] = { '\\', 'x', hex[c >> 4], hex[c & 15] };
          if ( !emit(escaped, 4) ) return error::output_limit;
        } else if ( !emit(name.data + i, 1) )
          return error::output_limit;
      }
      return emit("\")", 2) ? error::none : error::output_limit;
    }
    const char *operation = nullptr;
    usize operation_size = 0;
    switch ( value.type ) {
    case kind::add:
      operation = "(+ ";
      operation_size = 3;
      break;
    case kind::multiply:
      operation = "(* ";
      operation_size = 3;
      break;
    case kind::divide:
      operation = "(/ ";
      operation_size = 3;
      break;
    case kind::power:
      operation = "(math.pow ";
      operation_size = 10;
      break;
    case kind::sin:
      operation = "(math.sin ";
      operation_size = 10;
      break;
    case kind::cos:
      operation = "(math.cos ";
      operation_size = 10;
      break;
    case kind::exp:
      operation = "(math.exp ";
      operation_size = 10;
      break;
    case kind::log:
      operation = "(math.log ";
      operation_size = 10;
      break;
    case kind::sqrt:
      operation = "(math.sqrt ";
      operation_size = 11;
      break;
    case kind::abs:
      operation = "(math.abs ";
      operation_size = 10;
      break;
    case kind::derivative:
      operation = "(sym.derivative ";
      operation_size = 16;
      break;
    case kind::integral:
      operation = "(sym.integral ";
      operation_size = 14;
      break;
    default:
      return error::invalid_node;
    }
    if ( !emit(operation, operation_size) ) return error::output_limit;
    auto code = write_node(value.left, sink, scratch, remaining);
    if ( code != error::none ) return code;
    if ( value.right != invalid_node ) {
      if ( !emit(" ", 1) ) return error::output_limit;
      code = write_node(value.right, sink, scratch, remaining);
      if ( code != error::none ) return code;
    } else if ( value.type == kind::power ) {
      if ( !emit(" ", 1) ) return error::output_limit;
      if ( !scratch.ensure(65) ) return scratch.overflow ? error::allocation : error::output_limit;
      const usize bytes = micron::math::to_chars(scratch.bytes, scratch.capacity, arbint<64>(value.exponent));
      if ( !bytes || !emit(scratch.bytes, bytes) ) return error::output_limit;
    }
    return emit(")", 1) ? error::none : error::output_limit;
  }

  static result
  fail(error code) noexcept
  {
    return { invalid_node, code };
  }

  static u64
  rational_hash(const rational &q) noexcept
  {
    u64 hashed = __impl::mix(0x736d626f6c696301ULL, static_cast<u8>(kind::rational));
    hashed = __impl::mix(hashed, static_cast<u64>(q.sign() + 1));
    hashed = __impl::mix(hashed, q.numerator().size());
    for ( usize i = 0; i < q.numerator().size(); ++i ) hashed = __impl::mix(hashed, q.numerator().limbs()[i]);
    hashed = __impl::mix(hashed, q.denominator().size());
    for ( usize i = 0; i < q.denominator().size(); ++i ) hashed = __impl::mix(hashed, q.denominator().limbs()[i]);
    return hashed;
  }

  u64
  hash(const node &value) const noexcept
  {
    if ( value.type == kind::rational ) return rational_hash(__constants[value.payload]);
    u64 hashed = __impl::mix(0x736d626f6c696301ULL, static_cast<u8>(value.type));
    hashed = __impl::mix(hashed, value.left);
    hashed = __impl::mix(hashed, value.right);
    hashed = __impl::mix(hashed, static_cast<u64>(value.exponent));
    return __impl::mix(hashed, value.payload);
  }

  bool
  same(const node &a, const node &b) const noexcept
  {
    if ( a.type != b.type ) return false;
    if ( a.type == kind::rational ) return __constants[a.payload] == __constants[b.payload];
    return a.left == b.left && a.right == b.right && a.exponent == b.exponent && a.payload == b.payload;
  }

  result
  intern(node value)
  {
    if ( __state != error::none ) return fail(__state);
    if ( value.depth > __limits.max_depth ) return fail(error::depth_limit);
    const usize mask = __table.size() - 1;
    usize slot = hash(value) & mask;
    while ( __table[slot] != invalid_node ) {
      const node_id found = __table[slot];
      if ( same(__nodes[found], value) ) return { found };
      slot = (slot + 1) & mask;
    }
    if ( __nodes.size() == __limits.max_nodes ) return fail(error::node_limit);
    const node_id id = static_cast<node_id>(__nodes.size());
    if ( !__nodes.try_resize(__nodes.size() + 1) ) return fail(error::allocation);
    __nodes[id] = value;
    __table[slot] = id;
    return { id };
  }

  result
  binary(kind type, node_id left, node_id right, assumptions proven = assumptions::none)
  {
    if ( !get(left) || !get(right) ) return fail(error::invalid_node);
    const auto depth = __nodes[left].depth > __nodes[right].depth ? __nodes[left].depth : __nodes[right].depth;
    return intern({ type, left, right, 0, 0, depth + 1, __impl::closure(proven) });
  }

  bool
  literal(node_id id, i64 value) const noexcept
  {
    const auto *q = coefficient(id);
    return q && q->is_integer() && q->numerator() == value;
  }

  result
  rational_node(rational_result value)
  {
    if ( !value ) return fail(__impl::convert(value.error));
    return constant(value.value);
  }

  bool
  depends(node_id expression, node_id variable, context &work) const noexcept
  {
    if ( expression < work.dependent.size() && work.dependent[expression] != -1 ) return work.dependent[expression] != 0;
    if ( !work.step() ) return true;
    const auto &value = __nodes[expression];
    bool dependent = expression == variable;
    if ( !dependent && value.left != invalid_node ) dependent = depends(value.left, variable, work);
    if ( !dependent && value.right != invalid_node ) dependent = depends(value.right, variable, work);
    if ( expression < work.dependent.size() ) work.dependent[expression] = dependent ? 1 : 0;
    return dependent;
  }

  result
  derivative_of(node_id expression, node_id variable, context &work)
  {
    if ( expression < work.memo.size() && work.memo[expression] != invalid_node ) return { work.memo[expression] };
    if ( !work.step() ) return fail(work.failure);
    const node source = __nodes[expression];
    result output;
    if ( source.type == kind::rational || source.type == kind::symbol ) {
      output = integer(expression == variable ? 1 : 0);
    } else if ( source.type == kind::integral && source.right == variable ) {
      output = { source.left };
    } else if ( source.type == kind::derivative || source.type == kind::integral ) {
      output = binary(kind::derivative, expression, variable);
    } else {
      const auto dl = derivative_of(source.left, variable, work);
      if ( !dl ) return dl;
      if ( source.type == kind::add || source.type == kind::multiply || source.type == kind::divide ) {
        const auto dr = derivative_of(source.right, variable, work);
        if ( !dr ) return dr;
        if ( source.type == kind::add ) {
          output = add(dl.id, dr.id);
        } else {
          const auto first = multiply(dl.id, source.right), second = multiply(source.left, dr.id);
          if ( !first ) return first;
          if ( !second ) return second;
          if ( source.type == kind::multiply ) {
            output = add(first.id, second.id);
          } else {
            const auto top = subtract(first.id, second.id), bottom = power(source.right, 2);
            if ( !top ) return top;
            if ( !bottom ) return bottom;
            output = divide(top.id, bottom.id);
          }
        }
      } else if ( source.type == kind::power ) {
        if ( source.exponent == (-9223372036854775807LL - 1) || source.exponent == 0 ) {
          output = binary(kind::derivative, expression, variable);
        } else {
          const auto factor = integer(source.exponent), lower = power(source.left, source.exponent - 1);
          if ( !factor ) return factor;
          if ( !lower ) return lower;
          const auto outer = multiply(factor.id, lower.id);
          if ( !outer ) return outer;
          output = multiply(outer.id, dl.id);
        }
      } else {
        result outer;
        switch ( source.type ) {
        case kind::sin:
          outer = function(kind::cos, source.left);
          break;
        case kind::cos: {
          const auto sine = function(kind::sin, source.left);
          if ( !sine ) return sine;
          outer = negate(sine.id);
          break;
        }
        case kind::exp:
          outer = { expression };
          break;
        case kind::log: {
          const auto one = integer(1);
          if ( !one ) return one;
          outer = divide(one.id, source.left);
          break;
        }
        case kind::sqrt: {
          const auto two = integer(2);
          if ( !two ) return two;
          const auto bottom = multiply(two.id, expression), one = integer(1);
          if ( !bottom ) return bottom;
          if ( !one ) return one;
          outer = divide(one.id, bottom.id);
          break;
        }
        case kind::abs:
          if ( has(__nodes[source.left].proven, assumptions::positive) )
            outer = integer(1);
          else
            return binary(kind::derivative, expression, variable);
          break;
        default:
          return binary(kind::derivative, expression, variable);
        }
        if ( !outer ) return outer;
        output = multiply(outer.id, dl.id);
      }
    }
    if ( output && expression < work.memo.size() ) work.memo[expression] = output.id;
    return output;
  }

  polynomial
  polynomial_storage(usize count)
  {
    polynomial output;
    if ( count > static_cast<usize>(__limits.max_polynomial_degree) + 1 )
      output.failure = error::polynomial_limit;
    else if ( !output.terms.initialize(static_cast<usize>(__limits.max_polynomial_degree) + 1) || !output.terms.try_resize(count) )
      output.failure = error::allocation;
    return output;
  }

  polynomial
  polynomial_multiply(const polynomial &a, const polynomial &b, context &work)
  {
    auto out = polynomial_storage(a.terms.size() + b.terms.size() - 1);
    if ( out.failure != error::none ) return out;
    for ( usize i = 0; i < a.terms.size(); ++i )
      for ( usize j = 0; j < b.terms.size(); ++j ) {
        if ( !work.step() ) {
          out.failure = work.failure;
          return out;
        }
        const auto product = rational_multiply(a.terms[i], b.terms[j], __limits.rational);
        if ( !product ) {
          out.failure = __impl::convert(product.error);
          return out;
        }
        auto sum = rational_add(out.terms[i + j], product.value, __limits.rational);
        if ( !sum ) {
          out.failure = __impl::convert(sum.error);
          return out;
        }
        out.terms[i + j] = micron::move(sum.value);
      }
    return out;
  }

  polynomial
  polynomial_of(node_id expression, node_id variable, context &work)
  {
    polynomial out;
    if ( !work.step() ) {
      out.failure = work.failure;
      return out;
    }
    const node value = __nodes[expression];
    if ( value.type == kind::rational ) {
      out = polynomial_storage(1);
      if ( out.failure == error::none ) out.terms[0] = __constants[value.payload];
      return out;
    }
    if ( expression == variable ) {
      out = polynomial_storage(2);
      if ( out.failure == error::none ) out.terms[1] = make_rational(1).value;
      return out;
    }
    if ( value.type != kind::add && value.type != kind::multiply && value.type != kind::divide && value.type != kind::power ) {
      out.recognized = false;
      return out;
    }
    auto left = polynomial_of(value.left, variable, work);
    if ( left.failure != error::none || !left.recognized ) return left;
    if ( value.type == kind::power ) {
      if ( value.exponent < 0 || (value.exponent == 0 && !has(__nodes[value.left].proven, assumptions::nonzero)) ) {
        left.recognized = false;
        return left;
      }
      if ( left.terms.size() > 1 && static_cast<u64>(value.exponent) > __limits.max_polynomial_degree / (left.terms.size() - 1) ) {
        left.failure = error::polynomial_limit;
        return left;
      }
      out = polynomial_storage(1);
      if ( out.failure != error::none ) return out;
      out.terms[0] = make_rational(1).value;
      u64 remaining = static_cast<u64>(value.exponent);
      while ( remaining ) {
        if ( remaining & 1 ) {
          out = polynomial_multiply(out, left, work);
          if ( out.failure != error::none ) return out;
        }
        remaining >>= 1;
        if ( remaining ) {
          left = polynomial_multiply(left, left, work);
          if ( left.failure != error::none ) return left;
        }
      }
      return out;
    }
    const auto right = polynomial_of(value.right, variable, work);
    if ( right.failure != error::none || !right.recognized ) {
      out.failure = right.failure;
      out.recognized = right.recognized;
      return out;
    }
    if ( value.type == kind::multiply ) return polynomial_multiply(left, right, work);
    if ( value.type == kind::divide ) {
      if ( right.terms.size() != 1 ) {
        left.recognized = false;
        return left;
      }
      for ( usize i = 0; i < left.terms.size(); ++i ) {
        auto quotient = rational_divide(left.terms[i], right.terms[0], __limits.rational);
        if ( !quotient ) {
          left.failure = __impl::convert(quotient.error);
          return left;
        }
        left.terms[i] = micron::move(quotient.value);
      }
      return left;
    }
    out = polynomial_storage(left.terms.size() > right.terms.size() ? left.terms.size() : right.terms.size());
    if ( out.failure != error::none ) return out;
    for ( usize i = 0; i < out.terms.size(); ++i ) {
      if ( !work.step() ) {
        out.failure = work.failure;
        return out;
      }
      if ( i >= left.terms.size() )
        out.terms[i] = right.terms[i];
      else if ( i >= right.terms.size() )
        out.terms[i] = left.terms[i];
      else {
        auto sum = rational_add(left.terms[i], right.terms[i], __limits.rational);
        if ( !sum ) {
          out.failure = __impl::convert(sum.error);
          return out;
        }
        out.terms[i] = micron::move(sum.value);
      }
    }
    return out;
  }

  result
  substitute_in(node_id expression, node_id variable, node_id replacement, context &work)
  {
    if ( expression == variable ) return { replacement };
    if ( expression < work.memo.size() && work.memo[expression] != invalid_node ) return { work.memo[expression] };
    if ( !work.step() ) return fail(work.failure);
    const node value = __nodes[expression];
    if ( value.type == kind::rational || value.type == kind::symbol ) return { expression };
    // An unevaluated calculus node has an explicit binding slot. Do not silently
    // rename it into a non-symbol or capture a replacement beneath the binder.
    if ( value.type == kind::integral || value.type == kind::derivative ) return fail(error::invalid_variable);
    const auto left = substitute_in(value.left, variable, replacement, work);
    if ( !left ) return left;
    result out;
    if ( value.right != invalid_node ) {
      const auto right = substitute_in(value.right, variable, replacement, work);
      if ( !right ) return right;
      switch ( value.type ) {
      case kind::add:
        out = add(left.id, right.id);
        break;
      case kind::multiply:
        out = multiply(left.id, right.id);
        break;
      case kind::divide:
        out = divide(left.id, right.id);
        break;
      default:
        return fail(error::invalid_node);
      }
    } else if ( value.type == kind::power ) {
      out = power(left.id, value.exponent);
    } else {
      out = function(value.type, left.id);
    }
    if ( out && expression < work.memo.size() ) work.memo[expression] = out.id;
    return out;
  }

public:
  explicit arena(limits configuration = {}) : __limits(configuration)
  {
    if ( !__limits.max_nodes || __limits.max_nodes > (1u << 28) || !__limits.max_symbols || !__limits.max_text_bytes || !__limits.max_depth
         || __limits.max_depth > 512 || __limits.max_polynomial_degree > 4096 || !__limits.rational.max_limbs
         || __limits.rational.max_work_limbs < __limits.rational.max_limbs ) {
      __state = error::invalid_limits;
      return;
    }
    usize table_size = 1;
    while ( table_size < static_cast<usize>(__limits.max_nodes) * 2 ) table_size *= 2;
    if ( !__nodes.initialize(__limits.max_nodes) || !__constants.initialize(__limits.max_nodes)
         || !__symbols.initialize(__limits.max_symbols) || !__text.initialize(__limits.max_text_bytes) || !__table.initialize(table_size)
         || !__table.try_resize(table_size) ) {
      __state = error::allocation;
      return;
    }
    for ( usize i = 0; i < table_size; ++i ) __table[i] = invalid_node;
  }

  arena(const arena &) = delete;
  arena &operator=(const arena &) = delete;
  arena(arena &&) noexcept = default;
  arena &operator=(arena &&) noexcept = default;

  error
  status() const noexcept
  {
    return __state;
  }

  usize
  size() const noexcept
  {
    return __nodes.size();
  }

  const limits &
  configuration() const noexcept
  {
    return __limits;
  }

  const node *
  get(node_id id) const noexcept
  {
    return id < __nodes.size() ? __nodes.data() + id : nullptr;
  }

  error
  clone_to(arena &destination) const
  {
    if ( __state != error::none ) return __state;
    if ( this == &destination ) return error::none;
    arena copied(__limits);
    if ( copied.status() != error::none ) return copied.status();
    if ( !copied.__nodes.try_copy(__nodes) || !copied.__constants.try_copy(__constants) || !copied.__symbols.try_copy(__symbols)
         || !copied.__text.try_copy(__text) || !copied.__table.try_copy(__table) )
      return error::allocation;
    destination = micron::move(copied);
    return error::none;
  }

  template<typename Sink>
  error
  write_csexp(node_id expression, Sink &&sink) const
  {
    if ( !get(expression) ) return error::invalid_node;
    if ( __limits.rational.max_limbs > (~usize{ 0 } - 80) / mpn::limb_bits ) return error::limb_limit;
    char local[256];
    micron::reserved_vector<char> overflow;
    __text_buffer scratch{ local, sizeof(local), &overflow, __limits.rational.max_limbs * mpn::limb_bits + 80 };
    u64 remaining = __limits.max_rewrites;
    return write_node(expression, sink, scratch, remaining);
  }

  // Scratch is caller-owned and reused across writes; sinks consume each span synchronously.
  template<typename Sink>
  error
  write_csexp(node_id expression, Sink &&sink, char *buffer, usize capacity) const
  {
    if ( !get(expression) ) return error::invalid_node;
    if ( __limits.rational.max_limbs > (~usize{ 0 } - 80) / mpn::limb_bits ) return error::limb_limit;
    if ( !buffer && capacity ) return error::output_limit;
    __text_buffer scratch{ buffer, capacity };
    u64 remaining = __limits.max_rewrites;
    return write_node(expression, sink, scratch, remaining);
  }

  const rational *
  coefficient(node_id id) const noexcept
  {
    const auto *value = get(id);
    return value && value->type == kind::rational ? __constants.data() + value->payload : nullptr;
  }

  text_view
  symbol_name(node_id id) const noexcept
  {
    const auto *value = get(id);
    if ( !value || value->type != kind::symbol ) return {};
    const auto &symbol = __symbols[value->payload];
    return { __text.data() + symbol.offset, symbol.length };
  }

  result
  constant(const rational &value)
  {
    if ( __state != error::none ) return fail(__state);
    if ( !micron::math::__impl::fits(value, __limits.rational) ) return fail(error::limb_limit);
    const usize mask = __table.size() - 1;
    usize slot = rational_hash(value) & mask;
    while ( __table[slot] != invalid_node ) {
      const auto found = __table[slot];
      if ( __nodes[found].type == kind::rational && __constants[__nodes[found].payload] == value ) return { found };
      slot = (slot + 1) & mask;
    }
    if ( __constants.size() == __limits.max_nodes ) return fail(error::node_limit);
    const auto index = static_cast<u32>(__constants.size());
    if ( !__constants.try_resize(__constants.size() + 1) ) return fail(error::allocation);
    __constants[index] = value;
    const auto properties = value.sign() > 0   ? assumptions::positive
                            : value.sign() < 0 ? assumptions::real | assumptions::nonzero
                                               : assumptions::real | assumptions::nonnegative;
    const auto out = intern({ kind::rational, invalid_node, invalid_node, 0, index, 1, __impl::closure(properties) });
    if ( !out || __nodes[out.id].payload != index ) (void)__constants.try_resize(index);
    return out;
  }

  result
  integer(i64 value)
  {
    return rational_node(make_rational(value, 1, __limits.rational));
  }

  result
  symbol(text_view name, assumptions properties = assumptions::none)
  {
    if ( __state != error::none ) return fail(__state);
    if ( !name.data || !name.size || static_cast<u8>(properties) > 15 ) return fail(error::invalid_symbol);
    if ( name.size > __limits.max_text_bytes ) return fail(error::text_limit);
    properties = __impl::closure(properties);
    for ( usize i = 0; i < name.size; ++i )
      if ( !name.data[i] ) return fail(error::invalid_symbol);
    for ( usize i = 0; i < __symbols.size(); ++i ) {
      const auto &old = __symbols[i];
      if ( old.length != name.size ) continue;
      bool equal = true;
      for ( usize j = 0; j < name.size; ++j )
        if ( __text[old.offset + j] != name.data[j] ) {
          equal = false;
          break;
        }
      if ( equal ) return old.declared == properties ? result{ old.id } : fail(error::assumption_conflict);
    }
    if ( name.size > __limits.max_text_bytes - __text.size() ) return fail(error::text_limit);
    if ( __symbols.size() == __limits.max_symbols ) return fail(error::symbol_limit);
    const auto offset = static_cast<u32>(__text.size()), index = static_cast<u32>(__symbols.size());
    if ( !__text.try_resize(__text.size() + name.size) ) return fail(error::allocation);
    if ( !__symbols.try_resize(__symbols.size() + 1) ) {
      (void)__text.try_resize(offset);
      return fail(error::allocation);
    }
    for ( usize i = 0; i < name.size; ++i ) __text[offset + i] = name.data[i];
    const auto out = intern({ kind::symbol, invalid_node, invalid_node, 0, index, 1, properties });
    if ( out )
      __symbols[index] = { offset, static_cast<u32>(name.size), out.id, properties };
    else {
      (void)__symbols.try_resize(index);
      (void)__text.try_resize(offset);
    }
    return out;
  }

  template<usize N>
  result
  symbol(const char (&name)[N], assumptions properties = assumptions::none)
  {
    return symbol({ name, N - 1 }, properties);
  }

  result
  add(node_id left, node_id right)
  {
    if ( !get(left) || !get(right) ) return fail(error::invalid_node);
    if ( literal(left, 0) ) return { right };
    if ( literal(right, 0) ) return { left };
    const auto *a = coefficient(left), *b = coefficient(right);
    if ( a && b ) return rational_node(rational_add(*a, *b, __limits.rational));
    if ( left == right ) {
      const auto two = integer(2);
      return two ? multiply(two.id, left) : two;
    }
    if ( right < left ) {
      const auto temporary = left;
      left = right;
      right = temporary;
    }
    assumptions properties = assumptions::none;
    if ( has(__nodes[left].proven, assumptions::real) && has(__nodes[right].proven, assumptions::real) ) properties = assumptions::real;
    if ( has(__nodes[left].proven, assumptions::nonnegative) && has(__nodes[right].proven, assumptions::nonnegative) ) {
      properties = properties | assumptions::nonnegative;
      if ( has(__nodes[left].proven, assumptions::positive) || has(__nodes[right].proven, assumptions::positive) )
        properties = properties | assumptions::positive;
    }
    return binary(kind::add, left, right, properties);
  }

  result
  negate(node_id value)
  {
    const auto negative = integer(-1);
    return negative ? multiply(negative.id, value) : negative;
  }

  result
  subtract(node_id left, node_id right)
  {
    const auto negative = negate(right);
    return negative ? add(left, negative.id) : negative;
  }

  result
  multiply(node_id left, node_id right)
  {
    if ( !get(left) || !get(right) ) return fail(error::invalid_node);
    if ( literal(left, 0) || literal(right, 0) ) return integer(0);
    if ( literal(left, 1) ) return { right };
    if ( literal(right, 1) ) return { left };
    const auto *a = coefficient(left), *b = coefficient(right);
    if ( a && b ) return rational_node(rational_multiply(*a, *b, __limits.rational));
    if ( left == right ) return power(left, 2);
    if ( right < left ) {
      const auto temporary = left;
      left = right;
      right = temporary;
    }
    assumptions properties = assumptions::none;
    if ( has(__nodes[left].proven, assumptions::real) && has(__nodes[right].proven, assumptions::real) ) properties = assumptions::real;
    if ( has(__nodes[left].proven, assumptions::positive) && has(__nodes[right].proven, assumptions::positive) )
      properties = properties | assumptions::positive;
    if ( has(__nodes[left].proven, assumptions::nonzero) && has(__nodes[right].proven, assumptions::nonzero) )
      properties = properties | assumptions::nonzero;
    return binary(kind::multiply, left, right, properties);
  }

  result
  divide(node_id left, node_id right)
  {
    if ( !get(left) || !get(right) ) return fail(error::invalid_node);
    if ( literal(right, 0) ) return fail(error::domain);
    if ( literal(right, 1) ) return { left };
    const auto *a = coefficient(left), *b = coefficient(right);
    if ( a && b ) return rational_node(rational_divide(*a, *b, __limits.rational));
    if ( left == right && has(__nodes[right].proven, assumptions::nonzero) ) return integer(1);
    if ( literal(left, 0) && has(__nodes[right].proven, assumptions::nonzero) ) return integer(0);
    assumptions properties = assumptions::none;
    if ( has(__nodes[left].proven, assumptions::real) && has(__nodes[right].proven, assumptions::real) ) properties = assumptions::real;
    if ( has(__nodes[left].proven, assumptions::positive) && has(__nodes[right].proven, assumptions::positive) )
      properties = properties | assumptions::positive;
    if ( has(__nodes[left].proven, assumptions::nonzero) && has(__nodes[right].proven, assumptions::nonzero) )
      properties = properties | assumptions::nonzero;
    return binary(kind::divide, left, right, properties);
  }

  result
  power(node_id base, i64 exponent)
  {
    if ( !get(base) ) return fail(error::invalid_node);
    const u64 magnitude = exponent < 0 ? static_cast<u64>(-(exponent + 1)) + 1 : static_cast<u64>(exponent);
    if ( magnitude > __limits.rational.max_power ) return fail(error::exponent_limit);
    if ( const auto *q = coefficient(base) ) return rational_node(rational_power(*q, exponent, __limits.rational));
    if ( exponent == 1 ) return { base };
    if ( !exponent && has(__nodes[base].proven, assumptions::nonzero) ) return integer(1);
    assumptions properties = assumptions::none;
    if ( has(__nodes[base].proven, assumptions::real) ) {
      properties = assumptions::real;
      if ( exponent > 0 && (exponent & 1) == 0 ) properties = properties | assumptions::nonnegative;
    }
    if ( has(__nodes[base].proven, assumptions::nonzero) ) properties = properties | assumptions::nonzero;
    if ( has(__nodes[base].proven, assumptions::positive) ) properties = properties | assumptions::positive;
    return intern({ kind::power, base, invalid_node, exponent, 0, __nodes[base].depth + 1, __impl::closure(properties) });
  }

  result
  function(kind type, node_id argument)
  {
    const auto *value = get(argument);
    if ( !value ) return fail(error::invalid_node);
    if ( type != kind::sin && type != kind::cos && type != kind::exp && type != kind::log && type != kind::sqrt && type != kind::abs )
      return fail(error::invalid_node);
    if ( const auto *q = coefficient(argument) ) {
      if ( (type == kind::log && q->sign() <= 0) || (type == kind::sqrt && q->sign() < 0) ) return fail(error::domain);
      if ( type == kind::abs ) return q->sign() >= 0 ? result{ argument } : negate(argument);
      if ( q->is_zero() ) return integer(type == kind::cos || type == kind::exp ? 1 : 0);
      if ( type == kind::log && literal(argument, 1) ) return integer(0);
      if ( type == kind::sqrt && literal(argument, 1) ) return integer(1);
    }
    if ( type == kind::abs && has(value->proven, assumptions::nonnegative) ) return { argument };
    if ( type == kind::exp && value->type == kind::log && has(__nodes[value->left].proven, assumptions::positive) ) return { value->left };
    if ( type == kind::log && value->type == kind::exp && has(__nodes[value->left].proven, assumptions::real) ) return { value->left };
    if ( type == kind::sqrt && value->type == kind::power && value->exponent == 2 && has(__nodes[value->left].proven, assumptions::real) )
      return function(kind::abs, value->left);
    assumptions properties = assumptions::none;
    if ( has(value->proven, assumptions::real) ) {
      if ( type == kind::sin || type == kind::cos || type == kind::abs ) properties = assumptions::real;
      if ( type == kind::exp ) properties = assumptions::positive;
      if ( type == kind::abs ) properties = properties | assumptions::nonnegative;
      if ( type == kind::sqrt && has(value->proven, assumptions::nonnegative) ) properties = assumptions::nonnegative;
      if ( type == kind::log && has(value->proven, assumptions::positive) ) properties = assumptions::real;
    }
    return intern({ type, argument, invalid_node, 0, 0, value->depth + 1, __impl::closure(properties) });
  }

  result
  differentiate(node_id expression, node_id variable)
  {
    if ( !get(expression) || !get(variable) ) return fail(error::invalid_node);
    if ( __nodes[variable].type != kind::symbol ) return fail(error::invalid_variable);
    context work(__nodes.size(), __limits.max_rewrites);
    if ( work.failure != error::none ) return fail(work.failure);
    return derivative_of(expression, variable, work);
  }

  result
  integrate(node_id expression, node_id variable)
  {
    if ( !get(expression) || !get(variable) ) return fail(error::invalid_node);
    if ( __nodes[variable].type != kind::symbol ) return fail(error::invalid_variable);
    context work(__nodes.size(), __limits.max_rewrites);
    if ( work.failure != error::none ) return fail(work.failure);
    if ( !depends(expression, variable, work) ) return multiply(expression, variable);
    if ( work.failure != error::none ) return fail(work.failure);
    const auto coefficients = polynomial_of(expression, variable, work);
    if ( coefficients.failure != error::none ) return fail(coefficients.failure);
    if ( !coefficients.recognized ) return binary(kind::integral, expression, variable);
    result sum = integer(0);
    for ( usize i = 0; i < coefficients.terms.size(); ++i ) {
      if ( coefficients.terms[i].is_zero() ) continue;
      if ( !work.step() ) return fail(work.failure);
      const auto denominator = make_rational(static_cast<i64>(i + 1), 1, __limits.rational);
      if ( !denominator ) return fail(__impl::convert(denominator.error));
      const auto q = rational_divide(coefficients.terms[i], denominator.value, __limits.rational);
      if ( !q ) return fail(__impl::convert(q.error));
      const auto factor = constant(q.value), term_power = power(variable, static_cast<i64>(i + 1));
      if ( !factor ) return factor;
      if ( !term_power ) return term_power;
      const auto term = multiply(factor.id, term_power.id);
      if ( !term ) return term;
      if ( !sum ) return sum;
      sum = add(sum.id, term.id);
    }
    return sum;
  }

  result
  substitute(node_id expression, node_id variable, node_id replacement)
  {
    if ( !get(expression) || !get(variable) || !get(replacement) ) return fail(error::invalid_node);
    if ( __nodes[variable].type != kind::symbol ) return fail(error::invalid_variable);
    // A substitution may not invalidate assumptions used to simplify the source.
    const auto required = __nodes[variable].proven;
    if ( !has(__nodes[replacement].proven, required) ) return fail(error::assumption_conflict);
    context work(__nodes.size(), __limits.max_rewrites);
    if ( work.failure != error::none ) return fail(work.failure);
    return substitute_in(expression, variable, replacement, work);
  }
};

class expression
{
  arena __arena;
  node_id __root = invalid_node;

public:
  expression() noexcept : __arena(arena::empty_tag{}) { }

  expression(arena &&owner, node_id root) noexcept : __arena(micron::move(owner)), __root(root) { }

  expression(const expression &) = delete;
  expression &operator=(const expression &) = delete;
  expression(expression &&) noexcept = default;
  expression &operator=(expression &&) noexcept = default;

  const arena &
  owner() const noexcept
  {
    return __arena;
  }

  node_id
  root() const noexcept
  {
    return __root;
  }

  bool
  valid() const noexcept
  {
    return __arena.get(__root) != nullptr;
  }

  error
  clone_to(expression &destination) const
  {
    if ( !valid() ) return error::invalid_node;
    if ( this == &destination ) return error::none;
    arena copied(arena::empty_tag{});
    const auto code = __arena.clone_to(copied);
    if ( code != error::none ) return code;
    destination = expression(micron::move(copied), __root);
    return error::none;
  }

  template<typename Sink>
  error
  write_csexp(Sink &&sink) const
  {
    return __arena.write_csexp(__root, micron::forward<Sink>(sink));
  }

  template<typename Sink>
  error
  write_csexp(Sink &&sink, char *buffer, usize capacity) const
  {
    return __arena.write_csexp(__root, micron::forward<Sink>(sink), buffer, capacity);
  }
};

};      // namespace micron::math::symbolic
