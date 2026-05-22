#include "test.h"
#include "stdio.h"
#include "appliance_config.h"
#include "tmpfs.h"
#include "ebpf.h"
#include "smp.h"
#include "blk_mq.h"

static int test_config(void) {
    appliance_config_init();
    const appliance_config_t* c = appliance_config_get();
    return c && c->http_port == 80 ? 0 : -1;
}

static int test_tmpfs(void) {
    char buf[16];
    vfs_write_path("/tmp/roadmap_test", "ok", 2);
    ssize_t n = vfs_read_path("/tmp/roadmap_test", buf, sizeof(buf));
    return n == 2 && buf[0] == 'o' ? 0 : -1;
}

static int test_smp(void) {
    return smp_get_cpu_count() >= 1 ? 0 : -1;
}

static int test_ebpf(void) {
    typedef struct { u8 opcode; u8 dst; u8 src; s16 off; s32 imm; } insn_t;
    insn_t prog[] = {
        {0x04, 0, 0, 0, 10},
        {0x95, 0, 0, 0, 0},
    };
    u64 r = ebpf_execute_prog(prog, sizeof(prog), 5);
    return r == 15 ? 0 : -1;
}

void register_roadmap_tests(void) {
    test_register("roadmap_config", test_config);
    test_register("roadmap_tmpfs", test_tmpfs);
    test_register("roadmap_smp", test_smp);
    test_register("roadmap_ebpf", test_ebpf);
}
