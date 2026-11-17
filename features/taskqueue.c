/*
 * PrabroroOS — EDF Priority Task Queue
 * Sorted linked list: ascending remaining_seconds (Earliest Deadline First)
 * Protected by mutex for thread-safe access from PIT callback + CLI.
 */

#include "taskqueue.h"
#include "../mm/kmalloc.h"
#include "../sync/mutex.h"
#include "../drivers/vga.h"
#include "../kernel/types.h"

/* ---- Internal helpers (no stdlib) ------------------------------------ */
static size_t tq_strlen(const char *s) {
    size_t n = 0;
    while (s && s[n]) n++;
    return n;
}
static void tq_strcpy(char *dst, const char *src, size_t max) {
    size_t i = 0;
    while (i + 1 < max && src && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}
static void tq_memset(void *p, uint8_t v, size_t n) {
    uint8_t *b = (uint8_t *)p;
    while (n--) *b++ = v;
}
static void tq_print_uint(uint32_t v) {
    if (v == 0) { vga_putchar('0'); return; }
    char buf[12]; int i = 0;
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) vga_putchar(buf[i]);
}
static void tq_print_padded(uint32_t v, int width) {
    /* right-align in 'width' chars */
    char buf[12]; int len = 0;
    if (v == 0) { buf[len++] = '0'; }
    else { uint32_t x = v; while (x) { buf[len++] = '0' + (x%10); x/=10; } }
    int pad = width - len;
    while (pad-- > 0) vga_putchar(' ');
    for (int i = len-1; i >= 0; i--) vga_putchar(buf[i]);
}

/* ---- State ----------------------------------------------------------- */
static task_entry_t *tq_head   = NULL;
static mutex_t       tq_mutex;
static uint32_t      tq_id_ctr = 1;
static uint32_t      tq_tick_acc = 0;  /* accumulate PIT ticks → seconds */

/* ---- Init ------------------------------------------------------------ */
void taskqueue_init(void) {
    tq_head    = NULL;
    tq_id_ctr  = 1;
    tq_tick_acc = 0;
    mutex_init(&tq_mutex, "taskqueue");
}

/* ---- Count ----------------------------------------------------------- */
int taskqueue_count(void) {
    int n = 0;
    task_entry_t *cur = tq_head;
    while (cur) { n++; cur = cur->next; }
    return n;
}

