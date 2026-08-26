//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/graph.hpp"
#include "../../src/math/graph.hpp"
#include "../snowball/snowball.hpp"

namespace mm = micron::math;
namespace mg = micron::math::graphs;
namespace mig = micron::io::graph;

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

static u64
next_random(u64 &state) noexcept
{
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

template<typename G>
static void
packed_mutation_fuzz(u64 seed)
{
  constexpr usize limit = 20;
  micron::vector<u8> adjacency(limit * limit, u8{});
  const auto oracle = [&](usize source, usize target) -> u8 & { return adjacency.data()[source * limit + target]; };
  usize vertices = 12;
  G graph;
  require_true(graph.add_vertices(vertices).size() == vertices);

  auto verify = [&] {
    usize edges = 0;
    require_true(graph.vertices_count() == vertices && graph.vertex_slots() == vertices);
    for ( usize u = 0; u < vertices; ++u ) {
      usize degree = 0;
      bool seen[limit]{};
      for ( auto neighbor : graph.out_neighbors(mm::vertex_id<u32>(static_cast<u32>(u))) ) {
        require_true(static_cast<usize>(neighbor.value) < vertices);
        require_true(!seen[neighbor.value]);
        seen[neighbor.value] = true;
        ++degree;
      }
      for ( usize v = 0; v < vertices; ++v ) {
        require_true(graph.has_edge(static_cast<u32>(u), static_cast<u32>(v)) == static_cast<bool>(oracle(u, v)));
        require_true(seen[v] == static_cast<bool>(oracle(u, v)));
        if ( u < v && oracle(u, v) ) ++edges;
      }
      require_true(graph.out_degree(mm::vertex_id<u32>(static_cast<u32>(u))) == degree);
    }
    require_true(graph.edges_count() == edges);
  };

  for ( usize step = 0; step < 1200; ++step ) {
    const u64 operation = next_random(seed) % 6;
    if ( operation == 5 && vertices < limit ) {
      require_true(graph.add_vertex() == mm::vertex_id<u32>(static_cast<u32>(vertices)));
      ++vertices;
    } else if ( operation == 4 && vertices > 1 ) {
      const usize erased = static_cast<usize>(next_random(seed) % vertices);
      const usize last = vertices - 1;
      require_true(graph.remove_vertex(static_cast<u32>(erased)));
      if ( erased != last ) {
        for ( usize i = 0; i < last; ++i ) {
          if ( i == erased ) continue;
          oracle(erased, i) = oracle(last, i);
          oracle(i, erased) = oracle(i, last);
        }
        oracle(erased, erased) = 0;
      }
      for ( usize i = 0; i < limit; ++i ) oracle(last, i) = oracle(i, last) = 0;
      --vertices;
    } else if ( vertices > 1 ) {
      const usize u = static_cast<usize>(next_random(seed) % vertices);
      usize v = static_cast<usize>(next_random(seed) % vertices);
      if ( u == v ) v = (v + 1) % vertices;
      if ( operation < 3 ) {
        const auto inserted = graph.add_edge(static_cast<u32>(u), static_cast<u32>(v));
        require_true(inserted.inserted() != static_cast<bool>(oracle(u, v)));
        oracle(u, v) = oracle(v, u) = 1;
      } else {
        require_true(graph.remove_edge(static_cast<u32>(u), static_cast<u32>(v)) == static_cast<bool>(oracle(u, v)));
        oracle(u, v) = oracle(v, u) = 0;
      }
    }
    verify();
  }
}

struct non_default_property {
  int value;
  non_default_property() = delete;

  explicit non_default_property(int input) : value(input) { }
};

struct tracked_property {
  inline static int live{};
  int value{};

  explicit tracked_property(int input) : value(input) { ++live; }

  tracked_property(const tracked_property &other) : value(other.value) { ++live; }

  tracked_property(tracked_property &&other) noexcept : value(other.value) { ++live; }

  tracked_property &operator=(const tracked_property &) = default;
  tracked_property &operator=(tracked_property &&) = default;

  ~tracked_property() { --live; }
};

int
main()
{
  test_case("packed adjacency and edge-list use swap erasure");
  {
    mm::graph<> packed;
    (void)packed.add_vertices(4);
    const auto first = packed.add_edge(0, 1).id;
    const auto middle = packed.add_edge(1, 2).id;
    const auto last = packed.add_edge(2, 3).id;
    require_true(packed.remove_edge(first));
    require_true(packed.has_edge(first));
    require_true(packed.source(first) == mm::vertex_id<u32>(2));
    require_true(packed.target(first) == mm::vertex_id<u32>(3));
    require_true(packed.has_edge(middle) && !packed.has_edge(last));
    require_true(packed.remove_vertex(1u));
    require_true(packed.vertices_count() == 3 && packed.vertex_slots() == 3);

    packed_mutation_fuzz<mm::graph<>>(0x9e3779b97f4a7c15ull);
    packed_mutation_fuzz<mm::edge_list_graph<>>(0xd1b54a32d192ed03ull);
  }
  end_test_case();

  test_case("undirected loops contribute two incidences in every mutable representation");
  {
    auto verify = []<typename G>(G &graph) {
      (void)graph.add_vertices(2);
      const auto loop = graph.add_edge(0, 0).id;
      const auto link = graph.add_edge(0, 1).id;
      require_true(loop.valid() && link.valid() && graph.edges_count() == 2);
      usize edges = 0;
      for ( auto edge : graph.edges() ) {
        (void)edge;
        ++edges;
      }
      usize incidences = 0;
      usize loop_incidences = 0;
      for ( auto edge : graph.out_edges(mm::vertex_id<u32>(0)) ) {
        ++incidences;
        loop_incidences += edge == loop;
      }
      usize neighbors = 0;
      usize self_neighbors = 0;
      for ( auto neighbor : graph.out_neighbors(mm::vertex_id<u32>(0)) ) {
        ++neighbors;
        self_neighbors += neighbor == mm::vertex_id<u32>(0);
      }
      require_true(edges == 2 && incidences == 3 && loop_incidences == 2);
      require_true(neighbors == 3 && self_neighbors == 2);
      require_true(graph.out_degree(mm::vertex_id<u32>(0)) == 3 && graph.degree(mm::vertex_id<u32>(0)) == 3);
    };

    using packed_type
        = mm::graph<mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::undirected_t, mg::simple_t, mg::allow_loops_t>;
    using edge_list_type = mm::edge_list_graph<mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::undirected_t,
                                               mg::simple_t, mg::allow_loops_t>;
    using dense_type
        = mm::dense_adjacency_graph<mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::undirected_t, mg::allow_loops_t>;
    using bit_type = mm::bit_adjacency_graph<mm::empty_property, mm::empty_property, u32, mg::undirected_t, mg::allow_loops_t>;
    packed_type packed;
    edge_list_type edge_list;
    dense_type dense;
    bit_type bit;
    verify(packed);
    verify(edge_list);
    verify(dense);
    verify(bit);
  }
  end_test_case();

  test_case("dense and bit matrices rebuild descriptors and match the oracle");
  {
    mm::dense_adjacency_graph<> dense;
    (void)dense.add_vertices(3);
    const auto old_dense = dense.add_edge(1, 2).id;
    require_true(old_dense == mm::edge_id<u32>(5));
    (void)dense.add_vertex();
    require_true(!dense.has_edge(old_dense));
    const auto new_dense = dense.find_edge(1u, 2u);
    require_true(new_dense == mm::edge_id<u32>(6));

    mm::bit_adjacency_graph<> bit;
    (void)bit.add_vertices(3);
    const auto old_bit = bit.add_edge(1, 2).id;
    (void)bit.add_vertex();
    require_true(!bit.has_edge(old_bit));
    require_true(bit.find_edge(1u, 2u) == mm::edge_id<u32>(6));
    const auto row = bit.neighbor_words(mm::vertex_id<u32>(1));
    require_true(row.words == 1 && (row.data[0] & (u64(1) << 2)) != 0);

    mm::bit_adjacency_graph<> wide;
    (void)wide.add_vertices(130);
    (void)wide.add_edge(0, 3);
    (void)wide.add_edge(0, 64);
    (void)wide.add_edge(0, 129);
    const u32 expected_neighbors[]{ 3, 64, 129 };
    usize neighbor_index = 0;
    for ( auto neighbor : wide.out_neighbors(mm::vertex_id<u32>(0)) )
      require_true(neighbor_index < 3 && neighbor.value == expected_neighbors[neighbor_index++]);
    require_true(neighbor_index == 3);

    using directed_bit = mm::bit_adjacency_graph<mm::empty_property, mm::empty_property, u32, mg::directed_t>;
    directed_bit directed;
    (void)directed.add_vertices(130);
    (void)directed.add_edge(3, 129);
    (void)directed.add_edge(64, 129);
    usize incoming = 0;
    for ( auto neighbor : directed.in_neighbors(mm::vertex_id<u32>(129)) ) incoming += neighbor.value == 3 || neighbor.value == 64;
    require_true(incoming == 2);

    packed_mutation_fuzz<mm::dense_adjacency_graph<>>(0x94d049bb133111ebull);
    packed_mutation_fuzz<mm::bit_adjacency_graph<>>(0x2545f4914f6cdd1dull);

    mm::dense_adjacency_graph<mm::empty_property, mm::empty_property, mm::empty_property, u8> tiny_dense;
    mm::bit_adjacency_graph<mm::empty_property, mm::empty_property, u8> tiny_bit;
    require_true(tiny_dense.add_vertices(15).size() == 15);
    require_true(tiny_bit.add_vertices(15).size() == 15);
    require_true(!tiny_dense.add_vertex().valid() && !tiny_bit.add_vertex().valid());
  }
  end_test_case();

  test_case("dense cells honor property lifetime");
  {
    require_true(tracked_property::live == 0);
    {
      mm::dense_adjacency_graph<mm::empty_property, tracked_property> graph;
      (void)graph.add_vertices(4);
      const auto edge = graph.add_edge(0u, 3u, tracked_property(7)).id;
      require_true(graph.edge_property_unchecked(edge).value == 7);
      (void)graph.add_vertex();
      const auto moved = graph.find_edge(0u, 3u);
      require_true(moved.valid() && graph.edge_property_unchecked(moved).value == 7);
      require_true(graph.remove_edge(moved));
    }
    require_true(tracked_property::live == 0);
  }
  end_test_case();

  test_case("stable holes support non-default properties and conversion remaps");
  {
    using stable_type = mm::stable_adjacency_graph<non_default_property, non_default_property>;
    stable_type source;
    const auto v0 = source.add_vertex(non_default_property(10));
    const auto v1 = source.add_vertex(non_default_property(20));
    const auto v2 = source.add_vertex(non_default_property(30));
    (void)source.add_edge(v0, v1, non_default_property(1));
    (void)source.add_edge(v1, v2, non_default_property(2));
    const auto kept = source.add_edge(v0, v2, non_default_property(3)).id;
    require_true(source.remove_vertex(v1));
    require_true(source.vertex_slots() == 3 && source.edge_slots() == 3);

    auto packed = mg::to_compact_adjacency(source);
    require_true(packed.value.vertices_count() == 2 && packed.value.edges_count() == 1);
    require_true(!packed.vertex_remap[1].valid() && packed.vertex_remap[2] == mm::vertex_id<u32>(1));
    require_true(packed.edge_remap[kept.value] == mm::edge_id<u32>(0));
    auto restored = packed.value.thaw_stable();
    require_true(restored.value.vertices_count() == 2 && restored.value.edges_count() == 1);
  }
  end_test_case();

  test_case("CSR rows are sorted and all representations convert");
  {
    mm::stable_adjacency_graph<> source;
    (void)source.add_vertices(7);
    (void)source.add_edge(0, 6);
    (void)source.add_edge(0, 2);
    (void)source.add_edge(0, 5);
    (void)source.add_edge(0, 1);
    (void)source.add_edge(1, 2);
    (void)source.add_edge(1, 5);
    (void)source.remove_vertex(3u);
    auto frozen = source.freeze();
    require_true(frozen.value.vertices_count() == 6);
    u32 previous = 0;
    bool first = true;
    for ( auto neighbor : frozen.value.out_neighbors(mm::vertex_id<u32>(0)) ) {
      require_true(first || previous < neighbor.value);
      previous = neighbor.value;
      first = false;
    }
    require_true(frozen.value.find_edge(0u, frozen.vertex_remap[6].value).valid());

    auto edge_list = mg::to_edge_list(source);
    auto dense = mg::to_dense_adjacency(source);
    auto bit = mg::to_bit_adjacency(source);
    require_true(edge_list && dense && bit && frozen);
    require_true(edge_list.value.edges_count() == source.edges_count());
    require_true(dense.value.edges_count() == source.edges_count());
    require_true(bit.value.edges_count() == source.edges_count());
    require_true(mg::common_neighbors(edge_list.value, mm::vertex_id<u32>(0), mm::vertex_id<u32>(1)) == 2);
    require_true(mg::common_neighbors(frozen.value, mm::vertex_id<u32>(0), mm::vertex_id<u32>(1)) == 2);
    require_true(mg::common_neighbors(dense.value, mm::vertex_id<u32>(0), mm::vertex_id<u32>(1)) == 2);
    require_true(mg::common_neighbors(bit.value, mm::vertex_id<u32>(0), mm::vertex_id<u32>(1)) == 2);
    require_true(dense.value.freeze().value.edges_count() == source.edges_count());
    require_true(bit.value.thaw().value.edges_count() == source.edges_count());
    require_true(frozen.value.thaw_stable().value.edges_count() == source.edges_count());

    mm::multigraph<> parallel;
    (void)parallel.add_edge(0, 1);
    (void)parallel.add_edge(0, 1);
    const auto rejected_dense = mg::to_dense_adjacency(parallel);
    const auto rejected_bit = mg::to_bit_adjacency(parallel);
    require_true(!rejected_dense && rejected_dense.status == mg::algorithm_status::invalid_graph);
    require_true(!rejected_bit && rejected_bit.status == mg::algorithm_status::invalid_graph);
    require_true(mg::to_edge_list(parallel).value.edges_count() == 2);
    require_true(parallel.freeze().value.edges_count() == 2);

    using tiny_graph = mm::graph<mm::empty_property, mm::empty_property, mm::empty_property, u8>;
    tiny_graph too_wide_for_matrix;
    (void)too_wide_for_matrix.add_vertices(16);
    require_true(mg::to_dense_adjacency(too_wide_for_matrix).status == mg::algorithm_status::overflow);
    require_true(mg::to_bit_adjacency(too_wide_for_matrix).status == mg::algorithm_status::overflow);
  }
  end_test_case();

  test_case("binary v3 preserves stable slots and densifies packed targets");
  {
    using stable_type = mm::stable_adjacency_graph<non_default_property, non_default_property>;
    stable_type source;
    const auto v0 = source.add_vertex(non_default_property(4));
    const auto v1 = source.add_vertex(non_default_property(5));
    const auto v2 = source.add_vertex(non_default_property(6));
    (void)source.add_edge(v0, v1, non_default_property(11));
    (void)source.add_edge(v1, v2, non_default_property(12));
    const auto live_edge = source.add_edge(v0, v2, non_default_property(13)).id;
    require_true(source.remove_vertex(v1));
    mig::native_property_codec codec;
    const auto bytes = mig::binary(source, codec);
    auto stable = mig::parse_binary<stable_type>(bytes, codec);
    require_true(stable && stable.value.vertex_slots() == 3 && stable.value.edge_slots() == 3);
    require_true(!stable.value.has_vertex(v1) && stable.value.has_edge(live_edge));
    require_true(stable.value.vertex_property_unchecked(v2).value == 6);
    require_true(stable.value.edge_property_unchecked(live_edge).value == 13);

    using packed_type = mm::graph<non_default_property, non_default_property>;
    auto packed = mig::parse_binary<packed_type>(bytes, codec);
    require_true(packed && packed.value.vertex_slots() == 2 && packed.value.edge_slots() == 1);
    require_true(!packed.vertex_remap[1].valid() && packed.vertex_remap[2] == mm::vertex_id<u32>(1));
    require_true(packed.edge_remap[live_edge.value] == mm::edge_id<u32>(0));

    using csr_type = mm::csr_graph<non_default_property, non_default_property>;
    auto csr = mig::parse_binary<csr_type>(bytes, codec);
    require_true(csr && csr.value.vertices_count() == 2 && csr.value.edges_count() == 1);
    require_true(csr.value.vertex_property_unchecked(csr.vertex_remap[2]).value == 6);
    require_true(csr.value.edge_property_unchecked(csr.edge_remap[live_edge.value]).value == 13);
  }
  end_test_case();

  test_case("binary v3 decodes every representation and rejects malformed records");
  {
    mm::stable_adjacency_graph<> source;
    (void)source.add_vertices(6);
    (void)source.add_edge(0, 1);
    (void)source.add_edge(1, 2);
    const auto kept = source.add_edge(2, 5).id;
    require_true(source.remove_vertex(3u));
    const auto bytes = mig::binary(source);

    auto edge_list = mig::parse_binary<mm::edge_list_graph<>>(bytes);
    auto dense = mig::parse_binary<mm::dense_adjacency_graph<>>(bytes);
    auto bit = mig::parse_binary<mm::bit_adjacency_graph<>>(bytes);
    auto csr = mig::parse_binary<mm::csr_graph<>>(bytes);
    using bidirectional_type
        = mm::bidirectional_csr_graph<mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::undirected_t>;
    auto bidirectional = mig::parse_binary<bidirectional_type>(bytes);
    require_true(edge_list && dense && bit && csr && bidirectional);
    require_true(edge_list.value.edges_count() == 3 && dense.value.edges_count() == 3 && bit.value.edges_count() == 3);
    require_true(csr.value.edges_count() == 3 && bidirectional.value.edges_count() == 3);
    require_true(csr.vertex_remap[5] == mm::vertex_id<u32>(4));
    require_true(csr.edge_remap[kept.value].valid() && bidirectional.edge_remap[kept.value].valid());

    require_true(mig::parse_binary<>(bytes.data(), bytes.size() - 1).status.code == mig::parse_code::truncated);
    auto malformed = bytes;
    malformed[4] = 2;
    require_true(mig::parse_binary<>(malformed).status.code == mig::parse_code::unknown_version);
    malformed = bytes;
    malformed[8] ^= byte(1);
    require_true(mig::parse_binary<>(malformed).status.code == mig::parse_code::incompatible_graph);
    malformed = bytes;
    malformed.push_back(byte(0));
    require_true(mig::parse_binary<>(malformed).status.code == mig::parse_code::corrupt);
  }
  end_test_case();

  return 1;
}
