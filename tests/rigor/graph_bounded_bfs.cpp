//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/linux/process/fork.hpp"
#include "../../src/math/graph/bounded_bfs.hpp"
#include "../../src/math/graph/fixed.hpp"
#include "../../src/math/graph/representations.hpp"
#include "../../src/math/graph/views.hpp"
#include "../../src/math/rng/engines.hpp"
#include "../../src/sec/seccomp.hpp"
#include "../snowball/snowball.hpp"

namespace mm = micron::math;
namespace mg = micron::math::graphs;

template<class G>
void
compare(const G &graph, typename G::vertex_descriptor source)
{
  const auto expected = mg::bfs(graph, source);
  mg::fixed_bfs_workspace<typename G::index_type, 32> fixed;
  const auto result = mg::bounded_bfs_into(graph, source, fixed);
  sb::require_true(result.status == expected.status && result.complete && fixed.order.size() == expected.order.size());
  for ( usize i = 0; i < fixed.order.size(); ++i ) sb::require_true(fixed.order[i] == expected.order[i]);
  for ( usize i = 0; i < graph.vertex_slots(); ++i ) {
    sb::require_true(fixed.discovered[i] == expected.reached[i]);
    sb::require_true(fixed.parent[i] == expected.parent[i] && fixed.depth[i] == expected.depth[i]);
  }
}

