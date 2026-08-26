//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "matrix.hpp"

namespace micron::math::graphs
{

enum class kuratowski_kind : u8 { none = 0, k5, k33 };

template<micron::integral I> struct planarity_result {
  algorithm_status status{ algorithm_status::ok };
  bool planar{ true };
  micron::vector<micron::vector<edge_id<I>, micron::allocator_serial<>, false>, micron::allocator_serial<>, false> rotation;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> kuratowski_edges;

  [[nodiscard]] bool
  is_planar() const noexcept
  {
    return status == algorithm_status::ok && planar;
  }

  [[nodiscard]] explicit
  operator bool() const noexcept
  {
    return is_planar();
  }
};

template<micron::integral I> struct planarity_workspace {
  micron::vector<byte, micron::allocator_serial<>, false> scratch;

  void
  reserve(usize vertices, usize edges)
  {
    const usize wanted = vertices > edges ? vertices : edges;
    scratch.reserve(wanted);
  }
};

namespace __impl
{

template<typename T> using __planar_vec = micron::vector<T, micron::allocator_serial<>, false>;

template<micron::integral I> struct __planar_input_edge {
  usize source{};
  usize target{};
  edge_id<I> original{};
};

template<micron::integral I> struct __planar_core_edge {
  usize source{};
  usize target{};
  usize originals_begin{};
  usize originals_count{};
};

template<micron::integral I> struct __planar_core {
  usize vertices{};
  __planar_vec<vertex_id<I>> dense_to_vertex;
  __planar_vec<__planar_core_edge<I>> edges;
  __planar_vec<edge_id<I>> originals;
  __planar_vec<__planar_vec<edge_id<I>>> loops;
};

template<typename T, typename Key>
void
__stable_count_sort(__planar_vec<T> &values, __planar_vec<T> &temporary, usize key_count, Key key)
{
  if ( values.size() < 2 ) return;
  __planar_vec<usize> offsets(key_count + 1, usize(0));
  for ( const auto &value : values ) ++offsets.data()[key(value) + 1];
  for ( usize i = 1; i < offsets.size(); ++i ) offsets.data()[i] += offsets.data()[i - 1];
  temporary.resize(values.size());
  for ( const auto &value : values ) temporary.data()[offsets.data()[key(value)]++] = value;
  values = micron::move(temporary);
  temporary.clear();
}

template<graph_model G>
[[nodiscard]] auto
__make_planar_core(const G &graph)
{
  using I = typename G::index_type;
  auto mapping = __dense_vertex_mapping(graph);
  __planar_core<I> core;
  core.vertices = mapping.dense_to_vertex.size();
  core.dense_to_vertex = micron::move(mapping.dense_to_vertex);
  core.loops.resize(core.vertices);

  __planar_vec<__planar_input_edge<I>> input;
  input.reserve(graph.edges_count());
  for ( auto edge : graph.edges() ) {
    const usize u = static_cast<usize>(mapping.vertex_to_dense.data()[static_cast<usize>(edge.source.value)]);
    const usize v = static_cast<usize>(mapping.vertex_to_dense.data()[static_cast<usize>(edge.target.value)]);
    if ( u == v ) {
      core.loops.data()[u].push_back(edge.id);
      continue;
    }
    input.push_back({ u < v ? u : v, u < v ? v : u, edge.id });
  }
  __planar_vec<__planar_input_edge<I>> temporary;
  __stable_count_sort(input, temporary, core.vertices, [](const auto &edge) { return edge.target; });
  __stable_count_sort(input, temporary, core.vertices, [](const auto &edge) { return edge.source; });

  for ( usize first = 0; first < input.size(); ) {
    usize last = first + 1;
    while ( last < input.size() && input.data()[last].source == input.data()[first].source
            && input.data()[last].target == input.data()[first].target )
      ++last;
    const usize begin = core.originals.size();
    for ( usize i = first; i < last; ++i ) core.originals.push_back(input.data()[i].original);
    core.edges.push_back({ input.data()[first].source, input.data()[first].target, begin, last - first });
    first = last;
  }
  return core;
}

struct __lr_interval {
  int low{ -1 };
  int high{ -1 };

