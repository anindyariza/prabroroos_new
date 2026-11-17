/*
 * PrabroroOS — drivers/vga.c
 * VGA text mode driver: 80x25, 2-byte cells, scrolling, hardware cursor.
 */
#include "vga.h"
#include "../kernel/types.h"
#include "../include/common.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_BUF    ((volatile uint16_t *)0xB8000)

static int     vga_row   = 0;
static int     vga_col   = 0;
static uint8_t vga_attr  = 0x0F; /* White on black */

static inline uint16_t vga_cell(char c, uint8_t attr) {
    return (uint16_t)(uint8_t)c | ((uint16_t)attr << 8);
}

static void vga_scroll(void) {
    for (int r = 1; r < VGA_HEIGHT; r++)
        for (int c = 0; c < VGA_WIDTH; c++)
            VGA_BUF[(r-1)*VGA_WIDTH + c] = VGA_BUF[r*VGA_WIDTH + c];
    for (int c = 0; c < VGA_WIDTH; c++)
        VGA_BUF[(VGA_HEIGHT-1)*VGA_WIDTH + c] = vga_cell(' ', vga_attr);
    vga_row = VGA_HEIGHT - 1;
}

void vga_update_hw_cursor(void) {
    uint16_t pos = (uint16_t)(vga_row * VGA_WIDTH + vga_col);
    outb(0x3D4, 14); outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    outb(0x3D4, 15); outb(0x3D5, (uint8_t)(pos & 0xFF));
}

void vga_init(void) {
    vga_row  = 0;
    vga_col  = 0;
    vga_attr = 0x0F;
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BUF[i] = vga_cell(' ', vga_attr);
    vga_row = 0;
    vga_col = 0;
    vga_update_hw_cursor();
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        if (++vga_row >= VGA_HEIGHT) vga_scroll();
        return;
    }
    if (c == '\r') {
        vga_col = 0;
        return;
    }
    if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            VGA_BUF[vga_row * VGA_WIDTH + vga_col] = vga_cell(' ', vga_attr);
        }
        return;
    }
    if (c == '\t') {
        int next = (vga_col + 8) & ~7;
        if (next >= VGA_WIDTH) next = VGA_WIDTH - 1;
        while (vga_col < next) {
            VGA_BUF[vga_row * VGA_WIDTH + vga_col] = vga_cell(' ', vga_attr);
            vga_col++;
        }
        return;
    }
    VGA_BUF[vga_row * VGA_WIDTH + vga_col] = vga_cell(c, vga_attr);
    if (++vga_col >= VGA_WIDTH) {
        vga_col = 0;
        if (++vga_row >= VGA_HEIGHT) vga_scroll();
    }
}

void vga_print(const char *s) {
    while (s && *s) vga_putchar(*s++);
    vga_update_hw_cursor();
}

void vga_println(const char *s) {
    vga_print(s);
    vga_putchar('\n');
    vga_update_hw_cursor();
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_attr = (uint8_t)(((uint8_t)bg << 4) | ((uint8_t)fg & 0x0F));
}

void vga_get_cursor(int *row, int *col) {
    if (row) *row = vga_row;
    if (col) *col = vga_col;
}

void vga_print_at(int row, int col, const char *s) {
    int saved_row = vga_row, saved_col = vga_col;
    vga_row = row; vga_col = col;
    while (s && *s) vga_putchar(*s++);
    vga_row = saved_row; vga_col = saved_col;
}

void vga_print_colored(const char *s, vga_color_t fg, vga_color_t bg) {
    uint8_t saved = vga_attr;
    vga_set_color(fg, bg);
    vga_print(s);
    vga_attr = saved;
}

void vga_println_colored(const char *s, vga_color_t fg, vga_color_t bg) {
    uint8_t saved = vga_attr;
    vga_set_color(fg, bg);
    vga_println(s);
    vga_attr = saved;
}

void vga_print_hex(uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    vga_print("0x");
    for (int i = 7; i >= 0; i--)
        vga_putchar(hex[(val >> (i*4)) & 0xF]);
    vga_update_hw_cursor();
}

void vga_print_dec(int32_t val) {
    if (val < 0) { vga_putchar('-'); vga_print_udec((uint32_t)(-val)); }
    else          vga_print_udec((uint32_t)val);
}

void vga_print_udec(uint32_t val) {
    if (val == 0) { vga_putchar('0'); vga_update_hw_cursor(); return; }
    char buf[12]; int i = 0;
    while (val) { buf[i++] = '0' + (val % 10); val /= 10; }
    while (i--) vga_putchar(buf[i]);
    vga_update_hw_cursor();
}
