#include "md.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "device.h"

#define MAX_MD_DEVICES 32

static md_device_t md_devices[MAX_MD_DEVICES];
static u32 next_md_major = 9;
static u32 next_md_minor = 0;
static spinlock_t md_lock = SPINLOCK_INIT;

int md_init(void) {
    memset(md_devices, 0, sizeof(md_devices));
    DEBUG_INFO("%s", "MD (Multiple Devices/RAID) initialized");
    return 0;
}

int md_create(const char* name, u32 raid_level, void** disks, u32 num_disks) {
    VALIDATE_STRING(name, 64);
    VALIDATE_PTR_RET(disks, -1);
    
    if (num_disks == 0 || num_disks > 16) {
        DEBUG_ERROR("Invalid number of disks: %u", num_disks);
        return -1;
    }
    
    if (raid_level > MD_RAID_LEVEL_6) {
        DEBUG_ERROR("Invalid RAID level: %u", raid_level);
        return -1;
    }
    
    spinlock_lock(&md_lock);
    
    for (u32 i = 0; i < MAX_MD_DEVICES; i++) {
        if (!md_devices[i].active) {
            md_device_t* dev = &md_devices[i];
            strncpy(dev->name, name, sizeof(dev->name) - 1);
            dev->name[sizeof(dev->name) - 1] = '\0';
            dev->major = next_md_major;
            dev->minor = next_md_minor++;
            dev->raid_level = raid_level;
            dev->num_disks = num_disks;
            
            for (u32 j = 0; j < num_disks; j++) {
                dev->disks[j] = disks[j];
            }
            for (u32 j = num_disks; j < 16; j++) {
                dev->disks[j] = NULL;
            }
            
            dev->active = true;
            
            spinlock_unlock(&md_lock);
            DEBUG_INFO("MD device created: name=%s, level=%u, disks=%u", 
                      name, raid_level, num_disks);
            return 0;
        }
    }
    
    spinlock_unlock(&md_lock);
    DEBUG_ERROR("%s", "Maximum MD devices reached");
    return -1;
}

int md_remove(const char* name) {
    VALIDATE_STRING(name, 64);
    
    spinlock_lock(&md_lock);
    
    for (u32 i = 0; i < MAX_MD_DEVICES; i++) {
        if (md_devices[i].active && strcmp(md_devices[i].name, name) == 0) {
            md_devices[i].active = false;
            memset(&md_devices[i], 0, sizeof(md_devices[i]));
            
            spinlock_unlock(&md_lock);
            DEBUG_INFO("MD device removed: name=%s", name);
            return 0;
        }
    }
    
    spinlock_unlock(&md_lock);
    return -1;
}

int md_add_disk(const char* name, void* disk) {
    VALIDATE_STRING(name, 64);
    VALIDATE_PTR_RET(disk, -1);
    
    spinlock_lock(&md_lock);
    
    for (u32 i = 0; i < MAX_MD_DEVICES; i++) {
        if (md_devices[i].active && strcmp(md_devices[i].name, name) == 0) {
            md_device_t* dev = &md_devices[i];
            
            if (dev->num_disks >= 16) {
                spinlock_unlock(&md_lock);
                DEBUG_ERROR("%s", "Maximum disks per MD device reached");
                return -1;
            }
            
            dev->disks[dev->num_disks++] = disk;
            
            spinlock_unlock(&md_lock);
            DEBUG_INFO("Disk added to MD: name=%s", name);
            return 0;
        }
    }
    
    spinlock_unlock(&md_lock);
    return -1;
}

int md_remove_disk(const char* name, void* disk) {
    VALIDATE_STRING(name, 64);
    VALIDATE_PTR_RET(disk, -1);
    
    spinlock_lock(&md_lock);
    
    for (u32 i = 0; i < MAX_MD_DEVICES; i++) {
        if (md_devices[i].active && strcmp(md_devices[i].name, name) == 0) {
            md_device_t* dev = &md_devices[i];
            
            for (u32 j = 0; j < dev->num_disks; j++) {
                if (dev->disks[j] == disk) {
                    /* Shift remaining disks */
                    for (u32 k = j; k < dev->num_disks - 1; k++) {
                        dev->disks[k] = dev->disks[k + 1];
                    }
                    dev->disks[dev->num_disks - 1] = NULL;
                    dev->num_disks--;
                    
                    spinlock_unlock(&md_lock);
                    DEBUG_INFO("Disk removed from MD: name=%s", name);
                    return 0;
                }
            }
            
            spinlock_unlock(&md_lock);
            return -1;
        }
    }
    
    spinlock_unlock(&md_lock);
    return -1;
}

int md_status(const char* name, md_device_t* status) {
    VALIDATE_STRING(name, 64);
    VALIDATE_PTR_RET(status, -1);
    
    spinlock_lock(&md_lock);
    
    for (u32 i = 0; i < MAX_MD_DEVICES; i++) {
        if (md_devices[i].active && strcmp(md_devices[i].name, name) == 0) {
            *status = md_devices[i];
            spinlock_unlock(&md_lock);
            return 0;
        }
    }
    
    spinlock_unlock(&md_lock);
    return -1;
}