  [[nodiscard]] bool
  empty() const noexcept
  {
    return low == -1;
  }
};

struct __lr_conflict_pair {
  __lr_interval left;
  __lr_interval right;
};

struct __lr_edge {
  int source{};
  int target{};
  int lowpoint{};
  int second_lowpoint{};
  int nesting{};
  int reference{ -1 };
  int side{ 1 };
  int core_edge{};
};

class __lr_planarity
{
  using row_type = __planar_vec<int>;
  usize __n{};
  __planar_vec<usize> __core_source;
  __planar_vec<usize> __core_target;
  const u8 *__active{};
  __planar_vec<row_type> __adjacency;
  __planar_vec<int> __oriented_core;
  __planar_vec<__lr_edge> __edge;
  __planar_vec<int> __height;
  __planar_vec<int> __parent_edge;
  __planar_vec<int> __roots;
  __planar_vec<row_type> __ordered;
  __planar_vec<__lr_conflict_pair> __conflicts;
  __planar_vec<usize> __stack_bottom;
  __planar_vec<int> __lowpoint_edge;
  __planar_vec<int> __left_reference;
  __planar_vec<int> __right_reference;
  __planar_vec<int> __dart_next;
  __planar_vec<int> __dart_previous;
  __planar_vec<int> __rotation_head;

  void
  __finish_orientation_edge(int edge)
  {
    auto &current = __edge.data()[static_cast<usize>(edge)];
    current.nesting = current.lowpoint * 2 + (current.second_lowpoint < __height.data()[current.source] ? 1 : 0);
    const int parent = __parent_edge.data()[current.source];
    if ( parent == -1 ) return;
    auto &up = __edge.data()[static_cast<usize>(parent)];
    if ( current.lowpoint < up.lowpoint ) {
      up.second_lowpoint = up.lowpoint < current.second_lowpoint ? up.lowpoint : current.second_lowpoint;
      up.lowpoint = current.lowpoint;
    } else if ( current.lowpoint > up.lowpoint ) {
      if ( current.lowpoint < up.second_lowpoint ) up.second_lowpoint = current.lowpoint;
    } else if ( current.second_lowpoint < up.second_lowpoint ) {
      up.second_lowpoint = current.second_lowpoint;
    }
  }

  void
  __orient()
  {
    struct frame {
      int vertex{};
      usize next{};
    };

    __planar_vec<frame> stack;
    for ( usize root = 0; root < __n; ++root ) {
      if ( __height.data()[root] != -1 ) continue;
      __height.data()[root] = 0;
      __roots.push_back(static_cast<int>(root));
      stack.push_back({ static_cast<int>(root), 0 });
      while ( !stack.empty() ) {
        auto &top = stack.data()[stack.size() - 1];
        const usize vertex = static_cast<usize>(top.vertex);
        if ( top.next == __adjacency.data()[vertex].size() ) {
          const int parent = __parent_edge.data()[vertex];
          stack.pop_back();
          if ( parent != -1 ) __finish_orientation_edge(parent);
          continue;
        }
        const int core_edge = __adjacency.data()[vertex].data()[top.next++];
        if ( __oriented_core.data()[static_cast<usize>(core_edge)] != -1 ) continue;
        const usize a = __core_source.data()[static_cast<usize>(core_edge)];
        const usize b = __core_target.data()[static_cast<usize>(core_edge)];
        const usize target = a == vertex ? b : a;
        const int oriented = static_cast<int>(__edge.size());
        __oriented_core.data()[static_cast<usize>(core_edge)] = oriented;
        __edge.push_back(
            { static_cast<int>(vertex), static_cast<int>(target), __height.data()[vertex], __height.data()[vertex], 0, -1, 1, core_edge });
        if ( __height.data()[target] == -1 ) {
          __height.data()[target] = __height.data()[vertex] + 1;
          __parent_edge.data()[target] = oriented;
          stack.push_back({ static_cast<int>(target), 0 });
        } else {
          __edge.data()[static_cast<usize>(oriented)].lowpoint = __height.data()[target];
          __finish_orientation_edge(oriented);
        }
      }
    }
  }

  void
  __build_ordered(bool signed_depth)
  {
    for ( auto &row : __ordered ) row.clear();
    if ( __edge.empty() ) return;
    const usize range = signed_depth ? __n * 4 + 3 : __n * 2 + 2;
    const int offset = signed_depth ? static_cast<int>(__n * 2 + 1) : 0;
    __planar_vec<int> order;
    __planar_vec<int> temporary;
    order.reserve(__edge.size());
    for ( usize i = 0; i < __edge.size(); ++i ) order.push_back(static_cast<int>(i));
    __stable_count_sort(order, temporary, range, [&](int edge) {
      const auto &value = __edge.data()[static_cast<usize>(edge)];
      const int depth = signed_depth ? value.side * value.nesting : value.nesting;
      return static_cast<usize>(depth + offset);
    });
    __stable_count_sort(order, temporary, __n,
                        [&](int edge) { return static_cast<usize>(__edge.data()[static_cast<usize>(edge)].source); });
    for ( int edge : order ) __ordered.data()[static_cast<usize>(__edge.data()[static_cast<usize>(edge)].source)].push_back(edge);
  }

