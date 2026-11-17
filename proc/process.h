#pragma once
#include "../kernel/types.h"

#define MAX_THREADS      16
#define THREAD_STACK_SIZE 8192  // 8KB per thread

typedef enum {
    THREAD_NEW = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_WAITING,
    THREAD_TERMINATED
} thread_state_t;

typedef struct tcb {
    uint32_t tid;                    // Thread ID
    char name[32];                   // Thread name
    thread_state_t state;            // Current state
    uint32_t esp;                    // Saved stack pointer (MUST be first for asm)
    uint32_t *stack_base;            // Base of allocated stack
    uint32_t priority;               // 0=lowest, 255=highest
    uint32_t time_slice;             // Time slice in ticks
    uint32_t time_used;              // Total ticks used
    void (*entry)(void);             // Entry function
    struct tcb *next;                // Linked list
} tcb_t;

void process_init(void);
tcb_t *thread_create(const char *name, void (*entry)(void), uint32_t priority);
void thread_exit(void);
void thread_sleep(uint32_t ticks);
void thread_yield(void);
const char *thread_state_str(thread_state_t state);
tcb_t *thread_get_current(void);
void thread_set_state(tcb_t *t, thread_state_t state);
int thread_list(tcb_t **out, int max); // fills out[] with all TCBs, returns count
tcb_t *process_get_idle_thread(void);
