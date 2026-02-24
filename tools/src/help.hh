#pragma once

#include "io/console.hpp"

void
help(void)
{
  mc::console("duck is a command line build tool for c/cpp/asm projects");
  mc::console("");
  mc::console("Usage:");
  mc::console("");
  mc::console("          duck <command> [arguments]");
  mc::console("");
  mc::console("");
  mc::console("");
  mc::console("The commands are:");
  mc::console("");
  mc::console("build   --       compiles and links project files serially, waiting for each process to finish");
  mc::console("compile --       compiles project files, doesn't wait for them to finish");
  mc::console("debug   --       compiles project files with the debug recipe (-d/-g -w)");
  mc::console("run     --       compiles and runs a project file in place (replacing current process)");
  mc::console("make    --       creates a new project based on a template");
  mc::console("test    --       compiles and runs sources, checking their return");
  mc::console("help    --       prints help (this screen)");
}
