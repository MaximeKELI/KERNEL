#ifndef SIGNALFD_H
#define SIGNALFD_H

#include "types.h"
#include "signal.h"

/* Signalfd flags */
#define SFD_CLOEXEC     0x00000001
#define SFD_NONBLOCK    0x00000002

/* Signalfd signal info structure */
typedef struct signalfd_siginfo {
    u32 ssi_signo;
    u32 ssi_errno;
    i32 ssi_code;
    u32 ssi_pid;
    u32 ssi_uid;
    i32 ssi_fd;
    u32 ssi_tid;
    u32 ssi_band;
    u32 ssi_overrun;
    u32 ssi_trapno;
    i32 ssi_status;
    i32 ssi_int;
    u64 ssi_ptr;
    u64 ssi_utime;
    u64 ssi_stime;
    u64 ssi_addr;
    u16 ssi_addr_lsb;
    u8 __pad[46];
} signalfd_siginfo_t;

/* Create signalfd */
i32 signalfd(i32 fd, const sigset_t* mask, i32 flags);

/* Read from signalfd */
ssize_t signalfd_read(i32 fd, signalfd_siginfo_t* info);

#endif /* SIGNALFD_H */
