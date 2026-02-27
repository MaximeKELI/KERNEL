#ifndef ALSA_H
#define ALSA_H

#include "types.h"

/* ALSA Device */
typedef struct alsa_device alsa_device_t;

/* Initialize ALSA */
void alsa_init(void);

/* Allocate/free ALSA device */
alsa_device_t* alsa_alloc_device(void);
void alsa_free_device(alsa_device_t* dev);

/* Set audio parameters */
int alsa_set_params(alsa_device_t* dev, u32 sample_rate, u32 channels, u32 bits_per_sample);

/* Read/write audio data */
int alsa_write(alsa_device_t* dev, void* data, size_t len);
int alsa_read(alsa_device_t* dev, void* buffer, size_t len);

/* Find device by name */
alsa_device_t* alsa_find_device(const char* name);

/* Get device count */
u32 alsa_get_device_count(void);

#endif /* ALSA_H */
