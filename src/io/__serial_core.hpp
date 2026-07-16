//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../errno.hpp"
#include "../memory/cmemory/memcpy.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//  micron framed representation
//
// 8-byte header {
//   'M','F','R','1',
//   u8 flags (bit0 = little-endian),
//   u8 kind (0 seq / 1 map / 2 set),
//   u16 reserved }
// body grammar (recursive, packed, native-endian):
//   trivially-copyable element              -> raw sizeof(E) bytes
//   string element                          -> u64 byte_len + raw bytes
//   contiguous container of TC elements     -> u64 count + one count*sizeof(vt) blit
//   node/nested container                   -> u64 count + frame(element) each
//   map                                     -> u64 pair_count + frame(key) frame(value) each

namespace micron
{
namespace io
{
namespace serialize
{

namespace __impl
{

constexpr usize __mfr_header = 8;
constexpr byte __mfr_magic[4] = { 'M', 'F', 'R', '1' };
constexpr usize __mfr_max_depth = 64;

constexpr bool
__little_endian(void) noexcept
{
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
  return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#else
  return true;
#endif
}

// singly/doubly linked node-chain containers (micron::list / micron::doublelist)
template<typename T>
concept node_chain = requires(const T t) {
  { t.ibegin()->next };
  { t.ibegin()->data };
};

template<typename T, typename K, typename V>
concept __kv_rebuildable = requires(T t, K k, V v) { t.insert(micron::move(k), micron::move(v)); }
                           || requires(T t, K k, V v) { t.insert_unhash(micron::move(k), micron::move(v)); };

template<typename T>
concept kv_key_iter = requires(const T c, T t) {
  { c.begin()->key };
  { c.begin()->value };
  requires __kv_rebuildable<T, micron::remove_cvref_t<decltype(c.begin()->key)>, micron::remove_cvref_t<decltype(c.begin()->value)>>;
};

template<typename T>
concept kv_ab_iter = requires(const T c, T t) {
  { (*c.begin()).a };
  { (*c.begin()).b };
  requires __kv_rebuildable<T, micron::remove_cvref_t<decltype((*c.begin()).a)>, micron::remove_cvref_t<decltype((*c.begin()).b)>>;
} && !kv_key_iter<T>;

template<typename T>
concept map_like = kv_key_iter<T> || kv_ab_iter<T>;

template<typename E>
concept frameable_leaf = micron::is_trivially_copyable_v<micron::remove_cvref_t<E>> && !is_string<micron::remove_cvref_t<E>>;

// NOTE: a leaf reconstructed from untrusted MFR1 bytes is safe only if every bit pattern is a valid value
template<typename T>
concept __has_pod_safe_tag = requires { typename micron::remove_cvref_t<T>::mfr_pod_safe_tag; };

template<typename T>
inline constexpr bool mfr_pod_safe
    = (micron::is_arithmetic_v<micron::remove_cvref_t<T>> && !micron::is_same_v<micron::remove_cvref_t<T>, bool>) || __has_pod_safe_tag<T>;

// contiguous container whose elements can be blitted in one copy
template<typename T>
concept blit_container = is_contiguous_container<T> && frameable_leaf<typename T::value_type>;

// anything we can walk with real iterators (maps handled separately; node chains excluded)
template<typename T>
concept iterable_generic = requires(const T t) {
  t.begin();
  t.end();
} && !node_chain<T> && !map_like<T> && !has_cstr<T> && requires { typename T::value_type; };

inline void
__put_bytes(byte *dst, usize &off, const void *src, usize n) noexcept
{
  micron::bytecpy(dst + off, static_cast<const byte *>(src), n);
  off += n;
}

inline void
__put_u64(byte *dst, usize &off, u64 v) noexcept
{
  __put_bytes(dst, off, &v, sizeof(u64));
}

inline bool
__get_bytes(const byte *src, usize len, usize &off, void *dst, usize n) noexcept
{
  if ( off + n > len ) [[unlikely]]
    return false;
  micron::bytecpy(static_cast<byte *>(dst), src + off, n);
  off += n;
  return true;
}

template<typename T>
inline void
__blit_set_len(T &c, usize elems) noexcept
{
  if constexpr ( requires(T t, usize n) { t.set_size(n); } )
    c.set_size(elems);
  else if constexpr ( requires(T t, usize n) { t._buf_set_length(n); } )
    c._buf_set_length(elems);
}

inline bool
__get_u64(const byte *src, usize len, usize &off, u64 &v) noexcept
{
  return __get_bytes(src, len, off, &v, sizeof(u64));
}

template<usize D, typename E>
max_t
frame_element(byte *dst, usize cap, usize &off, const E &e) noexcept
{
  static_assert(D <= __mfr_max_depth, "micron::io::serialize: MFR1 nesting depth exceeded");
  using U = micron::remove_cvref_t<E>;

  if constexpr ( is_string<U> ) {
    const u64 blen = static_cast<u64>(e.size()) * sizeof(typename U::value_type);
    if ( dst ) {
      if ( off + sizeof(u64) + blen > cap ) [[unlikely]]
        return -error::invalid_arg;
      __put_u64(dst, off, blen);
      __put_bytes(dst, off, e.c_str(), static_cast<usize>(blen));
    } else {
      off += sizeof(u64) + static_cast<usize>(blen);
    }
    return 0;
  } else if constexpr ( blit_container<U> ) {
    const u64 cnt = static_cast<u64>(e.size());
    const usize blen = static_cast<usize>(cnt) * sizeof(typename U::value_type);
    if ( dst ) {
      if ( off + sizeof(u64) + blen > cap ) [[unlikely]]
        return -error::invalid_arg;
      __put_u64(dst, off, cnt);
      __put_bytes(dst, off, e.data(), blen);
    } else {
      off += sizeof(u64) + blen;
    }
    return 0;
  } else if constexpr ( node_chain<U> ) {
    u64 cnt = 0;
    for ( auto *p = e.ibegin(); p != nullptr; p = p->next ) ++cnt;
    if ( dst ) {
      if ( off + sizeof(u64) > cap ) [[unlikely]]
        return -error::invalid_arg;
      __put_u64(dst, off, cnt);
    } else {
      off += sizeof(u64);
    }
    for ( auto *p = e.ibegin(); p != nullptr; p = p->next )
      if ( max_t r = frame_element<D + 1>(dst, cap, off, p->data); r < 0 ) [[unlikely]]
        return r;
    return 0;
  } else if constexpr ( kv_key_iter<U> ) {
    const u64 cnt = static_cast<u64>(e.size());
    if ( dst ) {
      if ( off + sizeof(u64) > cap ) [[unlikely]]
        return -error::invalid_arg;
      __put_u64(dst, off, cnt);
    } else {
      off += sizeof(u64);
    }
    for ( auto itr = e.begin(); itr != e.end(); ++itr ) {
      if ( !itr->key ) continue;      // skip empty slots (same convention as the print engine)
      if ( max_t r = frame_element<D + 1>(dst, cap, off, itr->key); r < 0 ) [[unlikely]]
        return r;
      if ( max_t r = frame_element<D + 1>(dst, cap, off, itr->value); r < 0 ) [[unlikely]]
        return r;
    }
    return 0;
  } else if constexpr ( kv_ab_iter<U> ) {
    const u64 cnt = static_cast<u64>(e.size());
    if ( dst ) {
      if ( off + sizeof(u64) > cap ) [[unlikely]]
        return -error::invalid_arg;
      __put_u64(dst, off, cnt);
    } else {
      off += sizeof(u64);
    }
    for ( auto itr = e.begin(); itr != e.end(); ++itr ) {
      if ( max_t r = frame_element<D + 1>(dst, cap, off, (*itr).a); r < 0 ) [[unlikely]]
        return r;
      if ( max_t r = frame_element<D + 1>(dst, cap, off, (*itr).b); r < 0 ) [[unlikely]]
        return r;
    }
    return 0;
  } else if constexpr ( iterable_generic<U> && !frameable_leaf<U> ) {
    u64 cnt = 0;
    for ( auto itr = e.begin(); itr != e.end(); ++itr ) ++cnt;
    if ( dst ) {
      if ( off + sizeof(u64) > cap ) [[unlikely]]
        return -error::invalid_arg;
      __put_u64(dst, off, cnt);
    } else {
      off += sizeof(u64);
    }
    for ( auto itr = e.begin(); itr != e.end(); ++itr )
      if ( max_t r = frame_element<D + 1>(dst, cap, off, *itr); r < 0 ) [[unlikely]]
        return r;
    return 0;
  } else if constexpr ( frameable_leaf<U> ) {
    if ( dst ) {
      if ( off + sizeof(U) > cap ) [[unlikely]]
        return -error::invalid_arg;
      __put_bytes(dst, off, &e, sizeof(U));
    } else {
      off += sizeof(U);
    }
    return 0;
  } else {
    static_assert(sizeof(U) == 0, "micron::io::serialize: type is not MFR1-frameable");
    return -error::invalid_arg;
  }
}

// key/value type extraction for map reconstruction
template<kv_key_iter M> auto __map_key(M &m) -> decltype(m.begin()->key);
template<kv_key_iter M> auto __map_value(M &m) -> decltype(m.begin()->value);
template<kv_ab_iter M> auto __map_key(M &m) -> decltype((*m.begin()).a);
template<kv_ab_iter M> auto __map_value(M &m) -> decltype((*m.begin()).b);

template<usize D, typename E>
max_t
unframe_element(const byte *src, usize len, usize &off, E &out) noexcept
{
  static_assert(D <= __mfr_max_depth, "micron::io::serialize: MFR1 nesting depth exceeded");
  using U = micron::remove_cvref_t<E>;

  if constexpr ( is_string<U> ) {
    u64 blen = 0;
    if ( !__get_u64(src, len, off, blen) ) [[unlikely]]
      return -error::invalid_arg;
    if ( blen > len - off ) [[unlikely]]      // overflow safe
      return -error::invalid_arg;
    const usize elems = static_cast<usize>(blen) / sizeof(typename U::value_type);
    out = U();
    if constexpr ( requires(U u, usize n) { u.reserve(n); } ) out.reserve(elems + 1);
    for ( usize i = 0; i < elems; ++i ) {
      typename U::value_type ch{};
      __get_bytes(src, len, off, &ch, sizeof(ch));
      out.push_back(ch);
    }
    off += static_cast<usize>(blen) - elems * sizeof(typename U::value_type);      // skip any trailing partial element
    return 0;
  } else if constexpr ( blit_container<U> ) {
    static_assert(mfr_pod_safe<typename U::value_type>,
                  "micron::io::serialize::unframe: refusing to reconstruct a contiguous container of a pointer/bool/"
                  "enum/opaque-POD element from possibly-untrusted MFR1 bytes. Add using mfr_pod_safe_tag = void; to "
                  "the element type if every bit pattern is a valid value.");
    u64 cnt = 0;
    if ( !__get_u64(src, len, off, cnt) ) [[unlikely]]
      return -error::invalid_arg;
    if ( cnt > (len - off) / sizeof(typename U::value_type) ) [[unlikely]]
      return -error::invalid_arg;
    const usize nbytes = static_cast<usize>(cnt) * sizeof(typename U::value_type);
    out = U();
    if constexpr ( requires(U u, usize n) { u.reserve(n); } ) out.reserve(static_cast<usize>(cnt) + 1);
    if constexpr ( (requires(U u, usize n) { u.data(); u.set_size(n); } || requires(U u, usize n) { u.data(); u._buf_set_length(n); })
                   && requires(U u, usize n) { u.reserve(n); } ) {
      // contiguous target with a size setter
      if ( !__get_bytes(src, len, off, out.data(), nbytes) ) [[unlikely]]
        return -error::invalid_arg;
      __blit_set_len(out, static_cast<usize>(cnt));
    } else if constexpr ( requires(U u, typename U::value_type v) { u.push_back(v); } ) {
      for ( u64 i = 0; i < cnt; ++i ) {
        typename U::value_type v;
        __get_bytes(src, len, off, &v, sizeof(v));
        out.push_back(v);
      }
    } else {
      usize i = 0;
      for ( ; i < static_cast<usize>(cnt) && i < out.size(); ++i ) __get_bytes(src, len, off, &out[i], sizeof(typename U::value_type));
      off += (static_cast<usize>(cnt) - i) * sizeof(typename U::value_type);
    }
    return 0;
  } else if constexpr ( node_chain<U> ) {
    u64 cnt = 0;
    if ( !__get_u64(src, len, off, cnt) ) [[unlikely]]
      return -error::invalid_arg;
    out = U();
    for ( u64 i = 0; i < cnt; ++i ) {
      typename U::value_type v{};
      if ( max_t r = unframe_element<D + 1>(src, len, off, v); r < 0 ) [[unlikely]]
        return r;
      out.push_back(micron::move(v));
    }
    return 0;
  } else if constexpr ( map_like<U> ) {
    u64 cnt = 0;
    if ( !__get_u64(src, len, off, cnt) ) [[unlikely]]
      return -error::invalid_arg;
    out = U();
    using K = micron::remove_cvref_t<decltype(__map_key(out))>;
    using V = micron::remove_cvref_t<decltype(__map_value(out))>;
    for ( u64 i = 0; i < cnt; ++i ) {
      K k{};
      V v{};
      if ( max_t r = unframe_element<D + 1>(src, len, off, k); r < 0 ) [[unlikely]]
        return r;
      if ( max_t r = unframe_element<D + 1>(src, len, off, v); r < 0 ) [[unlikely]]
        return r;
      if constexpr ( requires(U u) { u.insert_unhash(k, micron::move(v)); } )
        out.insert_unhash(k, micron::move(v));      // tagged maps store hashes; do not rehash
      else
        out.insert(micron::move(k), micron::move(v));
    }
    return 0;
  } else if constexpr ( iterable_generic<U> && !frameable_leaf<U> ) {
    u64 cnt = 0;
    if ( !__get_u64(src, len, off, cnt) ) [[unlikely]]
      return -error::invalid_arg;
    out = U();
    for ( u64 i = 0; i < cnt; ++i ) {
      typename U::value_type v{};
      if ( max_t r = unframe_element<D + 1>(src, len, off, v); r < 0 ) [[unlikely]]
        return r;
      if constexpr ( requires(U u, typename U::value_type x) { u.push_back(micron::move(x)); } )
        out.push_back(micron::move(v));
      else if constexpr ( requires(U u, typename U::value_type x) { u.insert(micron::move(x)); } )
        out.insert(micron::move(v));
      else
        static_assert(sizeof(U) == 0, "micron::io::serialize: no way to insert into this container");
    }
    return 0;
  } else if constexpr ( frameable_leaf<U> ) {
    static_assert(mfr_pod_safe<U>, "micron::io::serialize::unframe: refusing to reconstruct this type from possibly-untrusted MFR1 "
                                   "bytes; a pointer/bool/enum/opaque-POD leaf can form an invalid value (wild pointer / UB). Add "
                                   "using mfr_pod_safe_tag = void; to the type if every bit pattern is a valid value.");
    if ( !__get_bytes(src, len, off, &out, sizeof(U)) ) [[unlikely]]
      return -error::invalid_arg;
    return 0;
  } else {
    static_assert(sizeof(U) == 0, "micron::io::serialize: type is not MFR1-unframeable");
    return -error::invalid_arg;
  }
}

template<typename C>
constexpr byte
__mfr_kind(void) noexcept
{
  if constexpr ( map_like<micron::remove_cvref_t<C>> )
    return 1;
  else
    return 0;
}

};      // namespace __impl

template<typename C>
max_t
framed_size(const C &c) noexcept
{
  usize off = 0;
  if ( max_t r = __impl::frame_element<0>(nullptr, 0, off, c); r < 0 ) [[unlikely]]
    return r;
  return static_cast<max_t>(off + __impl::__mfr_header);
}

// serialize c into [dst, dst+cap)
template<typename C>
max_t
frame_into(byte *dst, usize cap, const C &c) noexcept
{
  if ( dst == nullptr || cap < __impl::__mfr_header ) [[unlikely]]
    return -error::invalid_arg;
  usize off = 0;
  dst[off++] = __impl::__mfr_magic[0];
  dst[off++] = __impl::__mfr_magic[1];
  dst[off++] = __impl::__mfr_magic[2];
  dst[off++] = __impl::__mfr_magic[3];
  dst[off++] = __impl::__little_endian() ? byte{ 1 } : byte{ 0 };
  dst[off++] = __impl::__mfr_kind<C>();
  dst[off++] = 0;
  dst[off++] = 0;
  if ( max_t r = __impl::frame_element<0>(dst, cap, off, c); r < 0 ) [[unlikely]]
    return r;
  return static_cast<max_t>(off);
}

template<typename C>
max_t
unframe_from(const byte *src, usize len, C &out) noexcept
{
  if ( src == nullptr || len < __impl::__mfr_header ) [[unlikely]]
    return -error::invalid_arg;
  if ( src[0] != __impl::__mfr_magic[0] || src[1] != __impl::__mfr_magic[1] || src[2] != __impl::__mfr_magic[2]
       || src[3] != __impl::__mfr_magic[3] ) [[unlikely]]
    return -error::invalid_arg;
  if ( (src[4] != 0) != __impl::__little_endian() ) [[unlikely]]
    return -error::invalid_arg;      // cross-endian streams are detectable, not readable
  usize off = __impl::__mfr_header;
  if ( max_t r = __impl::unframe_element<0>(src, len, off, out); r < 0 ) [[unlikely]]
    return r;
  return static_cast<max_t>(off);
}

};      // namespace serialize
};      // namespace io
};      // namespace micron
