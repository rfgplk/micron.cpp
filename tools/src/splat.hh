#pragma once

// duck splat <cmdline>
//
// prints, in raw ascii, the command(s) duck would have issued

namespace splat
{
inline bool __active = false;

inline bool
active()
{
  return __active;
}
};      // namespace splat