/* ---- Add (EDF sorted insert) ---------------------------------------- */
void taskqueue_add(const char *matkul, uint32_t days, uint32_t hours,
                   uint32_t mins, const char *keterangan) {
    /* Validate */
    if (!matkul || tq_strlen(matkul) == 0) {
        vga_println_colored("[ERROR] Mata kuliah tidak boleh kosong.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    if (!keterangan || tq_strlen(keterangan) == 0) {
        vga_println_colored("[ERROR] Keterangan tidak boleh kosong.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    if (hours > 23 || mins > 59) {
        vga_println_colored("[ERROR] Format waktu tidak valid (H: 0-23, M: 0-59).", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    uint32_t total = days * 86400 + hours * 3600 + mins * 60;
    if (total == 0) {
        vga_println_colored("[ERROR] Waktu selesai harus lebih dari 0.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    task_entry_t *node = (task_entry_t *)kmalloc(sizeof(task_entry_t));
    if (!node) {
        vga_println_colored("[ERROR] Out of Memory — tidak dapat menambah tugas.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    tq_memset(node, 0, sizeof(task_entry_t));
    node->task_id          = tq_id_ctr++;
    node->total_seconds    = total;
    node->remaining_seconds = total;
    tq_strcpy(node->matkul,     matkul,     TASKQUEUE_MAX_MATKUL);
    tq_strcpy(node->keterangan, keterangan, TASKQUEUE_MAX_KETERANGAN);
    node->next = NULL;

    mutex_lock(&tq_mutex);

    /* EDF sorted insert: ascending remaining_seconds */
    if (!tq_head || node->remaining_seconds <= tq_head->remaining_seconds) {
        node->next = tq_head;
        tq_head = node;
    } else {
        task_entry_t *cur = tq_head;
        while (cur->next && cur->next->remaining_seconds <= node->remaining_seconds)
            cur = cur->next;
        node->next = cur->next;
        cur->next  = node;
    }

    mutex_unlock(&tq_mutex);

    vga_print_colored("[OK] Tugas '", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print(matkul);
    vga_print("' ditambahkan (ID=");
    tq_print_uint(node->task_id);
    vga_println(")");
}

/* ---- Print all tasks ------------------------------------------------- */
void taskqueue_print_all(void) {
    mutex_lock(&tq_mutex);

    if (!tq_head) {
        vga_println_colored("  (Antrean kosong)", VGA_YELLOW, VGA_BLACK);
        mutex_unlock(&tq_mutex);
        return;
    }

    vga_println_colored("┌────┬──────────────────────┬─────────────────┬──────────────────────────────┐",
                        VGA_CYAN, VGA_BLACK);
    vga_println_colored("│ No │ Mata Kuliah           │ Sisa Waktu      │ Keterangan                   │",
                        VGA_CYAN, VGA_BLACK);
    vga_println_colored("├────┼──────────────────────┼─────────────────┼──────────────────────────────┤",
                        VGA_CYAN, VGA_BLACK);

    int num = 1;
    task_entry_t *cur = tq_head;
    while (cur) {
        uint32_t rem = cur->remaining_seconds;
        uint32_t d   = rem / 86400;
        uint32_t h   = (rem % 86400) / 3600;
        uint32_t m   = (rem % 3600)  / 60;

        vga_print("│");
        tq_print_padded(num, 3);
        vga_print(" │ ");
        /* Matkul - 20 chars */
        const char *mk = cur->matkul;
        int mklen = 0;
        while (mk[mklen] && mklen < 20) { vga_putchar(mk[mklen]); mklen++; }
        while (mklen++ < 20) vga_putchar(' ');
        vga_print(" │ ");
        /* Remaining: Dd Hh Mm */
        tq_print_uint(d); vga_print("d ");
        tq_print_uint(h); vga_print("h ");
        tq_print_uint(m); vga_print("m    │ ");
        /* Keterangan - 28 chars */
        const char *kt = cur->keterangan;
        int ktlen = 0;
        while (kt[ktlen] && ktlen < 28) { vga_putchar(kt[ktlen]); ktlen++; }
        while (ktlen++ < 28) vga_putchar(' ');
        vga_println(" │");

        cur = cur->next;
        num++;
    }
    vga_println_colored("└────┴──────────────────────┴─────────────────┴──────────────────────────────┘",
                        VGA_CYAN, VGA_BLACK);

    mutex_unlock(&tq_mutex);
}

/* ---- Complete task by 1-indexed number ------------------------------- */
void taskqueue_complete(uint32_t task_num) {
    if (task_num == 0) {
        vga_println_colored("[ERROR] Nomor tugas harus >= 1.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    mutex_lock(&tq_mutex);

    task_entry_t *prev = NULL;
    task_entry_t *cur  = tq_head;
    uint32_t idx = 1;

    while (cur && idx < task_num) {
        prev = cur;
        cur  = cur->next;
        idx++;
    }

    if (!cur) {
        mutex_unlock(&tq_mutex);
        vga_print_colored("[ERROR] Tugas nomor ", VGA_LIGHT_RED, VGA_BLACK);
        tq_print_uint(task_num);
        vga_println(" tidak ditemukan.");
        return;
    }

    /* Remove from list */
    if (prev) prev->next = cur->next;
    else       tq_head   = cur->next;

    mutex_unlock(&tq_mutex);

    vga_print_colored("[✓] Tugas '", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print(cur->matkul);
    vga_println("' diselesaikan dan dihapus dari antrean.");

    kfree(cur);
}

/* ---- Tick: called every 100 PIT ticks (= 1 second at 100Hz) ---------- */
void taskqueue_tick(void) {
    if (!tq_head) return;

    mutex_lock(&tq_mutex);

    if (tq_head->remaining_seconds > 0) {
        tq_head->remaining_seconds--;
    }

    if (tq_head->remaining_seconds == 0) {
        /* Auto-expire the head task */
        task_entry_t *expired = tq_head;
        tq_head = expired->next;
        mutex_unlock(&tq_mutex);

        vga_print_colored("\n[WAKTU HABIS] Tugas '", VGA_YELLOW, VGA_BLACK);
        vga_print(expired->matkul);
        vga_println("' telah kedaluwarsa dan dihapus.");
        kfree(expired);
        return;
    }

    mutex_unlock(&tq_mutex);
}
