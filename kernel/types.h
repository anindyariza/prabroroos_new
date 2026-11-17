#ifndef _KERNEL_TYPES_H_
#define _KERNEL_TYPES_H_

/*
 * PrabroroOS - Basic type definitions for freestanding x86 (32-bit) kernel
 */

/* Unsigned fixed-width integer types */
typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long     uint64_t;

/* Signed fixed-width integer types */
typedef signed char            int8_t;
typedef signed short           int16_t;
typedef signed int             int32_t;
typedef signed long long       int64_t;

/* Pointer arithmetic and size types for 32-bit x86 architecture */
typedef uint32_t               size_t;
typedef uint32_t               uintptr_t;
typedef int32_t                ptrdiff_t;

/* Null pointer constant */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Boolean type and boolean values */
#ifndef __cplusplus
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
typedef _Bool bool;
#else
typedef uint8_t bool;
#endif
#define true  1
#define false 0
#endif

/* Macro to suppress compiler warnings for unused parameters or variables */
#define UNUSED(x) ((void)(x))

#endif /* _KERNEL_TYPES_H_ */
