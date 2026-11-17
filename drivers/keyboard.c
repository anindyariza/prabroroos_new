/*
 * PrabroroOS — drivers/keyboard.c
 * PS/2 keyboard driver — Scancode Set 1 → ASCII, IRQ1 handler
 */
#include "keyboard.h"
#include "vga.h"
#include "../include/common.h"
#include "../cpu/pic.h"
#include "../kernel/types.h"

/* Forward declaration of IRQ registration (from cpu/isr_handler.c) */
extern void register_irq_handler(uint8_t irq, void (*handler)(void *));

/* ---- Scancode Set 1 → ASCII (US QWERTY) ----------------------------- */
static const char sc_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', 8,  9,
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,'a','s',
    'd','f','g','h','j','k','l',';','\'','`',  0,'\\','z','x','c','v',
    'b','n','m',',','.','/',  0,'*',  0,' ',  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const char sc_ascii_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')',  '_','+', 8,  9,
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,'A','S',
    'D','F','G','H','J','K','L',':','"','~',  0,'|','Z','X','C','V',
    'B','N','M','<','>','?',  0,'*',  0,' ',  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/* ---- Ring buffer ----------------------------------------------------- */
static volatile char kb_buf[KEY_BUFFER_SIZE];
static volatile uint32_t kb_head = 0;
static volatile uint32_t kb_tail = 0;

static int shift_pressed = 0;
static int caps_lock     = 0;

static void kb_push(char c) {
    uint32_t next = (kb_head + 1) % KEY_BUFFER_SIZE;
    if (next != kb_tail) {
        kb_buf[kb_head] = c;
        kb_head = next;
    }
}

static char kb_pop(void) {
    if (kb_head == kb_tail) return 0;
    char c = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) % KEY_BUFFER_SIZE;
    return c;
}

/* ---- IRQ1 handler ---------------------------------------------------- */
void keyboard_irq_handler(void *regs) {
    (void)regs;
    uint8_t sc = inb(0x60);

    /* Extended prefix: ignore for now */
    if (sc == 0xE0) { pic_send_eoi(1); return; }

    if (sc & 0x80) {
        /* Break code */
        sc &= 0x7F;
        if (sc == 0x2A || sc == 0x36) shift_pressed = 0;
        pic_send_eoi(1);
        return;
    }

    /* Make code */
    if (sc == 0x2A || sc == 0x36) { shift_pressed = 1; pic_send_eoi(1); return; }
    if (sc == 0x3A) { caps_lock ^= 1; pic_send_eoi(1); return; } /* Caps Lock */

    if (sc >= 128) { pic_send_eoi(1); return; }

    char ch;
    char raw     = sc_ascii[sc];
    char shifted = sc_ascii_shift[sc];

    if (raw >= 'a' && raw <= 'z') {
        /* Letter: caps XOR shift */
        ch = (caps_lock ^ shift_pressed) ? shifted : raw;
    } else {
        ch = shift_pressed ? shifted : raw;
    }

    if (ch) kb_push(ch);

    pic_send_eoi(1);
}

void keyboard_init(void) {
    kb_head = kb_tail = 0;
    shift_pressed = caps_lock = 0;
    register_irq_handler(1, keyboard_irq_handler);
}

int keyboard_has_input(void) {
    return kb_head != kb_tail;
}

char keyboard_getchar(void) {
    while (!keyboard_has_input())
        __asm__ volatile("hlt");
    return kb_pop();
}

void keyboard_readline(char *buf, uint32_t max_len) {
    uint32_t pos = 0;
    if (!buf || max_len == 0) return;
    while (pos + 1 < max_len) {
        char c = keyboard_getchar();
        if (c == '\n') {
            vga_putchar('\n');
            vga_update_hw_cursor();
            break;
        }
        if (c == '\b') {
            if (pos > 0) {
                pos--;
                vga_putchar('\b');
                vga_update_hw_cursor();
            }
            continue;
        }
        if (c >= 32 && c < 127) {
            buf[pos++] = c;
            vga_putchar(c);
            vga_update_hw_cursor();
        }
    }
    buf[pos] = '\0';
}
