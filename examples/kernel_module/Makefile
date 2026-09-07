# micron kernel-module demo
#
#   make            build micron_demo.ko against the running kernel
#   make check      assert the .ko can actually be loaded (no duplicate section names)
#
#   rmmod micron_demo 2>/dev/null; insmod micron_demo.ko; echo "rc=$$?"; \
#     dmesg | grep micron_demo | tail -20; rmmod micron_demo
#
# Needs kernel-devel (or linux-headers) for the running kernel. Loading an unsigned module needs
# Secure Boot off, or CONFIG_MODULE_SIG_FORCE unset.

KVER    ?= $(shell uname -r)
KDIR    ?= /lib/modules/$(KVER)/build
PWD     := $(shell pwd)
MICRON  := $(PWD)/../..

# mc_kport.c and mc_libgcc.cpp live in the crt directory
LINKED := mc_kport.c mc_libgcc.cpp

all: $(LINKED)
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	sh $(PWD)/check_sections.sh micron_demo.ko

mc_kport.c:
	ln -sf $(MICRON)/start/kernel/mc_kport.c $@

mc_libgcc.cpp:
	ln -sf $(MICRON)/start/kernel/mc_libgcc.cpp $@

check:
	sh $(PWD)/check_sections.sh micron_demo.ko

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(LINKED)

load: all
	-sudo rmmod micron_demo 2>/dev/null
	sudo insmod micron_demo.ko
	sudo dmesg | grep micron_demo | tail -20

unload:
	sudo rmmod micron_demo

.PHONY: all check clean load unload