  [[nodiscard]] bool
  __conflicting(const __lr_interval &interval, int edge) const noexcept
  {
    return !interval.empty()
           && __edge.data()[static_cast<usize>(interval.high)].lowpoint > __edge.data()[static_cast<usize>(edge)].lowpoint;
  }

  [[nodiscard]] int
  __lowest(const __lr_conflict_pair &pair) const noexcept
  {
    if ( pair.left.empty() ) return __edge.data()[static_cast<usize>(pair.right.low)].lowpoint;
    if ( pair.right.empty() ) return __edge.data()[static_cast<usize>(pair.left.low)].lowpoint;
    const int left = __edge.data()[static_cast<usize>(pair.left.low)].lowpoint;
    const int right = __edge.data()[static_cast<usize>(pair.right.low)].lowpoint;
    return left < right ? left : right;
  }

  [[nodiscard]] bool
  __add_constraints(int current, int parent)
  {
    __lr_conflict_pair merged;
    const usize bottom = __stack_bottom.data()[static_cast<usize>(current)];
    while ( __conflicts.size() > bottom ) {
      auto part = __conflicts.data()[__conflicts.size() - 1];
      __conflicts.pop_back();
      if ( !part.left.empty() ) micron::swap(part.left, part.right);
      if ( !part.left.empty() ) return false;
      if ( __edge.data()[static_cast<usize>(part.right.low)].lowpoint > __edge.data()[static_cast<usize>(parent)].lowpoint ) {
        if ( merged.right.empty() )
          merged.right = part.right;
        else
          __edge.data()[static_cast<usize>(merged.right.low)].reference = part.right.high;
        merged.right.low = part.right.low;
      } else {
        __edge.data()[static_cast<usize>(part.right.low)].reference = __lowpoint_edge.data()[static_cast<usize>(parent)];
      }
    }
    while ( !__conflicts.empty()
            && (__conflicting(__conflicts.data()[__conflicts.size() - 1].left, current)
                || __conflicting(__conflicts.data()[__conflicts.size() - 1].right, current)) ) {
      auto part = __conflicts.data()[__conflicts.size() - 1];
      __conflicts.pop_back();
      if ( __conflicting(part.right, current) ) micron::swap(part.left, part.right);
      if ( __conflicting(part.right, current) ) return false;
      if ( !merged.right.empty() ) __edge.data()[static_cast<usize>(merged.right.low)].reference = part.right.high;
      if ( !part.right.empty() ) merged.right.low = part.right.low;
      if ( merged.left.empty() )
        merged.left = part.left;
      else if ( !merged.left.empty() )
        __edge.data()[static_cast<usize>(merged.left.low)].reference = part.left.high;
      merged.left.low = part.left.low;
    }
    if ( !merged.left.empty() || !merged.right.empty() ) __conflicts.push_back(merged);
    return true;
  }

  void
  __remove_back_edges(int parent)
  {
    const int vertex = __edge.data()[static_cast<usize>(parent)].source;
    while ( !__conflicts.empty() && __lowest(__conflicts.data()[__conflicts.size() - 1]) == __height.data()[vertex] ) {
      auto part = __conflicts.data()[__conflicts.size() - 1];
      __conflicts.pop_back();
      if ( !part.left.empty() ) __edge.data()[static_cast<usize>(part.left.low)].side = -1;
    }
    if ( !__conflicts.empty() ) {
      auto part = __conflicts.data()[__conflicts.size() - 1];
      __conflicts.pop_back();
      while ( part.left.high != -1 && __edge.data()[static_cast<usize>(part.left.high)].target == vertex )
        part.left.high = __edge.data()[static_cast<usize>(part.left.high)].reference;
      if ( part.left.high == -1 && !part.left.empty() ) {
        __edge.data()[static_cast<usize>(part.left.low)].reference = part.right.low;
        __edge.data()[static_cast<usize>(part.left.low)].side = -1;
        part.left.low = -1;
      }
      while ( part.right.high != -1 && __edge.data()[static_cast<usize>(part.right.high)].target == vertex )
        part.right.high = __edge.data()[static_cast<usize>(part.right.high)].reference;
      if ( part.right.high == -1 && !part.right.empty() ) {
        __edge.data()[static_cast<usize>(part.right.low)].reference = part.left.low;
        __edge.data()[static_cast<usize>(part.right.low)].side = -1;
        part.right.low = -1;
      }
      __conflicts.push_back(part);
    }
    if ( __edge.data()[static_cast<usize>(parent)].lowpoint < __height.data()[vertex] ) {
      const auto &part = __conflicts.data()[__conflicts.size() - 1];
      const int left = part.left.high;
      const int right = part.right.high;
      if ( left != -1
           && (right == -1 || __edge.data()[static_cast<usize>(left)].lowpoint > __edge.data()[static_cast<usize>(right)].lowpoint) )
        __edge.data()[static_cast<usize>(parent)].reference = left;
      else
        __edge.data()[static_cast<usize>(parent)].reference = right;
    }
  }

