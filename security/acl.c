/*
 * PrabroroOS — security/acl.c
 * Role-Based Access Control
 * Roles: Guest (0) < User (1) < Admin (2)
 * Default on boot: User
 */
#include "acl.h"
#include "../drivers/vga.h"
#include "../kernel/types.h"

#define ADMIN_PASSWORD "prabro123"

static int current_role = ROLE_USER;

/* Internal strcmp */
static int acl_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

void acl_init(void) {
    current_role = ROLE_USER;
}

int acl_get_role(void) {
    return current_role;
}

void acl_set_role(int role) {
    if (role >= ROLE_GUEST && role <= ROLE_ADMIN)
        current_role = role;
}

const char *acl_role_name(int role) {
    switch (role) {
        case ROLE_GUEST: return "Guest";
        case ROLE_USER:  return "User";
        case ROLE_ADMIN: return "Admin";
        default:         return "Unknown";
    }
}

int acl_check(int required_role) {
    return current_role >= required_role;
}

int acl_login_admin(const char *password) {
    if (!password) return 0;
    if (acl_strcmp(password, ADMIN_PASSWORD) == 0) {
        current_role = ROLE_ADMIN;
        return 1;
    }
    return 0;
}

void acl_logout(void) {
    current_role = ROLE_USER;
}
