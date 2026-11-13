//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include <micron/io/canonical.hpp>
#include <micron/string/conversions/decimal_simd.hpp>

[[gnu::noinline]] bool
conversion_read8(const char *p, u32 &value)
{
  return micron::__impl::__digits::read8(p, value);
}

[[gnu::noinline]] bool
conversion_read16(const char *p, micron::__impl::__digits::__prefix16 &value)
{
  return micron::__impl::__digits::read16(p, value);
}

[[gnu::noinline]] void
conversion_write8(byte *p, u32 value)
{
  micron::__impl::__digits::write8(p, value);
}

[[gnu::noinline]] void
conversion_ascii(char *out, const byte *p, usize n)
{
  micron::__impl::__digits::ascii(out, p, n);
}

[[gnu::noinline]] max_t
conversion_frame(byte *out, usize capacity, const byte *p, usize n)
{
  return micron::io::serialize::canonical::frame_bytes(out, capacity, p, n);
}

int
main()
{
  return 1;
}
