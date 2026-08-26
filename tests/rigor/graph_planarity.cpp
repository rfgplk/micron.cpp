//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/graph.hpp"
#include "../snowball/snowball.hpp"

namespace mm = micron::math;
namespace mg = micron::math::graphs;

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main()
{
  test_case("Boyer-Myrvold embeds trees");
  {
    mm::graph<> tree;
    (void)tree.add_vertices(13);
    for ( u32 vertex = 1; vertex < 13; ++vertex ) (void)tree.add_edge((vertex - 1) / 2, vertex);
    auto tree_embedding = mg::boyer_myrvold_planarity(tree);
    require_true(tree_embedding.is_planar());
    require_true(mg::validate_planar_embedding(tree, tree_embedding));
  }
  end_test_case();

  test_case("Boyer-Myrvold embeds wheels");
  {
    auto wheel = mg::wheel_graph<>(13);
    auto wheel_embedding = mg::boyer_myrvold_planarity(wheel);
    require_true(wheel_embedding.is_planar());
    require_true(mg::validate_rotation_system(wheel, wheel_embedding));
  }
  end_test_case();

  test_case("planar rotations include parallel edges and loops");
  {
    mm::graph<mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::undirected_t, mg::parallel_t, mg::allow_loops_t>
        decorated;
    (void)decorated.add_vertices(8);
    for ( u32 vertex = 1; vertex < 6; ++vertex ) (void)decorated.add_edge(0u, vertex);
    (void)decorated.add_edge(0u, 1u);
    (void)decorated.add_edge(0u, 1u);
    (void)decorated.add_edge(0u, 0u);
    (void)decorated.add_edge(0u, 0u);
    (void)decorated.add_edge(6u, 7u);
    auto decorated_embedding = mg::boyer_myrvold_planarity(decorated);
    require_true(decorated_embedding.is_planar());
    require_true(mg::validate_planar_embedding(decorated, decorated_embedding));
  }
  end_test_case();

  test_case("Kuratowski certificates reduce to K5 and K3,3");
  {
    auto k5 = mg::complete_graph<>(5);
    auto k5_result = mg::boyer_myrvold_planarity(k5);
    require_true(!k5_result.planar && k5_result.status == mg::algorithm_status::ok);
    require_true(mg::reduce_kuratowski_witness(k5, k5_result.kuratowski_edges) == mg::kuratowski_kind::k5);
    require_true(mg::validate_kuratowski_witness(k5, k5_result.kuratowski_edges));

    mm::graph<> k33;
    (void)k33.add_vertices(6);
    for ( u32 u = 0; u < 3; ++u )
      for ( u32 v = 3; v < 6; ++v ) (void)k33.add_edge(u, v);
    auto k33_result = mg::boyer_myrvold_planarity(k33);
    require_true(!k33_result.planar);
    require_true(mg::reduce_kuratowski_witness(k33, k33_result.kuratowski_edges) == mg::kuratowski_kind::k33);

    mm::graph<> subdivision;
    (void)subdivision.add_vertices(7);
    for ( u32 u = 0; u < 3; ++u )
      for ( u32 v = 3; v < 6; ++v ) {
        if ( u == 0 && v == 3 ) continue;
        (void)subdivision.add_edge(u, v);
      }
    (void)subdivision.add_edge(0u, 6u);
    (void)subdivision.add_edge(6u, 3u);
    auto subdivision_result = mg::boyer_myrvold_planarity(subdivision);
    require_true(!subdivision_result.planar);
    require_true(mg::reduce_kuratowski_witness(subdivision, subdivision_result.kuratowski_edges) == mg::kuratowski_kind::k33);
  }
  end_test_case();

  test_case("directed inputs are rejected");
  {
    mm::digraph<> graph;
    (void)graph.add_edge(0u, 1u);
    auto result = mg::boyer_myrvold_planarity(graph);
    require_true(result.status == mg::algorithm_status::invalid_graph && !result.planar);
  }
  end_test_case();

  return 1;
}
