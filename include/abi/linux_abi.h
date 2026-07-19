#ifndef ABI_LINUX_ABI_H
#define ABI_LINUX_ABI_H

#include "types.h"

/*
 * Linux x86-64 ABI translation layer (P6). Dispatches a Linux __NR_* number to
 * this kernel's handlers and returns a value following the Linux convention:
 * a non-negative result on success, or -errno on failure. A process opts into
 * this ABI via process_t.linux_abi (set by exec for VFS-loaded ELF binaries),
 * while the legacy embedded blobs keep the internal numbering.
 */
long linux_syscall(long nr, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6);

/* Individual new syscalls exposed for unit tests. */
long lx_arch_prctl(u64 code, u64 addr);
long lx_set_tid_address(u64 tidptr);
long lx_uname(void* buf);
long lx_getcwd(void* buf, u64 size);

#endif /* ABI_LINUX_ABI_H */
