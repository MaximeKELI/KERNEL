#include "ebpf.h"
#include "bpf.h"
#include "stdio.h"
#include "memory.h"
#include "string.h"
#include "spinlock.h"

#define EBPF_MAX_INSN 64

typedef struct {
    u8 opcode;
    u8 dst;
    u8 src;
    i16 off;
    i32 imm;
} ebpf_insn_t;

static bool ebpf_enabled = false;

static u64 ebpf_run(const ebpf_insn_t* insns, u32 count, u64 r0) {
    u64 regs[11] = {0};
    regs[0] = r0;

    for (u32 i = 0; i < count && i < EBPF_MAX_INSN; i++) {
        const ebpf_insn_t* in = &insns[i];
        switch (in->opcode) {
        case 0x04: /* ADD imm */
            regs[in->dst] += (u64)in->imm;
            break;
        case 0x14: /* ADD reg */
            regs[in->dst] += regs[in->src];
            break;
        case 0xA4: /* XOR imm */
            regs[in->dst] ^= (u64)in->imm;
            break;
        case 0x95: /* EXIT */
            return regs[0];
        default:
            break;
        }
    }
    return regs[0];
}

void ebpf_init(void) {
    ebpf_enabled = true;
    printk("[eBPF] minimal interpreter ready\n");
}

u64 ebpf_execute_prog(void* prog_data, size_t size, u64 arg0) {
    if (!ebpf_enabled || !prog_data || size < sizeof(ebpf_insn_t)) {
        return 0;
    }
    u32 n = (u32)(size / sizeof(ebpf_insn_t));
    return ebpf_run((const ebpf_insn_t*)prog_data, n, arg0);
}
