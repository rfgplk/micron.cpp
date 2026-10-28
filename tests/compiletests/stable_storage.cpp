// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/graph/connectivity_iterative.hpp"
#include "../../src/vector/reserved_vector.hpp"
#include "../../src/vector/segmented_vector.hpp"

int
main()
{
  micron::reserved_vector<u64> contiguous;
  micron::segmented_vector<u64, 16> chunks;
  if ( !contiguous.initialize(4096) || !contiguous.try_resize(1025) || !chunks.try_resize(1025) ) return 2;
  contiguous[1024] = 17;
  chunks[1024] = 25;
  micron::math::graph<> graph;
  (void)graph.add_vertex();
  const auto components = micron::math::graphs::strongly_connected_components_iterative(graph);
  return contiguous[1024] + chunks[1024] + components.count == 43 ? 1 : 2;
}
