#include "scheduler.h"
#include "../proc/process.h"
#include "../mm/kmalloc.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

#define TIME_SLICE 10

static tcb_t *ready_queue_head = NULL;
static tcb_t *current_thread = NULL;
static uint32_t scheduler_ticks = 0;

extern void process_update_sleep(void);
extern tcb_t *process_get_idle_thread(void);

void scheduler_init(void) {
    ready_queue_head = NULL;
    scheduler_ticks = 0;
    current_thread = process_get_idle_thread();
    if (current_thread) {
        current_thread->state = THREAD_RUNNING;
    }
}

void scheduler_add(tcb_t *thread) {
    if (!thread) return;

    // If ready queue is empty, initialize single-node circular list
    if (ready_queue_head == NULL) {
        ready_queue_head = thread;
        thread->next = thread;
        return;
    }

    // Check if thread is already in the ready queue
    tcb_t *curr = ready_queue_head;
    do {
        if (curr == thread) {
            return; // Already present
        }
        curr = curr->next;
    } while (curr != ready_queue_head);

    // Find the tail (node pointing back to ready_queue_head)
    tcb_t *tail = ready_queue_head;
    while (tail->next != ready_queue_head) {
        tail = tail->next;
    }

    // Insert at tail
    tail->next = thread;
    thread->next = ready_queue_head;
}

void scheduler_remove(tcb_t *thread) {
    if (!thread || !ready_queue_head) return;

    // Single-node queue
    if (ready_queue_head == thread && thread->next == thread) {
        ready_queue_head = NULL;
        thread->next = NULL;
        return;
    }

    // Find the node before 'thread'
    tcb_t *curr = ready_queue_head;
    tcb_t *prev = NULL;
    do {
        if (curr == thread) {
            break;
        }
        prev = curr;
        curr = curr->next;
    } while (curr != ready_queue_head);

    if (curr != thread) {
        return; // Thread not in queue
    }

    // If removing the head node
    if (curr == ready_queue_head) {
        tcb_t *tail = ready_queue_head;
        while (tail->next != ready_queue_head) {
            tail = tail->next;
        }
        ready_queue_head = ready_queue_head->next;
        tail->next = ready_queue_head;
    } else {
        prev->next = curr->next;
    }

    thread->next = NULL;
}

static void do_schedule(void) {
    tcb_t *next = NULL;
    tcb_t *idle = process_get_idle_thread();

    if (ready_queue_head != NULL) {
        tcb_t *start = NULL;

        // Advance starting search from the next thread after current
        if (current_thread != NULL && current_thread != idle) {
            start = current_thread->next;
        }
        if (!start) {
            start = ready_queue_head;
        }

        tcb_t *curr = start;
        do {
            if (curr->state == THREAD_READY) {
                next = curr;
                break;
            }
            curr = curr->next;
        } while (curr != start);
    }

    // If no ready thread in queue, check if current can continue, otherwise fallback to idle
    if (next == NULL) {
        if (current_thread && (current_thread->state == THREAD_RUNNING || current_thread->state == THREAD_READY)) {
            next = current_thread;
        } else {
            next = idle;
        }
    }

    // Context switch if next thread is different from current
    if (next != NULL && next != current_thread) {
        tcb_t *prev = current_thread;
        if (prev != NULL && prev->state == THREAD_RUNNING) {
            prev->state = THREAD_READY;
        }
        next->state = THREAD_RUNNING;
        current_thread = next;

        if (prev != NULL) {
            context_switch(&prev->esp, next->esp);
        }
    } else if (current_thread != NULL && current_thread->state == THREAD_READY) {
        current_thread->state = THREAD_RUNNING;
    }
}

void scheduler_tick(void) {
    scheduler_ticks++;
    if (current_thread) {
        current_thread->time_used++;
    }

    process_update_sleep();

    if ((scheduler_ticks % TIME_SLICE) == 0) {
        do_schedule();
    }
}

void scheduler_yield(void) {
    do_schedule();
}

tcb_t *scheduler_get_current(void) {
    return current_thread;
}
