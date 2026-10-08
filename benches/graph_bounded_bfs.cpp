//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/math/graph/bounded_bfs.hpp"
#include "../src/math/graph/fixed.hpp"
#include "../src/math/graph/representations.hpp"
#include "../src/io/console.hpp"
#include "../external/bbench/bench.hpp"

namespace mg = micron::math::graphs;
using graph_type = micron::math::fixed_graph<512, 1536, micron::math::empty_property,
    micron::math::empty_property, micron::math::empty_property, u32, mg::directed_t>;

template<class G> bool
measure(const G &graph, const char *representation)
{
  mg::fixed_bfs_workspace<u32, 512> fixed;
  mg::traversal_workspace<u32> dynamic;
  dynamic.reserve(512);
  constexpr usize repetitions = 16;
  constexpr u64 expected = repetitions * (511ull * 512 / 2);
  const micron::math::vertex_id<u32> source(0);
  for ( unsigned mode = 0; mode < 2; ++mode ) {
    bool valid = true;
    u64 consumed = 0;
    auto run = [&]() {
      consumed = 0;
      for ( usize i = 0; i < repetitions; ++i ) {
        asm volatile("" : : "r"(micron::addressof(graph)) : "memory");
        if ( mode ) {
          const auto result = mg::bounded_bfs_into(graph, source, fixed);
          if ( result.status != mg::algorithm_status::ok || !result.complete || fixed.order.size() != 512 ) valid = false;
          for ( auto v : fixed.order ) consumed += v.value;
        } else {
          if ( mg::bfs_into(graph, source, dynamic) != mg::algorithm_status::ok || dynamic.order.size() != 512 ) valid = false;
          for ( auto v : dynamic.order ) consumed += v.value;
        }
      }
    };
    for ( unsigned warm = 0; warm < 2; ++warm ) run();
    if ( !valid || consumed != expected ) return false;
    for ( unsigned sample = 0; sample < 7; ++sample ) {
      const auto result = bbench::benchmark_batch<bbench::time_resolution::ns, bbench::event_group<bbench::hardware_cycles>>(run);
      if ( !valid || consumed != expected ) return false;
      micron::console("representation=", representation, " workspace=", mode ? "fixed" : "dynamic",
          " sample=", sample, " ns=", result.time, " consumed=", consumed);
    }
  }
  return true;
}

int
main()
{
  graph_type graph;
  for ( u32 i = 0; i < 512; ++i )
    for ( u32 delta : {1u, 7u, 31u} ) if ( !graph.add_edge(i, (i + delta) % 512).inserted() ) return 2;
  auto csr = mg::freeze(graph);
  bbench::hardware_cycles counter(bbench::quiet{}); counter.open();
  micron::console("vertices=512 edges=1536 repetitions=16 samples=7 warmups=2 allocator=MT,zero,provenance fixed_workspace_bytes=",
      sizeof(mg::fixed_bfs_workspace<u32, 512>));
  if ( counter.e_fd < 0 ) micron::console("hardware_counters=unavailable errno=", counter.open_error());
  else micron::console("hardware_counters=available");
  return measure(graph, "fixed_graph") && measure(csr.value, "csr") ? 0 : 2;
}
