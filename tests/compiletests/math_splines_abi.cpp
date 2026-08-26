// Compile-only ABI sentinels for the spline surface consumed by Meridionalis.

#include "../../src/math/splines.hpp"
#include "../../src/version.hpp"

using namespace micron;
using namespace micron::math;
using namespace micron::math::splines;

static_assert(MICRON_ABI == 9);
static_assert(static_cast<u32>(bc_kind::natural) == 0);
static_assert(static_cast<u32>(bc_kind::clamped) == 1);
static_assert(static_cast<u32>(bc_kind::not_a_knot) == 2);
static_assert(static_cast<u32>(extrap::clamp_to_endpoints) == 0);
static_assert(static_cast<u32>(extrap::linear_continue) == 1);
static_assert(static_cast<u32>(extrap::error_value) == 2);
static_assert(static_cast<u32>(build_status::ok) == 0);
static_assert(static_cast<u32>(build_status::invalid_argument) == 7);

static_assert(__builtin_offsetof(cubic_spline_1d<f64>, xs) == 0);
static_assert(__builtin_offsetof(cubic_spline_1d<f64>, seg) == sizeof(vector<f64>));
static_assert(__builtin_offsetof(cubic_spline_1d<f64>, last_hit) == 2 * sizeof(vector<f64>));
static_assert(__builtin_offsetof(cubic_curve_nd<f64, 6>, ts) == 0);
static_assert(__builtin_offsetof(cubic_curve_nd<f64, 6>, seg) == sizeof(vector<f64>));
static_assert(__builtin_offsetof(cubic_curve_nd<f64, 6>, last_hit) == 2 * sizeof(vector<f64>));

static_assert(sizeof(curve_seg<f32, 2>) == 64 && alignof(curve_seg<f32, 2>) == 16);
static_assert(sizeof(curve_seg<f64, 2>) == 64 && alignof(curve_seg<f64, 2>) == 16);
static_assert(sizeof(curve_seg<f32, 3>) == 64 && alignof(curve_seg<f32, 3>) == 16);
static_assert(sizeof(curve_seg<f64, 3>) == 128 && alignof(curve_seg<f64, 3>) == 32);
static_assert(sizeof(curve_seg<f32, 6>) == 128 && alignof(curve_seg<f32, 6>) == 32);
static_assert(sizeof(curve_seg<f64, 6>) == 256 && alignof(curve_seg<f64, 6>) == 64);

#if defined(__micron_arch_width_64)
static_assert(sizeof(nearest_1d<f32>) == 64 && sizeof(nearest_1d<f64>) == 64);
static_assert(sizeof(linear_1d<f32>) == 64 && sizeof(linear_1d<f64>) == 64);
static_assert(sizeof(cubic_spline_1d<f32>) == 64 && sizeof(cubic_spline_1d<f64>) == 64);
static_assert(sizeof(bspline<f32>) == 64 && sizeof(bspline<f64>) == 64);
static_assert(sizeof(cubic_curve_nd<f32, 6>) == 64 && sizeof(cubic_curve_nd<f64, 6>) == 64);
static_assert(sizeof(regular_cubic_curve_nd<f32, 6>) == 40);
static_assert(sizeof(regular_cubic_curve_nd<f64, 6>) == 48);
#endif

using make_cubic_f64_t
    = cubic_spline_1d<f64> (*)(raw_slice<const f64>, raw_slice<const f64>, bc_kind, f64, f64, build_info<f64> *) noexcept;
using eval_cubic_f64_t = f64 (*)(const cubic_spline_1d<f64> &, f64) noexcept;
using eval_cubic_batch_f64_t = void (*)(const cubic_spline_1d<f64> &, const f64 *, f64 *, usize) noexcept;
using make_curve_f64_6_t = cubic_curve_nd<f64, 6> (*)(raw_slice<const f64>, const vec<f64, 6> *, usize, bc_kind, vec<f64, 6>, vec<f64, 6>,
                                                      build_info<f64> *) noexcept;
using locate_f64_t = usize (*)(const f64 *, usize, f64, usize &) noexcept;

static_assert(static_cast<make_cubic_f64_t>(&make_cubic<f64>) != nullptr);
static_assert(static_cast<eval_cubic_f64_t>(&evaluate<f64>) != nullptr);
static_assert(static_cast<eval_cubic_batch_f64_t>(&evaluate<f64>) != nullptr);
static_assert(static_cast<make_curve_f64_6_t>(&make_cubic_curve<f64, 6>) != nullptr);
static_assert(static_cast<locate_f64_t>(&__impl_splines_bits::locate_segment<f64>) != nullptr);

int
main()
{
  return 1;
}
