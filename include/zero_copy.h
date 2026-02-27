#ifndef ZERO_COPY_H
#define ZERO_COPY_H

#include "types.h"
#include "net.h"

/* Zero-copy buffer */
typedef struct zero_copy_buffer zero_copy_buffer_t;

/* Initialize zero-copy networking */
void zero_copy_init(void);

/* Allocate/free zero-copy buffer */
zero_copy_buffer_t* zero_copy_alloc(size_t size);
void zero_copy_free(zero_copy_buffer_t* zcb);

/* Get data pointer */
void* zero_copy_get_data(zero_copy_buffer_t* zcb);

/* Send/receive using zero-copy */
int zero_copy_send(socket_t* sock, zero_copy_buffer_t* zcb, size_t len);
int zero_copy_recv(socket_t* sock, zero_copy_buffer_t* zcb, size_t len);

/* Get pool statistics */
u32 zero_copy_get_pool_count(void);

#endif /* ZERO_COPY_H */
