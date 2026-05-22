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
- **Writeback** : `writeback_tick()` → `cache_sync_all()` toutes les 5s

## D — Userland
- **vdso** : page `0xFFFFC0000000` (ticks)
- **sh** : shell minimal ABI Linux (`exec nettest`)
- **ISO** : `/boot/sh`, `/boot/nettest`

## Commandes

```text
make iso
make run
init-full
exec sh
epoll via nettest (syscalls 22+)
```
