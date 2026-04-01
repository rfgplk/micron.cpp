//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "fpalgorithm.hpp"
#include "fparith.hpp"
#include "fpdata.hpp"
#include "fperrors.hpp"
#include "fpfilter.hpp"

namespace micron
{

using fp::bad_zip_error;
using fp::division_by_zero_error;
using fp::empty_container_error;
using fp::index_out_of_bounds_error;
using fp::traverse_error;

// Curried scalars
using fp::add_c;
using fp::divide_c;
using fp::multiply_c;
using fp::pow_c;
using fp::subtract_c;

// Safe variants
using fp::safe_divide;
using fp::safe_divide_c;

using fp::add;
using fp::divide;
using fp::multiply;
using fp::subtract;

// Element-wise zip
using fp::add_zip;
using fp::divide_zip;
using fp::multiply_zip;
using fp::subtract_zip;

// Inner product
using fp::inner_product;

// Negate
using fp::negate;

using fp::filter_c;
using fp::reject;
using fp::reject_c;

using fp::partition;
using fp::partition_c;

using fp::drop;
using fp::drop_c;
using fp::take;
using fp::take_c;

using fp::drop_while;
using fp::drop_while_c;
using fp::take_while;
using fp::take_while_c;

using fp::sbreak;

using fp::nub;
using fp::unique;

using fp::fmap;
using fp::fmap_c;
using fp::fmap_into;

// scan
using fp::scan;
using fp::scanl;
using fp::scanr;

// zip_with
using fp::ap;
using fp::zip_with;
using fp::zip_with_c;
using fp::zip_with_trunc;

// unzip
using fp::unzip;

// traverse/sequence
using fp::traverse;
using fp::traverse_c;

// safe aggregates
using fp::safe_max;
using fp::safe_mean;
using fp::safe_min;
using fp::safe_sum;

// predicates
using fp::all_of_c;
using fp::any_of_c;
using fp::none_of_c;

// replicate
using fp::replicate;
using fp::replicate_c;

// combinators
using fp::on;

using fp::clamp_each_c;
using fp::fill_c;
using fp::reverse_c;
using fp::sort_by_c;
using fp::sort_c;
using fp::transform_c;

using fp::flat_map;
using fp::flat_map_c;
using fp::flatten;

using fp::chunk_into;
using fp::sliding;

using fp::intersperse;
using fp::intersperse_c;

using fp::intercalate;

using fp::group;
using fp::group_by;

using fp::transpose;

using fp::concat_c;
using fp::merge_c;

using fp::concat;
using fp::merge;

using fp::at;
using fp::find_first;
using fp::find_last;

using fp::head;
using fp::init;
using fp::last;
using fp::tail;

};     // namespace micron
