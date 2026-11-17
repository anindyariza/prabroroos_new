#pragma once
#include "../kernel/types.h"

#define TASKQUEUE_MAX_MATKUL   64
#define TASKQUEUE_MAX_KETERANGAN 256

typedef struct task_entry {
    uint32_t task_id;
    char     matkul[TASKQUEUE_MAX_MATKUL];
    char     keterangan[TASKQUEUE_MAX_KETERANGAN];
    uint32_t total_seconds;      /* initial deadline in seconds */
    uint32_t remaining_seconds;  /* countdown - EDF key */
    struct task_entry *next;
} task_entry_t;

void     taskqueue_init(void);
void     taskqueue_add(const char *matkul, uint32_t days, uint32_t hours,
                       uint32_t mins, const char *keterangan);
void     taskqueue_print_all(void);
void     taskqueue_complete(uint32_t task_num); /* 1-indexed */
void     taskqueue_tick(void);  /* called from PIT callback every 100 ticks (1 sec) */
int      taskqueue_count(void);
