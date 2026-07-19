#include "test.h"
#include "fs/vfs.h"
#include "fs/ramfs.h"
#include "ext2.h"
#include "ipc.h"
#include "string.h"
#include "memory.h"

/* ============ ramfs / VFS root (real hierarchical filesystem) ============ */

static test_result_t test_ramfs_create_read_write(void) {
    /* create -> write -> read back -> unlink, entirely through the VFS. */
    int fd = vfs_open_fd("/p5file", O_RDWR | O_CREAT | O_TRUNC);
    TEST_ASSERT(fd >= 0);
    const char* msg = "hello p5";
    ssize_t w = vfs_write_fd(fd, msg, 8);
    TEST_ASSERT_EQ(w, 8);
    vfs_lseek_fd(fd, 0, 0);
    char buf[16];
    memset(buf, 0, sizeof(buf));
    ssize_t r = vfs_read_fd(fd, buf, 8);
    TEST_ASSERT_EQ(r, 8);
    TEST_ASSERT(memcmp(buf, msg, 8) == 0);
    vfs_close_fd(fd);

    /* Persist within the boot: reopen and confirm contents. */
    int fd2 = vfs_open_fd("/p5file", O_RDONLY);
    TEST_ASSERT(fd2 >= 0);
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQ(vfs_read_fd(fd2, buf, 8), 8);
    TEST_ASSERT(memcmp(buf, msg, 8) == 0);
    vfs_close_fd(fd2);

    TEST_ASSERT_EQ(vfs_unlink("/p5file"), 0);
    TEST_ASSERT(vfs_open_fd("/p5file", O_RDONLY) < 0);
    return TEST_PASS;
}

static test_result_t test_ramfs_mkdir_getdents(void) {
    TEST_ASSERT_EQ(vfs_mkdir("/p5dir", 0755), 0);
    /* nested file */
    int fd = vfs_open_fd("/p5dir/inner", O_WRONLY | O_CREAT);
    TEST_ASSERT(fd >= 0);
    vfs_write_fd(fd, "x", 1);
    vfs_close_fd(fd);

    /* getdents via path helper: expect ".", "..", "inner". */
    vfs_dent_t ents[8];
    u32 pos = 0;
    int n = vfs_getdents("/p5dir", &pos, ents, 8);
    TEST_ASSERT(n >= 3);
    int seen_dot = 0, seen_dotdot = 0, seen_inner = 0;
    for (int i = 0; i < n; i++) {
        if (strcmp(ents[i].name, ".") == 0) seen_dot = 1;
        if (strcmp(ents[i].name, "..") == 0) seen_dotdot = 1;
        if (strcmp(ents[i].name, "inner") == 0) seen_inner = 1;
    }
    TEST_ASSERT(seen_dot && seen_dotdot && seen_inner);

    /* fd-based getdents on the open directory. */
    int dfd = vfs_open_fd("/p5dir", O_RDONLY);
    TEST_ASSERT(dfd >= 0);
    int m = vfs_getdents_fd(dfd, ents, 8);
    TEST_ASSERT(m >= 3);
    vfs_close_fd(dfd);

    /* cleanup */
    TEST_ASSERT_EQ(vfs_unlink("/p5dir/inner"), 0);
    TEST_ASSERT_EQ(vfs_rmdir("/p5dir"), 0);
    return TEST_PASS;
}

static test_result_t test_ramfs_big_file(void) {
    /* Grow a file well past its initial capacity, verify random offsets. */
    int fd = vfs_open_fd("/big", O_RDWR | O_CREAT | O_TRUNC);
    TEST_ASSERT(fd >= 0);
    char page[512];
    for (int i = 0; i < 512; i++) page[i] = (char)(i & 0xff);
    for (int k = 0; k < 40; k++) {          /* 40 * 512 = 20 KiB */
        TEST_ASSERT_EQ(vfs_write_fd(fd, page, 512), 512);
    }
    vfs_lseek_fd(fd, 512 * 39, 0);
    char rb[512];
    memset(rb, 0, sizeof(rb));
    TEST_ASSERT_EQ(vfs_read_fd(fd, rb, 512), 512);
    TEST_ASSERT(memcmp(rb, page, 512) == 0);
    vfs_close_fd(fd);
    TEST_ASSERT_EQ(vfs_unlink("/big"), 0);
    return TEST_PASS;
}

/* ============ pipes ============ */

