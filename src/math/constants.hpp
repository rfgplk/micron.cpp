//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <type_traits>

namespace micron
{
namespace math
{
template <typename T>
inline constexpr T constant_e
    = std::enable_if_t<std::is_floating_point_v<T>, T>(2.718281828459045235360287471352662498L);
template <typename T>
inline constexpr T constant_log2e
    = std::enable_if_t<std::is_floating_point_v<T>, T>(1.442695040888963407359924681001892137L);
template <typename T>
inline constexpr T constant_log10e
    = std::enable_if_t<std::is_floating_point_v<T>, T>(0.434294481903251827651128918916605082L);

template <typename T>
inline constexpr T constant_pi
    = std::enable_if_t<std::is_floating_point_v<T>, T>(3.141592653589793238462643383279502884L);

template <typename T>
inline constexpr T constant_pi_inv
    = std::enable_if_t<std::is_floating_point_v<T>, T>(0.318309886183790671537767526745028724L);
template <typename T>
inline constexpr T constant_sqrtpi_inv
    = std::enable_if_t<std::is_floating_point_v<T>, T>(0.564189583547756286948079451560772586L);

template <typename T>
inline constexpr T constant_ln2
    = std::enable_if_t<std::is_floating_point_v<T>, T>(0.693147180559945309417232121458176568L);

template <typename T>
inline constexpr T constant_ln10
    = std::enable_if_t<std::is_floating_point_v<T>, T>(2.302585092994045684017991454684364208L);

template <typename T>
inline constexpr T constant_sqrt2
    = std::enable_if_t<std::is_floating_point_v<T>, T>(1.414213562373095048801688724209698079L);
template <typename T>
inline constexpr T constant_sqrt3
    = std::enable_if_t<std::is_floating_point_v<T>, T>(1.732050807568877293527446341505872367L);
template <typename T>
inline constexpr T constant_sqrt3_inv
    = std::enable_if_t<std::is_floating_point_v<T>, T>(0.577350269189625764509148780501957456L);
template <typename T> inline constexpr T constant_sqrt4 = std::enable_if_t<std::is_floating_point_v<T>, T>(2.0L);
template <typename T>
inline constexpr T constant_sqrt5
    = std::enable_if_t<std::is_floating_point_v<T>, T>(2.236067977499789696409173668731276235L);
template <typename T>
inline constexpr T constant_sqrt6
    = std::enable_if_t<std::is_floating_point_v<T>, T>(2.449489742783178098197284074705891392L);
template <typename T>
inline constexpr T constant_sqrt7
    = std::enable_if_t<std::is_floating_point_v<T>, T>(2.645751311064590590501615753639260426L);
template <typename T>
inline constexpr T constant_sqrt8
    = std::enable_if_t<std::is_floating_point_v<T>, T>(2.828427124746190097603377448419396157L);
template <typename T> inline constexpr T constant_sqrt9 = std::enable_if_t<std::is_floating_point_v<T>, T>(3.00L);
template <typename T>
inline constexpr T constant_sqrt10
    = std::enable_if_t<std::is_floating_point_v<T>, T>(3.162277660168379331998893544432718534L);
template <typename T>
inline constexpr T constant_sqrt11
    = std::enable_if_t<std::is_floating_point_v<T>, T>(3.316624790355399849114932736670686684L);
template <typename T>
inline constexpr T constant_sqrt12
    = std::enable_if_t<std::is_floating_point_v<T>, T>(3.464101615137754587054892683011744734L);
template <typename T>
inline constexpr T constant_sqrt13
    = std::enable_if_t<std::is_floating_point_v<T>, T>(3.605551275463989293119221267470495946L);
template <typename T>
inline constexpr T constant_sqrt14
    = std::enable_if_t<std::is_floating_point_v<T>, T>(3.741657386773941385583748732316549302L);
template <typename T>
inline constexpr T constant_sqrt15
    = std::enable_if_t<std::is_floating_point_v<T>, T>(3.872983346207416885179265399782399611L);
template <typename T> inline constexpr T constant_sqrt16 = std::enable_if_t<std::is_floating_point_v<T>, T>(4.00L);
template <typename T>
inline constexpr T constant_sqrt17
    = std::enable_if_t<std::is_floating_point_v<T>, T>(4.123105625617660549821409855974077025L);
template <typename T>
inline constexpr T constant_sqrt18
    = std::enable_if_t<std::is_floating_point_v<T>, T>(4.242640687119285146405066172629094236L);
template <typename T>
inline constexpr T constant_sqrt19
    = std::enable_if_t<std::is_floating_point_v<T>, T>(4.358898943540673552236981983859615659L);
template <typename T>
inline constexpr T constant_sqrt20
    = std::enable_if_t<std::is_floating_point_v<T>, T>(4.472135954999579392818347337462552471L);
template <typename T>
inline constexpr T constant_egamma
    = std::enable_if_t<std::is_floating_point_v<T>, T>(0.577215664901532860606512090082402431L);
template <typename T>
inline constexpr T constant_phi
    = std::enable_if_t<std::is_floating_point_v<T>, T>(1.618033988749894848204586834365638118L);
inline constexpr double e = constant_e<double>;
inline constexpr double log2e = constant_log2e<double>;
inline constexpr double log10e = constant_log10e<double>;
inline constexpr double pi = constant_pi<double>;
inline constexpr double inv_pi = constant_pi_inv<double>;
inline constexpr double inv_sqrtpi = constant_sqrtpi_inv<double>;
inline constexpr double ln2 = constant_ln2<double>;
inline constexpr double ln10 = constant_ln10<double>;
inline constexpr double sqrt2 = constant_sqrt2<double>;
inline constexpr double sqrt3 = constant_sqrt3<double>;
inline constexpr double sqrt4 = constant_sqrt4<double>;
inline constexpr double sqrt5 = constant_sqrt5<double>;
inline constexpr double sqrt6 = constant_sqrt6<double>;
inline constexpr double sqrt7 = constant_sqrt7<double>;
inline constexpr double sqrt8 = constant_sqrt8<double>;
inline constexpr double sqrt9 = constant_sqrt9<double>;
inline constexpr double sqrt10 = constant_sqrt10<double>;
inline constexpr double sqrt11 = constant_sqrt11<double>;
inline constexpr double sqrt12 = constant_sqrt12<double>;
inline constexpr double sqrt13 = constant_sqrt13<double>;
inline constexpr double sqrt14 = constant_sqrt14<double>;
inline constexpr double sqrt15 = constant_sqrt15<double>;
inline constexpr double sqrt16 = constant_sqrt16<double>;
inline constexpr double sqrt17 = constant_sqrt17<double>;
inline constexpr double sqrt18 = constant_sqrt18<double>;
inline constexpr double sqrt19 = constant_sqrt19<double>;
inline constexpr double sqrt20 = constant_sqrt20<double>;
inline constexpr double inv_sqrt3 = constant_sqrt3_inv<double>;
inline constexpr double egamma = constant_egamma<double>;
inline constexpr double phi = constant_phi<double>;
};     // namespace math
};     // namespace micron
