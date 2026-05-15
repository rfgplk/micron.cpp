//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// storage of a sequence of Householder reflectors

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "householder.hpp"
#include "types.hpp"

namespace micron
{
namespace math
{
namespace linalg
{
namespace decomp
{

// fixed-size
template<ieee754_floating F, usize R_, usize C_>
  requires(R_ >= 2)
struct householder_sequence {
  static constexpr usize K = (R_ < C_) ? R_ : C_;

  mat<F, R_, K> V;      // column k = reflector v_k (zero for rows < k)
  F betas[K];           // K may be 1, don't use vec

  // apply Q from the left
  template<usize M_>
  inline void
  apply_left(mat<F, R_, M_> &A) const noexcept
  {
    for ( usize kk = K; kk-- > 0; ) {
      vec<F, R_> v{};
      for ( usize i = 0; i < R_; ++i ) v.data[i] = F(0);
      for ( usize i = kk; i < R_; ++i ) v.data[i] = V.data[i * K + kk];
      apply_householder_left<F, R_, M_>(A, v, betas[kk], kk);
    }
  }

  // apply Q^T from the left
  template<usize M_>
  inline void
  apply_left_transposed(mat<F, R_, M_> &A) const noexcept
  {
    for ( usize kk = 0; kk < K; ++kk ) {
      vec<F, R_> v{};
      for ( usize i = 0; i < R_; ++i ) v.data[i] = F(0);
      for ( usize i = kk; i < R_; ++i ) v.data[i] = V.data[i * K + kk];
      apply_householder_left<F, R_, M_>(A, v, betas[kk], kk);
    }
  }

  // apply Q from the right
  template<usize M_>
  inline void
  apply_right(mat<F, M_, R_> &A) const noexcept
  {
    for ( usize kk = 0; kk < K; ++kk ) {
      vec<F, R_> v{};
      for ( usize i = 0; i < R_; ++i ) v.data[i] = F(0);
      for ( usize i = kk; i < R_; ++i ) v.data[i] = V.data[i * K + kk];
      apply_householder_right<F, M_, R_>(A, v, betas[kk], kk);
    }
  }

  // apply Q^T from the right
  template<usize M_>
  inline void
  apply_right_transposed(mat<F, M_, R_> &A) const noexcept
  {
    for ( usize kk = K; kk-- > 0; ) {
      vec<F, R_> v{};
      for ( usize i = 0; i < R_; ++i ) v.data[i] = F(0);
      for ( usize i = kk; i < R_; ++i ) v.data[i] = V.data[i * K + kk];
      apply_householder_right<F, M_, R_>(A, v, betas[kk], kk);
    }
  }

  [[nodiscard]] inline mat<F, R_, R_>
  explicit_form() const noexcept
  {
    mat<F, R_, R_> Q = mat<F, R_, R_>::identity();
    apply_left<R_>(Q);
    return Q;
  }
};

// dynamic-shape compact sequence
template<ieee754_floating F> struct householder_sequence_dyn {
  dynmat<F> V;                                                     // R x K
  micron::vector<F, micron::allocator_serial<>, false> betas;      // length K
  usize R{ 0 };                                                    // rows of Q (Q is R x R)
  usize K{ 0 };                                                    // number of reflectors

  // apply Q from the left
  inline void
  apply_left(dynmat<F> &A) const noexcept
  {
    const usize Acols = A.cols;
    micron::vector<F, micron::allocator_serial<>, false> v(R, F(0));
    for ( usize kk = K; kk-- > 0; ) {
      for ( usize i = 0; i < R; ++i ) v.data()[i] = F(0);
      for ( usize i = kk; i < R; ++i ) v.data()[i] = V.at(i, kk);
      __impl_decomp::apply_householder_left_dyn<F>(A.data(), R, Acols, A.ld, v.data(), betas.data()[kk], kk);
    }
  }

  inline void
  apply_left_transposed(dynmat<F> &A) const noexcept
  {
    const usize Acols = A.cols;
    micron::vector<F, micron::allocator_serial<>, false> v(R, F(0));
    for ( usize kk = 0; kk < K; ++kk ) {
      for ( usize i = 0; i < R; ++i ) v.data()[i] = F(0);
      for ( usize i = kk; i < R; ++i ) v.data()[i] = V.at(i, kk);
      __impl_decomp::apply_householder_left_dyn<F>(A.data(), R, Acols, A.ld, v.data(), betas.data()[kk], kk);
    }
  }

  inline void
  apply_right(dynmat<F> &A) const noexcept
  {
    const usize Arows = A.rows;
    micron::vector<F, micron::allocator_serial<>, false> v(R, F(0));
    for ( usize kk = 0; kk < K; ++kk ) {
      for ( usize i = 0; i < R; ++i ) v.data()[i] = F(0);
      for ( usize i = kk; i < R; ++i ) v.data()[i] = V.at(i, kk);
      __impl_decomp::apply_householder_right_dyn<F>(A.data(), Arows, R, A.ld, v.data(), betas.data()[kk], kk);
    }
  }

  inline void
  apply_right_transposed(dynmat<F> &A) const noexcept
  {
    const usize Arows = A.rows;
    micron::vector<F, micron::allocator_serial<>, false> v(R, F(0));
    for ( usize kk = K; kk-- > 0; ) {
      for ( usize i = 0; i < R; ++i ) v.data()[i] = F(0);
      for ( usize i = kk; i < R; ++i ) v.data()[i] = V.at(i, kk);
      __impl_decomp::apply_householder_right_dyn<F>(A.data(), Arows, R, A.ld, v.data(), betas.data()[kk], kk);
    }
  }

  [[nodiscard]] inline dynmat<F>
  explicit_form() const noexcept
  {
    dynmat<F> Q = dynmat<F>::identity(R);
    apply_left(Q);
    return Q;
  }
};

};      // namespace decomp
};      // namespace linalg
};      // namespace math
};      // namespace micron