static test_result_t test_pipe_roundtrip(void) {
    pipe_t* rd = NULL;
    pipe_t* wr = NULL;
    TEST_ASSERT_EQ(pipe_create(&rd, &wr), 0);
    TEST_ASSERT_NOT_NULL(rd);

    const char* msg = "pipe-data";
    ssize_t w = pipe_write(wr, msg, 9);
    TEST_ASSERT_EQ(w, 9);

    char buf[16];
    memset(buf, 0, sizeof(buf));
    ssize_t r = pipe_read(rd, buf, 9);
    TEST_ASSERT_EQ(r, 9);
    TEST_ASSERT(memcmp(buf, msg, 9) == 0);

    /* Empty pipe reads 0 (non-blocking byte buffer). */
    TEST_ASSERT_EQ(pipe_read(rd, buf, 4), 0);

    pipe_close(rd);
    pipe_close(wr);
    return TEST_PASS;
}

/* ============ ext2 read-write (RAM-backed: bitmap/dir/indirect) ============ */

static test_result_t test_ext2_create_write_read(void) {
    TEST_ASSERT_EQ(ext2_mkfs_ram(1024), 0);      /* 1 MiB image */
    TEST_ASSERT(ext2_fs_create("/hello", 0) > 0);

    const char* data = "ext2 persistent bytes";
    ssize_t w = ext2_fs_pwrite("/hello", 0, data, 21);
    TEST_ASSERT_EQ(w, 21);

    char buf[32];
    memset(buf, 0, sizeof(buf));
    ssize_t r = ext2_fs_pread("/hello", 0, buf, 21);
    TEST_ASSERT_EQ(r, 21);
    TEST_ASSERT(memcmp(buf, data, 21) == 0);

    TEST_ASSERT_EQ(ext2_fs_unlink("/hello"), 0);
    TEST_ASSERT(ext2_fs_pread("/hello", 0, buf, 4) < 0);
    return TEST_PASS;
}

static test_result_t test_ext2_indirect_blocks(void) {
    /* A file > 12 KiB forces single-indirect allocation; > 268 KiB double. */
    TEST_ASSERT_EQ(ext2_mkfs_ram(1024), 0);
    TEST_ASSERT(ext2_fs_create("/wide", 0) > 0);

    u8 pattern[1024];
    for (int i = 0; i < 1024; i++) pattern[i] = (u8)((i * 7 + 3) & 0xff);

    /* Write 20 blocks (20 KiB): blocks 0..11 direct, 12..19 single-indirect. */
    for (u32 b = 0; b < 20; b++) {
        ssize_t w = ext2_fs_pwrite("/wide", (u64)b * 1024, pattern, 1024);
        TEST_ASSERT_EQ(w, 1024);
    }
    /* Read back an indirect-range block (block 15) and verify. */
    u8 rb[1024];
    memset(rb, 0, sizeof(rb));
    ssize_t r = ext2_fs_pread("/wide", 15ULL * 1024, rb, 1024);
    TEST_ASSERT_EQ(r, 1024);
    TEST_ASSERT(memcmp(rb, pattern, 1024) == 0);

    ext2_fs_unlink("/wide");
    return TEST_PASS;
}

static test_result_t test_ext2_dir_and_stat(void) {
    TEST_ASSERT_EQ(ext2_mkfs_ram(512), 0);
    TEST_ASSERT(ext2_fs_mkdir("/sub") > 0);
    TEST_ASSERT(ext2_fs_create("/sub/f1", 0) > 0);
    const char* d = "abc";
    TEST_ASSERT_EQ(ext2_fs_pwrite("/sub/f1", 0, d, 3), 3);
    char buf[8];
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQ(ext2_fs_pread("/sub/f1", 0, buf, 3), 3);
    TEST_ASSERT(memcmp(buf, d, 3) == 0);
    return TEST_PASS;
}

void register_fs_tests(void) {
    test_register("fs", "ramfs_create_read_write", test_ramfs_create_read_write);
    test_register("fs", "ramfs_mkdir_getdents", test_ramfs_mkdir_getdents);
    test_register("fs", "ramfs_big_file", test_ramfs_big_file);
    test_register("fs", "pipe_roundtrip", test_pipe_roundtrip);
    test_register("fs", "ext2_create_write_read", test_ext2_create_write_read);
    test_register("fs", "ext2_indirect_blocks", test_ext2_indirect_blocks);
    test_register("fs", "ext2_dir_and_stat", test_ext2_dir_and_stat);
}
