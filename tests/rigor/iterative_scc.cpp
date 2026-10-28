//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/math/graph/connectivity_iterative.hpp"
#include "../../src/math/graph/representations.hpp"
#include "../snowball/snowball.hpp"

namespace graphs = micron::math::graphs;
using allocator = micron::allocator_serial<micron::allocation_policy<0, 16, 2, 1>>;
using graph_type = micron::math::graph<micron::math::empty_property, micron::math::empty_property, micron::math::empty_property, u32,
                                       graphs::directed_t, graphs::simple_t, graphs::allow_loops_t, allocator>;

template<class Graph>
void
verify(const Graph &graph, const bool (&reach)[32][32], u32 count)
{
  const auto components = graphs::strongly_connected_components_iterative(graph);
  for ( u32 a = 0; a < count; ++a )
    for ( u32 b = 0; b < count; ++b )
      sb::require_true((components.component[a] == components.component[b]) == (reach[a][b] && reach[b][a]));
}

int
main()
{
  u64 random = 0x184142512;
  auto next = [&]() {
    random ^= random << 13;
    random ^= random >> 7;
    random ^= random << 17;
    return random;
  };
  for ( u32 round = 0; round < 10000; ++round ) {
    const u32 count = 1 + next() % 32;
    graph_type graph;
    bool reach[32][32]{};
    for ( u32 i = 0; i < count; ++i ) {
      (void)graph.add_vertex();
      reach[i][i] = true;
    }
    for ( u32 i = 0; i < count * 3; ++i ) {
      const u32 a = next() % count, b = next() % count;
      (void)graph.add_edge(a, b);
      reach[a][b] = true;
    }
    for ( u32 k = 0; k < count; ++k )
      for ( u32 a = 0; a < count; ++a )
        for ( u32 b = 0; b < count; ++b ) reach[a][b] |= reach[a][k] && reach[k][b];
    verify(graph, reach, count);
    const auto frozen = graphs::freeze_bidirectional(graph);
    verify(frozen.value, reach, count);
  }
  graph_type deep;
  for ( u32 i = 0; i < 200000; ++i ) {
    (void)deep.add_vertex();
    if ( i ) (void)deep.add_edge(i - 1, i);
  }
  auto components = graphs::strongly_connected_components_iterative(deep);
  sb::require_true(components.count == 200000);
  (void)deep.add_edge(199999, 0);
  components = graphs::strongly_connected_components_iterative(deep);
  sb::require_true(components.count == 1);
  return 1;
}