  [[nodiscard]] bool
  __test()
  {
    struct frame {
      int vertex{};
      usize next{};
      bool resumed{};
    };

    __planar_vec<frame> stack;
    __planar_vec<u8> started(__edge.size(), u8(0));
    for ( int root : __roots ) {
      stack.push_back({ root, 0, false });
      while ( !stack.empty() ) {
        auto &top = stack.data()[stack.size() - 1];
        auto &row = __ordered.data()[static_cast<usize>(top.vertex)];
        if ( top.next == row.size() ) {
          const int parent = __parent_edge.data()[static_cast<usize>(top.vertex)];
          stack.pop_back();
          if ( parent != -1 ) __remove_back_edges(parent);
          continue;
        }
        const int current = row.data()[top.next];
        const auto &current_edge = __edge.data()[static_cast<usize>(current)];
        if ( !started.data()[static_cast<usize>(current)] ) {
          started.data()[static_cast<usize>(current)] = 1;
          __stack_bottom.data()[static_cast<usize>(current)] = __conflicts.size();
          if ( __parent_edge.data()[static_cast<usize>(current_edge.target)] == current ) {
            stack.push_back({ current_edge.target, 0, false });
            continue;
          }
          __lowpoint_edge.data()[static_cast<usize>(current)] = current;
          __conflicts.push_back({ {}, { current, current } });
        }
        if ( current_edge.lowpoint < __height.data()[static_cast<usize>(top.vertex)] ) {
          const int parent = __parent_edge.data()[static_cast<usize>(top.vertex)];
          if ( top.next == 0 ) {
            if ( parent != -1 ) __lowpoint_edge.data()[static_cast<usize>(parent)] = __lowpoint_edge.data()[static_cast<usize>(current)];
          } else if ( !__add_constraints(current, parent) ) {
            return false;
          }
        }
        ++top.next;
      }
    }
    return true;
  }

  void
  __resolve_sides()
  {
    __planar_vec<int> path;
    for ( usize first = 0; first < __edge.size(); ++first ) {
      path.clear();
      int current = static_cast<int>(first);
      while ( __edge.data()[static_cast<usize>(current)].reference != -1 ) {
        path.push_back(current);
        current = __edge.data()[static_cast<usize>(current)].reference;
      }
      int absolute = __edge.data()[static_cast<usize>(current)].side;
      while ( !path.empty() ) {
        const int edge = path.data()[path.size() - 1];
        path.pop_back();
        __edge.data()[static_cast<usize>(edge)].side *= absolute;
        absolute = __edge.data()[static_cast<usize>(edge)].side;
        __edge.data()[static_cast<usize>(edge)].reference = -1;
      }
    }
  }

  void
  __insert_first(int vertex, int dart)
  {
    int &head = __rotation_head.data()[static_cast<usize>(vertex)];
    if ( head == -1 ) {
      head = dart;
      __dart_next.data()[static_cast<usize>(dart)] = dart;
      __dart_previous.data()[static_cast<usize>(dart)] = dart;
      return;
    }
    const int before = __dart_previous.data()[static_cast<usize>(head)];
    __dart_next.data()[static_cast<usize>(before)] = dart;
    __dart_previous.data()[static_cast<usize>(dart)] = before;
    __dart_next.data()[static_cast<usize>(dart)] = head;
    __dart_previous.data()[static_cast<usize>(head)] = dart;
    head = dart;
  }

  void
  __insert_after(int dart, int reference)
  {
    const int after = __dart_next.data()[static_cast<usize>(reference)];
    __dart_next.data()[static_cast<usize>(reference)] = dart;
    __dart_previous.data()[static_cast<usize>(dart)] = reference;
    __dart_next.data()[static_cast<usize>(dart)] = after;
    __dart_previous.data()[static_cast<usize>(after)] = dart;
  }

