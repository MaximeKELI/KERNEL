# Phases A–D (parité Linux)

## A — Réseau
- **Congestion** : slow start + congestion avoidance (`cwnd`, `ssthresh`, dup ACK)
- **Fenêtre glissante** : `flight_size` vs `min(cwnd, peer_rwnd)`
- **SACK** : parsing options TCP (kind 5)
- **epoll** : `EPOLL_CTL_ADD/DEL/MOD`, `epoll_wait` branché sur sockets
- **Syscalls** : 22–25 (`epoll_create/ctl/wait`, `poll`)

## B — Processus
- **CR3** par processus : `vmm_fork_clone()`, `vmm_switch_mm()` dans le scheduler
- **COW** : pages user en lecture seule au fork, `#PF` write → `vmm_cow_fault()`
- **Signaux** : `sigframe` sur pile user, `sys_sigreturn` restaure le contexte

## C — FS
- **Page cache** : `ext2_read` via `page_cache_get(ino, offset)`
- **Journal** : `ext2_journal_log/commit` (metadata)
- **Writeback** : `page_cache_sync` → `ext2_writeback_page` + `ata_write_sectors`, commit journal

## D — Userland
- **vdso** : `vdso_map_user()` à l’exec, base `0xFFFFC0000000`
- **Syscalls 26–27** : `getpid`, `clock_gettime`
- **sh / nettest** : getpid, TCP+connect+epoll
- **ABI** : `include/abi/linux_syscall.h` (mapping doc vs `K_SYS_*`)

## Commandes

```text
make iso
make run
init-full
phases
fork-test
exec sh
exec nettest
./scripts/run_phases.sh
```
