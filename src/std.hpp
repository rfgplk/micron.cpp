//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

/*
                     ..=.*~:.::*....::=~: .~...:::...:.=*
                :~..  :^([}}>*):[~^>>)}(}>#)
##[(((>)[[^ .: ..
            ....*~=)>>}[(^}>#}#%%%%##%##(%#@%#@@@}@@%[}%%))>:~. ~*
          ~. ^*^>)(}#}%%}}[[}%@%%%@%%##@%%#[@@@@%#@%([^)}([~)*^}>..:
        .. )=*(}#[>^%#%}}#%#}@@@#@%@@%%@%@@@%@@@@%@@%@#%@@@%}})(*(~ .
       ..:~^^^}([^^#}^%#%%@@%@@@@@@@@@@@@@@@@@@@@@@@@@@@}@}}}}*(=>^~ .*
      . .>~=^)>^}(#)
#^>[}#@@@@@>=*.:.*.^=)~==.~=(@%}@@%%%@@#@@@%(}##^..:
       .~*%#[%@}###}}(%#@%[.:*** : ....:.:*#[)((=((=.%@@@@%%#@%@(%%(>=.
     :.^~^}([%%%#%@#@%%@}=::.>(~.^**>>(^:}(:(>}*%#.([ *^#@@@@#@@@%(#^(~.:
    :. >^[>((##%%#%#%...:.  ::~:. ... >.:>.. ....= .[#:%%=):~%%@[#[^)=~.
     ::)}~}%[[)(%%@@>..~^^**)~*^=>).=:.% .   .. * :=%. #}#@(@[~@@}#(#==:..
    ..[*=###@@@@@=~..~.... >>=^})}}(:(}::[* ~=:)=~~~~}>(%#[[>[[.%@#}}}>* :
    . =^%#%%@#::.::)~*:::=.~:.=^##%>#(~.^>~^:::.[%%%(((()**(^^) >@@@@%=* :
    :.}((@))>%#(:*~:.:^>: ))~.^[}^^}}(>[()..}}.   .     . ..}~>.)@@@}#>=::
    ~ ))([>#%*..~.=(^):(=~~~=.~~.^*=~(> .:%)       ~=.*#%%@%:=~.@%%}[([).:
    :~(~([#..=...:.:.[)>#}. ^(:.:*...=~:.:%.   .  .>.~%@%@#@%  }@}[#^>)
#~.
    ~ ))[##>..>=^ * :...=..:.:^~~~==.=.[)^% ..  .:: .#@%@%%%@ (%%#}#@[#>..
    : >^>~~..~^ .~)*:)> ~~^ >}).:.~^..^:~(^%  **. ..^@@@%@@%@%@#(#@@#[=(~=
    : *=::... :.(~ :. =>..:=~.: =~... .~^==~#. ~^) ~ @[@@([(#(#@@%#[))()
    . ^  ..~: >.  . ..: .)=(^=}^..>.^:.~=~~=.:@. }# ).@#@@@@@%@@@##@%%#}..
    .~.:.. ....:..~)^^ *(>. .= [:~>.~. ..:~:==~^}: ..%##@%}}#}#[%(@@#### =
    : >()=)):.=.:~*:::>)::~:[ >> ~. .=: .     ...*%:  >^%@@#[%@%@@#)^^*) .
     :*. .>>.#[)>): ::=::~):: : .) ^~     ..  (=~=**%.@.*@@@@(%}[[>#%)~).*
    .  ):>=[}~>((~().=.*:: ^(**.^  (. ~ ~  .:}@%#@@. ^([ @@%@@@#@%^)^)(~ ~
    : :.:~= (=*=^~)~.. >=.(~^[*. .=:.:.  .  .:%%@@@@@. :}@@%((}[#>[>=))^.*
    ::~*~:.~= >=.()~>..:~~=::  .*:  .:)~ ~..~%@%@@%}%@@@%@#}%[#}}(:*>^)..*
    :. .=: .:)*~:[~*(  :.: .  :*=  =*..):: ^..*#})%%%%}##[##[[#^^*)^~(=  *
     . ..^. ^=*(^^:>~  :>.*>^ ^(=~ *^=~..^): ..:#(}[>[%}}#}[)}^~~=:=^^= .
     :  ~.=:..=:=.~. .*.  *...*^ ~>    = ~=~.: ..~>}>}[^)[>}(##%[>(=~) ~.
      ..==.=:~..=~*: . ..:.~:.~.*.:^. ..>^:~>.~ .):*%%^~^>})*~~>>=:.^) .
       . .=.:...::~ .= .^~. ).~:*:)>): =[(~ ..= .:=>~>^=^~=*^*=~:~~*.
        .. *::.*.* .:...==~:...=...>^:.::*> . .~=.:~>[*^=:^^*..>=^. .:
         .. ..:.**=.:: . ~^=: ~^. ~   .:: . .:.=~:.=::=:.*[>==^~*   *
           : . ~.^*=*~..*^=**::~   :~: :~..: : :=.*~.:== ~:.~^:   =
             *    ^>>~:==.=~===:>:~::  ~.:~..=~.===*.:).=>~.   ::
                >=  .. .  . ..   ...              :..   :  .:*
                         ~* .  ~ *^= =:~  ~::(^~:= ~=~
*/

