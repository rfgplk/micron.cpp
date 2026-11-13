// Copyright (c) 2026 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#ifndef MICRON_BB_MIN_BLOCK
#define MICRON_BB_MIN_BLOCK 16
#endif
#ifndef MICRON_BB_MAX_REGION_BYTES
#define MICRON_BB_MAX_REGION_BYTES (256u << 10)
#endif
#ifndef MICRON_BB_MAX_REGIONS
#define MICRON_BB_MAX_REGIONS 1
#endif
#ifndef MICRON_BB_CLASS_SMALL
#define MICRON_BB_CLASS_SMALL 256
#endif
#ifndef MICRON_BB_SHEET
#define MICRON_BB_SHEET (4u << 10)
#endif
#ifndef MICRON_BB_MAX_SHEETS
#define MICRON_BB_MAX_SHEETS 4
#endif
#ifndef MICRON_BB_SL_LOG2
#define MICRON_BB_SL_LOG2 2
#endif
#ifndef MICRON_BB_MAX_ALIGN
#define MICRON_BB_MAX_ALIGN 64
#endif
#ifndef MICRON_BB_CACHE_CAP
#define MICRON_BB_CACHE_CAP 0
#endif
#ifndef MICRON_BB_CACHE_MAX_ORDER
#define MICRON_BB_CACHE_MAX_ORDER 0
#endif
