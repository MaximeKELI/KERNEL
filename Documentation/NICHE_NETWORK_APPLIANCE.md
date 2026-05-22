# Créneau : appliance réseau (edge / lab)

Ce noyau cible en priorité une **appliance réseau légère** — pas un desktop généraliste.

## Cas d’usage

| Scénario | Pourquoi ce noyau |
|----------|-------------------|
| QEMU / lab | Boot rapide, stack TCP/UDP, DHCP, ping, DNS minimal |
| Edge x86 bare-metal | Driver RTL8139, virtio-net, footprint auditable |
| Formation | Code lisible, kshell, scorecard honnête |

## Parcours type (QEMU)

```text
make run
init-full
appliance      # DHCP DORA + ping 10.0.2.2
score
bench-ai
exec nettest   # userland embarqué
```

## Fichiers sur l’ISO

- `/boot/kernel.elf` — noyau Multiboot2
- `/boot/nettest` — binaire userland (même image embarquée dans le noyau pour `sys_exec`)

## Objectifs mesurables (vs Linux)

| Métrique | Cible |
|----------|--------|
| Boot minimal → shell | < 200 ms |
| Init réseau complet | `appliance` après `init-full` |
| Userland | `exec nettest` |
| Scheduler | `bench-ai` |

Linux reste la référence pour serveurs généralistes ; ce créneau privilégie **simplicité, boot et ops réseau intégrées**.
