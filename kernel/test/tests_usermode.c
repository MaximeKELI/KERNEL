#include "test.h"
#include "process.h"
#include "scheduler.h"
#include "memory.h"
#include "exec.h"
#include "tss.h"
#include "stdio.h"

/*
 * Real ring-3 round trip.
 *
 * We hand-assemble a tiny userspace program directly into a USER-mapped page,
 * drop to ring 3 via iretq, and let it exercise the whole userspace path:
 *
 *   write(1, "ring3 ok\n", 9)   -> SYSCALL into the kernel, SYSRET back to ring 3
 *   busy loop                   -> the PIT preempts us *in ring 3*, proving the
 *                                  TSS (rsp0) catches the ring3->ring0 interrupt
 *   exit(42)                    -> SYSCALL -> kthread_exit(42)
 *
 * The parent thread_join()s the probe and checks the exit status is 42, which
 * can only happen if code executed in ring 3, both syscalls dispatched, and no
 * userspace interrupt triple-faulted.
 */

#define SYS_EXIT   0
#define SYS_WRITE  1

/*
 * User VAs are placed ABOVE the PMM-managed physical range (512 MiB) and backed
 * by freshly-allocated physical pages. This keeps the userspace mapping from
 * colliding with the kernel heap / kthread stacks, which live in low identity-
 * mapped physical memory (0x400000 would land right inside the kernel heap).
 */
#define U_CODE_VA    0x30000000UL
#define U_STACK_TOP  0x30011000UL   /* stack page: [0x30010000, 0x30011000) */

static void build_user_blob(u8* code, u64 code_va) {
    size_t o = 0;
    size_t msg_imm_off, len_off, msg_off;
    static const char msg[] = "ring3 ok\n";

    /* mov eax, SYS_WRITE */
    code[o++] = 0xB8; code[o++] = SYS_WRITE; code[o++] = 0; code[o++] = 0; code[o++] = 0;
    /* mov edi, 1  (stdout) */
    code[o++] = 0xBF; code[o++] = 1; code[o++] = 0; code[o++] = 0; code[o++] = 0;
    /* mov rsi, imm64  (message address, patched below) */
    code[o++] = 0x48; code[o++] = 0xBE; msg_imm_off = o; o += 8;
    /* mov edx, imm32  (length, patched below) */
    code[o++] = 0xBA; len_off = o; o += 4;
    /* syscall  (write) -> returns to ring 3 via SYSRET */
    code[o++] = 0x0F; code[o++] = 0x05;

    /* mov ecx, 0x02000000  (busy-loop counter) */
    code[o++] = 0xB9; code[o++] = 0x00; code[o++] = 0x00; code[o++] = 0x00; code[o++] = 0x02;
    /* spin: sub ecx, 1 ; jnz spin  -> stays in ring 3 long enough for the PIT to
     * fire *while CPL=3*, which can only be handled if the TSS.rsp0 stack switch
     * works (otherwise the ring3->ring0 interrupt triple-faults). */
    code[o++] = 0x83; code[o++] = 0xE9; code[o++] = 0x01;
    code[o++] = 0x75; code[o++] = 0xFB;

    /* mov eax, SYS_EXIT */
    code[o++] = 0xB8; code[o++] = SYS_EXIT; code[o++] = 0; code[o++] = 0; code[o++] = 0;
    /* mov edi, 42  (exit code) */
    code[o++] = 0xBF; code[o++] = 42; code[o++] = 0; code[o++] = 0; code[o++] = 0;
    /* syscall  (exit) */
    code[o++] = 0x0F; code[o++] = 0x05;
    /* jmp $  (never reached; guards against a returning exit) */
    code[o++] = 0xEB; code[o++] = 0xFE;

    msg_off = o;
    for (size_t i = 0; i < sizeof(msg) - 1; i++) {
        code[o++] = (u8)msg[i];
    }
    *(u64*)(code + msg_imm_off) = code_va + msg_off;
    *(u32*)(code + len_off) = (u32)(sizeof(msg) - 1);
}

static void user_probe_thread(void* arg) {
    (void)arg;

    /* Back the user code/stack with dedicated physical pages (never the kernel's
     * heap or our own kernel stack), mapped USER at a collision-free VA. */
    void* code_phys = pmm_alloc(1);
    void* stack_phys = pmm_alloc(1);
    if (!code_phys || !stack_phys) {
        kthread_exit(-1);
    }
    vmm_map_page((void*)U_CODE_VA, code_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
    vmm_map_page((void*)(U_STACK_TOP - PAGE_SIZE), stack_phys,
                 PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

    build_user_blob((u8*)U_CODE_VA, U_CODE_VA);

    /* Ring3 traps/syscalls must land on this thread's kernel stack. */
    process_t* cur = process_current();
    u64 ktop = ((u64)cur->stack_base + cur->stack_size) & ~0xFULL;
    tss_set_rsp0(ktop);

    /* Enter ring 3; the only way back is SYS_EXIT -> kthread_exit(). */
    exec_iretq_user(U_CODE_VA, U_STACK_TOP - 16, 0x202);
}

static test_result_t test_usermode_ring3_roundtrip(void) {
    process_t* p = kthread_run(user_probe_thread, NULL, 32 * 1024);
    TEST_ASSERT_NOT_NULL(p);

    int status = -1;
    TEST_ASSERT_EQ(thread_join(p, &status), 0);
    TEST_ASSERT_EQ(status, 42);   /* exit(42) reached => full ring3 round trip */

    return TEST_PASS;
}

/*
 * Real userland ELF round trip: load the embedded /sh binary through the actual
 * exec path (ELF parse -> PT_LOAD mapping as USER pages -> iretq to e_entry) and
 * let it run in ring 3. /sh prints its banner, does getpid, read(0) (returns 0)
 * then exit(0), so it terminates cleanly and we can join it.
 */
static void sh_exec_thread(void* arg) {
    (void)arg;
    exec_run_path("/sh");
    kthread_exit(-1);   /* only reached if exec failed */
}

static test_result_t test_usermode_real_elf(void) {
    process_t* p = kthread_run(sh_exec_thread, NULL, 32 * 1024);
    TEST_ASSERT_NOT_NULL(p);

    int status = -1;
    TEST_ASSERT_EQ(thread_join(p, &status), 0);
    TEST_ASSERT_EQ(status, 0);   /* /sh exit(0) => real ELF ran in ring 3 */

    return TEST_PASS;
}

void register_usermode_tests(void) {
    test_register("usermode", "ring3_roundtrip", test_usermode_ring3_roundtrip);
    test_register("usermode", "real_elf_sh", test_usermode_real_elf);
}
