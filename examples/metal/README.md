# micron on bare metal

`mc::vector`, `mc::hopscotch_map`, `mc::string`, sort, hash and `println` — running on a machine
with no operating system. 

```sh
make                # build the i386 image and run the object gate
make run            # boot it under qemu and grade the result
make ARCH=arm64 run # ... and any of i386 amd64 arm32 arm64 stm32
make all-images     # build + gate all five
```

Expected output of `make run`:

```
micron-metal: image entered, no OS underneath
micron-metal: port::page_size=64 has_paging=false
micron-metal: pool total=15593472 free=15593472
micron-metal: .init_array ran=true
micron-metal: vector n=4096 sorted=true sum=204590517 min=22 max=99985
micron-metal: map n=2048 sum=6288384 expected=6288384 ok=true
micron-metal: string len=652 head_ok=true ok=true
micron-metal: addr_readable stack=true static=true heap=true null_rejected=true
micron-metal: pool free after=7204864 (was 15593472)
micron-metal: ALL CHECKS PASSED
