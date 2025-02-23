#pragma once

// call up main()
___attribute__((noreturn)) inline __attribute__((always_inline))
__call_main(int (*main)(int, char **, char **), int argc, char **argv)
{
  _Exit(main(argc, argv));
}

static inline __attribute__((always_inline)) int
_start(){
__call_main();
}
