#pragma once

#include "memory/cstring.hpp"
#include "std.hpp"

struct config_t {
  bool infer_binname;
  bool less_warning;
  int mode;
};

enum modes { optimized = 0, debug };

config_t
parse_argv(int argc, char **argv)
{
  config_t conf{};
  if ( argc < 2 )
    mc::cerror("Invalid command line arguments, should be build [file_name] [output_name]");

  if ( argc < 3 )
    conf.infer_binname = true;
  for ( u64 i = 0; i < argc; ++i ) {
    if ( mc::strcmp(argv[i], "-nw") == 0 ) {
      conf.less_warnings = true;
    }
    if ( mc::strcmp(argv[i], "-d") == 0 ) {
      conf.mode = modes::debug;
      if ( i == 2 )
        conf.infer_binname = true;
    }
  }
  return conf;
}
