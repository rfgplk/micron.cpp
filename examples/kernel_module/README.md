# micron in a Linux kernel module

`mc::vector`, `mc::hopscotch_map`, `mc::string`, sort, hash and `println` — running in ring 0.

```sh
make            # build micron_demo.ko against the running kernel
make load       # insmod + dmesg | tail   (needs root)
make unload     # rmmod
```

Needs `kernel-devel` (or `linux-headers`) for the running kernel. Loading an unsigned module needs
Secure Boot **off**, or `CONFIG_MODULE_SIG_FORCE` unset — check with `mokutil --sb-state` and
`cat /sys/kernel/security/lockdown`.

## What it prints

```
micron: hello from ring 0
micron: vector n=4096 sorted=true sum=... min=... max=...
micron: map n=2048 sum=6289408 expected=6289408 ok=true
micron: string len=646 head=micron-barebones
micron: hash64 = ...
micron: cpu=... pid=... tgid=...
micron: ram total=... free=...
micron: mono_ns=...
micron: ALL CHECKS PASSED
```

`mc_demo_start` returns `-EINVAL` if any check fails, so a broken build **refuses to load** rather
than reporting success into dmesg.
