#pragma once

#if defined(__GNUC__)
  #define GCC_VERSION_FULL (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
  #define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
#endif

#if !defined(__cplusplus) 
#pragma GCC error "Micron was made for C++, please compile with an appropriate compiler"
#endif

#if defined(__cplusplus) && __cplusplus <= 202300L
#pragma GCC error "Micron _requires_ full support for C++23 features, such as multidimensional subscript operators, constexpr and lambda changes). Please use a compliant compiler"
#endif