int
main()
{
  using directed = mm::fixed_graph<32, 128, mm::empty_property, mm::empty_property, mm::empty_property, u32, mg::directed_t, mg::parallel_t,
                                   mg::allow_loops_t>;
  sb::test_case("fixed BFS matches dynamic BFS across cycles, parallel edges, loops, holes, reverse views and CSR");
  directed graph;
  for ( u32 i = 0; i < 20; ++i ) (void)graph.add_vertex();
  for ( u32 i = 0; i < 18; ++i ) {
    sb::require_true(graph.add_edge(i, (i * 7 + 3) % 18).inserted());
    sb::require_true(graph.add_edge(i, (i + 1) % 18).inserted());
    sb::require_true(graph.add_edge(i, (i + 1) % 18).inserted());
  }
  sb::require_true(graph.add_edge(4u, 4u).inserted() && graph.remove_vertex(mm::vertex_id<u32>(7)));
  for ( auto source : graph.vertices() ) {
    compare(graph, source);
    compare(mg::reverse(graph), source);
  }
  auto frozen = mg::freeze(graph);
  for ( auto source : frozen.value.vertices() ) compare(frozen.value, source);
  mm::fixed_graph<32, 128> undirected;
  for ( u32 i = 0; i < 16; ++i ) sb::require_true(undirected.add_edge(i, (i + 1) % 16).inserted());
  for ( auto source : undirected.vertices() ) compare(undirected, source);
  sb::end_test_case();

  sb::test_case("seeded directed graphs match an independent all-pairs distance oracle at every radius");
  mm::rng::splitmix64 random(0x626f756e64656431ull);
  for ( unsigned sample = 0; sample < 64; ++sample ) {
    directed generated;
    bool adjacency[32][32]{}, live[32];
    for ( u32 i = 0; i < 32; ++i ) {
      (void)generated.add_vertex();
      live[i] = true;
    }
    for ( unsigned e = 0; e < 96; ++e ) {
      const u32 u = random.next() % 32, v = random.next() % 32;
      sb::require_true(generated.add_edge(u, v).inserted());
      adjacency[u][v] = true;
    }
    const u32 removed = random.next() % 32;
    sb::require_true(generated.remove_vertex(mm::vertex_id<u32>(removed)));
    live[removed] = false;
    u32 distance[32][32];
    for ( u32 u = 0; u < 32; ++u )
      for ( u32 v = 0; v < 32; ++v ) distance[u][v] = !live[u] || !live[v] ? 1000 : u == v ? 0 : adjacency[u][v] ? 1 : 1000;
    for ( u32 k = 0; k < 32; ++k )
      for ( u32 u = 0; u < 32; ++u )
        for ( u32 v = 0; v < 32; ++v )
          if ( distance[u][k] + distance[k][v] < distance[u][v] ) distance[u][v] = distance[u][k] + distance[k][v];
    mg::fixed_bfs_workspace<u32, 32> work;
    for ( u32 source = 0; source < 32; ++source )
      if ( live[source] ) {
        const usize radius = random.next() % 8;
        const auto result = mg::bounded_bfs_into(generated, mm::vertex_id<u32>(source), work, { radius, 32, 128 });
        sb::require_true(result.status == mg::algorithm_status::ok && result.edges <= 96);
        for ( u32 v = 0; v < 32; ++v ) {
          const bool reached = distance[source][v] <= radius;
          sb::require_true(bool(work.discovered[v]) == reached);
          if ( reached ) {
            sb::require_true(work.depth[v] == distance[source][v]);
            const auto parent = work.parent[v].value;
            sb::require_true(parent < 32 && live[parent]);
            if ( source == v )
              sb::require_true(parent == v);
            else
              sb::require_true(adjacency[parent][v] && distance[source][parent] + 1 == distance[source][v]);
          }
        }
      }
  }
  sb::end_test_case();

  sb::test_case("independent shortest-distance fixture verifies radius, vertex and edge budgets");
  directed chain;
  for ( u32 i = 0; i < 9; ++i ) sb::require_true(chain.add_edge(i, i + 1).inserted());
  mg::fixed_bfs_workspace<u32, 32> work;
  auto result = mg::bounded_bfs_into(chain, mm::vertex_id<u32>(0), work, { 3, 32, 128 });
  sb::require_true(result.status == mg::algorithm_status::ok && !result.complete && result.edges == 3 && work.order.size() == 4);
  for ( u32 i = 0; i < 10; ++i ) {
    sb::require_true(bool(work.discovered[i]) == (i <= 3));
    if ( i <= 3 ) sb::require_true(work.depth[i] == i && work.parent[i].value == (i ? i - 1 : 0));
  }
  result = mg::bounded_bfs_into(chain, mm::vertex_id<u32>(0), work, { 99, 3, 128 });
  sb::require_true(result.status == mg::algorithm_status::overflow && !result.complete && work.frontier.size() == 3);
  result = mg::bounded_bfs_into(chain, mm::vertex_id<u32>(0), work, { 99, 32, 2 });
  sb::require_true(result.status == mg::algorithm_status::overflow && result.edges == 2 && work.frontier.size() == 3);
  result = mg::bounded_bfs_into(chain, mm::vertex_id<u32>(0), work, { 0, 1, 0 });
  sb::require_true(result.status == mg::algorithm_status::ok && work.order.size() == 1 && result.edges == 0);
  sb::end_test_case();

  sb::test_case("invalid and undersized preflight preserves prior workspace while visitor stop is explicit");
  const auto previous = work.order.size();
  sb::require_true(mg::bounded_bfs_into(chain, mm::vertex_id<u32>(99), work).status == mg::algorithm_status::invalid_vertex);
  sb::require_true(work.order.size() == previous
                   && mg::bounded_bfs_into(chain, mm::vertex_id<u32>(0), work, { 0, 0, 0 }).status == mg::algorithm_status::overflow);
  sb::require_true(work.order.size() == previous);
  mg::fixed_bfs_workspace<u32, 2> tiny;
  mm::fixed_graph<2, 1> small;
  sb::require_true(small.add_edge(0u, 1u).inserted());
  sb::require_true(mg::bounded_bfs_into(small, mm::vertex_id<u32>(0), tiny).complete && tiny.order.size() == 2);
  sb::require_true(mg::bounded_bfs_into(chain, mm::vertex_id<u32>(0), tiny).status == mg::algorithm_status::overflow
                   && tiny.order.size() == 2);

  struct stop {
    bool
    discover_vertex(const directed &, mm::vertex_id<u32> value)
    {
      return value.value != 2;
    }
  };

  result = mg::bounded_bfs_into(chain, mm::vertex_id<u32>(0), work, {}, stop{});
  sb::require_true(result.status == mg::algorithm_status::ok && !result.complete && work.frontier.size() == 3 && work.order.size() == 2);
  sb::end_test_case();

  sb::test_case("fixed graph BFS remains allocation-free with mmap and brk denied");
  const int child = micron::try_fork();
  sb::require_true(child >= 0);
  if ( !child ) {
    micron::sec::seccomp::filter_builder<64> filter;
    filter.require_native_arch()
        .deny_errno(SYS_mmap, micron::error::out_of_memory)
        .deny_errno(SYS_brk, micron::error::out_of_memory)
        .default_allow();
    sb::require_true(micron::sec::seccomp::load(filter) == 0);
    const auto bounded = mg::bounded_bfs_into(graph, mm::vertex_id<u32>(0), work);
    sb::require_true(bounded.status == mg::algorithm_status::ok && bounded.complete && !work.order.empty());
    micron::sys_group_exit(1);
  }
  int code = 0;
  sb::require_true(micron::waitpid(child, &code, 0) == child && micron::wifexited(code) && micron::wexitstatus(code) == 1);
  sb::end_test_case();
  return 1;
}
