#pragma once
#include "../kernel/types.h"

void kernel_panic(const char *msg) __attribute__((noreturn));
void kernel_assert(int condition, const char *msg);
