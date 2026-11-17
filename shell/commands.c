/*
 * PrabroroOS — shell/commands.c
 * All CLI command implementations
 */
#include "commands.h"
#include "shell.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../mm/kmalloc.h"
#include "../mm/meminfo.h"
#include "../proc/process.h"
#include "../proc/scheduler.h"
#include "../fs/vfs.h"
#include "../security/acl.h"
#include "../features/taskqueue.h"
#include "../features/memmonitor.h"
#include "../sync/mutex.h"
#include "../include/common.h"
#include "../kernel/types.h"

/* ---- Internal string helpers ---------------------------------------- */
static size_t cmd_strlen(const char *s) {
    size_t n = 0; while (s && s[n]) n++; return n;
}
static int cmd_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
static void cmd_strcpy(char *d, const char *s) {
    while (*s) *d++ = *s++;
    *d = '\0';
}
static void cmd_print_uint(uint32_t v) {
    if (!v) { vga_putchar('0'); return; }
    char buf[12]; int i=0;
    while (v) { buf[i++]='0'+(v%10); v/=10; }
    while (i--) vga_putchar(buf[i]);
}
/* Skip leading whitespace, return pointer to first non-space */
static const char *cmd_skip(const char *s) {
    while (s && (*s == ' ' || *s == '\t')) s++;
    return s;
}
/* Parse a decimal number from string; return pointer past the number */
static const char *cmd_atoi(const char *s, int *out) {
    *out = 0;
    s = cmd_skip(s);
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') { *out = *out * 10 + (*s - '0'); s++; }
    if (neg) *out = -(*out);
    return s;
}

