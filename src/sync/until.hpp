#pragma once

#include <type_traits>

namespace micron {
namespace sync {
template <typename F, typename... Args>
requires std::is_function_v<F>
void until(F f, typename... args)

};
};
