#pragma once

#if defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "arch/bitwise_arm64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm32)
#include "arch/bitwise_arm32.hpp"
#elif defined(__micron_arch_x86) || defined(__micron_arch_amd64)
#include "arch/bitwise_amd64.hpp"
#endif
