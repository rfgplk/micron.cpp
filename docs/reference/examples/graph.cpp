#include <micron/math/graph.hpp>

int main()
{
  micron::math::graph<int> graph;
  auto a = graph.add_vertex(1);
  auto b = graph.add_vertex(2);
  (void)graph.add_edge(a, b);
  auto traversal = micron::math::graphs::bfs(graph, a);
  return traversal.order.size() == 2 ? 0 : 1;
}
