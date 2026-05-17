#ifndef NG_HW_HPP
#define NG_HW_HPP

/*
 * ng_hw.hpp — C-linkage wrapper for the NeoGeo hardware headers.
 *
 * macro.h and neogeo.h were written as plain C headers with no extern "C"
 * guards.  When included directly from a C++ translation unit the compiler
 * assigns C++ linkage (mangled names) to all their function declarations,
 * but the definitions live in neogeolib.c with plain C linkage.  The linker
 * then fails with "undefined reference to 'vram_SCB234(unsigned short, ...)'"
 * (the mangled form) because it can only find the unmangled C symbol.
 *
 * Include this header instead of macro.h / neogeo.h in every .cpp file.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "macro.h"
#include "neogeo.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NG_HW_HPP */
