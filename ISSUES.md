# Known Issues (in case you come across these):
- if compiling with aggressive enough optimizations (gcc >Ofast) _and_ if you omit -flto all __attribute__((constructor)) calls will be dropped/culled at link time. You may get global_pointer operator->() null deference throws (will be fixed)
