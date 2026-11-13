//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
int
main(int argc, char **argv)
{
  return argc == 0 && argv && !argv[0] ? 1 : 6;
}
