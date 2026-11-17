#include "mutex.h"
#include "../proc/process.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

static volatile uint32_t next_mutex_id = 1;

static inline uint32_t atomic_xchg(volatile uint32_t *ptr, uint32_t val) {
    uint32_t ret;
    __asm__ volatile("xchgl %0, %1" : "+m"(*ptr), "=a"(ret) : "1"(val) : "memory");
    return ret;
}

void mutex_init(mutex_t *m, const char *name) {
    if (!m) return;

    m->locked = 0;
    m->owner = NULL;
    m->mutex_id = next_mutex_id++;

    if (name) {
        int i = 0;
        for (; i < 31 && name[i] != '\0'; i++) {
            m->name[i] = name[i];
        }
        m->name[i] = '\0';
    } else {
        m->name[0] = '\0';
    }
}

void mutex_lock(mutex_t *m) {
    if (!m) return;

    tcb_t *self = thread_get_current();
    if (m->owner == self && self != NULL) {
        return; // Prevent self-deadlock if recursively called
    }

    while (atomic_xchg(&m->locked, 1) != 0) {
        thread_yield();
    }

    m->owner = self;
}

void mutex_unlock(mutex_t *m) {
    if (!m) return;

    tcb_t *self = thread_get_current();
    if (m->owner != NULL && m->owner != self) {
        return; // Only current lock holder can release
    }

    m->owner = NULL;
    atomic_xchg(&m->locked, 0);
}

int mutex_trylock(mutex_t *m) {
    if (!m) return 0;

    tcb_t *self = thread_get_current();
    if (atomic_xchg(&m->locked, 1) == 0) {
        m->owner = self;
        return 1;
    }

    return 0;
}