// types FIRST
#include "concepts.hpp"
#include "bits/__posix_types.hpp"
#include "type_traits.hpp"
#include "types.hpp"

// comptime
#include "bits/__arch.hpp"
#include "bits/__ctasserts.hpp"

#include "cmalloc.hpp"
#include "defs.hpp"

#include "alloc.hpp"

#include "match.hpp"
#include "range.hpp"

#include "attributes.hpp"

#include "endian.hpp"

// includes sleeps through sync/pause

// exceptions
#include "errno.hpp"
#include "except.hpp"

#include "version.hpp"

// NOTE: Phase 4 dropped io/__std.hpp, linux/sys/signal.hpp and syscall.hpp from this umbrella.
// A syscall is no longer part of micron's public surface -- micron::port:: is the OS seam, and on
// a kernel or bare-metal backend there is no syscall to expose. Include port/port.hpp for it.

#if defined(__clang__)
#define COMPILER "Clang/LLVM"
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#define COMPILER "Intel"
#elif defined(__GNUC__) || defined(__GNUG__)
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#define GNUCC
#endif
#define COMPILER "GNU"
#if __GNUC__ < 15
#pragma GCC warning "This version of micron was made for GCC 15.x and up"
#endif
#elif defined(_MSC_VER)
#define COMPILER "MSVC"
#else
#define COMPILER "Unknown"
#endif

#if defined(_WIN32) || defined(_WIN64)
#error "The micron standard library wasn't made for Windows."
#endif

// the THIRD independent arch gate. bits/__arch.hpp:57 and bits/__ctasserts.hpp:60 learned about the
// scalar generic tier in Phase 1; this one did not, so MICRON_ALLOW_GENERIC_ARCH still could not get
// through std.hpp. barebones_core.cpp includes the module umbrellas directly, so no gate saw it.
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)     \
    || defined(__micron_arch_generic)
#else
#error "micron: unrecognised architecture. amd64/i386/armv7-a/aarch64 are supported directly; for anything else define MICRON_ALLOW_GENERIC_ARCH to enter the scalar generic tier."
#endif
#if !defined(__GNUC__) && !defined(__clang__)
#error "Only gcc or clang are currently supported compilers. Remove this if you're willing to take risks."
#endif

typedef void (*sig_t)(int);
#define __MICRON_SELF_ASSIGNMENT_GUARD

template<typename T>
  requires micron::is_integral_v<T>
constexpr byte
B(T t)
{
  return (t);
}

template<typename T>
  requires micron::is_integral_v<T>
constexpr byte
KB(T t)
{
  return (t << 10);
}

template<typename T>
  requires micron::is_integral_v<T>
constexpr byte
MB(T t)
{
  return (t << 20);
}

template<typename T>
  requires micron::is_integral_v<T>
constexpr byte
GB(T t)
{
  return (t << 30);
}

namespace mc = micron;
