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
  int mode = modes::optimized;
  if ( argc < 2 )
    mc::cerror("Invalid command line arguments, should be build [file_name] [output_name]");

  if ( argc == 3 )
    if ( mc::strcmp(argv[2], "-d") == 0 )
      mode = modes::debug;
  if ( argc == 4 )
    if ( mc::strcmp(argv[3], "-d") == 0 )
      mode = modes::debug;

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
  if ( argc > 2 )
    mc::console(bin_dir, "/", argv[2]);
  else
    mc::console(bin_dir);
  mc::set_color(mc::color::yellow);
  auto start = mc::now();

  if ( mode == modes::optimized ) {
    auto command = batch_optimized(argv[1], argv[2], compiler, bin_dir, standard);
    mc::console("with command: ", command);
    mc::set_color(mc::color::reset);
    mc::execute<mc::exec_wait>(compiler, command);
  } else {
    auto command = batch_debug(argv[1], argv[2], compiler, bin_dir, standard);
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

/*
int
main(int argc, char **argv)
{
  int mode = modes::optimized;
  if ( argc < 3 )
    mc::cerror("Invalid command line arguments, should be build [file_name] [output_name]");

  if ( argc == 4 )
    if ( mc::strcmp(argv[3], "-d") == 0 )
      mode = modes::debug;

  // TODO: eventually add all of these compiler flags to a dispatch system
  const string_type bin_dir = "bin";
  const string_type compiler = "/usr/bin/g++";
  const string_type standard = "-std=c++23";
  const string_type optimizations = make_flags(gcc::opt_flags::flags::optimize_fast, gcc::x86_flags::flags::mavx2,
                                               gcc::x86_flags::flags::march_native);
  const string_type debug = "-fsanitize=address -g -march=native";
  const string_type compile_libs = "-lc -lgcc -lgcc_s -lstdc++ -lm";
  const string_type flags_warn_base = "-Wall -Wextra -Wpedantic";
  const string_type flags_warn_extra
      = "-Wno-cpp -Wunused -Wshadow -Wconversion -Wcast-qual -Wconversion-null -Woverlength-strings -Wpointer-arith "
        "-Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla -Wwrite-strings -Wduplicated-cond -Wdouble-promotion "
        "-Wdisabled-optimization -Winline -Wfloat-equal -Wmissing-noreturn -Wpacked -Wnonnull -Wundef -Wtrampolines "
        "-Winline -Winit-self -Wcast-align -Wnarrowing -Wregister -Wmain -Wchanges-meaning -Wsequence-point "
        "-Wattributes";
  const string_type flags_errors_extra = "-Werror=missing-field-initializers -Werror=return-type";
  const string_type flags_warn_ignore = "-Wno-implicit-fallthrough -Wno-sign-conversion -Wno-variadic-macros";
  const string_type flags_extensions
      = "-fstack-protector-strong -fstack-clash-protection -fstrict-overflow -fopenmp "
        "-fext-numeric-literals -ffast-math -flto -fdiagnostics-color=always -fconcepts-diagnostics-depth=2";

  const string_type libs_location = "-L./libs";
  const string_type includes_location = "-Isrc";
  string_type command_pre_opt = make_command(compiler, standard, optimizations, flags_warn_base, flags_warn_extra,
                                             flags_warn_ignore, flags_errors_extra, flags_extensions);
  string_type command_pre_debug = make_command(compiler, standard, debug, flags_warn_base, flags_warn_extra,
                                               flags_warn_ignore, flags_errors_extra, flags_extensions);
  string_type command_post = make_command(compile_libs, includes_location);
  string_type command_release = make_command(command_pre_opt, argv[1], command_post, "-o", bin_dir + "/" + argv[2]);
  string_type command_debug = make_command(command_pre_debug, argv[1], command_post, "-o", bin_dir + "/" + argv[2]);
  mc::set_color(mc::color::blue);
  mc::consoled("Building ");
  mc::set_color(mc::color::green);
  mc::consoled(argv[1]);
  mc::set_color(mc::color::blue);
  mc::consoled(" into ");
  mc::set_color(mc::color::green);
  mc::console(bin_dir, "/", argv[2]);
  mc::set_color(mc::color::yellow);
  auto start = mc::now();
  if ( mode == modes::optimized ) {
    mc::console("with command: ", command_release);
    mc::set_color(mc::color::reset);
    mc::execute<mc::exec_wait>(compiler, command_release);

  } else if ( mode == modes::debug ) {
    mc::console("with command: ", command_debug);
    mc::set_color(mc::color::reset);
    mc::execute<mc::exec_wait>(compiler, command_debug);
  }
  auto end = mc::now();
  mc::set_color(mc::color::yellow);
  mc::console("Compilation took: ", end - start, " milliseconds");
  mc::set_color(mc::color::reset);
  return 0;
}*/
