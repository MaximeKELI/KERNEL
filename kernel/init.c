#include "init.h"
#include "scheduler.h"
#include "process.h"
#include "exec.h"
#include "kernel_init.h"
#include "stdio.h"

/*
 * PID 1 / init. Launches the ring-3 shell (/sh) and, like a real init,
 * respawns it whenever it exits. When init is active it owns the keyboard
 * (via sys_read on stdin), so the in-kernel kshell must not also drain input.
 */

bool g_init_active = false;

static void sh_child(void* arg) {
    (void)arg;
    if (exec_run_path("/sh") < 0) {
        printk("[init] exec /sh failed\n");
    }
    kthread_exit(0);   /* exec_run_path is noreturn on success */
}

static void init_supervisor(void* arg) {
    (void)arg;

    /*
     * Bring up the network stack before handing over to the shell, so userland
     * commands like `nettest` have a working stack. We deliberately use the
     * lightweight kernel_init_network() rather than kernel_init_extended():
     * the latter initialises hundreds of subsystems and exhausts the kernel
     * heap, leaving nothing to allocate /sh's kernel stack.
     */
    kernel_init_network();

    for (;;) {
        process_t* sh = kthread_run(sh_child, NULL, 32 * 1024);
        if (!sh) {
            printk("[init] cannot spawn /sh; init idle\n");
            for (;;) {
                __asm__ __volatile__("hlt");
            }
        }
        int status = 0;
        thread_join(sh, &status);
        printk("\n[init] shell exited (status %d), restarting...\n", status);
    }
}

void init_start(void) {
    g_init_active = true;
    kthread_run(init_supervisor, NULL, 16 * 1024);
}