  void
  __insert_before(int dart, int reference)
  {
    const int before = __dart_previous.data()[static_cast<usize>(reference)];
    __insert_after(dart, before);
  }

  void
  __embed()
  {
    __dart_next = __planar_vec<int>(__edge.size() * 2, int(-1));
    __dart_previous = __planar_vec<int>(__edge.size() * 2, int(-1));
    __rotation_head = __planar_vec<int>(__n, int(-1));
    for ( usize vertex = 0; vertex < __n; ++vertex ) {
      int previous = -1;
      for ( int edge : __ordered.data()[vertex] ) {
        const int dart = edge * 2;
        if ( previous == -1 )
          __insert_first(static_cast<int>(vertex), dart);
        else
          __insert_after(dart, previous);
        previous = dart;
      }
    }

    struct frame {
      int vertex{};
      usize next{};
    };

    __planar_vec<frame> stack;
    for ( int root : __roots ) {
      stack.push_back({ root, 0 });
      while ( !stack.empty() ) {
        auto &top = stack.data()[stack.size() - 1];
        auto &row = __ordered.data()[static_cast<usize>(top.vertex)];
        if ( top.next == row.size() ) {
          stack.pop_back();
          continue;
        }
        const int edge = row.data()[top.next++];
        const auto &value = __edge.data()[static_cast<usize>(edge)];
        if ( __parent_edge.data()[static_cast<usize>(value.target)] == edge ) {
          __insert_first(value.target, edge * 2 + 1);
          __left_reference.data()[static_cast<usize>(value.source)] = edge * 2;
          __right_reference.data()[static_cast<usize>(value.source)] = edge * 2;
          stack.push_back({ value.target, 0 });
        } else if ( value.side == 1 ) {
          __insert_after(edge * 2 + 1, __right_reference.data()[static_cast<usize>(value.target)]);
        } else {
          __insert_before(edge * 2 + 1, __left_reference.data()[static_cast<usize>(value.target)]);
          __left_reference.data()[static_cast<usize>(value.target)] = edge * 2 + 1;
        }
      }
    }
  }

public:
  template<micron::integral I>
  __lr_planarity(usize vertices, const __planar_vec<__planar_core_edge<I>> &edges, const u8 *active)
      : __n(vertices), __active(active), __adjacency(vertices), __oriented_core(edges.size(), int(-1)), __height(vertices, int(-1)),
        __parent_edge(vertices, int(-1)), __ordered(vertices), __left_reference(vertices, int(-1)), __right_reference(vertices, int(-1))
  {
    __core_source.reserve(edges.size());
    __core_target.reserve(edges.size());
    for ( usize i = 0; i < edges.size(); ++i ) {
      __core_source.push_back(edges.data()[i].source);
      __core_target.push_back(edges.data()[i].target);
      if ( active && active[i] == 0 ) continue;
      __adjacency.data()[edges.data()[i].source].push_back(static_cast<int>(i));
      __adjacency.data()[edges.data()[i].target].push_back(static_cast<int>(i));
    }
  }

  [[nodiscard]] bool
  run(bool embedding)
  {
    usize active_edges = 0;
    for ( usize i = 0; i < __core_source.size(); ++i )
      if ( !__active || __active[i] ) ++active_edges;
    if ( __n > 2 && active_edges > __n * 3 - 6 ) return false;
    __edge.reserve(active_edges);
    __orient();
    __stack_bottom = __planar_vec<usize>(__edge.size(), usize(0));
    __lowpoint_edge = __planar_vec<int>(__edge.size(), int(-1));
    __build_ordered(false);
    if ( !__test() ) return false;
    if ( embedding ) {
      __resolve_sides();
      __build_ordered(true);
      __embed();
    }
    return true;
  }

