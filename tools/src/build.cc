#include "linux/process/exec.hpp"

#include "chrono.hpp"

#include "io/console.hpp"

#include "linux/std.hpp"
#include "std.hpp"

#include "batch.hh"

enum modes { debug, optimized };

int
main(int argc, char **argv)
{
  bool infer_binname = false;
  bool less_warnings = false;
  int mode = modes::optimized;
  if ( argc < 2 )
    mc::cerror("Invalid command line arguments, should be build [file_name] [output_name]");

  if ( argc < 3 )
    infer_binname = true;
  for ( u64 i = 0; i < argc; ++i ) {
    if ( mc::strcmp(argv[i], "-nw") == 0 ) {
      less_warnings = true;
    }
    if ( mc::strcmp(argv[i], "-d") == 0 ) {
      mode = modes::debug;
      if ( i == 2 )
        infer_binname = true;
    }
  }
  const string_type bin_dir = "bin";
  const string_type compiler = "/usr/bin/g++";
  const string_type standard = "-std=c++23";

  mc::set_color(mc::color::blue);
  mc::consoled("Building ");
  mc::set_color(mc::color::green);
  mc::consoled(argv[1]);
  mc::set_color(mc::color::blue);
  mc::consoled(" into ");
  mc::set_color(mc::color::green);
  if ( !infer_binname )
    mc::console(bin_dir, "/", argv[2]);
  else
    mc::console(bin_dir);
  mc::set_color(mc::color::yellow);
  auto start = mc::now();

  if ( mode == modes::optimized ) {
    auto command = batch_optimized(argv[1], infer_binname == true ? nullptr : argv[2], compiler, bin_dir, standard,
                                   less_warnings);
    mc::console("with command: ", command);
    mc::set_color(mc::color::reset);
    mc::execute<mc::exec_wait>(compiler, command);
  } else {
    auto command
        = batch_debug(argv[1], infer_binname == true ? nullptr : argv[2], compiler, bin_dir, standard, less_warnings);
    mc::console("with command: ", command);
    mc::set_color(mc::color::reset);
    mc::execute<mc::exec_wait>(compiler, command);
  }

  auto end = mc::now();
  mc::set_color(mc::color::yellow);
  mc::console("Compilation took: ", end - start, " milliseconds");
  mc::set_color(mc::color::reset);
  return 0;
}
