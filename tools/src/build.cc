#include "linux/process/exec.hpp"

#include "chrono.hpp"

#include "io/console.hpp"

#include "linux/std.hpp"
#include "std.hpp"

#include "batch.hh"

int
main(int argc, char **argv)
{
  config_t conf = parse_argv(argc, argv);

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
  if ( !conf.infer_binname )
    mc::console(bin_dir, "/", argv[2]);
  else
    mc::console(bin_dir);
  mc::set_color(mc::color::yellow);
  auto start = mc::now();

  if ( conf.mode == modes::optimized ) {
    auto command = batch_optimized(argv[1], conf.infer_binname == true ? nullptr : argv[2], compiler, bin_dir, standard,
                                   conf.less_warnings);
    mc::console("with command: ", command);
    mc::set_color(mc::color::reset);
    mc::execute<mc::exec_wait>(compiler, command);
  } else {
    auto command = batch_debug(argv[1], conf.infer_binname == true ? nullptr : argv[2], compiler, bin_dir, standard,
                               conf.less_warnings);
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
