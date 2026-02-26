#ifndef THERMAL_H
#define THERMAL_H

#include "types.h"

/* Thermal zones */
typedef struct thermal_zone {
    u32 id;
    char name[64];
    i32 temperature;
    i32 trip_points[8];
    u32 trip_count;
    bool active;
    struct thermal_zone* next;
} thermal_zone_t;

/* Thermal cooling device */
typedef struct thermal_cooling {
    u32 id;
    char name[64];
    u32 max_state;
    u32 cur_state;
    bool active;
    struct thermal_cooling* next;
} thermal_cooling_t;

/* Initialize thermal management */
int thermal_init(void);

/* Register thermal zone */
int thermal_zone_register(thermal_zone_t* zone);

/* Unregister thermal zone */
int thermal_zone_unregister(u32 zone_id);

/* Get temperature */
int thermal_get_temp(u32 zone_id, i32* temp);

/* Set trip point */
int thermal_set_trip_point(u32 zone_id, u32 trip_idx, i32 temp);

/* Register cooling device */
int thermal_cooling_register(thermal_cooling_t* cooling);

/* Set cooling state */
int thermal_cooling_set_state(u32 cooling_id, u32 state);

/* Get cooling state */
int thermal_cooling_get_state(u32 cooling_id, u32* state);

#endif /* THERMAL_H */
