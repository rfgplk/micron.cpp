//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"

namespace micron
{
namespace math
{
template <typename T>
inline constexpr T constant_e = micron::enable_if_t<micron::is_floating_point_v<T>, T>(2.718281828459045235360287471352662498L);
template <typename T>
inline constexpr T constant_log2e = micron::enable_if_t<micron::is_floating_point_v<T>, T>(1.442695040888963407359924681001892137L);
template <typename T>
inline constexpr T constant_log10e = micron::enable_if_t<micron::is_floating_point_v<T>, T>(0.434294481903251827651128918916605082L);

template <typename T>
inline constexpr T constant_pi = micron::enable_if_t<micron::is_floating_point_v<T>, T>(3.141592653589793238462643383279502884L);

template <typename T>
inline constexpr T constant_pi_inv = micron::enable_if_t<micron::is_floating_point_v<T>, T>(0.318309886183790671537767526745028724L);
template <typename T>
inline constexpr T constant_sqrtpi_inv = micron::enable_if_t<micron::is_floating_point_v<T>, T>(0.564189583547756286948079451560772586L);

template <typename T>
inline constexpr T constant_ln2 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(0.693147180559945309417232121458176568L);

template <typename T>
inline constexpr T constant_ln10 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(2.302585092994045684017991454684364208L);

template <typename T>
inline constexpr T constant_sqrt2 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(1.414213562373095048801688724209698079L);
template <typename T>
inline constexpr T constant_sqrt3 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(1.732050807568877293527446341505872367L);
template <typename T>
inline constexpr T constant_sqrt3_inv = micron::enable_if_t<micron::is_floating_point_v<T>, T>(0.577350269189625764509148780501957456L);
template <typename T> inline constexpr T constant_sqrt4 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(2.0L);
template <typename T>
inline constexpr T constant_sqrt5 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(2.236067977499789696409173668731276235L);
template <typename T>
inline constexpr T constant_sqrt6 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(2.449489742783178098197284074705891392L);
template <typename T>
inline constexpr T constant_sqrt7 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(2.645751311064590590501615753639260426L);
template <typename T>
inline constexpr T constant_sqrt8 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(2.828427124746190097603377448419396157L);
template <typename T> inline constexpr T constant_sqrt9 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(3.00L);
template <typename T>
inline constexpr T constant_sqrt10 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(3.162277660168379331998893544432718534L);
template <typename T>
inline constexpr T constant_sqrt11 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(3.316624790355399849114932736670686684L);
template <typename T>
inline constexpr T constant_sqrt12 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(3.464101615137754587054892683011744734L);
template <typename T>
inline constexpr T constant_sqrt13 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(3.605551275463989293119221267470495946L);
template <typename T>
inline constexpr T constant_sqrt14 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(3.741657386773941385583748732316549302L);
template <typename T>
inline constexpr T constant_sqrt15 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(3.872983346207416885179265399782399611L);
template <typename T> inline constexpr T constant_sqrt16 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(4.00L);
template <typename T>
inline constexpr T constant_sqrt17 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(4.123105625617660549821409855974077025L);
template <typename T>
inline constexpr T constant_sqrt18 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(4.242640687119285146405066172629094236L);
template <typename T>
inline constexpr T constant_sqrt19 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(4.358898943540673552236981983859615659L);
template <typename T>
inline constexpr T constant_sqrt20 = micron::enable_if_t<micron::is_floating_point_v<T>, T>(4.472135954999579392818347337462552471L);
template <typename T>
inline constexpr T constant_egamma = micron::enable_if_t<micron::is_floating_point_v<T>, T>(0.577215664901532860606512090082402431L);
template <typename T>
inline constexpr T constant_phi = micron::enable_if_t<micron::is_floating_point_v<T>, T>(1.618033988749894848204586834365638118L);
template <typename T> constexpr T pi_t() noexcept;
template <>
constexpr float
pi_t<float>() noexcept
{
  return 3.14159265358979323846f;
}
template <>
constexpr double
pi_t<double>() noexcept
{
  return 3.14159265358979323846;
}
template <>
constexpr long double
pi_t<long double>() noexcept
{
  return 3.14159265358979323846L;
}

template <typename T> constexpr T default_eps() noexcept;
template <>
constexpr float
default_eps<float>() noexcept
{
  return 1e-6f;
}
template <>
constexpr double
default_eps<double>() noexcept
{
  return 1e-12;
}
template <>
constexpr long double
default_eps<long double>() noexcept
{
  return 1e-15L;
}

inline constexpr float e_float = constant_e<float>;
inline constexpr float log2e_float = constant_log2e<float>;
inline constexpr float log10e_float = constant_log10e<float>;
inline constexpr float pi_float = constant_pi<float>;
inline constexpr float inv_pi_float = constant_pi_inv<float>;
inline constexpr float inv_sqrtpi_float = constant_sqrtpi_inv<float>;
inline constexpr float ln2_float = constant_ln2<float>;
inline constexpr float ln10_float = constant_ln10<float>;
inline constexpr float sqrt2_float = constant_sqrt2<float>;
inline constexpr float sqrt3_float = constant_sqrt3<float>;
inline constexpr float sqrt4_float = constant_sqrt4<float>;
inline constexpr float sqrt5_float = constant_sqrt5<float>;
inline constexpr float sqrt6_float = constant_sqrt6<float>;
inline constexpr float sqrt7_float = constant_sqrt7<float>;
inline constexpr float sqrt8_float = constant_sqrt8<float>;
inline constexpr float sqrt9_float = constant_sqrt9<float>;
inline constexpr float sqrt10_float = constant_sqrt10<float>;
inline constexpr float sqrt11_float = constant_sqrt11<float>;
inline constexpr float sqrt12_float = constant_sqrt12<float>;
inline constexpr float sqrt13_float = constant_sqrt13<float>;
inline constexpr float sqrt14_float = constant_sqrt14<float>;
inline constexpr float sqrt15_float = constant_sqrt15<float>;
inline constexpr float sqrt16_float = constant_sqrt16<float>;
inline constexpr float sqrt17_float = constant_sqrt17<float>;
inline constexpr float sqrt18_float = constant_sqrt18<float>;
inline constexpr float sqrt19_float = constant_sqrt19<float>;
inline constexpr float sqrt20_float = constant_sqrt20<float>;
inline constexpr float inv_sqrt3_float = constant_sqrt3_inv<float>;
inline constexpr float egamma_float = constant_egamma<float>;
inline constexpr float phi_float = constant_phi<float>;

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
