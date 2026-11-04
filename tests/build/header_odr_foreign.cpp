//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// No Micron declarations or definitions: these calls must resolve to the exported replacement family.
extern "C" void *malloc(__SIZE_TYPE__) noexcept;
extern "C" void free(void *) noexcept;
extern "C" int posix_memalign(void **, __SIZE_TYPE__, __SIZE_TYPE__) noexcept;

void *
header_odr_foreign_malloc()
{
  return malloc(96);
}

void
header_odr_foreign_free(void *p)
{
  free(p);
}

unsigned long *
header_odr_foreign_new()
{
  return new unsigned long(0x12345678ul);
}

void
header_odr_foreign_delete(unsigned long *p)
{
  delete p;
}

void *
header_odr_foreign_aligned()
{
  void *p = nullptr;
  return posix_memalign(&p, 256, 512) == 0 ? p : nullptr;
}
