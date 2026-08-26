//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../math/graph.hpp"
#include "../string/conversions/chars.hpp"
#include "../string/strings.hpp"
#include "../sum.hpp"
#include "bits.hpp"
#include "fsys.hpp"

namespace micron::io::graph
{

enum class parse_code : u8 {
  ok = 0,
  invalid_token,
  missing_endpoint,
  index_overflow,
  topology_rejected,
  unsupported,
  truncated,
  corrupt,
  unknown_version,
  incompatible_graph
};

struct parse_status {
  parse_code code{ parse_code::ok };
  usize byte{};
  usize line{ 1 };
  usize column{ 1 };
  math::graphs::edge_insert_status insertion{ math::graphs::edge_insert_status::inserted };

  [[nodiscard]] constexpr explicit
  operator bool() const noexcept
  {
    return code == parse_code::ok;
  }
};

struct native_property_codec {
  template<typename P>
    requires micron::is_trivially_copyable_v<P>
  [[nodiscard]] bool
  encode(const P &property, micron::vector<byte, micron::allocator_serial<>, false> &output) const
  {
    const byte *data = reinterpret_cast<const byte *>(micron::addressof(property));
    output.reserve(output.size() + sizeof(P));
    for ( usize i = 0; i < sizeof(P); ++i ) output.push_back(data[i]);
    return true;
  }

  template<typename P>
    requires micron::is_trivially_copyable_v<P>
  [[nodiscard]] bool
  decode(const byte *data, usize size, P &property) const noexcept
  {
    if ( size != sizeof(P) ) return false;
    byte *output = reinterpret_cast<byte *>(micron::addressof(property));
    for ( usize i = 0; i < sizeof(P); ++i ) output[i] = data[i];
    return true;
  }

