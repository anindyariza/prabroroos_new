#pragma once
#include "../kernel/types.h"
#include "../proc/process.h"

typedef struct {
    volatile uint32_t locked;   // 0=free, 1=locked
    tcb_t *owner;               // thread holding the lock
    uint32_t mutex_id;          // for deadlock ordering
    char name[32];
} mutex_t;

void mutex_init(mutex_t *m, const char *name);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);
int mutex_trylock(mutex_t *m); // returns 1 if acquired, 0 if busy
