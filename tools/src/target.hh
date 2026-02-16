#pragma once
#include "string/sstring.hpp"

#include "linux/std.hpp"

using string_type = mc::sstring<4096>;

enum class target_type_t : i32 { object, asm, static_lib, dynamic_lib, binary, __end };

struct target_t {
  target_type_t type;
  string_type objects;
  string_type source_name;
  string_type out_dir;
  string_type out_name;
};
