#include "lvm.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"

#define MAX_VGS 32
#define MAX_LVS 256

static lvm_vg_t* volume_groups = NULL;
static lvm_lv_t* logical_volumes = NULL;
static spinlock_t lvm_lock = SPINLOCK_INIT;

int lvm_init(void) {
    DEBUG_INFO("%s", "LVM (Logical Volume Manager) initialized");
    return 0;
}

int lvm_vg_create(const char* name, void** pvs, u32 pv_count) {
    VALIDATE_STRING(name, 64);
    VALIDATE_PTR_RET(pvs, -1);
    
    if (pv_count == 0 || pv_count > 32) {
        DEBUG_ERROR("Invalid PV count: %u", pv_count);
        return -1;
    }
    
    spinlock_lock(&lvm_lock);
    
    /* Check if VG already exists */
    lvm_vg_t* vg = volume_groups;
    while (vg) {
        if (strcmp(vg->name, name) == 0) {
            spinlock_unlock(&lvm_lock);
            DEBUG_WARN("Volume group already exists: %s", name);
            return -1;
        }
        vg = vg->next;
    }
    
    /* Create new VG */
    vg = (lvm_vg_t*)kmalloc(sizeof(lvm_vg_t));
    if (!vg) {
        spinlock_unlock(&lvm_lock);
        return -1;
    }
    
    strncpy(vg->name, name, sizeof(vg->name) - 1);
    vg->name[sizeof(vg->name) - 1] = '\0';
    vg->pv_count = pv_count;
    vg->total_size = 0;
    vg->free_size = 0;
    
    for (u32 i = 0; i < pv_count; i++) {
        vg->physical_volumes[i] = pvs[i];
        /* Would calculate size from PV */
        vg->total_size += 10 * 1024 * 1024 * 1024; /* 10GB per PV placeholder */
    }
    for (u32 i = pv_count; i < 32; i++) {
        vg->physical_volumes[i] = NULL;
    }
    
    vg->free_size = vg->total_size;
    vg->next = volume_groups;
    volume_groups = vg;
    
    spinlock_unlock(&lvm_lock);
    DEBUG_INFO("Volume group created: name=%s, PVs=%u, size=%llu GB", 
              name, pv_count, (unsigned long long)(vg->total_size / (1024*1024*1024)));
    return 0;
}

int lvm_vg_remove(const char* name) {
    VALIDATE_STRING(name, 64);
    
    spinlock_lock(&lvm_lock);
    
    lvm_vg_t** prev = &volume_groups;
    lvm_vg_t* vg = volume_groups;
    
    while (vg) {
        if (strcmp(vg->name, name) == 0) {
            /* Check for logical volumes */
            lvm_lv_t* lv = logical_volumes;
            while (lv) {
                if (lv->vg == vg) {
                    spinlock_unlock(&lvm_lock);
                    DEBUG_ERROR("Cannot remove VG with logical volumes");
                    return -1;
                }
                lv = lv->next;
            }
            
            *prev = vg->next;
            kfree(vg);
            
            spinlock_unlock(&lvm_lock);
            DEBUG_INFO("Volume group removed: name=%s", name);
            return 0;
        }
        prev = &vg->next;
        vg = vg->next;
    }
    
    spinlock_unlock(&lvm_lock);
    return -1;
}

int lvm_lv_create(const char* vg_name, const char* lv_name, u64 size) {
    VALIDATE_STRING(vg_name, 64);
    VALIDATE_STRING(lv_name, 64);
    
    spinlock_lock(&lvm_lock);
    
    /* Find VG */
    lvm_vg_t* vg = volume_groups;
    while (vg) {
        if (strcmp(vg->name, vg_name) == 0) {
            break;
        }
        vg = vg->next;
    }
    
    if (!vg) {
        spinlock_unlock(&lvm_lock);
        DEBUG_ERROR("Volume group not found: %s", vg_name);
        return -1;
    }
    
    if (size > vg->free_size) {
        spinlock_unlock(&lvm_lock);
        DEBUG_ERROR("Insufficient space in VG: requested=%llu, free=%llu",
                   (unsigned long long)size, (unsigned long long)vg->free_size);
        return -1;
    }
    
    /* Create LV */
    lvm_lv_t* lv = (lvm_lv_t*)kmalloc(sizeof(lvm_lv_t));
    if (!lv) {
        spinlock_unlock(&lvm_lock);
        return -1;
    }
    
    strncpy(lv->name, lv_name, sizeof(lv->name) - 1);
    lv->name[sizeof(lv->name) - 1] = '\0';
    lv->vg = vg;
    lv->size = size;
    lv->segments = 1;
    lv->segments_data = NULL;
    lv->next = logical_volumes;
    logical_volumes = lv;
    
    vg->free_size -= size;
    
    spinlock_unlock(&lvm_lock);
    DEBUG_INFO("Logical volume created: vg=%s, lv=%s, size=%llu MB",
              vg_name, lv_name, (unsigned long long)(size / (1024*1024)));
    return 0;
}

