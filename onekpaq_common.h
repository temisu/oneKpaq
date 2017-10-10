/* Copyright (C) Teemu Suutari */

#ifndef ONEKPAQ_COMMON_H
#define ONEKPAQ_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

// move along, nothing but hacks here

#include <stdint.h>

typedef uint8_t u8;

#ifndef LATURI_INTEGRATION
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint_fast32_t uint;
typedef uintptr_t ulong;

extern void DebugPrint(const char *str,...);
extern void DebugPrintAndDie(const char *str,...);

#define TICK() DebugPrint(".")
#define LAST_TICK() DebugPrint(".\n")

#define INFO(str,...) DebugPrint("I " str "\n",## __VA_ARGS__)
#define ABORT(str,...) DebugPrintAndDie("E " str "\n",## __VA_ARGS__)
#ifdef DEBUG_BUILD
#define DEBUG(str,...) DebugPrint("D " str "\n",## __VA_ARGS__)
#else
#define DEBUG(str,...) do {} while (false)
#endif
#define ASSERT(x,str,...) ({if (!(x)) ABORT("[Assert failure] " str,## __VA_ARGS__);})

#endif

#ifdef __cplusplus
}
#endif

#endif
