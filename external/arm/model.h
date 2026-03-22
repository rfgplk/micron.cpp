// nanoflagsarm (for C99 and onwards)
// https://github.com/rfgplk/nanoflagsarm
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2024 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "features.h"

#define MX(IMPL, PART) (((unsigned int)(IMPL) << 16) | ((unsigned int)(PART) & 0xFFFF))

__inline__ const char *
model_name(const unsigned char impl, const unsigned short part)
{
  switch ( MX(impl, part) ) {
  /* ARM Ltd. */
  case MX(IMPL_ARM, PART_A53):         return "Cortex-A53";
  case MX(IMPL_ARM, PART_A55):         return "Cortex-A55";
  case MX(IMPL_ARM, PART_A57):         return "Cortex-A57";
  case MX(IMPL_ARM, PART_A72):         return "Cortex-A72";
  case MX(IMPL_ARM, PART_A73):         return "Cortex-A73";
  case MX(IMPL_ARM, PART_A75):         return "Cortex-A75";
  case MX(IMPL_ARM, PART_A76):         return "Cortex-A76";
  case MX(IMPL_ARM, PART_A77):         return "Cortex-A77";
  case MX(IMPL_ARM, PART_A78):         return "Cortex-A78";
  case MX(IMPL_ARM, PART_A78C):        return "Cortex-A78C";
  case MX(IMPL_ARM, PART_A710):        return "Cortex-A710";
  case MX(IMPL_ARM, PART_A715):        return "Cortex-A715";
  case MX(IMPL_ARM, PART_A720):        return "Cortex-A720";
  case MX(IMPL_ARM, PART_X1):          return "Cortex-X1";
  case MX(IMPL_ARM, PART_X2):          return "Cortex-X2";
  case MX(IMPL_ARM, PART_X3):          return "Cortex-X3";
  case MX(IMPL_ARM, PART_X4):          return "Cortex-X4";
  case MX(IMPL_ARM, PART_V1):          return "Neoverse-V1";
  case MX(IMPL_ARM, PART_V2):          return "Neoverse-V2";
  case MX(IMPL_ARM, PART_N1):          return "Neoverse-N1";
  case MX(IMPL_ARM, PART_N2):          return "Neoverse-N2";
  case MX(IMPL_ARM, PART_N3):          return "Neoverse-N3";
  case MX(IMPL_ARM, PART_NEOVERSE_E1): return "Neoverse-E1";

  /* Apple */
  case MX(IMPL_APPLE, PART_APPLE_M1_F): return "Apple M1 Firestorm";
  case MX(IMPL_APPLE, PART_APPLE_M1_P): return "Apple M1 Icestorm";
  case MX(IMPL_APPLE, PART_APPLE_M2_F): return "Apple M2 Avalanche";
  case MX(IMPL_APPLE, PART_APPLE_M2_P): return "Apple M2 Blizzard";

  default: return "Unknown";
  }
}