  template<typename P>
    requires micron::is_trivially_copyable_v<P>
  [[nodiscard]] bool
  decode_construct(const byte *data, usize size, void *output) const noexcept
  {
    if ( size != sizeof(P) || output == nullptr ) return false;

    struct bytes_type {
      byte value[sizeof(P)];
    } bytes{};

    for ( usize i = 0; i < sizeof(P); ++i ) bytes.value[i] = data[i];
    micron::construct_at(reinterpret_cast<P *>(output), __builtin_bit_cast(P, bytes));
    return true;
  }
};

template<typename G> struct parse_result {
  parse_status status{};
  G value{};
  micron::vector<math::vertex_id<typename G::index_type>, micron::allocator_serial<>, false> vertex_remap;
  micron::vector<math::edge_id<typename G::index_type>, micron::allocator_serial<>, false> edge_remap;

  [[nodiscard]] explicit
  operator bool() const noexcept
  {
    return status.code == parse_code::ok;
  }
};

namespace __impl
{

struct token {
  const char *data{};
  usize size{};
  usize column{};
};

inline bool
next_token(const char *line, usize size, usize &position, token &out) noexcept
{
  while ( position < size && (line[position] == ' ' || line[position] == '\t' || line[position] == '\r') ) ++position;
  if ( position >= size || line[position] == '#' || line[position] == '%' ) return false;
  const usize begin = position;
  while ( position < size && line[position] != ' ' && line[position] != '\t' && line[position] != '\r' && line[position] != '#'
          && line[position] != '%' )
    ++position;
  out = token{ line + begin, position - begin, begin + 1 };
  return out.size != 0;
}

template<typename T>
[[nodiscard]] bool
parse_number(const token &input, T &value)
{
  return micron::from_chars(value, input.data, input.size);
}

inline parse_code
insertion_code(math::graphs::edge_insert_status status) noexcept
{
  using S = math::graphs::edge_insert_status;
  if ( status == S::index_overflow ) return parse_code::index_overflow;
  return parse_code::topology_rejected;
}

template<typename G>
bool
insert(G &graph, typename G::index_type source, typename G::index_type target, const token *property, parse_status &status)
{
  math::edge_insert_result<typename G::index_type> inserted;
  if constexpr ( math::graphs::weighted_bundle<typename G::edge_property_type> ) {
    using weight_type = typename G::edge_property_type::weight_type;
    if ( property == nullptr ) {
      status.code = parse_code::missing_endpoint;
      return false;
    }
    weight_type weight{};
    if ( !parse_number(*property, weight) ) {
      status.code = parse_code::invalid_token;
      status.column = property->column;
      return false;
    }
    inserted = graph.add_edge(source, target, weight);
  } else {
    inserted = graph.add_edge(source, target);
  }
  if ( !inserted.inserted() ) {
    status.code = insertion_code(inserted.status);
    status.insertion = inserted.status;
    return false;
  }
  return true;
}

template<typename G>
parse_result<G>
fail(parse_status status)
{
  parse_result<G> result;
  result.status = status;
  return result;
}

inline void
append_char(micron::string &out, char value)
{
  out.push_back(value);
}

inline void
append_text(micron::string &out, const char *value, usize size)
{
  out.append(value, size);
}

template<typename T>
bool
append_number(micron::string &out, T value)
{
  char buffer[160];
  const usize size = micron::to_chars(buffer, sizeof(buffer), value);
  if ( size == 0 ) return false;
  out.append(buffer, size);
  return true;
}

inline void
put_u16(micron::vector<byte, micron::allocator_serial<>, false> &out, u16 value)
{
  out.push_back(static_cast<byte>(value));
  out.push_back(static_cast<byte>(value >> 8u));
}

inline void
put_u64(micron::vector<byte, micron::allocator_serial<>, false> &out, u64 value)
{
  for ( usize i = 0; i < 8; ++i ) out.push_back(static_cast<byte>(value >> (i * 8u)));
}

inline bool
get_u16(const byte *data, usize size, usize &position, u16 &value)
{
  if ( size - position < 2 ) return false;
  value = static_cast<u16>(data[position]) | static_cast<u16>(static_cast<u16>(data[position + 1]) << 8u);
  position += 2;
  return true;
}

inline bool
get_u64(const byte *data, usize size, usize &position, u64 &value)
{
  if ( size - position < 8 ) return false;
  value = 0;
  for ( usize i = 0; i < 8; ++i ) value |= static_cast<u64>(data[position + i]) << (i * 8u);
  position += 8;
  return true;
}

[[nodiscard]] inline u64
index_slot_limit(byte width) noexcept
{
  if ( width == 1 ) return 0xffull;
  if ( width == 2 ) return 0xffffull;
  if ( width == 4 ) return 0xffffffffull;
  return micron::numeric_limits<u64>::max();
}

template<typename Codec, typename P>
bool
put_property(micron::vector<byte, micron::allocator_serial<>, false> &output, const Codec &codec, const P &property)
{
  micron::vector<byte, micron::allocator_serial<>, false> encoded;
  if ( !codec.encode(property, encoded) ) return false;
  put_u64(output, encoded.size());
  output.reserve(output.size() + encoded.size());
  for ( byte value : encoded ) output.push_back(value);
  return true;
}

template<typename Codec, typename P>
bool
get_property(const byte *data, usize size, usize &position, const Codec &codec, P &property, bool &truncated)
{
  u64 encoded_size{};
  if ( !get_u64(data, size, position, encoded_size) || encoded_size > static_cast<u64>(size - position) ) {
    truncated = true;
    return false;
  }
  const bool decoded = codec.decode(data + position, static_cast<usize>(encoded_size), property);
  position += static_cast<usize>(encoded_size);
  return decoded;
}

template<typename P> struct decoded_property {
  micron::aligned_storage_t<sizeof(P), alignof(P)> storage;
  bool live{};

  decoded_property() = default;
  decoded_property(const decoded_property &) = delete;
  decoded_property &operator=(const decoded_property &) = delete;

  ~decoded_property()
  {
    if ( live ) micron::destroy_at(get());
  }

  [[nodiscard]] P *
  get() noexcept
  {
    return reinterpret_cast<P *>(micron::addressof(storage));
  }
};

template<typename Codec, typename P>
bool
get_property(const byte *data, usize size, usize &position, const Codec &codec, decoded_property<P> &property, bool &truncated)
{
  u64 encoded_size{};
  if ( !get_u64(data, size, position, encoded_size) || encoded_size > static_cast<u64>(size - position) ) {
    truncated = true;
    return false;
  }
  bool decoded = false;
  if constexpr ( requires { codec.template decode_construct<P>(data, usize{}, static_cast<void *>(property.get())); } ) {
    decoded = codec.template decode_construct<P>(data + position, static_cast<usize>(encoded_size), property.get());
    property.live = decoded;
  } else if constexpr ( micron::is_default_constructible_v<P> ) {
    micron::construct_at(property.get());
    property.live = true;
    decoded = codec.decode(data + position, static_cast<usize>(encoded_size), *property.get());
  }
  position += static_cast<usize>(encoded_size);
  return decoded;
}

};      // namespace __impl

template<typename G = math::graph<>>
[[nodiscard]] parse_result<G>
parse_edge_list(const char *data, usize size)
{
  parse_result<G> result;
  usize position = 0;
  usize line_number = 1;
  while ( position < size ) {
    const usize line_start = position;
    while ( position < size && data[position] != '\n' ) ++position;
    const usize line_size = position - line_start;
    usize cursor = 0;
    __impl::token source_token{}, target_token{}, property_token{};
    if ( __impl::next_token(data + line_start, line_size, cursor, source_token) ) {
      if ( !__impl::next_token(data + line_start, line_size, cursor, target_token) )
        return __impl::fail<G>({ parse_code::missing_endpoint, line_start + cursor, line_number, cursor + 1 });
      typename G::index_type source{}, target{};
      if ( !__impl::parse_number(source_token, source) )
        return __impl::fail<G>({ parse_code::invalid_token, line_start + source_token.column - 1, line_number, source_token.column });
      if ( !__impl::parse_number(target_token, target) )
        return __impl::fail<G>({ parse_code::invalid_token, line_start + target_token.column - 1, line_number, target_token.column });
      const bool has_property = __impl::next_token(data + line_start, line_size, cursor, property_token);
      result.status = { parse_code::ok, line_start, line_number, source_token.column };
      if ( !__impl::insert(result.value, source, target, has_property ? micron::addressof(property_token) : nullptr, result.status) ) {
        result.status.byte = line_start + result.status.column - 1;
        result.status.line = line_number;
        result.value.clear();
        return result;
      }
    }
    if ( position < size ) ++position;
    ++line_number;
  }
  return result;
}

template<typename G = math::graph<>, micron::is_string S>
[[nodiscard]] parse_result<G>
parse_edge_list(const S &text)
{
  return parse_edge_list<G>(text.c_str(), micron::string_len(text));
}

template<typename G = math::graph<>>
[[nodiscard]] parse_result<G>
parse_adjacency_list(const char *data, usize size)
{
  parse_result<G> result;
  usize position = 0;
  usize line_number = 1;
  while ( position < size ) {
    const usize line_start = position;
    while ( position < size && data[position] != '\n' ) ++position;
    const usize line_size = position - line_start;
    usize cursor = 0;
    __impl::token source_token{};
    if ( __impl::next_token(data + line_start, line_size, cursor, source_token) ) {
      bool colon = source_token.size && source_token.data[source_token.size - 1] == ':';
      if ( colon ) --source_token.size;
      typename G::index_type source{};
      if ( !__impl::parse_number(source_token, source) )
        return __impl::fail<G>({ parse_code::invalid_token, line_start + source_token.column - 1, line_number, source_token.column });
      if ( !result.value.has_vertex(source) ) {
        if constexpr ( micron::is_default_constructible_v<typename G::vertex_property_type> ) {
          while ( result.value.vertex_slots() <= static_cast<usize>(source) )
            if ( !result.value.add_vertex().valid() ) return __impl::fail<G>({ parse_code::index_overflow, line_start, line_number, 1 });
        }
      }
      __impl::token target_token{};
      while ( __impl::next_token(data + line_start, line_size, cursor, target_token) ) {
        if ( target_token.size == 1 && target_token.data[0] == ':' ) continue;
        typename G::index_type target{};
        if ( !__impl::parse_number(target_token, target) ) {
          result.value.clear();
          return __impl::fail<G>({ parse_code::invalid_token, line_start + target_token.column - 1, line_number, target_token.column });
        }
        result.status = { parse_code::ok, line_start, line_number, target_token.column };
        if ( !__impl::insert(result.value, source, target, nullptr, result.status) ) {
          result.value.clear();
          return result;
        }
      }
    }
    if ( position < size ) ++position;
    ++line_number;
  }
  return result;
}

template<typename G = math::graph<>, micron::is_string S>
[[nodiscard]] parse_result<G>
parse_adjacency_list(const S &text)
{
  return parse_adjacency_list<G>(text.c_str(), micron::string_len(text));
}

template<typename G = math::graph<>>
[[nodiscard]] parse_result<G>
parse_matrix_market(const char *data, usize size)
{
  // Coordinate Matrix Market: banner, optional '%' comments, dimensions,
  // followed by one-based row/column[/value] records.
  usize position = 0;
  usize line = 1;
  bool dimensions = false;
  usize expected = 0;
  usize records = 0;
  parse_result<G> result;
  while ( position < size ) {
    const usize start = position;
    while ( position < size && data[position] != '\n' ) ++position;
    const usize length = position - start;
    if ( length && data[start] != '%' ) {
      usize cursor = 0;
      __impl::token a{}, b{}, c{};
      if ( __impl::next_token(data + start, length, cursor, a) ) {
        if ( !__impl::next_token(data + start, length, cursor, b) )
          return __impl::fail<G>({ parse_code::missing_endpoint, start, line, 1 });
        if ( !dimensions ) {
          if ( !__impl::next_token(data + start, length, cursor, c) || !__impl::parse_number(c, expected) )
            return __impl::fail<G>({ parse_code::invalid_token, start, line, 1 });
          usize rows{}, columns{};
          if ( !__impl::parse_number(a, rows) || !__impl::parse_number(b, columns) )
            return __impl::fail<G>({ parse_code::invalid_token, start, line, 1 });
          const usize vertices = rows > columns ? rows : columns;
          if constexpr ( micron::is_default_constructible_v<typename G::vertex_property_type> ) (void)result.value.add_vertices(vertices);
          dimensions = true;
        } else {
          typename G::index_type row{}, column{};
          if ( !__impl::parse_number(a, row) || !__impl::parse_number(b, column) || row == 0 || column == 0 )
            return __impl::fail<G>({ parse_code::invalid_token, start, line, 1 });
          --row;
          --column;
          const bool has_value = __impl::next_token(data + start, length, cursor, c);
          result.status = { parse_code::ok, start, line, 1 };
          if ( !__impl::insert(result.value, row, column, has_value ? micron::addressof(c) : nullptr, result.status) ) {
            result.value.clear();
            return result;
          }
          ++records;
        }
      }
    }
    if ( position < size ) ++position;
    ++line;
  }
  if ( !dimensions || records != expected ) return __impl::fail<G>({ parse_code::truncated, position, line, 1 });
  return result;
}

template<typename G = math::graph<>, micron::is_string S>
[[nodiscard]] parse_result<G>
parse_matrix_market(const S &text)
{
  return parse_matrix_market<G>(text.c_str(), micron::string_len(text));
}

template<typename G = math::graph<>>
[[nodiscard]] parse_result<G>
parse_dimacs(const char *data, usize size)
{
  parse_result<G> result;
  usize position = 0;
  usize line = 1;
  while ( position < size ) {
    const usize start = position;
    while ( position < size && data[position] != '\n' ) ++position;
    const usize length = position - start;
    usize cursor = 0;
    __impl::token kind{};
    if ( __impl::next_token(data + start, length, cursor, kind) && !(kind.size == 1 && kind.data[0] == 'c') ) {
      if ( kind.size == 1 && kind.data[0] == 'p' ) {
        __impl::token type{}, vertices{}, edges{};
        if ( !__impl::next_token(data + start, length, cursor, type) || !__impl::next_token(data + start, length, cursor, vertices)
             || !__impl::next_token(data + start, length, cursor, edges) )
          return __impl::fail<G>({ parse_code::invalid_token, start, line, 1 });
        usize count{};
        if ( !__impl::parse_number(vertices, count) ) return __impl::fail<G>({ parse_code::invalid_token, start, line, 1 });
        (void)result.value.add_vertices(count);
      } else if ( kind.size == 1 && (kind.data[0] == 'e' || kind.data[0] == 'a') ) {
        __impl::token a{}, b{}, weight{};
        if ( !__impl::next_token(data + start, length, cursor, a) || !__impl::next_token(data + start, length, cursor, b) )
          return __impl::fail<G>({ parse_code::missing_endpoint, start, line, 1 });
        typename G::index_type u{}, v{};
        if ( !__impl::parse_number(a, u) || !__impl::parse_number(b, v) || u == 0 || v == 0 )
          return __impl::fail<G>({ parse_code::invalid_token, start, line, 1 });
        --u;
        --v;
        const bool has_weight = __impl::next_token(data + start, length, cursor, weight);
        result.status = { parse_code::ok, start, line, 1 };
        if ( !__impl::insert(result.value, u, v, has_weight ? micron::addressof(weight) : nullptr, result.status) ) {
          result.value.clear();
          return result;
        }
      }
    }
    if ( position < size ) ++position;
    ++line;
  }
  return result;
}

template<typename G = math::graph<>, micron::is_string S>
[[nodiscard]] parse_result<G>
parse_dimacs(const S &text)
{
  return parse_dimacs<G>(text.c_str(), micron::string_len(text));
}

template<math::graphs::graph_model G>
[[nodiscard]] micron::string
edge_list(const G &graph)
{
  micron::string output;
  output.reserve(graph.edges_count() * 24);
  for ( auto edge : graph.edges() ) {
    (void)__impl::append_number(output, edge.source.value);
    __impl::append_char(output, ' ');
    (void)__impl::append_number(output, edge.target.value);
    if constexpr ( math::graphs::weighted_bundle<typename G::edge_property_type> ) {
      __impl::append_char(output, ' ');
      (void)__impl::append_number(output, edge.property.weight);
    }
    __impl::append_char(output, '\n');
  }
  return output;
}

template<math::graphs::graph_model G>
[[nodiscard]] micron::string
adjacency_list(const G &graph)
{
  micron::string output;
  for ( auto vertex : graph.vertices() ) {
    (void)__impl::append_number(output, vertex.value);
    __impl::append_text(output, ":", 1);
    for ( auto neighbor : graph.out_neighbors(vertex) ) {
      if constexpr ( !G::is_directed )
        if ( neighbor.value < vertex.value ) continue;
      __impl::append_char(output, ' ');
      (void)__impl::append_number(output, neighbor.value);
    }
    __impl::append_char(output, '\n');
  }
  return output;
}

template<math::graphs::graph_model G>
[[nodiscard]] micron::string
matrix_market(const G &graph)
{
  micron::string output("%%MatrixMarket matrix coordinate integer general\n");
  (void)__impl::append_number(output, graph.vertex_slots());
  __impl::append_char(output, ' ');
  (void)__impl::append_number(output, graph.vertex_slots());
  __impl::append_char(output, ' ');
  (void)__impl::append_number(output, graph.edges_count());
  __impl::append_char(output, '\n');
  for ( auto edge : graph.edges() ) {
    (void)__impl::append_number(output, static_cast<u64>(edge.source.value) + 1);
    __impl::append_char(output, ' ');
    (void)__impl::append_number(output, static_cast<u64>(edge.target.value) + 1);
    __impl::append_char(output, ' ');
    if constexpr ( math::graphs::weighted_bundle<typename G::edge_property_type> )
      (void)__impl::append_number(output, edge.property.weight);
    else
      __impl::append_char(output, '1');
    __impl::append_char(output, '\n');
  }
  return output;
}

template<math::graphs::graph_model G>
[[nodiscard]] micron::string
dimacs(const G &graph)
{
  micron::string output("p edge ");
  (void)__impl::append_number(output, graph.vertex_slots());
  __impl::append_char(output, ' ');
  (void)__impl::append_number(output, graph.edges_count());
  __impl::append_char(output, '\n');
  for ( auto edge : graph.edges() ) {
    __impl::append_text(output, G::is_directed ? "a " : "e ", 2);
    (void)__impl::append_number(output, static_cast<u64>(edge.source.value) + 1);
    __impl::append_char(output, ' ');
    (void)__impl::append_number(output, static_cast<u64>(edge.target.value) + 1);
    if constexpr ( math::graphs::weighted_bundle<typename G::edge_property_type> ) {
      __impl::append_char(output, ' ');
      (void)__impl::append_number(output, edge.property.weight);
    }
    __impl::append_char(output, '\n');
  }
  return output;
}

template<math::graphs::graph_model G>
[[nodiscard]] micron::vector<byte, micron::allocator_serial<>, false>
binary(const G &graph)
  requires(micron::is_same_v<typename G::vertex_property_type, math::empty_property>
           && micron::is_same_v<typename G::edge_property_type, math::empty_property>
           && micron::is_same_v<typename G::graph_property_type, math::empty_property>)
{
  micron::vector<byte, micron::allocator_serial<>, false> output;
  output.reserve(44 + graph.vertices_count() * 8 + graph.edges_count() * 24);
  output.push_back('M');
  output.push_back('C');
  output.push_back('G');
  output.push_back('F');
  __impl::put_u16(output, 3);
  output.push_back(1);      // little endian
  output.push_back(static_cast<byte>(sizeof(typename G::index_type)));
  output.push_back(static_cast<byte>((G::is_directed ? 1u : 0u) | (G::is_simple ? 0u : 2u) | (G::allows_loops ? 4u : 0u)));
  output.push_back(0);      // no property records
  output.push_back(0);
  output.push_back(0);
  __impl::put_u64(output, graph.vertex_slots());
  __impl::put_u64(output, graph.edge_slots());
  __impl::put_u64(output, graph.vertices_count());
  __impl::put_u64(output, graph.edges_count());
  for ( usize slot = 0; slot < graph.vertex_slots(); ++slot ) {
    const typename G::vertex_descriptor vertex(static_cast<typename G::index_type>(slot));
    if ( graph.has_vertex(vertex) ) __impl::put_u64(output, slot);
  }
  for ( usize slot = 0; slot < graph.edge_slots(); ++slot ) {
    const typename G::edge_descriptor id(static_cast<typename G::index_type>(slot));
    if ( graph.has_edge(id) ) {
      __impl::put_u64(output, slot);
      __impl::put_u64(output, static_cast<u64>(graph.source(id).value));
      __impl::put_u64(output, static_cast<u64>(graph.target(id).value));
    }
  }
  return output;
}

template<math::graphs::graph_model G, typename Codec>
[[nodiscard]] micron::vector<byte, micron::allocator_serial<>, false>
binary(const G &graph, const Codec &codec)
  requires requires(micron::vector<byte, micron::allocator_serial<>, false> &output,
                    const typename G::vertex_property_type &vertex_property, const typename G::edge_property_type &edge_property,
                    const typename G::graph_property_type &graph_property) {
    { codec.encode(vertex_property, output) } -> micron::convertible_to<bool>;
    { codec.encode(edge_property, output) } -> micron::convertible_to<bool>;
    { codec.encode(graph_property, output) } -> micron::convertible_to<bool>;
  }
{
  micron::vector<byte, micron::allocator_serial<>, false> output;
  output.reserve(64 + graph.vertices_count() * 16 + graph.edges_count() * 40);
  output.push_back('M');
  output.push_back('C');
  output.push_back('G');
  output.push_back('F');
  __impl::put_u16(output, 3);
  output.push_back(1);
  output.push_back(static_cast<byte>(sizeof(typename G::index_type)));
  output.push_back(static_cast<byte>((G::is_directed ? 1u : 0u) | (G::is_simple ? 0u : 2u) | (G::allows_loops ? 4u : 0u)));
  output.push_back(1);      // typed property records present
  output.push_back(0);
  output.push_back(0);
  __impl::put_u64(output, graph.vertex_slots());
  __impl::put_u64(output, graph.edge_slots());
  __impl::put_u64(output, graph.vertices_count());
  __impl::put_u64(output, graph.edges_count());
  if ( !__impl::put_property(output, codec, graph.graph_property()) ) return {};
  for ( usize slot = 0; slot < graph.vertex_slots(); ++slot ) {
    const typename G::vertex_descriptor vertex(static_cast<typename G::index_type>(slot));
    if ( !graph.has_vertex(vertex) ) continue;
    __impl::put_u64(output, slot);
    if ( !__impl::put_property(output, codec, graph.vertex_property_unchecked(vertex)) ) return {};
  }
  for ( usize slot = 0; slot < graph.edge_slots(); ++slot ) {
    const typename G::edge_descriptor edge(static_cast<typename G::index_type>(slot));
    if ( !graph.has_edge(edge) ) continue;
    __impl::put_u64(output, slot);
    __impl::put_u64(output, static_cast<u64>(graph.source(edge).value));
    __impl::put_u64(output, static_cast<u64>(graph.target(edge).value));
    if ( !__impl::put_property(output, codec, graph.edge_property_unchecked(edge)) ) return {};
  }
  return output;
}

template<typename G = math::graph<>>
[[nodiscard]] parse_result<G>
parse_binary(const byte *data, usize size)
  requires(micron::is_same_v<typename G::vertex_property_type, math::empty_property>
           && micron::is_same_v<typename G::edge_property_type, math::empty_property>
           && micron::is_same_v<typename G::graph_property_type, math::empty_property> && math::graphs::mutable_graph_model<G>)
{
  parse_result<G> result;
  if ( size < 44 ) return __impl::fail<G>({ parse_code::truncated, size, 1, size + 1 });
  if ( data[0] != 'M' || data[1] != 'C' || data[2] != 'G' || data[3] != 'F' ) return __impl::fail<G>({ parse_code::corrupt, 0, 1, 1 });
  usize position = 4;
  u16 version{};
  if ( !__impl::get_u16(data, size, position, version) ) return __impl::fail<G>({ parse_code::truncated, position });
  if ( version != 3 ) return __impl::fail<G>({ parse_code::unknown_version, 4, 1, 5 });
  const byte endian = data[position++];
  const byte index_width = data[position++];
  const byte flags = data[position++];
  const byte property_flags = data[position++];
  const byte reserved_b = data[position++];
  const byte reserved_c = data[position++];
  if ( (flags & ~byte(7)) != 0 || property_flags != 0 || reserved_b != 0 || reserved_c != 0 )
    return __impl::fail<G>({ parse_code::corrupt, 9, 1, 10 });
  const bool valid_index_width = index_width == 1 || index_width == 2 || index_width == 4 || index_width == 8;
  if ( endian != 1 || !valid_index_width || index_width > sizeof(typename G::index_type) || static_cast<bool>(flags & 1u) != G::is_directed
       || static_cast<bool>(flags & 2u) == G::is_simple || static_cast<bool>(flags & 4u) != G::allows_loops )
    return __impl::fail<G>({ parse_code::incompatible_graph, 6, 1, 7 });
  u64 vertex_slots{}, edge_slots{}, live_vertices{}, live_edges{};
  if ( !__impl::get_u64(data, size, position, vertex_slots) || !__impl::get_u64(data, size, position, edge_slots)
       || !__impl::get_u64(data, size, position, live_vertices) || !__impl::get_u64(data, size, position, live_edges) )
    return __impl::fail<G>({ parse_code::truncated, position });
  if ( vertex_slots > static_cast<u64>(math::vertex_id<typename G::index_type>::invalid_value())
       || edge_slots > static_cast<u64>(math::edge_id<typename G::index_type>::invalid_value())
       || vertex_slots > __impl::index_slot_limit(index_width) || edge_slots > __impl::index_slot_limit(index_width)
       || vertex_slots > static_cast<u64>(micron::numeric_limits<usize>::max())
       || edge_slots > static_cast<u64>(micron::numeric_limits<usize>::max()) )
    return __impl::fail<G>({ parse_code::index_overflow, position });
  if ( live_vertices > vertex_slots || live_edges > edge_slots ) return __impl::fail<G>({ parse_code::corrupt, position });
  result.vertex_remap = decltype(result.vertex_remap)(static_cast<usize>(vertex_slots), math::vertex_id<typename G::index_type>::invalid());
  result.edge_remap = decltype(result.edge_remap)(static_cast<usize>(edge_slots), math::edge_id<typename G::index_type>::invalid());
  constexpr bool stable_target = micron::is_same_v<typename G::storage_type, math::graphs::stable_adjacency_t>;
  u64 previous_vertex = micron::numeric_limits<u64>::max();
  for ( u64 record = 0; record < live_vertices; ++record ) {
    u64 slot{};
    if ( !__impl::get_u64(data, size, position, slot) ) return __impl::fail<G>({ parse_code::truncated, position });
    if ( slot >= vertex_slots || (record != 0 && slot <= previous_vertex) ) return __impl::fail<G>({ parse_code::corrupt, position - 8 });
    if constexpr ( stable_target )
      while ( result.value.vertex_slots() < static_cast<usize>(slot) )
        if ( !result.value.__import_dead_vertex_slot() ) return __impl::fail<G>({ parse_code::index_overflow, position - 8 });
    const auto inserted = result.value.add_vertex();
    if ( !inserted.valid() ) return __impl::fail<G>({ parse_code::index_overflow, position - 8 });
    result.vertex_remap.data()[static_cast<usize>(slot)] = inserted;
    previous_vertex = slot;
  }
  if constexpr ( stable_target )
    while ( result.value.vertex_slots() < static_cast<usize>(vertex_slots) )
      if ( !result.value.__import_dead_vertex_slot() ) return __impl::fail<G>({ parse_code::index_overflow, position });

  u64 previous_edge = micron::numeric_limits<u64>::max();
  for ( u64 record = 0; record < live_edges; ++record ) {
    u64 slot{}, source{}, target{};
    if ( !__impl::get_u64(data, size, position, slot) || !__impl::get_u64(data, size, position, source)
         || !__impl::get_u64(data, size, position, target) )
      return __impl::fail<G>({ parse_code::truncated, position });
    if ( slot >= edge_slots || (record != 0 && slot <= previous_edge) || source >= vertex_slots || target >= vertex_slots
         || !result.vertex_remap.data()[static_cast<usize>(source)].valid()
         || !result.vertex_remap.data()[static_cast<usize>(target)].valid() )
      return __impl::fail<G>({ parse_code::corrupt, position - 24 });
    if constexpr ( stable_target )
      while ( result.value.edge_slots() < static_cast<usize>(slot) )
        if ( !result.value.__import_dead_edge_slot() ) return __impl::fail<G>({ parse_code::index_overflow, position - 24 });
    auto inserted = result.value.add_edge(result.vertex_remap.data()[static_cast<usize>(source)],
                                          result.vertex_remap.data()[static_cast<usize>(target)]);
    if ( !inserted.inserted() ) return __impl::fail<G>({ parse_code::topology_rejected, position - 24 });
    result.edge_remap.data()[static_cast<usize>(slot)] = inserted.id;
    previous_edge = slot;
  }
  if constexpr ( stable_target )
    while ( result.value.edge_slots() < static_cast<usize>(edge_slots) )
      if ( !result.value.__import_dead_edge_slot() ) return __impl::fail<G>({ parse_code::index_overflow, position });
  if ( position != size || result.value.vertices_count() != live_vertices || result.value.edges_count() != live_edges )
    return __impl::fail<G>({ parse_code::corrupt, position });
  return result;
}

template<typename G>
[[nodiscard]] parse_result<G>
parse_binary(const byte *data, usize size)
  requires(micron::is_same_v<typename G::vertex_property_type, math::empty_property>
           && micron::is_same_v<typename G::edge_property_type, math::empty_property>
           && micron::is_same_v<typename G::graph_property_type, math::empty_property>
           && (micron::is_same_v<typename G::storage_type, math::graphs::csr_t>
               || micron::is_same_v<typename G::storage_type, math::graphs::bidirectional_csr_t>))
{
  using Temp = math::graphs::thawed_graph_t<G>;
  auto decoded = parse_binary<Temp>(data, size);
  if ( !decoded ) return __impl::fail<G>(decoded.status);
  auto finish = [&](auto frozen) {
    parse_result<G> result;
    result.value = micron::move(frozen.value);
    result.vertex_remap.resize(decoded.vertex_remap.size(), math::vertex_id<typename G::index_type>::invalid());
    result.edge_remap.resize(decoded.edge_remap.size(), math::edge_id<typename G::index_type>::invalid());
    for ( usize slot = 0; slot < decoded.vertex_remap.size(); ++slot ) {
      const auto temporary = decoded.vertex_remap.data()[slot];
      if ( temporary.valid() ) result.vertex_remap.data()[slot] = frozen.vertex_remap.data()[static_cast<usize>(temporary.value)];
    }
    for ( usize slot = 0; slot < decoded.edge_remap.size(); ++slot ) {
      const auto temporary = decoded.edge_remap.data()[slot];
      if ( temporary.valid() ) result.edge_remap.data()[slot] = frozen.edge_remap.data()[static_cast<usize>(temporary.value)];
    }
    return result;
  };
  if constexpr ( micron::is_same_v<typename G::storage_type, math::graphs::bidirectional_csr_t> )
    return finish(math::graphs::freeze_bidirectional(decoded.value));
  else
    return finish(math::graphs::freeze(decoded.value));
}

template<typename G = math::graph<>>
[[nodiscard]] parse_result<G>
parse_binary(const micron::vector<byte, micron::allocator_serial<>, false> &data)
{
  return parse_binary<G>(data.data(), data.size());
}

template<typename G = math::graph<>, typename Codec>
[[nodiscard]] parse_result<G>
parse_binary(const byte *data, usize size, const Codec &codec)
  requires(micron::is_default_constructible_v<typename G::graph_property_type> && math::graphs::mutable_graph_model<G>)
{
  parse_result<G> result;
  if ( size < 44 ) return __impl::fail<G>({ parse_code::truncated, size, 1, size + 1 });
  if ( data[0] != 'M' || data[1] != 'C' || data[2] != 'G' || data[3] != 'F' ) return __impl::fail<G>({ parse_code::corrupt, 0, 1, 1 });
  usize position = 4;
  u16 version{};
  if ( !__impl::get_u16(data, size, position, version) ) return __impl::fail<G>({ parse_code::truncated, position });
  if ( version != 3 ) return __impl::fail<G>({ parse_code::unknown_version, 4, 1, 5 });
  const byte endian = data[position++];
  const byte index_width = data[position++];
  const byte flags = data[position++];
  const byte property_flags = data[position++];
  const byte reserved_a = data[position++];
  const byte reserved_b = data[position++];
  if ( (flags & ~byte(7)) != 0 || reserved_a != 0 || reserved_b != 0 ) return __impl::fail<G>({ parse_code::corrupt, 10, 1, 11 });
  const bool valid_index_width = index_width == 1 || index_width == 2 || index_width == 4 || index_width == 8;
  if ( endian != 1 || property_flags != 1 || !valid_index_width || index_width > sizeof(typename G::index_type)
       || static_cast<bool>(flags & 1u) != G::is_directed || static_cast<bool>(flags & 2u) == G::is_simple
       || static_cast<bool>(flags & 4u) != G::allows_loops )
    return __impl::fail<G>({ parse_code::incompatible_graph, 6, 1, 7 });
  u64 vertex_slots{}, edge_slots{}, live_vertices{}, live_edges{};
  if ( !__impl::get_u64(data, size, position, vertex_slots) || !__impl::get_u64(data, size, position, edge_slots)
       || !__impl::get_u64(data, size, position, live_vertices) || !__impl::get_u64(data, size, position, live_edges) )
    return __impl::fail<G>({ parse_code::truncated, position });
  if ( vertex_slots > static_cast<u64>(math::vertex_id<typename G::index_type>::invalid_value())
       || edge_slots > static_cast<u64>(math::edge_id<typename G::index_type>::invalid_value())
       || vertex_slots > __impl::index_slot_limit(index_width) || edge_slots > __impl::index_slot_limit(index_width)
       || vertex_slots > static_cast<u64>(micron::numeric_limits<usize>::max())
       || edge_slots > static_cast<u64>(micron::numeric_limits<usize>::max()) )
    return __impl::fail<G>({ parse_code::index_overflow, position });
  if ( live_vertices > vertex_slots || live_edges > edge_slots ) return __impl::fail<G>({ parse_code::corrupt, position });
  result.vertex_remap = decltype(result.vertex_remap)(static_cast<usize>(vertex_slots), math::vertex_id<typename G::index_type>::invalid());
  result.edge_remap = decltype(result.edge_remap)(static_cast<usize>(edge_slots), math::edge_id<typename G::index_type>::invalid());
  constexpr bool stable_target = micron::is_same_v<typename G::storage_type, math::graphs::stable_adjacency_t>;

  bool truncated = false;
  if ( !__impl::get_property(data, size, position, codec, result.value.graph_property(), truncated) )
    return __impl::fail<G>({ truncated ? parse_code::truncated : parse_code::corrupt, position });
  u64 previous_vertex = micron::numeric_limits<u64>::max();
  for ( u64 record = 0; record < live_vertices; ++record ) {
    u64 slot{};
    if ( !__impl::get_u64(data, size, position, slot) ) return __impl::fail<G>({ parse_code::truncated, position });
    if ( slot >= vertex_slots || (record != 0 && slot <= previous_vertex) ) return __impl::fail<G>({ parse_code::corrupt, position - 8 });
    if constexpr ( stable_target )
      while ( result.value.vertex_slots() < static_cast<usize>(slot) )
        if ( !result.value.__import_dead_vertex_slot() ) return __impl::fail<G>({ parse_code::index_overflow, position - 8 });
    __impl::decoded_property<typename G::vertex_property_type> property;
    truncated = false;
    if ( !__impl::get_property(data, size, position, codec, property, truncated) )
      return __impl::fail<G>({ truncated ? parse_code::truncated : parse_code::corrupt, position });
    const auto inserted = result.value.add_vertex(micron::move(*property.get()));
    if ( !inserted.valid() ) return __impl::fail<G>({ parse_code::index_overflow, position });
    result.vertex_remap.data()[static_cast<usize>(slot)] = inserted;
    previous_vertex = slot;
  }
  if constexpr ( stable_target )
    while ( result.value.vertex_slots() < static_cast<usize>(vertex_slots) )
      if ( !result.value.__import_dead_vertex_slot() ) return __impl::fail<G>({ parse_code::index_overflow, position });

  u64 previous_edge = micron::numeric_limits<u64>::max();
  for ( u64 record = 0; record < live_edges; ++record ) {
    u64 slot{}, source{}, target{};
    if ( !__impl::get_u64(data, size, position, slot) || !__impl::get_u64(data, size, position, source)
         || !__impl::get_u64(data, size, position, target) )
      return __impl::fail<G>({ parse_code::truncated, position });
    if ( slot >= edge_slots || (record != 0 && slot <= previous_edge) || source >= vertex_slots || target >= vertex_slots
         || !result.vertex_remap.data()[static_cast<usize>(source)].valid()
         || !result.vertex_remap.data()[static_cast<usize>(target)].valid() )
      return __impl::fail<G>({ parse_code::corrupt, position - 24 });
    __impl::decoded_property<typename G::edge_property_type> property;
    truncated = false;
    if ( !__impl::get_property(data, size, position, codec, property, truncated) )
      return __impl::fail<G>({ truncated ? parse_code::truncated : parse_code::corrupt, position });
    if constexpr ( stable_target )
      while ( result.value.edge_slots() < static_cast<usize>(slot) )
        if ( !result.value.__import_dead_edge_slot() ) return __impl::fail<G>({ parse_code::index_overflow, position });
    auto inserted = result.value.add_edge(result.vertex_remap.data()[static_cast<usize>(source)],
                                          result.vertex_remap.data()[static_cast<usize>(target)], micron::move(*property.get()));
    if ( !inserted.inserted() ) return __impl::fail<G>({ parse_code::topology_rejected, position });
    result.edge_remap.data()[static_cast<usize>(slot)] = inserted.id;
    previous_edge = slot;
  }
  if constexpr ( stable_target )
    while ( result.value.edge_slots() < static_cast<usize>(edge_slots) )
      if ( !result.value.__import_dead_edge_slot() ) return __impl::fail<G>({ parse_code::index_overflow, position });
  if ( position != size || result.value.vertices_count() != live_vertices || result.value.edges_count() != live_edges )
    return __impl::fail<G>({ parse_code::corrupt, position });
  return result;
}

template<typename G = math::graph<>, typename Codec>
[[nodiscard]] parse_result<G>
parse_binary(const byte *data, usize size, const Codec &codec)
  requires(micron::is_default_constructible_v<typename G::graph_property_type>
           && (micron::is_same_v<typename G::storage_type, math::graphs::csr_t>
               || micron::is_same_v<typename G::storage_type, math::graphs::bidirectional_csr_t>))
{
  using Temp = math::graphs::thawed_graph_t<G>;
  auto decoded = parse_binary<Temp>(data, size, codec);
  if ( !decoded ) return __impl::fail<G>(decoded.status);
  auto finish = [&](auto frozen) {
    parse_result<G> result;
    result.value = micron::move(frozen.value);
    result.vertex_remap.resize(decoded.vertex_remap.size(), math::vertex_id<typename G::index_type>::invalid());
    result.edge_remap.resize(decoded.edge_remap.size(), math::edge_id<typename G::index_type>::invalid());
    for ( usize slot = 0; slot < decoded.vertex_remap.size(); ++slot ) {
      const auto temporary = decoded.vertex_remap.data()[slot];
      if ( temporary.valid() ) result.vertex_remap.data()[slot] = frozen.vertex_remap.data()[static_cast<usize>(temporary.value)];
    }
    for ( usize slot = 0; slot < decoded.edge_remap.size(); ++slot ) {
      const auto temporary = decoded.edge_remap.data()[slot];
      if ( temporary.valid() ) result.edge_remap.data()[slot] = frozen.edge_remap.data()[static_cast<usize>(temporary.value)];
    }
    return result;
  };
  if constexpr ( micron::is_same_v<typename G::storage_type, math::graphs::bidirectional_csr_t> )
    return finish(math::graphs::freeze_bidirectional(decoded.value));
  else
    return finish(math::graphs::freeze(decoded.value));
}

template<typename G = math::graph<>, typename Codec>
[[nodiscard]] parse_result<G>
parse_binary(const micron::vector<byte, micron::allocator_serial<>, false> &data, const Codec &codec)
{
  return parse_binary<G>(data.data(), data.size(), codec);
}

template<math::graphs::graph_model G>
max_t
write_edge_list(const io::path_t &path, const G &graph)
{
  return io::write_file(path, edge_list(graph));
}

template<math::graphs::graph_model G>
max_t
write_adjacency_list(const io::path_t &path, const G &graph)
{
  return io::write_file(path, adjacency_list(graph));
}

template<math::graphs::graph_model G>
max_t
write_matrix_market(const io::path_t &path, const G &graph)
{
  return io::write_file(path, matrix_market(graph));
}

template<math::graphs::graph_model G>
max_t
write_dimacs(const io::path_t &path, const G &graph)
{
  return io::write_file(path, dimacs(graph));
}

template<math::graphs::graph_model G>
max_t
write_binary(const io::path_t &path, const G &graph)
{
  return io::write_file(path, binary(graph));
}

template<math::graphs::graph_model G, typename Codec>
max_t
write_binary(const io::path_t &path, const G &graph, const Codec &codec)
{
  auto data = binary(graph, codec);
  return data.empty() ? max_t(-22) : io::write_file(path, data);
}

template<typename G = math::graph<>>
[[nodiscard]] micron::option<parse_result<G>, io::error_t>
read_edge_list(const io::path_t &path)
{
  auto contents = io::read_file<micron::string>(path);
  if ( contents.is_second() ) return micron::option<parse_result<G>, io::error_t>{ contents.template cast<io::error_t>() };
  auto parsed = parse_edge_list<G>(contents.template cast<micron::string>());
  return micron::option<parse_result<G>, io::error_t>{ micron::move(parsed) };
}

template<typename G = math::graph<>>
[[nodiscard]] micron::option<parse_result<G>, io::error_t>
read_adjacency_list(const io::path_t &path)
{
  auto contents = io::read_file<micron::string>(path);
  if ( contents.is_second() ) return micron::option<parse_result<G>, io::error_t>{ contents.template cast<io::error_t>() };
  auto parsed = parse_adjacency_list<G>(contents.template cast<micron::string>());
  return micron::option<parse_result<G>, io::error_t>{ micron::move(parsed) };
}

template<typename G = math::graph<>>
[[nodiscard]] micron::option<parse_result<G>, io::error_t>
read_matrix_market(const io::path_t &path)
{
  auto contents = io::read_file<micron::string>(path);
  if ( contents.is_second() ) return micron::option<parse_result<G>, io::error_t>{ contents.template cast<io::error_t>() };
  auto parsed = parse_matrix_market<G>(contents.template cast<micron::string>());
  return micron::option<parse_result<G>, io::error_t>{ micron::move(parsed) };
}

template<typename G = math::graph<>>
[[nodiscard]] micron::option<parse_result<G>, io::error_t>
read_dimacs(const io::path_t &path)
{
  auto contents = io::read_file<micron::string>(path);
  if ( contents.is_second() ) return micron::option<parse_result<G>, io::error_t>{ contents.template cast<io::error_t>() };
  auto parsed = parse_dimacs<G>(contents.template cast<micron::string>());
  return micron::option<parse_result<G>, io::error_t>{ micron::move(parsed) };
}

template<typename G = math::graph<>>
[[nodiscard]] micron::option<parse_result<G>, io::error_t>
read_binary(const io::path_t &path)
{
  auto contents = io::read_file<micron::vector<byte, micron::allocator_serial<>, false>>(path);
  if ( contents.is_second() ) return micron::option<parse_result<G>, io::error_t>{ contents.template cast<io::error_t>() };
  auto parsed = parse_binary<G>(contents.template cast<micron::vector<byte, micron::allocator_serial<>, false>>());
  return micron::option<parse_result<G>, io::error_t>{ micron::move(parsed) };
}

template<typename G = math::graph<>, typename Codec>
[[nodiscard]] micron::option<parse_result<G>, io::error_t>
read_binary(const io::path_t &path, const Codec &codec)
{
  auto contents = io::read_file<micron::vector<byte, micron::allocator_serial<>, false>>(path);
  if ( contents.is_second() ) return micron::option<parse_result<G>, io::error_t>{ contents.template cast<io::error_t>() };
  auto parsed = parse_binary<G>(contents.template cast<micron::vector<byte, micron::allocator_serial<>, false>>(), codec);
  return micron::option<parse_result<G>, io::error_t>{ micron::move(parsed) };
}

};      // namespace micron::io::graph
