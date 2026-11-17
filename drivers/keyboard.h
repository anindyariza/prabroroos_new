/*
 * PrabroroOS — drivers/keyboard.h
 */
#pragma once
#include "../kernel/types.h"

#define KEY_BUFFER_SIZE 256

void keyboard_init(void);
void keyboard_irq_handler(void *regs);
char keyboard_getchar(void);
int  keyboard_has_input(void);
void keyboard_readline(char *buf, uint32_t max_len);