/* ------------------------------------------------------------------ */
void cmd_help(const char *args) {
    (void)args;
    vga_println_colored("╔════════════════════════════════════════════════════════╗", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_println_colored("║               PrabroroOS — Perintah CLI               ║", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_println_colored("╠═══════════════════════╦════════════════════════════════╣", VGA_LIGHT_CYAN, VGA_BLACK);

    struct { const char *cmd; const char *desc; } cmds[] = {
        { "help",          "Tampilkan daftar perintah ini" },
        { "clear",         "Bersihkan layar" },
        { "uname",         "Info versi PrabroroOS" },
        { "ps",            "Daftar thread kernel aktif" },
        { "memori",        "Monitor penggunaan memori" },
        { "whoami",        "Tampilkan role saat ini" },
        { "su admin",      "Login sebagai Admin (password)" },
        { "su user",       "Logout kembali ke User" },
        { "test_sync",     "Demo race condition vs mutex [Admin]" },
        { "test_deadlock", "Demo deadlock & solusi [Admin]" },
        { "ls",            "Daftar file di VFS/RAMDisk" },
        { "create <f>",    "Buat file baru" },
        { "write <f> <isi>","Tulis ke file" },
        { "read <f>",      "Baca isi file" },
        { "delete <f>",    "Hapus file [Admin]" },
        { "tambah tugas",  "Tambah tugas ke Priority Queue" },
        { "semua antrean", "Tampilkan semua tugas aktif" },
        { "selesai <n>",   "Selesaikan tugas ke-n" },
        { "shutdown",      "Matikan sistem [Admin]" },
        { "reboot",        "Restart sistem [Admin]" },
    };
    for (size_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++) {
        vga_print_colored("║ ", VGA_LIGHT_CYAN, VGA_BLACK);
        const char *c = cmds[i].cmd;
        int clen = 0;
        while (c[clen]) { vga_putchar(c[clen]); clen++; }
        while (clen++ < 21) vga_putchar(' ');
        vga_print_colored("║ ", VGA_LIGHT_CYAN, VGA_BLACK);
        vga_print(cmds[i].desc);
        int dlen = (int)cmd_strlen(cmds[i].desc);
        while (dlen++ < 30) vga_putchar(' ');
        vga_println_colored(" ║", VGA_LIGHT_CYAN, VGA_BLACK);
    }
    vga_println_colored("╚═══════════════════════╩════════════════════════════════╝", VGA_LIGHT_CYAN, VGA_BLACK);
}

void cmd_clear(const char *args) {
    (void)args;
    vga_clear();
}

void cmd_uname(const char *args) {
    (void)args;
    vga_println_colored("PrabroroOS v1.0 — Real x86 32-bit Kernel", VGA_YELLOW, VGA_BLACK);
    vga_println("Architecture : i386 Protected Mode (32-bit)");
    vga_println("Bootloader   : GRUB2 Multiboot1");
    vga_println("Scheduler    : Preemptive Round Robin (PIT 100Hz)");
    vga_println("Memory       : Dynamic Heap Allocator (First-Fit)");
    vga_println("Filesystem   : RAMDisk VFS (32 files × 4KB)");
    vga_println("Build date   : 2026-11-17");
}

void cmd_ps(const char *args) {
    (void)args;
    tcb_t *list[MAX_THREADS];
    int count = thread_list(list, MAX_THREADS);

    vga_println_colored("┌──────┬──────────────────┬─────────────┬──────────┐", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_println_colored("│  TID │ Name             │ State       │ Priority │", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_println_colored("├──────┼──────────────────┼─────────────┼──────────┤", VGA_LIGHT_CYAN, VGA_BLACK);

    for (int i = 0; i < count; i++) {
        vga_print("│  ");
        cmd_print_uint(list[i]->tid);
        vga_print("   │ ");
        int nlen = 0;
        const char *nm = list[i]->name;
        while (nm[nlen] && nlen < 16) { vga_putchar(nm[nlen]); nlen++; }
        while (nlen++ < 16) vga_putchar(' ');
        vga_print(" │ ");
        const char *st = thread_state_str(list[i]->state);
        int slen = 0;
        while (st[slen] && slen < 11) { vga_putchar(st[slen]); slen++; }
        while (slen++ < 11) vga_putchar(' ');
        vga_print(" │    ");
        cmd_print_uint(list[i]->priority);
        vga_println("     │");
    }
    vga_println_colored("└──────┴──────────────────┴─────────────┴──────────┘", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("  "); cmd_print_uint(count); vga_println(" thread(s) aktif.");
}

void cmd_meminfo(const char *args) {
    (void)args;
    memmonitor_print();
}

/* ---- Sync demo ------------------------------------------------------- */
static volatile uint32_t shared_counter = 0;
static mutex_t sync_mutex;
static int sync_mutex_inited = 0;

static void demo_increment_unsafe(void) {
    /* Simulate: read, delay, write (race condition) */
    for (int i = 0; i < 1000; i++) {
        uint32_t tmp = shared_counter;
        /* Artificial interleave point */
        for (volatile int d = 0; d < 10; d++) {}
        shared_counter = tmp + 1;
    }
}

static void demo_increment_safe(void) {
    for (int i = 0; i < 1000; i++) {
        mutex_lock(&sync_mutex);
        shared_counter++;
        mutex_unlock(&sync_mutex);
    }
}

void cmd_test_sync(const char *args) {
    (void)args;
    if (!acl_check(ROLE_ADMIN)) {
        vga_println_colored("[AKSES DITOLAK] Perintah ini hanya untuk Admin.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    vga_println_colored("=== Demo Sinkronisasi & Race Condition ===", VGA_YELLOW, VGA_BLACK);
    vga_println("");

    if (!sync_mutex_inited) {
        mutex_init(&sync_mutex, "sync_demo");
        sync_mutex_inited = 1;
    }

    /* Demo 1: Tanpa mutex (race condition) */
    vga_println_colored("--- Test 1: Tanpa Mutex (Race Condition) ---", VGA_LIGHT_RED, VGA_BLACK);
    vga_println("  2 thread berbagi counter, increment 1000x masing-masing.");
    vga_println("  Expected: 2000 | Actual (dengan race): kemungkinan < 2000");
    shared_counter = 0;
    demo_increment_unsafe();
    demo_increment_unsafe();   /* Sequential simulation of 2 threads */
    vga_print("  Hasil counter (simulasi sequential): ");
    cmd_print_uint(shared_counter);
    vga_println("");
    vga_println_colored("  CATATAN: Pada multithreading nyata, nilai < 2000 membuktikan race condition.", VGA_LIGHT_RED, VGA_BLACK);
    vga_println("");

    /* Demo 2: Dengan mutex (konsisten) */
    vga_println_colored("--- Test 2: Dengan Mutex (Thread-Safe) ---", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_println("  Mutex melindungi operasi increment.");
    vga_println("  Expected: 2000 | Actual: selalu 2000 (atomic increment)");
    shared_counter = 0;
    demo_increment_safe();
    demo_increment_safe();
    vga_print("  Hasil counter (mutex-protected): ");
    cmd_print_uint(shared_counter);
    vga_println(" ✓");
    vga_println("");
    vga_println_colored("  KESIMPULAN: Mutex menjamin konsistensi shared data.", VGA_LIGHT_GREEN, VGA_BLACK);
}

/* ---- Deadlock demo -------------------------------------------------- */
void cmd_test_deadlock(const char *args) {
    (void)args;
    if (!acl_check(ROLE_ADMIN)) {
        vga_println_colored("[AKSES DITOLAK] Perintah ini hanya untuk Admin.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    vga_println_colored("=== Demo Deadlock & Pencegahan ===", VGA_YELLOW, VGA_BLACK);
    vga_println("");

    vga_println_colored("--- Skenario Deadlock ---", VGA_LIGHT_RED, VGA_BLACK);
    vga_println("  Thread A: lock(mutex1) -> lock(mutex2)");
    vga_println("  Thread B: lock(mutex2) -> lock(mutex1)");
    vga_println("");
    vga_println("  [Thread A] Mengunci mutex1... OK");
    vga_println("  [Thread B] Mengunci mutex2... OK");
    vga_println("  [Thread A] Mencoba mutex2... BLOCKED (Thread B pegang)");
    vga_println("  [Thread B] Mencoba mutex1... BLOCKED (Thread A pegang)");
    vga_println("  >>> DEADLOCK! Kedua thread saling menunggu selamanya. <<<");
    vga_println("");

    vga_println_colored("--- Deteksi Deadlock (Timeout) ---", VGA_YELLOW, VGA_BLACK);
    vga_println("  Setelah N iterasi spin tanpa dapat lock:");
    vga_println("  Thread melapor timeout dan melepas semua kunci yang dipegang.");
    vga_println("  Sistem kembali beroperasi normal.");
    vga_println("");

    vga_println_colored("--- Solusi: Lock Ordering (Pencegahan Circular Wait) ---", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_println("  Aturan: Selalu ambil mutex dengan ID lebih kecil dahulu.");
    vga_println("  Thread A: lock(mutex1) -> lock(mutex2)  [ID 1 < 2]");
    vga_println("  Thread B: lock(mutex1) -> lock(mutex2)  [ID 1 < 2, BUKAN mutex2 dulu]");
    vga_println("");
    vga_println("  [Thread A] Mengunci mutex1 (ID=1)... OK");
    vga_println("  [Thread B] Mencoba mutex1 (ID=1)... BLOCKED (menunggu)");
    vga_println("  [Thread A] Mengunci mutex2 (ID=2)... OK");
    vga_println("  [Thread A] Selesai, melepas mutex2 dan mutex1.");
    vga_println("  [Thread B] Mendapat mutex1... OK -> mutex2... OK");
    vga_println_colored("  >>> BEBAS DEADLOCK! Lock ordering mencegah circular wait. <<<", VGA_LIGHT_GREEN, VGA_BLACK);
}

/* ---- FS commands ----------------------------------------------------- */
void cmd_ls(const char *args) {
    (void)args;
    vfs_ls();
}

void cmd_create(const char *args) {
    args = cmd_skip(args);
    if (!args || !*args) {
        vga_println_colored("[ERROR] Usage: create <nama_file>", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    int role = acl_get_role();
    vfs_create(args, (uint8_t)role);
}

void cmd_write_file(const char *args) {
    args = cmd_skip(args);
    if (!args || !*args) {
        vga_println_colored("[ERROR] Usage: write <file> <konten>", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    /* Find first space to split filename and content */
    const char *p = args;
    while (*p && *p != ' ') p++;
    if (!*p) {
        vga_println_colored("[ERROR] Usage: write <file> <konten>", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    /* Copy filename */
    char filename[64];
    int len = (int)(p - args);
    if (len > 63) len = 63;
    for (int i = 0; i < len; i++) filename[i] = args[i];
    filename[len] = '\0';
    const char *content = cmd_skip(p);
    vfs_write(filename, content);
}

void cmd_read_file(const char *args) {
    args = cmd_skip(args);
    if (!args || !*args) {
        vga_println_colored("[ERROR] Usage: read <nama_file>", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    char buf[4096];
    int r = vfs_read(args, buf, sizeof(buf));
    if (r < 0) return;
    vga_println_colored("--- Isi File ---", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_println(buf);
    vga_println_colored("--- Akhir File ---", VGA_LIGHT_CYAN, VGA_BLACK);
}

void cmd_delete_file(const char *args) {
    args = cmd_skip(args);
    if (!args || !*args) {
        vga_println_colored("[ERROR] Usage: delete <nama_file>", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    vfs_delete(args);
}

/* ---- Access control -------------------------------------------------- */
void cmd_whoami(const char *args) {
    (void)args;
    int role = acl_get_role();
    vga_print("Role saat ini: ");
    vga_color_t clr = (role == ROLE_ADMIN) ? VGA_LIGHT_RED :
                      (role == ROLE_USER)  ? VGA_LIGHT_GREEN : VGA_DARK_GREY;
    vga_println_colored(acl_role_name(role), clr, VGA_BLACK);
}

void cmd_su(const char *args) {
    args = cmd_skip(args);
    if (!args || !*args) {
        vga_println("Usage: su admin | su user");
        return;
    }

    int is_admin = (args[0]=='a' && args[1]=='d' && args[2]=='m' &&
                    args[3]=='i' && args[4]=='n' && (args[5]=='\0'||args[5]==' '));
    int is_user  = (args[0]=='u' && args[1]=='s' && args[2]=='e' &&
                    args[3]=='r' && (args[4]=='\0'||args[4]==' '));

    if (is_admin) {
        vga_print("Password: ");
        char pw[64];
        keyboard_readline(pw, sizeof(pw));
        if (acl_login_admin(pw)) {
            vga_println_colored("[OK] Login sebagai Admin berhasil.", VGA_LIGHT_GREEN, VGA_BLACK);
        } else {
            vga_println_colored("[ERROR] Password salah!", VGA_LIGHT_RED, VGA_BLACK);
        }
    } else if (is_user) {
        acl_logout();
        vga_println_colored("[OK] Kembali ke mode User.", VGA_LIGHT_GREEN, VGA_BLACK);
    } else {
        vga_println_colored("[ERROR] Role tidak dikenal. Gunakan: su admin | su user", VGA_LIGHT_RED, VGA_BLACK);
    }
}

/* ---- Task Queue commands --------------------------------------------- */
void cmd_tambah_tugas(const char *args) {
    (void)args;
    char matkul[64], keterangan[256], waktu_str[32];

    vga_println_colored("=== Tambah Tugas Baru ===", VGA_YELLOW, VGA_BLACK);

    vga_print("Mata Kuliah : ");
    keyboard_readline(matkul, sizeof(matkul));

    vga_print("Waktu (D H M): ");
    keyboard_readline(waktu_str, sizeof(waktu_str));

    vga_print("Keterangan  : ");
    keyboard_readline(keterangan, sizeof(keterangan));

    /* Validate empty fields */
    if (cmd_strlen(matkul) == 0) {
        vga_println_colored("[ERROR] Mata kuliah tidak boleh kosong.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    if (cmd_strlen(keterangan) == 0) {
        vga_println_colored("[ERROR] Keterangan tidak boleh kosong.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    /* Parse "D H M" */
    int d = 0, h = 0, m = 0;
    const char *p = waktu_str;
    p = cmd_atoi(p, &d);
    p = cmd_atoi(p, &h);
    p = cmd_atoi(p, &m);

    if (d < 0 || h < 0 || h > 23 || m < 0 || m > 59) {
        vga_println_colored("[ERROR] Format waktu tidak valid (D>=0, H 0-23, M 0-59).", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    if (d == 0 && h == 0 && m == 0) {
        vga_println_colored("[ERROR] Waktu selesai tidak boleh nol.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }

    taskqueue_add(matkul, (uint32_t)d, (uint32_t)h, (uint32_t)m, keterangan);
}

void cmd_semua_antrean(const char *args) {
    (void)args;
    vga_println_colored("=== Semua Antrean Tugas (EDF Order) ===", VGA_YELLOW, VGA_BLACK);
    taskqueue_print_all();
    vga_print("Total: ");
    cmd_print_uint((uint32_t)taskqueue_count());
    vga_println(" tugas.");
}

void cmd_selesai(const char *args) {
    args = cmd_skip(args);
    if (!args || !*args) {
        vga_println_colored("[ERROR] Usage: selesai <nomor_tugas>", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    int n = 0;
    cmd_atoi(args, &n);
    if (n <= 0) {
        vga_println_colored("[ERROR] Nomor tugas harus bilangan positif.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    taskqueue_complete((uint32_t)n);
}

/* ---- System commands ------------------------------------------------- */
void cmd_shutdown(const char *args) {
    (void)args;
    if (!acl_check(ROLE_ADMIN)) {
        vga_println_colored("[AKSES DITOLAK] Hanya Admin yang bisa shutdown.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    vga_println_colored("Shutting down PrabroroOS...", VGA_YELLOW, VGA_BLACK);
    __asm__ volatile("cli");
    /* QEMU ACPI power-off sequence */
    outb(0x604, 0x2000);  /* QEMU/Bochs ACPI S5 */
    outb(0xB004, 0x2000); /* Older QEMU */
    outb(0x4004, 0x3400); /* VirtualBox */
    /* Fallback: halt */
    for (;;) __asm__ volatile("hlt");
}

void cmd_reboot(const char *args) {
    (void)args;
    if (!acl_check(ROLE_ADMIN)) {
        vga_println_colored("[AKSES DITOLAK] Hanya Admin yang bisa reboot.", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    vga_println_colored("Rebooting PrabroroOS...", VGA_YELLOW, VGA_BLACK);
    __asm__ volatile("cli");
    /* Pulse PS/2 controller reset line */
    uint8_t tmp;
    do { tmp = inb(0x64); } while (tmp & 0x02); /* wait input empty */
    outb(0x64, 0xFE);  /* reset CPU via PS/2 controller */
    for (;;) __asm__ volatile("hlt");
}
