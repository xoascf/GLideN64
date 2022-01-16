#pragma once

#include <stdint.h>

#if UINTPTR_MAX == 0xffffffff
#elif UINTPTR_MAX == 0xffffffffffffffff
#define IS_64_BIT 1
#else
/* wtf */
#endif

#ifdef NATIVE
#ifdef IS_64_BIT
#define word uint64_t
#else
#define word uint32_t
#endif
#else
#define word uint32_t
#endif
