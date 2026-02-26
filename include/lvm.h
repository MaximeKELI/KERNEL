#ifndef LVM_H
#define LVM_H

#include "types.h"

/* LVM volume group */
typedef struct lvm_vg {
    char name[64];
    void* physical_volumes[32];
    u32 pv_count;
    u64 total_size;
    u64 free_size;
    struct lvm_vg* next;
} lvm_vg_t;

/* LVM logical volume */
typedef struct lvm_lv {
    char name[64];
    lvm_vg_t* vg;
    u64 size;
    u32 segments;
    void* segments_data;
    struct lvm_lv* next;
} lvm_lv_t;

/* Initialize LVM */
int lvm_init(void);

/* Create volume group */
int lvm_vg_create(const char* name, void** pvs, u32 pv_count);

/* Remove volume group */
int lvm_vg_remove(const char* name);

/* Create logical volume */
int lvm_lv_create(const char* vg_name, const char* lv_name, u64 size);

/* Remove logical volume */
int lvm_lv_remove(const char* vg_name, const char* lv_name);

/* Extend logical volume */
int lvm_lv_extend(const char* vg_name, const char* lv_name, u64 size);

/* Reduce logical volume */
int lvm_lv_reduce(const char* vg_name, const char* lv_name, u64 size);

/* List volume groups */
lvm_vg_t* lvm_vg_list(void);

/* List logical volumes */
lvm_lv_t* lvm_lv_list(const char* vg_name);

#endif /* LVM_H */
