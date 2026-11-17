/*
 * PrabroroOS — fs/vfs.c
 * VFS abstraction layer — thin wrapper over RAMDisk backend
 */
#include "vfs.h"
#include "ramdisk.h"
#include "../drivers/vga.h"
#include "../security/acl.h"

void vfs_init(void) {
    /* RAMDisk is already initialized by ramdisk_init() in kernel_main */
}

int vfs_create(const char *path, uint8_t owner_role) {
    int r = ramdisk_create(path, owner_role);
    if (r == -2) {
        vga_print_colored("[VFS] File sudah ada: ", VGA_YELLOW, VGA_BLACK);
        vga_println(path);
        return -1;
    }
    if (r < 0) {
        vga_println_colored("[VFS] Gagal membuat file (penuh atau nama tidak valid).", VGA_LIGHT_RED, VGA_BLACK);
        return -1;
    }
    vga_print_colored("[VFS] File dibuat: ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_println(path);
    return 0;
}

int vfs_write(const char *path, const char *data) {
    if (!ramdisk_exists(path)) {
        vga_print_colored("[VFS] File tidak ada: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_println(path);
        return -1;
    }
    size_t len = 0;
    while (data && data[len]) len++;
    int r = ramdisk_write(path, data, (uint32_t)len);
    if (r < 0) {
        vga_println_colored("[VFS] Gagal menulis ke file.", VGA_LIGHT_RED, VGA_BLACK);
        return -1;
    }
    vga_print_colored("[VFS] Ditulis ke: ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_println(path);
    return 0;
}

int vfs_read(const char *path, char *buf, uint32_t max) {
    if (!ramdisk_exists(path)) {
        vga_print_colored("[VFS] File tidak ada: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_println(path);
        return -1;
    }
    return ramdisk_read(path, buf, max);
}

int vfs_delete(const char *path) {
    /* Only admins can delete files */
    if (!acl_check(ROLE_ADMIN)) {
        vga_println_colored("[VFS] Akses ditolak! Hanya Admin yang bisa menghapus file.", VGA_LIGHT_RED, VGA_BLACK);
        return -1;
    }
    if (!ramdisk_exists(path)) {
        vga_print_colored("[VFS] File tidak ada: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_println(path);
        return -1;
    }
    ramdisk_delete(path);
    vga_print_colored("[VFS] File dihapus: ", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_println(path);
    return 0;
}

void vfs_ls(void) {
    ramdisk_list();
}
