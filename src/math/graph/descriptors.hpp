//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../numerics.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "tags.hpp"

namespace micron::math
{

struct empty_property {
  using micron_printable_tag = void;

  template<typename Out>
  constexpr void
  __micron_print(Out &out) const
  {
    out.raw("{}", 2);
  }

  friend constexpr bool operator==(empty_property, empty_property) noexcept = default;
};

template<micron::integral I> struct vertex_id {
  using index_type = I;
  using micron_printable_tag = void;

  I value{ invalid_value() };

  constexpr vertex_id() noexcept = default;

  constexpr explicit vertex_id(I v) noexcept : value(v) { }

  template<micron::integral J>
    requires(sizeof(J) <= sizeof(I))
  constexpr explicit vertex_id(J v) noexcept : value(static_cast<I>(v))
  {
  }

  [[nodiscard]] static constexpr I
  invalid_value() noexcept
  {
    return micron::numeric_limits<I>::max();
  }

  [[nodiscard]] static constexpr vertex_id
  invalid() noexcept
  {
    return vertex_id(invalid_value());
  }

  [[nodiscard]] constexpr bool
  valid() const noexcept
  {
    return value != invalid_value();
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return valid();
  }

  constexpr explicit
  operator I() const noexcept
  {
    return value;
  }

  template<typename Out>
  constexpr void
  __micron_print(Out &out) const
  {
    if ( valid() )
      out.num(static_cast<u64>(value));
    else
      out.raw("invalid", 7);
  }

  friend constexpr bool
  operator==(vertex_id a, vertex_id b) noexcept
  {
    return a.value == b.value;
  }

  friend constexpr bool
  operator!=(vertex_id a, vertex_id b) noexcept
  {
    return a.value != b.value;
  }

  friend constexpr bool
  operator<(vertex_id a, vertex_id b) noexcept
  {
    return a.value < b.value;
  }

  friend constexpr bool
  operator>(vertex_id a, vertex_id b) noexcept
  {
    return a.value > b.value;
  }

  friend constexpr bool
  operator<=(vertex_id a, vertex_id b) noexcept
  {
    return a.value <= b.value;
  }

  friend constexpr bool
  operator>=(vertex_id a, vertex_id b) noexcept
  {
    return a.value >= b.value;
  }
};

template<micron::integral I> struct edge_id {
  using index_type = I;
  using micron_printable_tag = void;

  I value{ invalid_value() };

  constexpr edge_id() noexcept = default;

  constexpr explicit edge_id(I v) noexcept : value(v) { }

  template<micron::integral J>
    requires(sizeof(J) <= sizeof(I))
  constexpr explicit edge_id(J v) noexcept : value(static_cast<I>(v))
  {
  }

  [[nodiscard]] static constexpr I
  invalid_value() noexcept
  {
    return micron::numeric_limits<I>::max();
  }

  [[nodiscard]] static constexpr edge_id
  invalid() noexcept
  {
    return edge_id(invalid_value());
  }

  [[nodiscard]] constexpr bool
  valid() const noexcept
  {
    return value != invalid_value();
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return valid();
  }

  constexpr explicit
  operator I() const noexcept
  {
    return value;
  }

  template<typename Out>
  constexpr void
  __micron_print(Out &out) const
  {
    if ( valid() )
      out.num(static_cast<u64>(value));
    else
      out.raw("invalid", 7);
  }

  friend constexpr bool
  operator==(edge_id a, edge_id b) noexcept
  {
    return a.value == b.value;
  }

  friend constexpr bool
  operator!=(edge_id a, edge_id b) noexcept
  {
    return a.value != b.value;
  }

  friend constexpr bool
  operator<(edge_id a, edge_id b) noexcept
  {
    return a.value < b.value;
  }

  friend constexpr bool
  operator>(edge_id a, edge_id b) noexcept
  {
    return a.value > b.value;
  }

  friend constexpr bool
  operator<=(edge_id a, edge_id b) noexcept
  {
    return a.value <= b.value;
  }

  friend constexpr bool
  operator>=(edge_id a, edge_id b) noexcept
  {
    return a.value >= b.value;
  }
};

static_assert(micron::is_trivially_copyable_v<vertex_id<u32>>);
static_assert(micron::is_trivially_copyable_v<edge_id<u32>>);

template<micron::integral I> struct edge_insert_result {
  graphs::edge_insert_status status{ graphs::edge_insert_status::invalid_vertex };
  edge_id<I> id{};

  [[nodiscard]] constexpr bool
  inserted() const noexcept
  {
    return status == graphs::edge_insert_status::inserted;
  }

  [[nodiscard]] constexpr bool
  duplicate() const noexcept
  {
    return status == graphs::edge_insert_status::duplicate;
  }

  [[nodiscard]] constexpr explicit
  operator bool() const noexcept
  {
    return inserted();
  }
};

template<micron::integral I> struct compact_result {
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> vertex_remap;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edge_remap;
};

template<micron::integral I = u32, typename P = empty_property> struct edge {
  using index_type = I;
  using property_type = P;

  I source{};
  I target{};
  [[no_unique_address]] P property{};
};

template<micron::integral I> edge(I, I) -> edge<I, empty_property>;
template<micron::integral I, typename P> edge(I, I, P) -> edge<I, micron::remove_cvref_t<P>>;

template<typename Weight, typename Property = empty_property> struct weighted_property {
  using weight_type = Weight;
  using property_type = Property;

  Weight weight{};
  [[no_unique_address]] Property property{};

  constexpr weighted_property()
    requires(micron::is_default_constructible_v<Weight> && micron::is_default_constructible_v<Property>)
  = default;

  constexpr explicit weighted_property(const Weight &w)
    requires micron::is_default_constructible_v<Property>
      : weight(w), property()
  {
  }

  constexpr explicit weighted_property(Weight &&w)
    requires micron::is_default_constructible_v<Property>
      : weight(micron::move(w)), property()
  {
  }

  constexpr weighted_property(const Weight &w, const Property &p) : weight(w), property(p) { }

  constexpr weighted_property(Weight &&w, Property &&p) : weight(micron::move(w)), property(micron::move(p)) { }

  using micron_printable_tag = void;

  template<typename Out>
  constexpr void
  __micron_print(Out &out) const
  {
    out.raw("{ weight: ", 10);
    out.elem(weight);
    if constexpr ( !micron::is_same_v<Property, empty_property> ) {
      out.raw(", property: ", 12);
      out.elem(property);
    }
    out.raw(" }", 2);
  }
};

template<typename Label, typename Property = empty_property> struct labeled_property {
  using label_type = Label;
  using property_type = Property;

  Label label;
  [[no_unique_address]] Property property;

  constexpr labeled_property()
    requires(micron::is_default_constructible_v<Label> && micron::is_default_constructible_v<Property>)
  = default;

  constexpr explicit labeled_property(const Label &l)
    requires micron::is_default_constructible_v<Property>
      : label(l), property()
  {
  }

  constexpr explicit labeled_property(Label &&l)
    requires micron::is_default_constructible_v<Property>
      : label(micron::move(l)), property()
  {
  }

  constexpr labeled_property(const Label &l, const Property &p) : label(l), property(p) { }

  constexpr labeled_property(Label &&l, Property &&p) : label(micron::move(l)), property(micron::move(p)) { }

  using micron_printable_tag = void;

  template<typename Out>
  constexpr void
  __micron_print(Out &out) const
  {
    out.raw("{ label: ", 9);
    out.elem(label);
    if constexpr ( !micron::is_same_v<Property, empty_property> ) {
      out.raw(", property: ", 12);
      out.elem(property);
    }
    out.raw(" }", 2);
  }
};

namespace graphs
{
template<typename T>
concept weighted_bundle = requires(T t) {
  typename T::weight_type;
  t.weight;
};

template<typename T>
concept labeled_bundle = requires(T t) {
  typename T::label_type;
  t.label;
};
};      // namespace graphs

};      // namespace micron::math
