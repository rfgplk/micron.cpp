//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/math/graph.hpp"
#include "../src/io/console.hpp"
#include "../src/string/format.hpp"

int
main()
{
  namespace mm = micron::math;
  namespace graphs = micron::math::graphs;

  mm::graph<micron::string> social;
  auto ada = social.add_vertex("Ada");
  auto grace = social.add_vertex("Grace");
  auto edsger = social.add_vertex("Edsger");
  (void)social.add_edge(ada, grace);
  (void)social.add_edge(grace, edsger);

  auto traversal = graphs::bfs(social, ada);
  micron::io::println("BFS order: ", traversal.order);
  micron::io::println(micron::format::format("{:a}", social));

  mm::weighted_digraph<u32> routes;
  (void)routes.add_edge(0, 1, 7u);
  (void)routes.add_edge(0, 2, 2u);
  (void)routes.add_edge(2, 1, 1u);
  auto shortest = graphs::shortest_path(routes, mm::vertex_id<u32>(0), mm::vertex_id<u32>(1));
  if ( shortest.status == graphs::algorithm_status::ok ) micron::io::println("route cost: ", shortest.distance, ", path: ", shortest.path);

  return 0;
}
