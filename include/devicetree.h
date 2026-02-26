#ifndef DEVICETREE_H
#define DEVICETREE_H

#include "types.h"

/* Device tree node */
typedef struct dt_node {
    char name[64];
    char* compatible;
    u32 phandle;
    void* properties;
    struct dt_node* parent;
    struct dt_node* child;
    struct dt_node* sibling;
} dt_node_t;

/* Device tree property */
typedef struct dt_property {
    char name[32];
    u32 length;
    void* value;
    struct dt_property* next;
} dt_property_t;

/* Initialize device tree */
void devicetree_init(void* dtb);

/* Find node by path */
dt_node_t* dt_find_node(const char* path);

/* Find node by compatible */
dt_node_t* dt_find_compatible(const char* compatible);

/* Get property */
void* dt_get_property(dt_node_t* node, const char* name, u32* length);

/* Get property value as u32 */
u32 dt_get_property_u32(dt_node_t* node, const char* name, u32 default_val);

#endif /* DEVICETREE_H */