  template<micron::integral I>
  void
  make_rotation(const __planar_core<I> &core,
                micron::vector<micron::vector<edge_id<I>, micron::allocator_serial<>, false>, micron::allocator_serial<>, false> &out,
                usize vertex_slots) const
  {
    out.clear();
    out.resize(vertex_slots);
    for ( usize dense = 0; dense < __n; ++dense ) {
      auto &row = out.data()[static_cast<usize>(core.dense_to_vertex.data()[dense].value)];
      const int head = __rotation_head.data()[dense];
      if ( head != -1 ) {
        int dart = head;
        do {
          const int oriented = dart / 2;
          const auto &simple = core.edges.data()[static_cast<usize>(__edge.data()[static_cast<usize>(oriented)].core_edge)];
          if ( dense == simple.source ) {
            for ( usize i = 0; i < simple.originals_count; ++i ) row.push_back(core.originals.data()[simple.originals_begin + i]);
          } else {
            for ( usize i = simple.originals_count; i != 0; --i ) row.push_back(core.originals.data()[simple.originals_begin + i - 1]);
          }
          dart = __dart_next.data()[static_cast<usize>(dart)];
        } while ( dart != head );
      }
      for ( auto loop : core.loops.data()[dense] ) {
        row.push_back(loop);
        row.push_back(loop);
      }
    }
  }
};

template<micron::integral I>
[[nodiscard]] bool
__core_is_planar(const __planar_core<I> &core, const u8 *active)
{
  __lr_planarity test(core.vertices, core.edges, active);
  return test.run(false);
}

template<micron::integral I>
void
__extract_kuratowski(const __planar_core<I> &core, __planar_vec<edge_id<I>> &witness)
{
  __planar_vec<u8> active(core.edges.size(), u8(1));
  for ( usize edge = 0; edge < core.edges.size(); ++edge ) {
    active.data()[edge] = 0;
    if ( __core_is_planar(core, active.data()) ) active.data()[edge] = 1;
  }
  for ( usize edge = 0; edge < core.edges.size(); ++edge )
    if ( active.data()[edge] ) witness.push_back(core.originals.data()[core.edges.data()[edge].originals_begin]);
}

};      // namespace __impl

template<graph_model G>
[[nodiscard]] auto
boyer_myrvold_planarity(const G &graph, planarity_workspace<typename G::index_type> &workspace)
{
  using I = typename G::index_type;
  planarity_result<I> result;
  result.rotation.resize(graph.vertex_slots());
  if constexpr ( G::is_directed ) {
    result.status = algorithm_status::invalid_graph;
    result.planar = false;
    return result;
  }
  workspace.reserve(graph.vertices_count(), graph.edges_count());
  auto core = __impl::__make_planar_core(graph);
  __impl::__lr_planarity test(core.vertices, core.edges, nullptr);
  result.planar = test.run(true);
  if ( result.planar )
    test.make_rotation(core, result.rotation, graph.vertex_slots());
  else
    __impl::__extract_kuratowski(core, result.kuratowski_edges);
  return result;
}

template<graph_model G>
[[nodiscard]] auto
boyer_myrvold_planarity(const G &graph)
{
  planarity_workspace<typename G::index_type> workspace;
  return boyer_myrvold_planarity(graph, workspace);
}

template<graph_model G>
[[nodiscard]] auto
boyer_myrvold(const G &graph)
{
  return boyer_myrvold_planarity(graph);
}

template<graph_model G>
[[nodiscard]] bool
is_planar(const G &graph)
{
  return boyer_myrvold_planarity(graph).is_planar();
}

template<graph_model G>
[[nodiscard]] bool
validate_rotation_system(const G &graph, const planarity_result<typename G::index_type> &embedding)
{
  using I = typename G::index_type;
  if constexpr ( G::is_directed ) return false;
  if ( !embedding.is_planar() || embedding.rotation.size() != graph.vertex_slots() ) return false;
  const usize edge_slots = graph.edge_slots();
  micron::vector<u8, micron::allocator_serial<>, false> incidence_count(edge_slots, u8(0));
  micron::vector<usize, micron::allocator_serial<>, false> position_a(edge_slots, usize(-1));
  micron::vector<usize, micron::allocator_serial<>, false> position_b(edge_slots, usize(-1));
  for ( auto vertex : graph.vertices() ) {
    const usize raw = static_cast<usize>(vertex.value);
    const auto &row = embedding.rotation.data()[raw];
    for ( usize position = 0; position < row.size(); ++position ) {
      const auto edge = row.data()[position];
      if ( !graph.has_edge(edge) ) return false;
      const auto source = graph.source(edge);
      const auto target = graph.target(edge);
      if ( source != vertex && target != vertex ) return false;
      auto &count = incidence_count.data()[static_cast<usize>(edge.value)];
      if ( source == target ) {
        if ( count == 0 )
          position_a.data()[static_cast<usize>(edge.value)] = position;
        else if ( count == 1 )
          position_b.data()[static_cast<usize>(edge.value)] = position;
        else
          return false;
      } else if ( vertex == source ) {
        if ( position_a.data()[static_cast<usize>(edge.value)] != usize(-1) ) return false;
        position_a.data()[static_cast<usize>(edge.value)] = position;
      } else {
        if ( position_b.data()[static_cast<usize>(edge.value)] != usize(-1) ) return false;
        position_b.data()[static_cast<usize>(edge.value)] = position;
      }
      ++count;
    }
  }
  for ( auto edge : graph.edges() )
    if ( incidence_count.data()[static_cast<usize>(edge.id.value)] != 2 ) return false;

  micron::vector<u8, micron::allocator_serial<>, false> visited(edge_slots * 2, u8(0));
  micron::vector<usize, micron::allocator_serial<>, false> component(graph.vertex_slots(), usize(-1));
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> queue;
  micron::vector<usize, micron::allocator_serial<>, false> component_vertices;
  micron::vector<usize, micron::allocator_serial<>, false> component_edges;
  usize components = 0;
  for ( auto root : graph.vertices() ) {
    if ( component.data()[static_cast<usize>(root.value)] != usize(-1) ) continue;
    usize vertices = 0;
    usize degree_sum = 0;
    queue.clear();
    queue.push_back(root);
    component.data()[static_cast<usize>(root.value)] = components;
    for ( usize head = 0; head < queue.size(); ++head ) {
      const auto vertex = queue.data()[head];
      ++vertices;
      degree_sum += embedding.rotation.data()[static_cast<usize>(vertex.value)].size();
      for ( auto edge : graph.out_edges(vertex) ) {
        const auto next = graph.opposite(edge, vertex);
        if ( component.data()[static_cast<usize>(next.value)] == usize(-1) ) {
          component.data()[static_cast<usize>(next.value)] = components;
          queue.push_back(next);
        }
      }
    }
    component_vertices.push_back(vertices);
    component_edges.push_back(degree_sum / 2);
    ++components;
  }
  micron::vector<usize, micron::allocator_serial<>, false> faces(components, usize(0));
  for ( auto edge : graph.edges() )
    for ( usize side = 0; side < 2; ++side ) {
      usize dart = static_cast<usize>(edge.id.value) * 2 + side;
      if ( visited.data()[dart] ) continue;
      const usize owner_component = component.data()[static_cast<usize>((side == 0 ? edge.source : edge.target).value)];
      ++faces.data()[owner_component];
      usize steps = 0;
      while ( !visited.data()[dart] ) {
        if ( ++steps > graph.edges_count() * 2 + 1 ) return false;
        visited.data()[dart] = 1;
        const edge_id<I> current(static_cast<I>(dart / 2));
        const usize current_side = dart & 1u;
        const auto source = graph.source(current);
        const auto target = graph.target(current);
        const auto arrival = current_side == 0 ? target : source;
        const usize twin_position = current_side == 0 ? position_b.data()[static_cast<usize>(current.value)]
                                                      : position_a.data()[static_cast<usize>(current.value)];
        const auto &row = embedding.rotation.data()[static_cast<usize>(arrival.value)];
        if ( row.empty() || twin_position >= row.size() ) return false;
        const auto next_edge = row.data()[(twin_position + 1) % row.size()];
        const auto next_source = graph.source(next_edge);
        const auto next_target = graph.target(next_edge);
        usize next_side = 0;
        if ( next_source == next_target ) {
          const usize next_position = (twin_position + 1) % row.size();
          next_side = position_a.data()[static_cast<usize>(next_edge.value)] == next_position ? 0 : 1;
        } else {
          next_side = next_source == arrival ? 0 : 1;
        }
        dart = static_cast<usize>(next_edge.value) * 2 + next_side;
      }
    }
  for ( usize component_id = 0; component_id < components; ++component_id ) {
    if ( component_edges.data()[component_id] == 0 ) continue;
    if ( component_vertices.data()[component_id] + faces.data()[component_id] != component_edges.data()[component_id] + 2 ) return false;
  }
  return true;
}

template<graph_model G>
[[nodiscard]] bool
validate_planar_embedding(const G &graph, const planarity_result<typename G::index_type> &embedding)
{
  return validate_rotation_system(graph, embedding);
}

template<graph_model G, typename Range>
[[nodiscard]] kuratowski_kind
reduce_kuratowski_witness(const G &graph, const Range &witness)
{
  using I = typename G::index_type;
  if constexpr ( G::is_directed ) return kuratowski_kind::none;
  micron::vector<edge_id<I>, micron::allocator_serial<>, false> edges;
  micron::vector<u8, micron::allocator_serial<>, false> selected(graph.edge_slots(), u8(0));
  for ( auto edge : witness ) {
    if ( !graph.has_edge(edge) || selected.data()[static_cast<usize>(edge.value)] || graph.source(edge) == graph.target(edge) )
      return kuratowski_kind::none;
    selected.data()[static_cast<usize>(edge.value)] = 1;
    edges.push_back(edge);
  }
  if ( edges.empty() ) return kuratowski_kind::none;
  micron::vector<usize, micron::allocator_serial<>, false> degree(graph.vertex_slots(), usize(0));
  for ( auto edge : edges ) {
    ++degree.data()[static_cast<usize>(graph.source(edge).value)];
    ++degree.data()[static_cast<usize>(graph.target(edge).value)];
  }
  micron::vector<vertex_id<I>, micron::allocator_serial<>, false> branch;
  for ( auto vertex : graph.vertices() ) {
    const usize value = degree.data()[static_cast<usize>(vertex.value)];
    if ( value == 1 || value > 4 ) return kuratowski_kind::none;
    if ( value != 0 && value != 2 ) branch.push_back(vertex);
  }
  const bool maybe_k5 = branch.size() == 5;
  const bool maybe_k33 = branch.size() == 6;
  if ( !maybe_k5 && !maybe_k33 ) return kuratowski_kind::none;
  micron::vector<usize, micron::allocator_serial<>, false> branch_index(graph.vertex_slots(), usize(-1));
  for ( usize i = 0; i < branch.size(); ++i ) {
    const usize expected = maybe_k5 ? 4 : 3;
    if ( degree.data()[static_cast<usize>(branch.data()[i].value)] != expected ) return kuratowski_kind::none;
    branch_index.data()[static_cast<usize>(branch.data()[i].value)] = i;
  }
  micron::vector<u8, micron::allocator_serial<>, false> consumed(graph.edge_slots(), u8(0));
  micron::vector<u8, micron::allocator_serial<>, false> adjacency(branch.size() * branch.size(), u8(0));
  for ( usize start_index = 0; start_index < branch.size(); ++start_index ) {
    const auto start = branch.data()[start_index];
    for ( auto first : graph.out_edges(start) ) {
      if ( !selected.data()[static_cast<usize>(first.value)] || consumed.data()[static_cast<usize>(first.value)] ) continue;
      consumed.data()[static_cast<usize>(first.value)] = 1;
      auto current = graph.opposite(first, start);
      usize walked = 1;
      while ( branch_index.data()[static_cast<usize>(current.value)] == usize(-1) ) {
        edge_id<I> next = edge_id<I>::invalid();
        for ( auto candidate : graph.out_edges(current) )
          if ( selected.data()[static_cast<usize>(candidate.value)] && !consumed.data()[static_cast<usize>(candidate.value)] ) {
            next = candidate;
            break;
          }
        if ( !next.valid() || ++walked > edges.size() ) return kuratowski_kind::none;
        consumed.data()[static_cast<usize>(next.value)] = 1;
        current = graph.opposite(next, current);
      }
      const usize target_index = branch_index.data()[static_cast<usize>(current.value)];
      if ( target_index == start_index || adjacency.data()[start_index * branch.size() + target_index] ) return kuratowski_kind::none;
      adjacency.data()[start_index * branch.size() + target_index] = adjacency.data()[target_index * branch.size() + start_index] = 1;
    }
  }
  for ( auto edge : edges )
    if ( !consumed.data()[static_cast<usize>(edge.value)] ) return kuratowski_kind::none;
  if ( maybe_k5 ) {
    for ( usize u = 0; u < 5; ++u )
      for ( usize v = 0; v < 5; ++v )
        if ( (u != v) != static_cast<bool>(adjacency.data()[u * 5 + v]) ) return kuratowski_kind::none;
    return kuratowski_kind::k5;
  }
  micron::vector<int, micron::allocator_serial<>, false> color(6, int(-1));
  color.data()[0] = 0;
  micron::vector<usize, micron::allocator_serial<>, false> queue;
  queue.push_back(0);
  for ( usize head = 0; head < queue.size(); ++head ) {
    const usize u = queue.data()[head];
    for ( usize v = 0; v < 6; ++v ) {
      if ( !adjacency.data()[u * 6 + v] ) continue;
      if ( color.data()[v] == -1 ) {
        color.data()[v] = 1 - color.data()[u];
        queue.push_back(v);
      } else if ( color.data()[v] == color.data()[u] ) {
        return kuratowski_kind::none;
      }
    }
  }
  usize left = 0;
  for ( int value : color ) left += value == 0;
  return left == 3 ? kuratowski_kind::k33 : kuratowski_kind::none;
}

template<graph_model G, typename Range>
[[nodiscard]] bool
validate_kuratowski_witness(const G &graph, const Range &witness)
{
  return reduce_kuratowski_witness(graph, witness) != kuratowski_kind::none;
}

};      // namespace micron::math::graphs
