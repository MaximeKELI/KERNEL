#include "thermal.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "acpi.h"

static thermal_zone_t* thermal_zones = NULL;
static thermal_cooling_t* cooling_devices = NULL;
static u32 next_zone_id = 0;
static u32 next_cooling_id = 0;
static spinlock_t thermal_lock = SPINLOCK_INIT;

int thermal_init(void) {
    DEBUG_INFO("%s", "Thermal management initialized");
    return 0;
}

int thermal_zone_register(thermal_zone_t* zone) {
    VALIDATE_PTR_RET(zone, -1);
    VALIDATE_STRING(zone->name, 64);
    
    spinlock_lock(&thermal_lock);
    
    zone->id = next_zone_id++;
    zone->active = true;
    zone->next = thermal_zones;
    thermal_zones = zone;
    
    /* Read initial temperature from ACPI */
    u32 temp = 0;
    acpi_get_temperature(&temp);
    zone->temperature = (i32)temp;
    
    spinlock_unlock(&thermal_lock);
    
    DEBUG_INFO("Thermal zone registered: id=%u, name=%s, temp=%d°C", 
              zone->id, zone->name, zone->temperature);
    return 0;
}

int thermal_zone_unregister(u32 zone_id) {
    spinlock_lock(&thermal_lock);
    
    thermal_zone_t** prev = &thermal_zones;
    thermal_zone_t* zone = thermal_zones;
    
    while (zone) {
        if (zone->id == zone_id) {
            *prev = zone->next;
            kfree(zone);
            spinlock_unlock(&thermal_lock);
            DEBUG_INFO("Thermal zone unregistered: id=%u", zone_id);
            return 0;
        }
        prev = &zone->next;
        zone = zone->next;
    }
    
    spinlock_unlock(&thermal_lock);
    return -1;
}

int thermal_get_temp(u32 zone_id, i32* temp) {
    VALIDATE_PTR_RET(temp, -1);
    
    spinlock_lock(&thermal_lock);
    
    thermal_zone_t* zone = thermal_zones;
    while (zone) {
        if (zone->id == zone_id && zone->active) {
            *temp = zone->temperature;
            spinlock_unlock(&thermal_lock);
            return 0;
        }
        zone = zone->next;
    }
    
    spinlock_unlock(&thermal_lock);
    return -1;
}

int thermal_set_trip_point(u32 zone_id, u32 trip_idx, i32 temp) {
    if (trip_idx >= 8) {
        return -1;
    }
    
    spinlock_lock(&thermal_lock);
    
    thermal_zone_t* zone = thermal_zones;
    while (zone) {
        if (zone->id == zone_id && zone->active) {
            zone->trip_points[trip_idx] = temp;
            if (trip_idx >= zone->trip_count) {
                zone->trip_count = trip_idx + 1;
            }
            spinlock_unlock(&thermal_lock);
            DEBUG_INFO("Trip point set: zone=%u, trip=%u, temp=%d°C", 
                      zone_id, trip_idx, temp);
            return 0;
        }
        zone = zone->next;
    }
    
    spinlock_unlock(&thermal_lock);
    return -1;
}

int thermal_cooling_register(thermal_cooling_t* cooling) {
    VALIDATE_PTR_RET(cooling, -1);
    VALIDATE_STRING(cooling->name, 64);
    
    spinlock_lock(&thermal_lock);
    
    cooling->id = next_cooling_id++;
    cooling->active = true;
    cooling->next = cooling_devices;
    cooling_devices = cooling;
    
    spinlock_unlock(&thermal_lock);
    
    DEBUG_INFO("Cooling device registered: id=%u, name=%s", cooling->id, cooling->name);
    return 0;
}

int thermal_cooling_set_state(u32 cooling_id, u32 state) {
    spinlock_lock(&thermal_lock);
    
    thermal_cooling_t* cooling = cooling_devices;
    while (cooling) {
        if (cooling->id == cooling_id && cooling->active) {
            if (state > cooling->max_state) {
                spinlock_unlock(&thermal_lock);
                return -1;
            }
            cooling->cur_state = state;
            spinlock_unlock(&thermal_lock);
            DEBUG_INFO("Cooling state set: id=%u, state=%u", cooling_id, state);
            return 0;
        }
        cooling = cooling->next;
    }
    
    spinlock_unlock(&thermal_lock);
    return -1;
}

int thermal_cooling_get_state(u32 cooling_id, u32* state) {
    VALIDATE_PTR_RET(state, -1);
    
    spinlock_lock(&thermal_lock);
    
    thermal_cooling_t* cooling = cooling_devices;
    while (cooling) {
        if (cooling->id == cooling_id && cooling->active) {
            *state = cooling->cur_state;
            spinlock_unlock(&thermal_lock);
            return 0;
        }
        cooling = cooling->next;
    }
    
    spinlock_unlock(&thermal_lock);
    return -1;
}
