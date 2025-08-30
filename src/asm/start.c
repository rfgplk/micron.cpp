//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

int main(int argc, char** argv);

// at global ns
int __attribute__((noreturn, used, visibility("default")))
__micron_start(int argc, char **argv)
{
  return main(argc, argv);
}
