# ============================================================
# PrabroroOS — Makefile
# Cross-compiles a real x86 32-bit kernel for QEMU/GRUB
# ============================================================

# --- Toolchain ---
CC      := i686-elf-gcc
LD      := i686-elf-ld
AS      := nasm
# Fallback if no cross-compiler (native 32-bit host):
# CC    := gcc -m32

CFLAGS  := -m32 -ffreestanding -fno-pie -fno-stack-protector \
           -fno-builtin -nostdlib -nostdinc -Wall -Wextra \
           -std=c99 -O2 -g
ASFLAGS := -f elf32
LDFLAGS := -m elf_i386 -T linker.ld --warn-common

# --- Output ---
KERNEL_ELF := PrabroroOS.elf
ISO_DIR    := iso
ISO_FILE   := PrabroroOS.iso

# --- Source collection ---
C_SRCS := kernel/kernel.c    \
          kernel/panic.c      \
          cpu/gdt.c           \
          cpu/idt.c           \
          cpu/pic.c           \
          cpu/isr_handler.c   \
          drivers/vga.c       \
          drivers/keyboard.c  \
          drivers/pit.c       \
          mm/kmalloc.c        \
          mm/meminfo.c        \
          proc/process.c      \
          proc/scheduler.c    \
          sync/mutex.c        \
          sync/semaphore.c    \
          fs/ramdisk.c        \
          fs/vfs.c            \
          security/acl.c      \
          features/taskqueue.c \
          features/memmonitor.c \
          shell/shell.c       \
          shell/commands.c

ASM_SRCS := boot/boot.asm          \
            boot/gdt_flush.asm     \
            cpu/idt_load.asm       \
            cpu/isr.asm            \
            cpu/irq.asm            \
            proc/context_switch.asm

C_OBJS   := $(C_SRCS:.c=.o)
ASM_OBJS := $(ASM_SRCS:.asm=.o)
ALL_OBJS := $(ASM_OBJS) $(C_OBJS)

# --- Default target ---
.PHONY: all clean run run-kernel iso check-tools

all: $(KERNEL_ELF)

# --- Link ---
$(KERNEL_ELF): $(ALL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)
	@echo ""
	@echo "  *** PrabroroOS.elf built successfully ***"
	@echo ""

# --- Compile C ---
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- Assemble ---
%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# --- Create GRUB ISO ---
iso: $(KERNEL_ELF)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/PrabroroOS.elf
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)
	@echo "  *** $(ISO_FILE) created ***"

# --- Run via ISO (full GRUB boot) ---
run: iso
	qemu-system-i386 -cdrom $(ISO_FILE) -m 64M -vga std \
	    -serial stdio -no-reboot

# --- Run directly via Multiboot (faster, no ISO) ---
run-kernel: $(KERNEL_ELF)
	qemu-system-i386 -kernel $(KERNEL_ELF) -m 64M -vga std \
	    -serial stdio -no-reboot

# --- Run headless (serial only) ---
run-serial: $(KERNEL_ELF)
	qemu-system-i386 -kernel $(KERNEL_ELF) -m 64M \
	    -nographic -serial mon:stdio -no-reboot

# --- Verify Multiboot header ---
check-tools:
	@which i686-elf-gcc   || (echo "MISSING: i686-elf-gcc" && false)
	@which nasm           || (echo "MISSING: nasm" && false)
	@which qemu-system-i386 || (echo "MISSING: qemu-system-i386" && false)
	@echo "All required tools found."

# --- Clean ---
clean:
	rm -f $(ALL_OBJS) $(KERNEL_ELF) $(ISO_FILE)
	rm -rf $(ISO_DIR)
	@echo "Clean done."
