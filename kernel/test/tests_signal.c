#include "test.h"
#include "process.h"
#include "scheduler.h"
#include "memory.h"
#include "exec.h"
#include "tss.h"
#include "stdio.h"

/*
 * Real ring-3 signal delivery tests. Each hand-assembles a tiny userspace
 * program into a USER-mapped page, drops to ring 3, and exercises one branch of
 * the signal path. The kthread's exit status (via thread_join) is the oracle.
 *
 * Code page layout (single 4 KiB page at SCODE):
 *   0x000  _start
 *   0x300  signal handler
 *   0x400  struct sigaction scratch
 *   0x440  sigset scratch
 *   0x480  counter scratch
 */
#define SCODE     0x30000000UL
#define SSTACKTOP 0x30011000UL
#define SHANDLER  (SCODE + 0x300)
#define SACT      (SCODE + 0x400)
#define SSET      (SCODE + 0x440)
#define SCOUNTER  (SCODE + 0x480)

#define S_SYS_EXIT           0
#define S_SYS_GETPID         26
#define S_SYS_RT_SIGACTION   30
#define S_SYS_RT_SIGPROCMASK 31
#define S_SYS_KILL           32
#define S_SYS_FORK           5
#define S_SYS_WAIT           7
#define S_SIGUSR1            10

