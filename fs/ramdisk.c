/*
 * PrabroroOS — fs/ramdisk.c
 * In-memory filesystem: 32 files × 4 KB
 */
#include "ramdisk.h"
#include "../drivers/vga.h"
#include "../kernel/types.h"

static ramdisk_file_t files[RAMDISK_MAX_FILES];

/* String helpers (no stdlib) */
static size_t rd_strlen(const char *s) {
    size_t n = 0; while (s && s[n]) n++; return n;
}
static int rd_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
static void rd_strcpy(char *dst, const char *src, size_t max) {
    size_t i = 0;
    while (i + 1 < max && src && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}
static void rd_memset(void *p, uint8_t v, size_t n) {
    uint8_t *b = (uint8_t *)p; while (n--) *b++ = v;
}
static void rd_memcpy(void *d, const void *s, size_t n) {
    uint8_t *dd = (uint8_t *)d; const uint8_t *ss = (const uint8_t *)s;
    while (n--) *dd++ = *ss++;
}
static void rd_print_uint(uint32_t v) {
    if (!v) { vga_putchar('0'); return; }
    char buf[12]; int i=0;
    while (v) { buf[i++]='0'+(v%10); v/=10; }
    while (i--) vga_putchar(buf[i]);
}

void ramdisk_init(void) {
    rd_memset(files, 0, sizeof(files));
}

static ramdisk_file_t *find_slot(const char *name) {
    for (int i = 0; i < RAMDISK_MAX_FILES; i++)
        if (files[i].in_use && rd_strcmp(files[i].name, name) == 0)
            return &files[i];
    return NULL;
}

int ramdisk_create(const char *name, uint8_t owner_role) {
    if (!name || rd_strlen(name) == 0) return -1;
    if (find_slot(name)) return -2; /* already exists */
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (!files[i].in_use) {
            rd_memset(&files[i], 0, sizeof(ramdisk_file_t));
            rd_strcpy(files[i].name, name, RAMDISK_MAX_NAME);
            files[i].in_use     = 1;
            files[i].owner_role = owner_role;
            files[i].size       = 0;
            return 0;
        }
    }
    return -3; /* no free slots */
}

int ramdisk_write(const char *name, const char *data, uint32_t len) {
    ramdisk_file_t *f = find_slot(name);
    if (!f) return -1;
    if (len >= RAMDISK_MAX_SIZE) len = RAMDISK_MAX_SIZE - 1;
    rd_memcpy(f->data, data, len);
    f->data[len] = '\0';
    f->size = len;
    return 0;
}

int ramdisk_read(const char *name, char *out, uint32_t max_len) {
    ramdisk_file_t *f = find_slot(name);
    if (!f) return -1;
    uint32_t n = f->size < max_len - 1 ? f->size : max_len - 1;
    rd_memcpy(out, f->data, n);
    out[n] = '\0';
    return (int)n;
}

int ramdisk_delete(const char *name) {
    ramdisk_file_t *f = find_slot(name);
    if (!f) return -1;
    rd_memset(f, 0, sizeof(ramdisk_file_t));
    return 0;
}

int ramdisk_exists(const char *name) {
    return find_slot(name) != NULL;
}

ramdisk_file_t *ramdisk_get(const char *name) {
    return find_slot(name);
}

int ramdisk_count(void) {
    int n = 0;
    for (int i = 0; i < RAMDISK_MAX_FILES; i++)
        if (files[i].in_use) n++;
    return n;
}

void ramdisk_list(void) {
    int found = 0;
    vga_println_colored("--- RAMDisk Files ---", VGA_LIGHT_CYAN, VGA_BLACK);
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (files[i].in_use) {
            vga_print("  [");
            rd_print_uint(i);
            vga_print("] ");
            vga_print(files[i].name);
            vga_print("  (");
            rd_print_uint(files[i].size);
            vga_print(" bytes, owner=");
            vga_print(files[i].owner_role == 0 ? "admin" : "user");
            vga_println(")");
            found++;
        }
    }
    if (!found) vga_println("  (tidak ada file)");
    vga_print("  Total: "); rd_print_uint(found); vga_println(" file(s)");
}
