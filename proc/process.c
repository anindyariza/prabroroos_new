#include "process.h"
#include "../proc/scheduler.h"
#include "../mm/kmalloc.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

static tcb_t thread_table[MAX_THREADS];
static uint32_t thread_sleep_ticks[MAX_THREADS];
static uint32_t thread_id_counter = 1;
static int process_initialized = 0;

static void *kmemset(void *dest, int val, uint32_t count) {
    uint8_t *d = (uint8_t *)dest;
    for (uint32_t i = 0; i < count; i++) {
        d[i] = (uint8_t)val;
    }
    return dest;
}

static char *kstrncpy(char *dest, const char *src, uint32_t count) {
    uint32_t i = 0;
    if (!dest || !src) return dest;
    for (i = 0; i < count && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < count; i++) {
        dest[i] = '\0';
    }
    return dest;
}

static void idle_thread_func(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}

tcb_t *process_get_idle_thread(void) {
    return &thread_table[0];
}

void process_init(void) {
    kmemset(thread_table, 0, sizeof(thread_table));
    kmemset(thread_sleep_ticks, 0, sizeof(thread_sleep_ticks));
    thread_id_counter = 1;

    // Initialize slot 0 as the idle thread (tid = 0)
    tcb_t *idle = &thread_table[0];
    idle->tid = 0;
    kstrncpy(idle->name, "idle", sizeof(idle->name) - 1);
    idle->name[sizeof(idle->name) - 1] = '\0';
    idle->state = THREAD_READY;
    idle->priority = 0;
    idle->time_slice = 10;
    idle->time_used = 0;
    idle->entry = idle_thread_func;
    idle->next = NULL;

    idle->stack_base = (uint32_t *)kmalloc(THREAD_STACK_SIZE);
    if (idle->stack_base) {
        uint32_t top = ((uint32_t)idle->stack_base + THREAD_STACK_SIZE) & ~0x0F;
        uint32_t *stk = (uint32_t *)top;
        *(--stk) = (uint32_t)thread_exit;     // Fallback return address
        *(--stk) = (uint32_t)idle_thread_func;// Entry point for context_switch ret
        *(--stk) = 0;                         // ebp
        *(--stk) = 0;                         // ebx
        *(--stk) = 0;                         // esi
        *(--stk) = 0;                         // edi
        idle->esp = (uint32_t)stk;
    }

    process_initialized = 1;
}

tcb_t *thread_create(const char *name, void (*entry)(void), uint32_t priority) {
    if (!entry) {
        return NULL;
    }

    int slot = -1;

    // Search for an unallocated slot (slots 1 to MAX_THREADS - 1)
    for (int i = 1; i < MAX_THREADS; i++) {
        if (thread_table[i].stack_base == NULL && thread_table[i].state == THREAD_NEW) {
            slot = i;
            break;
        }
    }

    // If no new slot, check for a terminated thread slot that can be reclaimed
    if (slot == -1) {
        for (int i = 1; i < MAX_THREADS; i++) {
            if (thread_table[i].state == THREAD_TERMINATED) {
                if (thread_table[i].stack_base != NULL) {
                    kfree(thread_table[i].stack_base);
                    thread_table[i].stack_base = NULL;
                }
                kmemset(&thread_table[i], 0, sizeof(tcb_t));
                thread_sleep_ticks[i] = 0;
                slot = i;
                break;
            }
        }
    }

    if (slot == -1) {
        return NULL; // Out of thread slots
    }

    tcb_t *t = &thread_table[slot];
    uint32_t *stack = (uint32_t *)kmalloc(THREAD_STACK_SIZE);
    if (!stack) {
        return NULL;
    }
    kmemset(stack, 0, THREAD_STACK_SIZE);

    t->tid = thread_id_counter++;
    if (name) {
        kstrncpy(t->name, name, sizeof(t->name) - 1);
        t->name[sizeof(t->name) - 1] = '\0';
    } else {
        kstrncpy(t->name, "unnamed", sizeof(t->name) - 1);
        t->name[sizeof(t->name) - 1] = '\0';
    }
    t->state = THREAD_READY;
    t->stack_base = stack;
    t->priority = priority;
    t->time_slice = 10;
    t->time_used = 0;
    t->entry = entry;
    t->next = NULL;
    thread_sleep_ticks[slot] = 0;

    // Pre-populate stack for context switch
    uint32_t top = ((uint32_t)stack + THREAD_STACK_SIZE) & ~0x0F;
    uint32_t *stk = (uint32_t *)top;
    *(--stk) = (uint32_t)thread_exit;     // Return address if entry returns
    *(--stk) = (uint32_t)entry;           // Address popped by context_switch ret
    *(--stk) = 0;                         // ebp
    *(--stk) = 0;                         // ebx
    *(--stk) = 0;                         // esi
    *(--stk) = 0;                         // edi
    t->esp = (uint32_t)stk;

    // Add to scheduler ready queue
    scheduler_add(t);

    return t;
}

void thread_exit(void) {
    tcb_t *curr = thread_get_current();
    if (curr) {
        curr->state = THREAD_TERMINATED;
    }
    scheduler_yield();
    while (1) {
        __asm__ volatile("hlt");
    }
}

void thread_sleep(uint32_t ticks) {
    if (ticks == 0) {
        thread_yield();
        return;
    }

    tcb_t *curr = thread_get_current();
    if (!curr) return;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (&thread_table[i] == curr) {
            thread_sleep_ticks[i] = ticks;
            curr->state = THREAD_WAITING;
            scheduler_yield();
            break;
        }
    }
}

void thread_yield(void) {
    scheduler_yield();
}

const char *thread_state_str(thread_state_t state) {
    switch (state) {
        case THREAD_NEW:        return "NEW";
        case THREAD_READY:      return "READY";
        case THREAD_RUNNING:    return "RUNNING";
        case THREAD_WAITING:    return "WAITING";
        case THREAD_TERMINATED: return "TERMINATED";
        default:                return "UNKNOWN";
    }
}

tcb_t *thread_get_current(void) {
    return scheduler_get_current();
}

void thread_set_state(tcb_t *t, thread_state_t state) {
    if (t) {
        t->state = state;
    }
}

int thread_list(tcb_t **out, int max) {
    if (!out || max <= 0) return 0;
    int count = 0;
    for (int i = 0; i < MAX_THREADS && count < max; i++) {
        if (thread_table[i].stack_base != NULL || (i == 0 && process_initialized)) {
            out[count++] = &thread_table[i];
        }
    }
    return count;
}

void process_update_sleep(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].state == THREAD_WAITING && thread_sleep_ticks[i] > 0) {
            thread_sleep_ticks[i]--;
            if (thread_sleep_ticks[i] == 0) {
                thread_table[i].state = THREAD_READY;
            }
        }
    }
}