static void put32(u8* p, u32 v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static void put64(u8* p, u64 v) { for (int i = 0; i < 8; i++) p[i] = (u8)(v >> (8 * i)); }

/* inc dword [SCOUNTER] ; ret */
static void emit_handler_inc(u8* c) {
    size_t o = 0x300;
    c[o++] = 0xFF; c[o++] = 0x04; c[o++] = 0x25; put32(&c[o], (u32)SCOUNTER); o += 4;
    c[o++] = 0xC3;
}

/* mov edi, val ; xor eax,eax(SYS_EXIT) ; syscall ; jmp $ */
static void emit_handler_exit(u8* c, int val) {
    size_t o = 0x300;
    c[o++] = 0xBF; put32(&c[o], (u32)val); o += 4;
    c[o++] = 0x31; c[o++] = 0xC0;
    c[o++] = 0x0F; c[o++] = 0x05;
    c[o++] = 0xEB; c[o++] = 0xFE;
}

/* emit rt_sigaction(SIGUSR1, {SHANDLER,0,0}, NULL) at offset *po */
static void emit_install_handler(u8* c, size_t* po) {
    size_t o = *po;
    /* mov rax, SHANDLER ; mov [SACT], rax */
    c[o++] = 0x48; c[o++] = 0xB8; put64(&c[o], SHANDLER); o += 8;
    c[o++] = 0x48; c[o++] = 0x89; c[o++] = 0x04; c[o++] = 0x25; put32(&c[o], (u32)SACT); o += 4;
    /* xor eax,eax ; mov [SACT+8],rax ; mov [SACT+16],rax */
    c[o++] = 0x31; c[o++] = 0xC0;
    c[o++] = 0x48; c[o++] = 0x89; c[o++] = 0x04; c[o++] = 0x25; put32(&c[o], (u32)(SACT + 8)); o += 4;
    c[o++] = 0x48; c[o++] = 0x89; c[o++] = 0x04; c[o++] = 0x25; put32(&c[o], (u32)(SACT + 16)); o += 4;
    /* eax=30, edi=10, rsi=SACT, edx=0 ; syscall */
    c[o++] = 0xB8; put32(&c[o], S_SYS_RT_SIGACTION); o += 4;
    c[o++] = 0xBF; put32(&c[o], S_SIGUSR1); o += 4;
    c[o++] = 0x48; c[o++] = 0xBE; put64(&c[o], SACT); o += 8;
    c[o++] = 0x31; c[o++] = 0xD2;
    c[o++] = 0x0F; c[o++] = 0x05;
    *po = o;
}

/* mov eax,[SCOUNTER] ; mov edi,eax ; xor eax,eax ; syscall (exit(counter)) */
static void emit_exit_counter(u8* c, size_t* po) {
    size_t o = *po;
    c[o++] = 0x8B; c[o++] = 0x04; c[o++] = 0x25; put32(&c[o], (u32)SCOUNTER); o += 4;
    c[o++] = 0x89; c[o++] = 0xC7;
    c[o++] = 0x31; c[o++] = 0xC0;
    c[o++] = 0x0F; c[o++] = 0x05;
    c[o++] = 0xEB; c[o++] = 0xFE;
    *po = o;
}

/* eax=getpid;syscall; mov rbx,rax; eax=kill,rdi=rbx,esi=sig;syscall */
static void emit_kill_self(u8* c, size_t* po) {
    size_t o = *po;
    c[o++] = 0xB8; put32(&c[o], S_SYS_GETPID); o += 4;
    c[o++] = 0x0F; c[o++] = 0x05;
    c[o++] = 0x48; c[o++] = 0x89; c[o++] = 0xC3;       /* mov rbx,rax */
    c[o++] = 0xB8; put32(&c[o], S_SYS_KILL); o += 4;
    c[o++] = 0x48; c[o++] = 0x89; c[o++] = 0xDF;       /* mov rdi,rbx */
    c[o++] = 0xBE; put32(&c[o], S_SIGUSR1); o += 4;    /* mov esi,10 */
    c[o++] = 0x0F; c[o++] = 0x05;
    *po = o;
}

/* --- Test 1: install handler, kill self, handler runs and returns --- */
static void build_handler_return(u8* c) {
    emit_handler_inc(c);
    size_t o = 0;
    emit_install_handler(c, &o);
    emit_kill_self(c, &o);
    emit_exit_counter(c, &o);       /* expect counter == 1 */
}

/* --- Test 3: block SIGUSR1, kill self (deferred), then unblock (delivered) --- */
static void build_mask(u8* c) {
    emit_handler_inc(c);
    size_t o = 0;
    emit_install_handler(c, &o);
    /* build sigset {SIGUSR1} at SSET: qword = 1<<10 */
    c[o++] = 0xB8; put32(&c[o], 1u << S_SIGUSR1); o += 4;   /* mov eax, 0x400 */
    c[o++] = 0x48; c[o++] = 0x89; c[o++] = 0x04; c[o++] = 0x25; put32(&c[o], (u32)SSET); o += 4;
    /* rt_sigprocmask(SIG_BLOCK=0, SSET, NULL) */
    c[o++] = 0xB8; put32(&c[o], S_SYS_RT_SIGPROCMASK); o += 4;
    c[o++] = 0x31; c[o++] = 0xFF;                          /* xor edi,edi (SIG_BLOCK) */
    c[o++] = 0x48; c[o++] = 0xBE; put64(&c[o], SSET); o += 8;
    c[o++] = 0x31; c[o++] = 0xD2;                          /* xor edx,edx */
    c[o++] = 0x0F; c[o++] = 0x05;
    /* kill self: handler must NOT run yet (blocked) */
    emit_kill_self(c, &o);
    /* if counter != 0 -> exit(200) (masking failed) */
    c[o++] = 0x8B; c[o++] = 0x04; c[o++] = 0x25; put32(&c[o], (u32)SCOUNTER); o += 4; /* mov eax,[counter] */
    c[o++] = 0x85; c[o++] = 0xC0;                          /* test eax,eax */
    c[o++] = 0x74; size_t jz = o; c[o++] = 0x00;           /* jz ok */
    c[o++] = 0xBF; put32(&c[o], 200); o += 4;              /* mov edi,200 */
    c[o++] = 0x31; c[o++] = 0xC0; c[o++] = 0x0F; c[o++] = 0x05; /* exit(200) */
    c[jz] = (u8)(o - (jz + 1));
    /* unblock -> pending SIGUSR1 delivered on syscall return */
    c[o++] = 0xB8; put32(&c[o], S_SYS_RT_SIGPROCMASK); o += 4;
    c[o++] = 0xBF; put32(&c[o], 1); o += 4;                /* SIG_UNBLOCK */
    c[o++] = 0x48; c[o++] = 0xBE; put64(&c[o], SSET); o += 8;
    c[o++] = 0x31; c[o++] = 0xD2;
    c[o++] = 0x0F; c[o++] = 0x05;
    emit_exit_counter(c, &o);       /* expect counter == 1 */
}

/* --- Test 4: fork; parent kills child; child handler exit(77) --- */
static void build_kill_cross(u8* c) {
    emit_handler_exit(c, 77);
    size_t o = 0;
    /* fork */
    c[o++] = 0xB8; put32(&c[o], S_SYS_FORK); o += 4;
    c[o++] = 0x0F; c[o++] = 0x05;
    c[o++] = 0x48; c[o++] = 0x85; c[o++] = 0xC0;          /* test rax,rax */
    c[o++] = 0x74; size_t jz_child = o; c[o++] = 0x00;    /* jz child */

    /* ---- parent: rbx=child pid; spin; kill(child,SIGUSR1); wait; exit(status>>8) ---- */
    c[o++] = 0x48; c[o++] = 0x89; c[o++] = 0xC3;          /* mov rbx,rax (child pid) */
    /* spin: mov ecx, 0x03000000 ; loop */
    c[o++] = 0xB9; put32(&c[o], 0x03000000); o += 4;      /* mov ecx,imm */
    size_t sp = o;
    c[o++] = 0x83; c[o++] = 0xE9; c[o++] = 0x01;          /* sub ecx,1 */
    c[o++] = 0x75; c[o++] = (u8)(sp - (o + 1));           /* jnz spin */
    /* kill(child, SIGUSR1) */
    c[o++] = 0xB8; put32(&c[o], S_SYS_KILL); o += 4;
    c[o++] = 0x48; c[o++] = 0x89; c[o++] = 0xDF;          /* mov rdi,rbx */
    c[o++] = 0xBE; put32(&c[o], S_SIGUSR1); o += 4;
    c[o++] = 0x0F; c[o++] = 0x05;
    /* wait(0, &status@SCOUNTER) */
    c[o++] = 0xB8; put32(&c[o], S_SYS_WAIT); o += 4;
    c[o++] = 0x31; c[o++] = 0xFF;                         /* xor edi,edi */
    c[o++] = 0x48; c[o++] = 0xBE; put64(&c[o], SCOUNTER); o += 8;
    c[o++] = 0x0F; c[o++] = 0x05;
    /* eax = [status] >> 8 & 0xff ; exit(eax) */
    c[o++] = 0x8B; c[o++] = 0x04; c[o++] = 0x25; put32(&c[o], (u32)SCOUNTER); o += 4;
    c[o++] = 0xC1; c[o++] = 0xF8; c[o++] = 0x08;          /* sar eax,8 */
    c[o++] = 0x25; put32(&c[o], 0xFF); o += 4;            /* and eax,0xff */
    c[o++] = 0x89; c[o++] = 0xC7;                         /* mov edi,eax */
    c[o++] = 0x31; c[o++] = 0xC0; c[o++] = 0x0F; c[o++] = 0x05; /* exit */
    c[o++] = 0xEB; c[o++] = 0xFE;

    /* ---- child: install SIGUSR1 handler, then loop forever ---- */
    c[jz_child] = (u8)(o - (jz_child + 1));
    emit_install_handler(c, &o);
    /* infinite loop: jmp $ (timer IRQ delivers the signal -> handler exit(77)) */
    c[o++] = 0xEB; c[o++] = 0xFE;
}

/* --- Test 2: no handler; deref bad address -> SIGSEGV default terminate --- */
static void build_segv_default(u8* c) {
    size_t o = 0;
    /* mov rax, 0x20000000 (unmapped) ; mov rbx,[rax] */
    c[o++] = 0x48; c[o++] = 0xB8; put64(&c[o], 0x20000000UL); o += 8;
    c[o++] = 0x48; c[o++] = 0x8B; c[o++] = 0x18;          /* mov rbx,[rax] -> #PF */
    /* if it somehow returns, exit(0) */
    c[o++] = 0x31; c[o++] = 0xFF; c[o++] = 0x31; c[o++] = 0xC0; c[o++] = 0x0F; c[o++] = 0x05;
    c[o++] = 0xEB; c[o++] = 0xFE;
}

typedef void (*blob_builder_t)(u8*);
static blob_builder_t g_builder;

static void sig_probe_thread(void* arg) {
    (void)arg;
    void* code_phys = pmm_alloc(1);
    void* stack_phys = pmm_alloc(1);
    if (!code_phys || !stack_phys) {
        kthread_exit(-1);
    }
    vmm_map_page((void*)SCODE, code_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
    vmm_map_page((void*)(SSTACKTOP - PAGE_SIZE), stack_phys,
                 PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

    for (u32 i = 0; i < PAGE_SIZE; i++) {
        ((u8*)SCODE)[i] = 0;
    }
    g_builder((u8*)SCODE);

    process_t* cur = process_current();
    u64 ktop = ((u64)cur->stack_base + cur->stack_size) & ~0xFULL;
    tss_set_rsp0(ktop);

    exec_iretq_user(SCODE, SSTACKTOP - 16, 0x202);
}

static int run_sig_blob(blob_builder_t b) {
    g_builder = b;
    process_t* p = kthread_run(sig_probe_thread, NULL, 32 * 1024);
    if (!p) {
        return -0x7fff;
    }
    int status = -1;
    thread_join(p, &status);
    return status;
}

static test_result_t test_signal_handler_return(void) {
    TEST_ASSERT_EQ(run_sig_blob(build_handler_return), 1);
    return TEST_PASS;
}

static test_result_t test_signal_mask_defers(void) {
    TEST_ASSERT_EQ(run_sig_blob(build_mask), 1);
    return TEST_PASS;
}

static test_result_t test_signal_kill_cross(void) {
    TEST_ASSERT_EQ(run_sig_blob(build_kill_cross), 77);
    return TEST_PASS;
}

static test_result_t test_signal_segv_default(void) {
    /* default action for an uncaught SIGSEGV is terminate: 128 + SIGSEGV(11). */
    TEST_ASSERT_EQ(run_sig_blob(build_segv_default), 128 + 11);
    return TEST_PASS;
}

void register_signal_tests(void) {
    test_register("signal", "handler_return", test_signal_handler_return);
    test_register("signal", "mask_defers", test_signal_mask_defers);
    test_register("signal", "kill_cross", test_signal_kill_cross);
    test_register("signal", "segv_default", test_signal_segv_default);
}
