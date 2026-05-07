//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Gauss-Legendre nodes and weights for orders 5, 8, 16, 32

#include "../../../../types.hpp"
#include "../../../ieee.hpp"

namespace micron
{
namespace math
{
namespace integrate
{
namespace coeff
{
namespace gl
{

template <ieee754_floating F, usize Order> struct gl_table {
  static_assert(Order == 5 or Order == 8 or Order == 16 or Order == 32, "Gauss-Legendre table only available for orders 5, 8, 16, 32");
};

template <ieee754_floating F> struct gl_table<F, 5> {
  static constexpr F nodes[3] = {
    F(0.0L),
    F(0.5384693101056830910363144L),
    F(0.9061798459386639927976269L),
  };
  static constexpr F weights[3] = {
    F(0.5688888888888888888888889L),
    F(0.4786286704993664680412915L),
    F(0.2369268850561890875142640L),
  };
  static constexpr usize half = 3;
  static constexpr bool has_zero = true;
};

template <ieee754_floating F> struct gl_table<F, 8> {
  static constexpr F nodes[4] = {
    F(0.1834346424956498049394761L),
    F(0.5255324099163289858177390L),
    F(0.7966664774136267395915539L),
    F(0.9602898564975362316835609L),
  };
  static constexpr F weights[4] = {
    F(0.3626837833783619829651504L),
    F(0.3137066458778872873379622L),
    F(0.2223810344533744705443560L),
    F(0.1012285362903762591525314L),
  };
  static constexpr usize half = 4;
  static constexpr bool has_zero = false;
};

template <ieee754_floating F> struct gl_table<F, 16> {
  static constexpr F nodes[8] = {
    F(0.0950125098376374401853193L), F(0.2816035507792589132304605L), F(0.4580167776572273863424194L), F(0.6178762444026437484466718L),
    F(0.7554044083550030338951012L), F(0.8656312023878317438804679L), F(0.9445750230732325760779884L), F(0.9894009349916499325961542L),
  };
  static constexpr F weights[8] = {
    F(0.1894506104550684962853967L), F(0.1826034150449235888667637L), F(0.1691565193950025381893121L), F(0.1495959888165767320815017L),
    F(0.1246289712555338720524763L), F(0.0951585116824927848099251L), F(0.0622535239386478928628438L), F(0.0271524594117540948517806L),
  };
  static constexpr usize half = 8;
  static constexpr bool has_zero = false;
};

template <ieee754_floating F> struct gl_table<F, 32> {
  static constexpr F nodes[16] = {
    F(0.0483076656877383162348126L), F(0.1444719615827964934851864L), F(0.2392873622521370745446032L), F(0.3318686022821276497799168L),
    F(0.4213512761306353453641194L), F(0.5068999089322293900237475L), F(0.5877157572407623290407455L), F(0.6630442669302152009751152L),
    F(0.7321821187402896803874267L), F(0.7944837959679424069630973L), F(0.8493676137325699701336930L), F(0.8963211557660521239653072L),
    F(0.9349060759377396891709191L), F(0.9647622555875064307738119L), F(0.9856115115452683354001750L), F(0.9972638618494815635449811L),
  };
  static constexpr F weights[16] = {
    F(0.0965400885147278005667648L), F(0.0956387200792748594190820L), F(0.0938443990808045656391802L), F(0.0911738786957638847128686L),
    F(0.0876520930044038111427715L), F(0.0833119242269467552221991L), F(0.0781938957870703064717409L), F(0.0723457941088485062253994L),
    F(0.0658222227763618468376501L), F(0.0586840934785355471452836L), F(0.0509980592623761761961632L), F(0.0428358980222266806568786L),
    F(0.0342738629130214331026877L), F(0.0253920653092620594557526L), F(0.0162743947309056706051706L), F(0.0070186100094700966004071L),
  };
  static constexpr usize half = 16;
  static constexpr bool has_zero = false;
};

};     // namespace gl
};     // namespace coeff
};     // namespace integrate
};     // namespace math
};     // namespace micron
