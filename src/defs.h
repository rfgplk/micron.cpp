#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#define PAGE_SIZE_MICRON sysconf(_SC_PAGESIZE)
#elif defined(_WIN32)
#include <windows.h>
#define PAGE_SIZE GetPageSize()
#else
#error "Unsupported platform."
#endif
