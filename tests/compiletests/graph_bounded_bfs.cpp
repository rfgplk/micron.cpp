//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/math/graph/bounded_bfs.hpp"
#include "../../src/math/graph/fixed.hpp"

void
bounded_bfs_compile_contract()
{
  micron::math::fixed_graph<4, 4> graph;
  auto edge = graph.add_edge(0u, 1u);
  micron::math::graphs::fixed_bfs_workspace<u32, 4> work;
  (void)micron::math::graphs::bounded_bfs_into(graph, graph.source(edge.id), work, { 2, 4, 4 });
}
