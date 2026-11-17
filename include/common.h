#ifndef _INCLUDE_COMMON_H_
#define _INCLUDE_COMMON_H_

#include "../kernel/types.h"

/*
 * Alignment and utility macros
 */
#define ALIGN(x, a)         (((x) + ((a) - 1)) & ~((a) - 1))
#define MIN(a, b)           (((a) < (b)) ? (a) : (b))
#define MAX(a, b)           (((a) > (b)) ? (a) : (b))
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define PACKED              __attribute__((packed))

/*
 * x86 I/O port operations
 */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * I/O wait operation (delay for slow hardware / PIC bus)
 * Writes to unused POST port 0x80
 */
static inline void io_wait(void) {
    outb(0x80, 0);
}

/*
 * Barrier macros and CPU instructions
 */
#define barrier()           __asm__ volatile ("" ::: "memory")
#define memory_barrier()    __asm__ volatile ("lock; addl $0, 0(%%esp)" ::: "memory")
#define mb()                barrier()
#define rmb()               barrier()
#define wmb()               barrier()

#define cli()               __asm__ volatile ("cli" ::: "memory")
#define sti()               __asm__ volatile ("sti" ::: "memory")
#define hlt()               __asm__ volatile ("hlt")

#endif /* _INCLUDE_COMMON_H_ */
