/*  Copyright (c) 2026 David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 */

/* The module glue, in C
 *
 * module_init/module_exit/MODULE_LICENSE are macros from <linux/module.h>, and C++ cannot include it
 */

#include <linux/init.h>
#include <linux/module.h>

#include <micron/port/backends/__kport_abi.hpp>

/* implemented in mod_demo.cpp */
int mc_demo_start(void);
void mc_demo_stop(void);

static int __init
micron_demo_init(void)
{
  int rc;

  rc = mc_kport_init();
  if ( rc ) return rc;

  printk(KERN_INFO KBUILD_MODNAME ": demo module loading, kernel PAGE_SIZE=%lu\n", (unsigned long)mc_kport_page_size());

  rc = mc_demo_start();
  if ( rc ) {
    mc_kport_fini();
    return rc;
  }
  return 0;
}

static void __exit
micron_demo_exit(void)
{
  mc_demo_stop();
  printk(KERN_INFO KBUILD_MODNAME ": demo module unloading\n");
  mc_kport_fini();
}

module_init(micron_demo_init);
module_exit(micron_demo_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("David Lucius Severus");
MODULE_DESCRIPTION("micron barebones: containers, hashing and printing inside a kernel module");
