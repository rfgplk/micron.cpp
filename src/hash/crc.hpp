//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../types.hpp"

namespace micron
{

namespace crc
{

constexpr auto crc16_t10dif_lut = []() {
  struct __table {
    u16 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u16 crc = static_cast<u16>(i << 8);
    for ( int j = 0; j < 8; ++j ) crc = (crc & 0x8000u) ? static_cast<u16>((crc << 1) ^ 0x8BB7u) : static_cast<u16>(crc << 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc32_ieee_lut = []() {
  struct __table {
    u32 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u32 crc = i << 24;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 0x80000000u) ? ((crc << 1) ^ 0x04C11DB7u) : (crc << 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc32_gzip_refl_lut = []() {
  struct __table {
    u32 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u32 crc = i;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc32_iscsi_lut = []() {
  struct __table {
    u32 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u32 crc = i;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 1u) ? ((crc >> 1) ^ 0x82F63B78u) : (crc >> 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc64_ecma_norm_lut = []() {
  struct __table {
    u64 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u64 crc = static_cast<u64>(i) << 56;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 0x8000000000000000ull) ? ((crc << 1) ^ 0x42F0E1EBA9EA3693ull) : (crc << 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc64_ecma_refl_lut = []() {
  struct __table {
    u64 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u64 crc = i;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 1ull) ? ((crc >> 1) ^ 0xC96C5795D7870F42ull) : (crc >> 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc64_iso_norm_lut = []() {
  struct __table {
    u64 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u64 crc = static_cast<u64>(i) << 56;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 0x8000000000000000ull) ? ((crc << 1) ^ 0x000000000000001Bull) : (crc << 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc64_iso_refl_lut = []() {
  struct __table {
    u64 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u64 crc = i;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 1ull) ? ((crc >> 1) ^ 0xD800000000000000ull) : (crc >> 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc64_jones_norm_lut = []() {
  struct __table {
    u64 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u64 crc = static_cast<u64>(i) << 56;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 0x8000000000000000ull) ? ((crc << 1) ^ 0x95AC9329AC4BC9B5ull) : (crc << 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc64_jones_refl_lut = []() {
  struct __table {
    u64 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u64 crc = i;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 1ull) ? ((crc >> 1) ^ 0xAD93D23594C935A9ull) : (crc >> 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc64_rocksoft_norm_lut = []() {
  struct __table {
    u64 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u64 crc = static_cast<u64>(i) << 56;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 0x8000000000000000ull) ? ((crc << 1) ^ 0x6B2B957C8AF67BE0ull) : (crc << 1);
    t.data[i] = crc;
  }
  return t;
}();

constexpr auto crc64_rocksoft_refl_lut = []() {
  struct __table {
    u64 data[256]{};
  } t;
  for ( u32 i = 0; i < 256u; ++i ) {
    u64 crc = i;
    for ( int j = 0; j < 8; ++j ) crc = (crc & 1ull) ? ((crc >> 1) ^ 0x07D5B24186574BDBull) : (crc >> 1);
    t.data[i] = crc;
  }
  return t;
}();

}     // namespace crc

constexpr u16
crc16_t10dif(u16 init_crc, const u8 *buf, usize len) noexcept
{
  u16 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = static_cast<u16>((crc << 8) ^ crc::crc16_t10dif_lut.data[(crc >> 8) ^ buf[i]]);
  return crc;
}

template <is_iterable C>
constexpr u16
crc16_t10dif(u16 init_crc, const C &src) noexcept
{
  return crc16_t10dif(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u16
crc16_t10dif(u16 init_crc, const T &obj) noexcept
{
  return crc16_t10dif(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u32
crc32_ieee(u32 init_crc, const u8 *buf, usize len) noexcept
{
  u32 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = (crc << 8) ^ crc::crc32_ieee_lut.data[((crc >> 24) ^ buf[i]) & 0xFFu];
  return crc;
}

template <is_iterable C>
constexpr u32
crc32_ieee(u32 init_crc, const C &src) noexcept
{
  return crc32_ieee(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u32
crc32_ieee(u32 init_crc, const T &obj) noexcept
{
  return crc32_ieee(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u32
crc32_gzip_refl(u32 init_crc, const u8 *buf, usize len) noexcept
{
  u32 crc = init_crc ^ 0xFFFFFFFFu;
  for ( usize i = 0; i < len; ++i ) crc = (crc >> 8) ^ crc::crc32_gzip_refl_lut.data[(crc ^ buf[i]) & 0xFFu];
  return crc ^ 0xFFFFFFFFu;
}

template <is_iterable C>
constexpr u32
crc32_gzip_refl(u32 init_crc, const C &src) noexcept
{
  return crc32_gzip_refl(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u32
crc32_gzip_refl(u32 init_crc, const T &obj) noexcept
{
  return crc32_gzip_refl(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u32
crc32_iscsi(u32 init_crc, const u8 *buf, usize len) noexcept
{
  u32 crc = init_crc ^ 0xFFFFFFFFu;
  for ( usize i = 0; i < len; ++i ) crc = (crc >> 8) ^ crc::crc32_iscsi_lut.data[(crc ^ buf[i]) & 0xFFu];
  return crc ^ 0xFFFFFFFFu;
}

template <is_iterable C>
constexpr u32
crc32_iscsi(u32 init_crc, const C &src) noexcept
{
  return crc32_iscsi(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u32
crc32_iscsi(u32 init_crc, const T &obj) noexcept
{
  return crc32_iscsi(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u64
crc64_ecma_norm(u64 init_crc, const u8 *buf, usize len) noexcept
{
  u64 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = (crc << 8) ^ crc::crc64_ecma_norm_lut.data[((crc >> 56) ^ buf[i]) & 0xFFu];
  return crc;
}

template <is_iterable C>
constexpr u64
crc64_ecma_norm(u64 init_crc, const C &src) noexcept
{
  return crc64_ecma_norm(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u64
crc64_ecma_norm(u64 init_crc, const T &obj) noexcept
{
  return crc64_ecma_norm(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u64
crc64_ecma_refl(u64 init_crc, const u8 *buf, usize len) noexcept
{
  u64 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = (crc >> 8) ^ crc::crc64_ecma_refl_lut.data[(crc ^ buf[i]) & 0xFFu];
  return crc;
}

template <is_iterable C>
constexpr u64
crc64_ecma_refl(u64 init_crc, const C &src) noexcept
{
  return crc64_ecma_refl(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u64
crc64_ecma_refl(u64 init_crc, const T &obj) noexcept
{
  return crc64_ecma_refl(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u64
crc64_iso_norm(u64 init_crc, const u8 *buf, usize len) noexcept
{
  u64 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = (crc << 8) ^ crc::crc64_iso_norm_lut.data[((crc >> 56) ^ buf[i]) & 0xFFu];
  return crc;
}

template <is_iterable C>
constexpr u64
crc64_iso_norm(u64 init_crc, const C &src) noexcept
{
  return crc64_iso_norm(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u64
crc64_iso_norm(u64 init_crc, const T &obj) noexcept
{
  return crc64_iso_norm(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u64
crc64_iso_refl(u64 init_crc, const u8 *buf, usize len) noexcept
{
  u64 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = (crc >> 8) ^ crc::crc64_iso_refl_lut.data[(crc ^ buf[i]) & 0xFFu];
  return crc;
}

template <is_iterable C>
constexpr u64
crc64_iso_refl(u64 init_crc, const C &src) noexcept
{
  return crc64_iso_refl(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u64
crc64_iso_refl(u64 init_crc, const T &obj) noexcept
{
  return crc64_iso_refl(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u64
crc64_jones_norm(u64 init_crc, const u8 *buf, usize len) noexcept
{
  u64 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = (crc << 8) ^ crc::crc64_jones_norm_lut.data[((crc >> 56) ^ buf[i]) & 0xFFu];
  return crc;
}

template <is_iterable C>
constexpr u64
crc64_jones_norm(u64 init_crc, const C &src) noexcept
{
  return crc64_jones_norm(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u64
crc64_jones_norm(u64 init_crc, const T &obj) noexcept
{
  return crc64_jones_norm(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u64
crc64_jones_refl(u64 init_crc, const u8 *buf, usize len) noexcept
{
  u64 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = (crc >> 8) ^ crc::crc64_jones_refl_lut.data[(crc ^ buf[i]) & 0xFFu];
  return crc;
}

template <is_iterable C>
constexpr u64
crc64_jones_refl(u64 init_crc, const C &src) noexcept
{
  return crc64_jones_refl(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u64
crc64_jones_refl(u64 init_crc, const T &obj) noexcept
{
  return crc64_jones_refl(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u64
crc64_rocksoft_norm(u64 init_crc, const u8 *buf, usize len) noexcept
{
  u64 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = (crc << 8) ^ crc::crc64_rocksoft_norm_lut.data[((crc >> 56) ^ buf[i]) & 0xFFu];
  return crc;
}

template <is_iterable C>
constexpr u64
crc64_rocksoft_norm(u64 init_crc, const C &src) noexcept
{
  return crc64_rocksoft_norm(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u64
crc64_rocksoft_norm(u64 init_crc, const T &obj) noexcept
{
  return crc64_rocksoft_norm(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

constexpr u64
crc64_rocksoft_refl(u64 init_crc, const u8 *buf, usize len) noexcept
{
  u64 crc = init_crc;
  for ( usize i = 0; i < len; ++i ) crc = (crc >> 8) ^ crc::crc64_rocksoft_refl_lut.data[(crc ^ buf[i]) & 0xFFu];
  return crc;
}

template <is_iterable C>
constexpr u64
crc64_rocksoft_refl(u64 init_crc, const C &src) noexcept
{
  return crc64_rocksoft_refl(init_crc, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template <addressable T>
constexpr u64
crc64_rocksoft_refl(u64 init_crc, const T &obj) noexcept
{
  return crc64_rocksoft_refl(init_crc, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

using crc_t = u64;

};     // namespace micron
