//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"

namespace micron::math::graphs
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// topology policies

struct undirected_t {
};

struct directed_t {
};

struct simple_t {
};

struct parallel_t {
};

using multigraph_t = parallel_t;

struct no_loops_t {
};

struct allow_loops_t {
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// representation policies

struct stable_adjacency_t {
};

struct compact_adjacency_t {
};

struct edge_list_t {
};

struct csr_t {
};

struct bidirectional_csr_t {
};

struct dense_adjacency_t {
};

struct bit_adjacency_t {
};

template<typename T>
concept direction_policy = micron::is_same_v<T, undirected_t> || micron::is_same_v<T, directed_t>;

template<typename T>
concept multiplicity_policy = micron::is_same_v<T, simple_t> || micron::is_same_v<T, parallel_t>;

template<typename T>
concept loop_policy = micron::is_same_v<T, no_loops_t> || micron::is_same_v<T, allow_loops_t>;

enum class edge_insert_status : u8 { inserted = 0, duplicate, self_loop, invalid_vertex, property_required, index_overflow };

enum class algorithm_status : u8 {
  ok = 0,
  invalid_vertex,
  unreachable,
  negative_cycle,
  invalid_weight,
  overflow,
  non_convergent,
  not_a_dag,
  disconnected,
  invalid_graph,
  infeasible
};

enum class coalesce_policy : u8 { reject = 0, keep_first, keep_last, sum };

};      // namespace micron::math::graphs
