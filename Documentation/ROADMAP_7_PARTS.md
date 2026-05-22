# Feuille de route — 7 parties implémentées

## 1. Appliance réseau

- `kernel/net/http_client.c` — HTTP GET via TCP
- `kernel/appliance_config.c` — config persistante `/etc/appliance.conf` (tmpfs)
- `appliance` — DHCP + ping + HTTP GET gateway
- kshell : `http HOST [path]`, `config`

## 2. Fichiers

- `kernel/fs/vfs.c` — FD table, `vfs_open_fd`, read/write path
- `kernel/fs/tmpfs.c` — fichiers réels sous `/tmp`, `/etc`
- `kernel/fs/ext2.c` — lookup chemin + lecture inode

## 3. POSIX minimal

- `sys_open` / `sys_read` / `sys_close` branchés VFS
- `sys_fork` / `sys_wait` → `fork_process` / `wait_process`
- `sys_mmap` / `sys_munmap` — allocation user
- `sys_sigreturn` — stub handler
- `exec.c` — pages `PAGE_USER`, ELF depuis VFS

## 4. Vidéo & audio

- `kernel/media/fb_console.c` — console sur framebuffer
- `kernel/drivers/gpu/virtio_gpu.c` — détection PCI stub
- `kernel/media/codec_raw.c` — PCM passthrough
- `kernel/drivers/audio/pc_speaker.c` — bip PC speaker

## 5. Sécurité

- Landlock sur `sys_open`
- Audit sur syscalls sensibles
- Seccomp strict sur `sys_exec`
- `kspp_stack_check()` dans le handler syscall

## 6. SMP & I/O

- `smp.c` — 2 CPUs (stub AP)
- `timer.c` — `ai_tick` par CPU
- `block.c` — `blk_mq_submit` + dispatch
- `kernel/drivers/ahci.c` — probe AHCI + fallback ATA

## 7. Wow

- kshell `container list|create`
- `kernel/bpf/ebpf.c` — interpréteur eBPF minimal
- Tests `make RUN_TESTS=1` — suite `roadmap_*`

## Commandes

```text
make build/kernel.elf
make iso
make run
init-full
appliance
http 10.0.2.2 /
config
container create demo
make RUN_TESTS=1
```
