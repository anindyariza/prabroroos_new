#include "semaphore.h"
#include "../proc/process.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

static inline void atomic_inc(volatile int32_t *ptr) {
    __asm__ volatile("lock incl %0" : "+m"(*ptr) : : "memory");
}

static inline void atomic_dec(volatile int32_t *ptr) {
    __asm__ volatile("lock decl %0" : "+m"(*ptr) : : "memory");
}

static inline int32_t atomic_cmpxchg(volatile int32_t *ptr, int32_t old_val, int32_t new_val) {
    int32_t prev;
    __asm__ volatile(
        "lock cmpxchgl %2, %1"
        : "=a"(prev), "+m"(*ptr)
        : "r"(new_val), "0"(old_val)
        : "memory"
    );
    return prev;
}

void sem_init(semaphore_t *s, const char *name, int32_t initial) {
    if (!s) return;

    s->count = initial;

    if (name) {
        int i = 0;
        for (; i < 31 && name[i] != '\0'; i++) {
            s->name[i] = name[i];
        }
        s->name[i] = '\0';
    } else {
        s->name[0] = '\0';
    }
}

void sem_wait(semaphore_t *s) {
    if (!s) return;

    while (1) {
        while (s->count <= 0) {
            thread_yield();
        }

        int32_t current = s->count;
        if (current > 0) {
            if (atomic_cmpxchg(&s->count, current, current - 1) == current) {
                return;
            }
        }
    }
}

void sem_post(semaphore_t *s) {
    if (!s) return;
    atomic_inc(&s->count);
}

int sem_trywait(semaphore_t *s) {
    if (!s) return 0;

    while (1) {
        int32_t current = s->count;
        if (current <= 0) {
            return 0;
        }

        if (atomic_cmpxchg(&s->count, current, current - 1) == current) {
            return 1;
        }
    }
}
