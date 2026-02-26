#include "dm.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "device.h"

#define MAX_DM_DEVICES 64

static dm_device_t dm_devices[MAX_DM_DEVICES];
static u32 next_dm_major = 250;
static u32 next_dm_minor = 0;
static spinlock_t dm_lock = SPINLOCK_INIT;

int dm_init(void) {
    memset(dm_devices, 0, sizeof(dm_devices));
    DEBUG_INFO("%s", "Device Mapper initialized");
    return 0;
}

int dm_create(const char* name, u32 major, u32 minor) {
    VALIDATE_STRING(name, 64);
    
    spinlock_lock(&dm_lock);
    
    for (u32 i = 0; i < MAX_DM_DEVICES; i++) {
        if (!dm_devices[i].active) {
            dm_device_t* dev = &dm_devices[i];
            strncpy(dev->name, name, sizeof(dev->name) - 1);
            dev->name[sizeof(dev->name) - 1] = '\0';
            dev->major = (major == 0) ? next_dm_major : major;
            dev->minor = (minor == 0) ? next_dm_minor++ : minor;
            dev->table = NULL;
            dev->active = true;
            
            spinlock_unlock(&dm_lock);
            DEBUG_INFO("Device Mapper device created: name=%s, major=%u, minor=%u", 
                      name, dev->major, dev->minor);
            return 0;
        }
    }
    
    spinlock_unlock(&dm_lock);
    DEBUG_ERROR("%s", "Maximum DM devices reached");
    return -1;
}

int dm_remove(const char* name) {
    VALIDATE_STRING(name, 64);
    
    spinlock_lock(&dm_lock);
    
    for (u32 i = 0; i < MAX_DM_DEVICES; i++) {
        if (dm_devices[i].active && strcmp(dm_devices[i].name, name) == 0) {
            dm_device_t* dev = &dm_devices[i];
            
            /* Free table */
            dm_table_entry_t* entry = dev->table;
            while (entry) {
                dm_table_entry_t* next = entry->next;
                if (entry->target_data) {
                    kfree(entry->target_data);
                }
                kfree(entry);
                entry = next;
            }
            
            dev->active = false;
            memset(dev, 0, sizeof(*dev));
            
            spinlock_unlock(&dm_lock);
            DEBUG_INFO("Device Mapper device removed: name=%s", name);
            return 0;
        }
    }
    
    spinlock_unlock(&dm_lock);
    return -1;
}

int dm_table_load(const char* name, dm_table_entry_t* table) {
    VALIDATE_STRING(name, 64);
    VALIDATE_PTR_RET(table, -1);
    
    spinlock_lock(&dm_lock);
    
    for (u32 i = 0; i < MAX_DM_DEVICES; i++) {
        if (dm_devices[i].active && strcmp(dm_devices[i].name, name) == 0) {
            dm_device_t* dev = &dm_devices[i];
            
            /* Free old table */
            dm_table_entry_t* old = dev->table;
            while (old) {
                dm_table_entry_t* next = old->next;
                if (old->target_data) {
                    kfree(old->target_data);
                }
                kfree(old);
                old = next;
            }
            
            /* Copy new table */
            dev->table = NULL;
            dm_table_entry_t** dst = &dev->table;
            dm_table_entry_t* src = table;
            
            while (src) {
                *dst = (dm_table_entry_t*)kmalloc(sizeof(dm_table_entry_t));
                if (!*dst) {
                    spinlock_unlock(&dm_lock);
                    return -1;
                }
                **dst = *src;
                (*dst)->target_data = NULL;
                if (src->target_data) {
                    (*dst)->target_data = kmalloc(256); /* Would use actual size */
                    if ((*dst)->target_data) {
                        memcpy((*dst)->target_data, src->target_data, 256);
                    }
                }
                (*dst)->next = NULL;
                dst = &(*dst)->next;
                src = src->next;
            }
            
            spinlock_unlock(&dm_lock);
            DEBUG_INFO("Device Mapper table loaded: name=%s", name);
            return 0;
        }
    }
    
    spinlock_unlock(&dm_lock);
    return -1;
}

int dm_suspend(const char* name) {
    VALIDATE_STRING(name, 64);
    
    DEBUG_INFO("Device Mapper device suspended: name=%s", name);
    return 0;
}

int dm_resume(const char* name) {
    VALIDATE_STRING(name, 64);
    
    DEBUG_INFO("Device Mapper device resumed: name=%s", name);
    return 0;
}

int dm_info(const char* name, dm_device_t* info) {
    VALIDATE_STRING(name, 64);
    VALIDATE_PTR_RET(info, -1);
    
    spinlock_lock(&dm_lock);
    
    for (u32 i = 0; i < MAX_DM_DEVICES; i++) {
        if (dm_devices[i].active && strcmp(dm_devices[i].name, name) == 0) {
            *info = dm_devices[i];
            spinlock_unlock(&dm_lock);
            return 0;
        }
    }
    
    spinlock_unlock(&dm_lock);
    return -1;
}
