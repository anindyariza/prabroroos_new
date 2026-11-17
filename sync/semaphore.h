#pragma once
#include "../kernel/types.h"

typedef struct {
    volatile int32_t count;
    char name[32];
} semaphore_t;

void sem_init(semaphore_t *s, const char *name, int32_t initial);
void sem_wait(semaphore_t *s);    // P() - decrement, block if <0 (spinwait)
void sem_post(semaphore_t *s);    // V() - increment
int sem_trywait(semaphore_t *s);  // non-blocking, returns 1 if acquired
