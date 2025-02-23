#pragma once

#include "../mutex/token.hpp"
#include "../types.hpp"

#include <type_traits>

namespace micron
{
enum class states {
  RED, YELLOW, GREEN
};

enum class priority {
  low, regular, high, realtime
};

struct permit{};

// synchronization mechanism across threads
class semaphore
{
  micron::token tk;
public:

};
};     // namespace micron
