/*
 * PrabroroOS — mm/kmalloc.c
 * First-fit linked-list heap allocator.
 * No external dependencies (not even string.h).
 */
#include "kmalloc.h"
#include "../kernel/types.h"

#define HEAP_MAGIC  0xDEADBEEFU

typedef struct block_hdr {
    uint32_t        magic;
    uint32_t        size;      /* total bytes including this header */
    uint8_t         is_free;
    struct block_hdr *next;
    struct block_hdr *prev;
} __attribute__((packed, aligned(8))) block_hdr_t;

#define HDR_SIZE  sizeof(block_hdr_t)

static block_hdr_t *heap_list   = NULL;
static size_t       _heap_total = 0;

/* Internal helpers */
static void km_memcpy(void *dst, const void *src, size_t n) {
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
}

void heap_init(uintptr_t start, size_t size) {
    heap_list              = (block_hdr_t *)start;
    heap_list->magic       = HEAP_MAGIC;
    heap_list->size        = (uint32_t)size;
    heap_list->is_free     = 1;
    heap_list->next        = NULL;
    heap_list->prev        = NULL;
    _heap_total            = size;
}

void *kmalloc(size_t size) {
    if (!size) return NULL;
    /* Align to 8 bytes */
    size = (size + 7) & ~(size_t)7;
    size_t total = size + HDR_SIZE;

    block_hdr_t *cur = heap_list;
    while (cur) {
        if (cur->is_free && cur->size >= total) {
            /* Split if enough room for a new header + min 8 bytes */
            if (cur->size >= total + HDR_SIZE + 8) {
                block_hdr_t *split = (block_hdr_t *)((uintptr_t)cur + total);
                split->magic   = HEAP_MAGIC;
                split->size    = cur->size - (uint32_t)total;
                split->is_free = 1;
                split->next    = cur->next;
                split->prev    = cur;
                if (cur->next) cur->next->prev = split;
                cur->next = split;
                cur->size = (uint32_t)total;
            }
            cur->is_free = 0;
            return (void *)((uintptr_t)cur + HDR_SIZE);
        }
        cur = cur->next;
    }
    return NULL; /* OOM */
}

void *kmalloc_aligned(size_t size, size_t align) {
    /* Simple: over-allocate and align manually — good enough for hobby kernel */
    void *p = kmalloc(size + align);
    if (!p) return NULL;
    uintptr_t addr = (uintptr_t)p;
    uintptr_t aligned = (addr + align - 1) & ~(uintptr_t)(align - 1);
    return (void *)aligned;
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_hdr_t *hdr = (block_hdr_t *)((uintptr_t)ptr - HDR_SIZE);
    if (hdr->magic != HEAP_MAGIC || hdr->is_free) return; /* guard */
    hdr->is_free = 1;

    /* Coalesce next */
    if (hdr->next && hdr->next->is_free) {
        hdr->size += hdr->next->size;
        hdr->next  = hdr->next->next;
        if (hdr->next) hdr->next->prev = hdr;
    }
    /* Coalesce prev */
    if (hdr->prev && hdr->prev->is_free) {
        hdr->prev->size += hdr->size;
        hdr->prev->next  = hdr->next;
        if (hdr->next) hdr->next->prev = hdr->prev;
    }
}

void *krealloc(void *ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (!new_size) { kfree(ptr); return NULL; }
    block_hdr_t *hdr = (block_hdr_t *)((uintptr_t)ptr - HDR_SIZE);
    size_t old_data = hdr->size - HDR_SIZE;
    void *newp = kmalloc(new_size);
    if (!newp) return NULL;
    km_memcpy(newp, ptr, old_data < new_size ? old_data : new_size);
    kfree(ptr);
    return newp;
}

size_t heap_used(void) {
    size_t used = 0;
    block_hdr_t *cur = heap_list;
    while (cur) {
        if (!cur->is_free) used += cur->size - HDR_SIZE;
        cur = cur->next;
    }
    return used;
}

size_t heap_free(void) {
    size_t free_b = 0;
    block_hdr_t *cur = heap_list;
    while (cur) {
        if (cur->is_free) free_b += cur->size - HDR_SIZE;
        cur = cur->next;
    }
    return free_b;
}

size_t heap_total(void) {
    return _heap_total;
}
