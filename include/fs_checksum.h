#ifndef FS_CHECKSUM_H
#define FS_CHECKSUM_H

#include "types.h"

void fs_checksum_init(void);
u32 fs_checksum_crc32(const void* data, size_t len);
u32 fs_checksum_verify(const void* data, size_t len, u32 expected);

#endif /* FS_CHECKSUM_H */
