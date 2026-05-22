# Parité Linux — plan de production

Objectif : **même classe de noyau que Linux** (pas de stubs « simplifiés »), avec preuves par tests et QEMU.

## Déjà renforcé (cette passe)

| Composant | Avant | Maintenant |
|-----------|--------|------------|
| **Ring 3** | `jmp` en mode noyau | GDT user (0x23/0x2b) + `iretq` + STAR SYSCALL/SYSRET |
| **ELF** | Toutes pages RW | Permissions PT_LOAD (RX / RW) + `PAGE_USER` |
| **TCP** | Pas de RTO | RTO, retransmit, backoff, ACK cumulatif, SYN-ACK client, timer |

## Phases suivantes (ordre Linux)

### Phase A — Réseau (niveau kernel Linux net/)
- Congestion : slow start, congestion avoidance, fast retransmit
- Fenêtre glissante, SACK, timestamps RFC 7323
- Socket backlog, `SO_REUSEADDR`, poll/epoll sur sockets
- UDP checksum offload, GRO/GSO (virtio-net)

### Phase B — Mémoire & processus
- Page tables par processus (CR3), COW sur `fork`
- `clone`, namespaces pid/net/mnt réels
- Signaux : frames, `sigreturn`, delivery sur syscall exit

### Phase C — VFS & block
- ext4, page cache, dentry cache, path lookup complet
- writeback, journal, blk-mq multi-queue + AHCI/NVMe réel

### Phase D — Compatibilité userland
- ABI binaire partielle (libc musl static)
- Interpréteur dynamique, vdso (`clock_gettime`)

## Mesure « comme Linux »

```bash
make RUN_TESTS=1 build/kernel.elf
make run   # init-full, appliance, http, exec nettest
```

Critères : ping + HTTP + user ring3 sans #GP, TCP retransmit sous perte simulée.
