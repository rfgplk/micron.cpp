//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../concepts.hpp"
#include "../tags.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

// NOTE:
//   .. lockfree queues and bloom filters render a summary not their contents, impossible to render contents without stopping them entirely
//   .. heaps and hash containers render in storage (in mem) order, not sorted/insertion order

namespace micron
{
namespace __print
{

template<typename T> using __U = micron::remove_cvref_t<T>;

struct __probe1 {
  template<typename A>
  constexpr void
  operator()(const A &) const noexcept
  {
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// keyed containers (maps et al)

// arity is decided by the types own aliases, never by probing a bare for_each
// requires expression does not instantiate the body
template<typename T>
concept __kv_visitable
    = (micron::is_tagged_map<T> || micron::is_tree<T>) && requires { typename __U<T>::mapped_type; } && requires(const __U<T> t) {
        // const auto & for the key, NOT const key_type &; certain maps store hash key not the key itself (hopscotch), this also dodges any
        // issues with future maps
        t.for_each([](const auto &, const typename __U<T>::mapped_type &) { });
      };

template<typename T>
concept __set_visitable = (micron::is_tagged_map<T> || micron::is_tree<T>) && !requires { typename __U<T>::mapped_type; } && requires {
  typename __U<T>::value_type;
} && requires(const __U<T> t) { t.for_each(micron::__print::__probe1{}); };

template<typename T>
concept __seq_visitable = requires { typename __U<T>::value_type; } && !micron::is_printable_container<T>
                          && requires(const __U<T> t) { t.for_each([](const typename __U<T>::value_type &) { }); };

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// linked/chained containers

template<typename T>
concept __node_chain = requires(const __U<T> t) {
  { t.ibegin()->next };
  { t.ibegin()->data };
};

// WARNING: real maps can insert(k, v); vector<pair<A,B>> cannot
// without this pair .a/.b makes every vector<pair> appear as a map

template<typename T, typename K, typename V>
concept __kv_insertable = requires(__U<T> t, K k, V v) { t.insert(micron::move(k), micron::move(v)); }
                          || requires(__U<T> t, K k, V v) { t.insert_unhash(micron::move(k), micron::move(v)); };

template<typename T>
concept __kv_key_iter = requires(const __U<T> c) {
  { c.begin()->key };
  { c.begin()->value };
  requires __kv_insertable<T, micron::remove_cvref_t<decltype(c.begin()->key)>, micron::remove_cvref_t<decltype(c.begin()->value)>>;
};

template<typename T>
concept __kv_ab_iter = requires(const __U<T> c) {
  { (*c.begin()).a };
  { (*c.begin()).b };
  requires __kv_insertable<T, micron::remove_cvref_t<decltype((*c.begin()).a)>, micron::remove_cvref_t<decltype((*c.begin()).b)>>;
} && !__kv_key_iter<T>;

// %%%%%%%%%%%%%%%%%%
// sums
//
// NOTE: is_variant cannot be used here

template<typename T>
concept __sum_visitable = micron::has_value_check<T> && requires(const __U<T> t) {
  { t.index() } -> micron::convertible_to<usize>;
  t.visit(micron::__print::__probe1{});
};

template<usize> struct __const_usize {
};

template<typename T>
concept __static_length = requires { typename micron::__print::__const_usize<static_cast<usize>(__U<T>::length)>; };

template<typename T>
concept __not_iterable = !micron::is_printable_container<T>;

template<typename T>
concept __fixed_vector
    = __not_iterable<T> && requires { typename __U<T>::value_type; } && __static_length<T> && requires(const __U<T> t, usize i) {
        { t[i] } -> micron::convertible_to<const typename __U<T>::value_type &>;
      };

template<typename T>
concept __sparse_matrix = __not_iterable<T> && requires(const __U<T> t) {
  { t.rows } -> micron::convertible_to<usize>;
  { t.cols } -> micron::convertible_to<usize>;
  { t.nnz() } -> micron::convertible_to<usize>;
  { t.outer.size() } -> micron::convertible_to<usize>;
  { t.inner.size() } -> micron::convertible_to<usize>;
  { t.values.size() } -> micron::convertible_to<usize>;
};

template<typename T>
concept __sparse_vector = __not_iterable<T> && requires(const __U<T> t) {
  { t.n } -> micron::convertible_to<usize>;
  { t.nnz() } -> micron::convertible_to<usize>;
  { t.idx.size() } -> micron::convertible_to<usize>;
  { t.values.size() } -> micron::convertible_to<usize>;
};

template<typename T>
concept __dense_matrix = __not_iterable<T> && requires { typename __U<T>::value_type; } && requires(const __U<T> t, usize i) {
  { t.rows } -> micron::convertible_to<usize>;
  { t.cols } -> micron::convertible_to<usize>;
  { t.at(i, i) };
};

template<typename T>
concept __ndarray = micron::is_printable_container<T> && requires {
  typename micron::__print::__const_usize<static_cast<usize>(__U<T>::rank)>;
} && requires(const __U<T> t, usize i) {
  { t.shape(i) } -> micron::convertible_to<usize>;
};

template<typename T>
concept __soa_like = __not_iterable<T> && requires { typename micron::__print::__const_usize<static_cast<usize>(__U<T>::column_count)>; }
                     && requires(const __U<T> t) {
                          { t.size() } -> micron::convertible_to<usize>;
                          { t.template at<0>(usize{}) };
                        };

template<typename T>
concept __bitfield_like = requires {
  typename __U<T>::category_type;
  requires micron::is_same_v<typename __U<T>::category_type, micron::bitfield_tag>;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// fallbacks

template<typename T>
concept __indexable = !micron::is_printable_container<T> && requires(const __U<T> t, usize i) {
  { t.size() } -> micron::convertible_to<usize>;
  { t[i] };
};

template<typename T>
concept __smart_pointer = requires {
  typename __U<T>::category_type;
  typename __U<T>::element_type;
  requires micron::is_same_v<typename __U<T>::category_type, micron::pointer_tag>;
  requires !micron::is_void_v<typename __U<T>::element_type>;      // shared_ptr<void>
} && requires(const __U<T> t) {
  { t.get() } -> micron::convertible_to<const typename __U<T>::element_type *>;
} && !requires(const __U<T> t, usize i) { t[i]; };      // the unique_pointer<T[]> specialization

template<typename T>
concept __ptr_range = requires {
  typename __U<T>::ptr;
  typename __U<T>::const_ptr;
} && requires(const __U<T> t) {
  { t.begin() } -> micron::convertible_to<const void *>;
  { t.end() } -> micron::convertible_to<const void *>;
};

template<typename T>
concept __consuming = requires(__U<T> t) { t.begin(); } && !requires(const __U<T> t) { t.begin(); };

template<typename T>
concept __opaque_tagged = requires { typename __U<T>::category_type; }
                          && (micron::is_same_v<typename __U<T>::category_type, micron::theap_tag>
                              || micron::is_same_v<typename __U<T>::category_type, micron::buffer_tag>
                              || micron::is_same_v<typename __U<T>::category_type, micron::map_tag>
                              || micron::is_same_v<typename __U<T>::category_type, micron::tree_tag>
                              || micron::is_same_v<typename __U<T>::category_type, micron::array_tag>
                              || micron::is_same_v<typename __U<T>::category_type, micron::vector_tag>
                              || micron::is_same_v<typename __U<T>::category_type, micron::list_tag>
                              || micron::is_same_v<typename __U<T>::category_type, micron::double_list_tag>
                              || micron::is_same_v<typename __U<T>::category_type, micron::slice_tag>);

enum class kind : u8 {
  none = 0,
  node_chain,
  kv_visit,
  set_visit,
  option_like,
  sum_visit,
  tuple_like,
  sparse_mat,
  sparse_vec,
  matrix,
  vector_n,
  ndarray,
  soa_rows,
  bits,
  kv_iter_key,
  kv_iter_ab,
  sequence,
  indexed,
  smart_ptr,
  ptr_range,
  nullable_opaque,
  opaque,
  consuming
};

template<typename T>
constexpr kind
classify(void)
{
  using U = micron::remove_cvref_t<T>;

  // scalars, strings, raw pointers, and enums
  if constexpr ( micron::has_cstr<U> || micron::is_arithmetic_v<U> || micron::is_pointer_v<U> || micron::is_enum_v<U> ) return kind::none;

  // node chains
  else if constexpr ( __node_chain<U> )
    return kind::node_chain;

  // keyed containers
  else if constexpr ( __kv_visitable<U> )
    return kind::kv_visit;
  else if constexpr ( __set_visitable<U> )
    return kind::set_visit;

  // sums before nullable/indexed
  else if constexpr ( micron::is_option<U> )
    return kind::option_like;
  else if constexpr ( __sum_visitable<U> )
    return kind::sum_visit;
  else if constexpr ( micron::tuple_like<U> )
    return kind::tuple_like;

  // shapes
  else if constexpr ( __sparse_matrix<U> )
    return kind::sparse_mat;
  else if constexpr ( __sparse_vector<U> )
    return kind::sparse_vec;
  else if constexpr ( __dense_matrix<U> )
    return kind::matrix;
  else if constexpr ( __fixed_vector<U> )
    return kind::vector_n;
  else if constexpr ( __ndarray<U> )
    return kind::ndarray;
  else if constexpr ( __soa_like<U> )
    return kind::soa_rows;
  else if constexpr ( __bitfield_like<U> )
    return kind::bits;

  // remaining maps
  else if constexpr ( __kv_key_iter<U> )
    return kind::kv_iter_key;
  else if constexpr ( __kv_ab_iter<U> )
    return kind::kv_iter_ab;

  else if constexpr ( micron::is_printable_container<U> )
    return kind::sequence;

  // enumerable but not iterable
  else if constexpr ( __seq_visitable<U> )
    return kind::set_visit;
  else if constexpr ( __indexable<U> )
    return kind::indexed;

  // wrappers and last resorts
  else if constexpr ( __smart_pointer<U> )
    return kind::smart_ptr;
  else if constexpr ( __ptr_range<U> )
    return kind::ptr_range;
  else if constexpr ( micron::nullable<U> )
    return kind::nullable_opaque;
  else if constexpr ( __consuming<U> )
    return kind::consuming;
  else if constexpr ( __opaque_tagged<U> )
    return kind::opaque;
  else
    return kind::none;
}

template<typename T> inline constexpr kind kind_of_v = micron::__print::classify<T>();

template<typename T>
concept printable = (micron::__print::kind_of_v<T> != micron::__print::kind::none);

namespace __impl
{

template<typename Out, typename T, usize... Is>
constexpr void
emit_product(Out &o, const T &x, micron::index_sequence<Is...>)
{
  bool first = true;
  auto one = [&](const auto &v) {
    if ( !first ) o.raw(", ", 2);
    first = false;
    o.elem(v);
  };
  (one(micron::get<Is>(x)), ...);
}

template<typename Out, typename T, usize... Is>
constexpr void
emit_soa_row(Out &o, const T &x, usize r, micron::index_sequence<Is...>)
{
  bool first = true;
  auto one = [&](const auto &v) {
    if ( !first ) o.raw(", ", 2);
    first = false;
    o.elem(v);
  };
  (one(x.template at<Is>(r)), ...);
}

template<typename Out, typename T>
constexpr void
emit_nd(Out &o, const T &x, usize axis, usize off, usize span)
{
  const usize n = x.shape(axis);
  o.raw("{ ", 2);
  if ( axis + 1u == static_cast<usize>(micron::remove_cvref_t<T>::rank) ) {
    for ( usize i = 0; i < n; ++i ) {
      if ( i ) o.raw(", ", 2);
      o.elem(x.begin()[off + i]);
    }
  } else {
    const usize sub = n ? span / n : 0;
    for ( usize i = 0; i < n; ++i ) {
      if ( i ) o.raw(", ", 2);
      micron::__print::__impl::emit_nd(o, x, axis + 1u, off + i * sub, sub);
    }
  }
  o.raw(" }", 2);
}

};      // namespace __impl

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// main print renderer
//
// { } collection, [ ] fixed arity product, ( ) mathematical vector, name(x) tagged value

template<typename Out, typename T>
constexpr void
render(Out &o, const T &x)
{
  using U = micron::remove_cvref_t<T>;
  constexpr micron::__print::kind K = micron::__print::kind_of_v<U>;

  if constexpr ( K == kind::node_chain ) {
    o.raw("{ ", 2);
    bool first = true;
    for ( auto *p = x.ibegin(); p != nullptr; p = p->next ) {
      if ( !first ) o.raw(", ", 2);
      first = false;
      o.elem(p->data);
    }
    o.raw(" }", 2);

  } else if constexpr ( K == kind::kv_visit ) {
    o.raw("{ ", 2);
    bool first = true;
    x.for_each([&](const auto &k, const auto &v) {
      if ( !first ) o.raw(", ", 2);
      first = false;
      o.elem(k);      // recursed, so stringy keys compile
      o.raw(": ", 2);
      o.elem(v);
    });
    o.raw(" }", 2);

  } else if constexpr ( K == kind::set_visit ) {
    o.raw("{ ", 2);
    bool first = true;
    x.for_each([&](const auto &v) {
      if ( !first ) o.raw(", ", 2);
      first = false;
      o.elem(v);
    });
    o.raw(" }", 2);

  } else if constexpr ( K == kind::option_like ) {
    using F1 = typename U::first_type;
    using F2 = typename U::second_type;
    if ( !x.has_value() ) {
      o.raw("none", 4);
    } else if ( x.is_first() ) {
      o.raw("first(", 6);
      if constexpr ( !micron::is_void_v<F1> ) o.elem(x.template cast<F1>());
      o.raw(")", 1);
    } else {
      o.raw("second(", 7);
      o.elem(x.template cast<F2>());
      o.raw(")", 1);
    }

  } else if constexpr ( K == kind::sum_visit ) {
    // the index is the only stable identity for an any<Ts...>
    if ( !x.has_value() ) {
      o.raw("none", 4);
    } else {
      o.raw("any#", 4);
      o.num(static_cast<u64>(x.index()));
      o.raw("(", 1);
      x.visit([&](const auto &v) { o.elem(v); });
      o.raw(")", 1);
    }

  } else if constexpr ( K == kind::tuple_like ) {
    o.raw("[", 1);
    micron::__print::__impl::emit_product(o, x, micron::make_index_sequence<micron::tuple_size_v<U>>{});
    o.raw("]", 1);

  } else if constexpr ( K == kind::sparse_mat ) {
    constexpr bool by_row = requires(const U t, usize i) { t.row_begin(i); };
    const usize major = x.outer.size() ? x.outer.size() - 1u : 0u;
    const usize lim = x.values.size() < x.inner.size() ? x.values.size() : x.inner.size();
    o.raw("{ ", 2);
    bool first = true;
    for ( usize m = 0; m < major; ++m ) {
      usize e = static_cast<usize>(x.outer.data()[m + 1]);
      if ( e > lim ) e = lim;      // a malformed/partially-built outer must not walk off inner
      for ( usize p = static_cast<usize>(x.outer.data()[m]); p < e; ++p ) {
        if ( !first ) o.raw(", ", 2);
        first = false;
        const usize minor = static_cast<usize>(x.inner.data()[p]);
        o.raw("(", 1);
        o.num(static_cast<u64>(by_row ? m : minor));
        o.raw(", ", 2);
        o.num(static_cast<u64>(by_row ? minor : m));
        o.raw("): ", 3);
        o.elem(x.values.data()[p]);
      }
    }
    o.raw(" }", 2);

  } else if constexpr ( K == kind::sparse_vec ) {
    o.raw("{ ", 2);
    for ( usize p = 0; p < x.values.size(); ++p ) {
      if ( p ) o.raw(", ", 2);
      o.num(static_cast<u64>(x.idx.data()[p]));
      o.raw(": ", 2);
      o.elem(x.values.data()[p]);
    }
    o.raw(" }", 2);

  } else if constexpr ( K == kind::matrix ) {
    const usize R = static_cast<usize>(x.rows);
    const usize C = static_cast<usize>(x.cols);
    o.raw("{ ", 2);
    for ( usize r = 0; r < R; ++r ) {
      if ( r ) o.raw(", ", 2);
      o.raw("{ ", 2);
      for ( usize c = 0; c < C; ++c ) {
        if ( c ) o.raw(", ", 2);
        o.elem(x.at(r, c));
      }
      o.raw(" }", 2);
    }
    o.raw(" }", 2);

  } else if constexpr ( K == kind::vector_n ) {
    o.raw("(", 1);
    for ( usize i = 0; i < static_cast<usize>(U::length); ++i ) {
      if ( i ) o.raw(", ", 2);
      o.elem(x[i]);
    }
    o.raw(")", 1);

  } else if constexpr ( K == kind::ndarray ) {
    if ( !x.size() ) [[unlikely]] {
      o.raw("{ }", 3);
    } else {
      micron::__print::__impl::emit_nd(o, x, 0, 0, static_cast<usize>(x.size()));
    }

  } else if constexpr ( K == kind::soa_rows ) {
    o.raw("{ ", 2);
    for ( usize r = 0; r < x.size(); ++r ) {
      if ( r ) o.raw(", ", 2);
      o.raw("[", 1);
      micron::__print::__impl::emit_soa_row(o, x, r, micron::make_index_sequence<U::column_count>{});
      o.raw("]", 1);
    }
    o.raw(" }", 2);

  } else if constexpr ( K == kind::bits ) {
    const usize nbits = static_cast<usize>(x.size()) * 8u;
    o.raw("0b", 2);
    char b[64];
    usize k = 0;
    for ( usize i = 0; i < nbits; ++i ) {
      b[k++] = x[i] ? '1' : '0';
      if ( k == sizeof(b) ) {
        o.raw(b, k);
        k = 0;
      }
    }
    if ( k ) o.raw(b, k);

  } else if constexpr ( K == kind::kv_iter_key ) {
    // residuals for untagged third party key/value maps
    o.raw("{ ", 2);
    bool first = true;
    for ( auto itr = x.begin(); itr != x.end(); ++itr ) {
      if constexpr ( requires {
                       { !itr->key } -> micron::convertible_to<bool>;
                     } ) {
        if ( !itr->key ) continue;
      }
      if ( !first ) o.raw(", ", 2);
      first = false;
      o.elem(itr->key);
      o.raw(": ", 2);
      o.elem(itr->value);
    }
    o.raw(" }", 2);

  } else if constexpr ( K == kind::kv_iter_ab ) {
    o.raw("{ ", 2);
    bool first = true;
    for ( auto itr = x.begin(); itr != x.end(); ++itr ) {
      if ( !first ) o.raw(", ", 2);
      first = false;
      auto entry = *itr;
      o.elem(entry.a);
      o.raw(": ", 2);
      o.elem(entry.b);
    }
    o.raw(" }", 2);

  } else if constexpr ( K == kind::sequence ) {
    o.raw("{ ", 2);
    bool first = true;
    for ( auto itr = x.cbegin(); itr != x.cend(); ++itr ) {
      if ( !first ) o.raw(", ", 2);
      first = false;
      // deducing from *itr mishandles cv-qualified and proxy iterators
      if constexpr ( requires { typename U::value_type; } )
        o.elem(static_cast<const typename U::value_type &>(*itr));
      else
        o.elem(*itr);
    }
    o.raw(" }", 2);

  } else if constexpr ( K == kind::indexed ) {
    o.raw("{ ", 2);
    for ( usize i = 0; i < static_cast<usize>(x.size()); ++i ) {
      if ( i ) o.raw(", ", 2);
      o.elem(x[i]);
    }
    o.raw(" }", 2);

  } else if constexpr ( K == kind::smart_ptr ) {
    if ( x.get() == nullptr ) [[unlikely]]
      o.raw("null", 4);
    else
      o.elem(*x.get());

  } else if constexpr ( K == kind::ptr_range ) {
    o.raw("<view ", 6);
    o.elem(static_cast<const void *>(x.begin()));
    if ( x.end() == nullptr ) [[unlikely]] {
      o.raw(" +?>", 4);
    } else {
      o.raw(" +", 2);
      o.num(static_cast<u64>(reinterpret_cast<const byte *>(x.end()) - reinterpret_cast<const byte *>(x.begin())));
      o.raw(">", 1);
    }

  } else if constexpr ( K == kind::nullable_opaque ) {
    if ( x.has_value() )
      o.raw("<opaque>", 8);
    else
      o.raw("none", 4);

  } else if constexpr ( K == kind::opaque ) {
    // '~' marks a container that cannot be enumerated coherently
    o.raw("{ ~", 3);
    if constexpr ( requires(const U t) {
                     { t.size() } -> micron::convertible_to<usize>;
                   } )
      o.num(static_cast<u64>(x.size()));
    o.raw(" }", 2);

  } else if constexpr ( K == kind::consuming ) {
    static_assert(sizeof(U) == 0, "micron::io: this range is single-pass and consuming (micron::generator); printing it would "
                                  "drain it. collect it first, e.g. io::echo(micron::vector<T>(g.begin(), g.end()))");
  }
}

};      // namespace __print
};      // namespace micron