int lvm_lv_remove(const char* vg_name, const char* lv_name) {
    VALIDATE_STRING(vg_name, 64);
    VALIDATE_STRING(lv_name, 64);
    
    spinlock_lock(&lvm_lock);
    
    lvm_lv_t** prev = &logical_volumes;
    lvm_lv_t* lv = logical_volumes;
    
    while (lv) {
        if (strcmp(lv->vg->name, vg_name) == 0 && strcmp(lv->name, lv_name) == 0) {
            lvm_vg_t* vg = lv->vg;
            vg->free_size += lv->size;
            
            *prev = lv->next;
            kfree(lv);
            
            spinlock_unlock(&lvm_lock);
            DEBUG_INFO("Logical volume removed: vg=%s, lv=%s", vg_name, lv_name);
            return 0;
        }
        prev = &lv->next;
        lv = lv->next;
    }
    
    spinlock_unlock(&lvm_lock);
    return -1;
}

int lvm_lv_extend(const char* vg_name, const char* lv_name, u64 size) {
    VALIDATE_STRING(vg_name, 64);
    VALIDATE_STRING(lv_name, 64);
    
    spinlock_lock(&lvm_lock);
    
    lvm_lv_t* lv = logical_volumes;
    while (lv) {
        if (strcmp(lv->vg->name, vg_name) == 0 && strcmp(lv->name, lv_name) == 0) {
            if (size > lv->vg->free_size) {
                spinlock_unlock(&lvm_lock);
                return -1;
            }
            
            lv->size += size;
            lv->vg->free_size -= size;
            
            spinlock_unlock(&lvm_lock);
            DEBUG_INFO("Logical volume extended: vg=%s, lv=%s, new_size=%llu MB",
                      vg_name, lv_name, (unsigned long long)(lv->size / (1024*1024)));
            return 0;
        }
        lv = lv->next;
    }
    
    spinlock_unlock(&lvm_lock);
    return -1;
}

int lvm_lv_reduce(const char* vg_name, const char* lv_name, u64 size) {
    VALIDATE_STRING(vg_name, 64);
    VALIDATE_STRING(lv_name, 64);
    
    spinlock_lock(&lvm_lock);
    
    lvm_lv_t* lv = logical_volumes;
    while (lv) {
        if (strcmp(lv->vg->name, vg_name) == 0 && strcmp(lv->name, lv_name) == 0) {
            if (size > lv->size) {
                spinlock_unlock(&lvm_lock);
                return -1;
            }
            
            lv->size -= size;
            lv->vg->free_size += size;
            
            spinlock_unlock(&lvm_lock);
            DEBUG_INFO("Logical volume reduced: vg=%s, lv=%s, new_size=%llu MB",
                      vg_name, lv_name, (unsigned long long)(lv->size / (1024*1024)));
            return 0;
        }
        lv = lv->next;
    }
    
    spinlock_unlock(&lvm_lock);
    return -1;
}

lvm_vg_t* lvm_vg_list(void) {
    return volume_groups;
}

lvm_lv_t* lvm_lv_list(const char* vg_name) {
    VALIDATE_STRING_NULL(vg_name, 64);
    
    lvm_lv_t* result = NULL;
    lvm_lv_t** tail = &result;
    
    lvm_lv_t* lv = logical_volumes;
    while (lv) {
        if (strcmp(lv->vg->name, vg_name) == 0) {
            *tail = lv;
            tail = &lv->next;
        }
        lv = lv->next;
    }
    *tail = NULL;
    
    return result;
}
