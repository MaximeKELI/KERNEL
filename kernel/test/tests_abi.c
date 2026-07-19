#include "test.h"
#include "abi/linux_abi.h"
#include "abi/linux_syscall.h"
#include "abi/errno.h"
#include "exec.h"
#include "vdso.h"
#include "process.h"
#include "scheduler.h"
#include "fs/vfs.h"
#include "string.h"
#include "io.h"

#define IA32_FS_BASE 0xC0000100

/* Linux __NR_getpid returns the same pid as the internal getpid. */
static test_result_t test_abi_getpid(void) {
    long a = linux_syscall(__NR_getpid, 0, 0, 0, 0, 0, 0);
    TEST_ASSERT(a >= 0);
    return TEST_PASS;
}

/* write/lseek/read of a ramfs fd through the Linux ABI (copy-free fd paths). */
static test_result_t test_abi_write_read(void) {
    int fd = vfs_open_fd("/abitest", O_RDWR | O_CREAT | O_TRUNC);
    TEST_ASSERT(fd >= 0);
    const char* msg = "linux-abi";
    long w = linux_syscall(__NR_write, (u64)fd, (u64)msg, 9, 0, 0, 0);
    TEST_ASSERT_EQ(w, 9);
    long off = linux_syscall(__NR_lseek, (u64)fd, 0, 0 /* SEEK_SET */, 0, 0, 0);
    TEST_ASSERT_EQ(off, 0);
    char buf[16];
    memset(buf, 0, sizeof(buf));
    long r = linux_syscall(__NR_read, (u64)fd, (u64)buf, 9, 0, 0, 0);
    TEST_ASSERT_EQ(r, 9);
    TEST_ASSERT(memcmp(buf, msg, 9) == 0);
    vfs_close_fd(fd);
    vfs_unlink("/abitest");
    return TEST_PASS;
}

/* A bad fd yields the Linux -errno convention, not a bare -1. */
static test_result_t test_abi_errno(void) {
    char buf[4];
    long r = linux_syscall(__NR_read, 999, (u64)buf, 4, 0, 0, 0);
    TEST_ASSERT_EQ(r, -EBADF);
    long e = linux_syscall(4242 /* unknown */, 0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQ(e, -ENOSYS);
    return TEST_PASS;
}

/* arch_prctl(ARCH_SET_FS) programs the %fs base MSR (TLS). */
static test_result_t test_abi_arch_prctl_tls(void) {
    u64 tls = 0xdead0000ULL;
    long r = linux_syscall(__NR_arch_prctl, ARCH_SET_FS, tls, 0, 0, 0, 0);
    TEST_ASSERT_EQ(r, 0);
    TEST_ASSERT_EQ(rdmsr(IA32_FS_BASE), tls);
    process_t* p = process_current();
    if (p) {
        TEST_ASSERT_EQ(p->fs_base, tls);
    }
    return TEST_PASS;
}

/* set_tid_address returns the tid and records the clear pointer. */
static test_result_t test_abi_set_tid_address(void) {
    long r = linux_syscall(__NR_set_tid_address, 0x1000, 0, 0, 0, 0, 0);
    TEST_ASSERT(r >= 0);
    process_t* p = process_current();
    if (p) {
        TEST_ASSERT_EQ(p->clear_child_tid, 0x1000ULL);
    }
    return TEST_PASS;
}

/* Static-PIE relocation: R_X86_64_RELATIVE patches *(bias+off) = bias+addend. */
static test_result_t test_abi_pie_reloc(void) {
    /* Target array we relocate a pointer inside of. */
    u64 target[4] = {0, 0, 0, 0};
    u64 bias = (u64)target;

    /* Build one Elf64_Rela: r_offset=8, r_info=R_X86_64_RELATIVE(8), addend=16. */
    struct __attribute__((packed)) { u64 off; u64 info; i64 add; } rela;
    rela.off = 8;                 /* patch target[1] */
    rela.info = 8;                /* R_X86_64_RELATIVE */
    rela.add = 16;                /* -> bias + 16 == &target[2] */

    exec_apply_rela(bias, (u64)&rela, sizeof(rela), sizeof(rela));
    TEST_ASSERT_EQ(target[1], bias + 16);
    return TEST_PASS;
}

/* The vDSO is a real ELF-magic'd executable page with the clock_gettime stub. */
static test_result_t test_abi_vdso_page(void) {
    TEST_ASSERT(vdso_ehdr_addr() != 0);
    TEST_ASSERT_EQ(vdso_selftest(), 0);
    return TEST_PASS;
}

void register_abi_tests(void) {
    test_register("abi", "getpid", test_abi_getpid);
    test_register("abi", "write_read", test_abi_write_read);
    test_register("abi", "errno", test_abi_errno);
    test_register("abi", "arch_prctl_tls", test_abi_arch_prctl_tls);
    test_register("abi", "set_tid_address", test_abi_set_tid_address);
    test_register("abi", "pie_reloc", test_abi_pie_reloc);
    test_register("abi", "vdso_page", test_abi_vdso_page);
}
