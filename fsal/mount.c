#include <ofsl/fsal/fsal.h>

#include <stdlib.h>
#include <string.h>

#include <ofsl/partition/partition.h>

#include "config.h"

struct volid_ll_elem {
    struct volid_ll_elem* next;
    struct volid_ll_elem* prev;
    OFSL_Partition* part;
    char volid[16];
    int mounted;
};

static struct volid_ll_elem* volid_ll_start = NULL;

static struct volid_ll_elem* volid_ll_insert(const char* volid)
{
    unsigned int volid_len;
    if ((volid_len = strnlen(volid, 16)) >= 15) {
        return NULL;
    }

    struct volid_ll_elem* ll_current = volid_ll_start;

    if (volid_ll_start) {
        while (ll_current->next) {
            if (strncmp(volid, ll_current->volid, sizeof(ll_current->volid)) == 0) {
                return NULL;
            }
            ll_current = ll_current->next;
        }
        ll_current->next = malloc(sizeof(struct volid_ll_elem));
        ll_current->next->prev = ll_current;
    } else {
        ll_current = malloc(sizeof(struct volid_ll_elem));
        ll_current->next = NULL;
        ll_current->prev = NULL;
        volid_ll_start = ll_current;
    }

    memcpy(ll_current->volid, volid, volid_len);
    return ll_current;
}

static int volid_ll_remove(const char* volid)
{
    struct volid_ll_elem* ll_current = volid_ll_start;

    while (ll_current) {
        if (strncmp(volid, ll_current->volid, sizeof(ll_current->volid)) == 0) {
            ll_current->prev->next = ll_current->next;
            ll_current->next->prev = ll_current->prev;
            free(volid);
            return 0;
        }
        ll_current = ll_current->next;
    }

    return 1;
}

OFSL_DTOR
static void volid_ll_destruct(void)
{
    struct volid_ll_elem* ll_current = volid_ll_start;
    struct volid_ll_elem* ll_prev;

    while (ll_current) {
        ll_prev = ll_current;
        ll_current = ll_current->next;
        free(ll_prev);
    }
}

static struct volid_ll_elem* volid_ll_find(const char* volid)
{
    struct volid_ll_elem* ll_current = volid_ll_start;

    while (ll_current) {
        if (strncmp(volid, ll_current->volid, sizeof(ll_current->volid)) == 0) {
            return ll_current;
        }
        ll_current = ll_current->next;
    }

    return NULL;
}

int fsal_mount(const char* id)
{
    struct volid_ll_elem* ll_elem = volid_ll_insert(id);
    if (!ll_elem) {
        return 1;
    }

    ll_elem->part = 
}


