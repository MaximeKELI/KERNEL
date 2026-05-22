# Objectif : un noyau meilleur que Linux — stratégie réaliste

Linux n’est pas « mauvais » : c’est 30M+ lignes, 30 ans, des milliers de contributeurs.  
**Battre Linux partout est irréaliste.** Battre Linux sur **des axes choisis et mesurés** est possible.

## Axes où ce noyau peut gagner

| Axe | Cible | Linux (typique) | Ce projet |
|-----|--------|-----------------|-----------|
| Boot jusqu’au shell | < 500 ms | 2–8 s | Init minimal + `init-full` différé |
| Taille auditable | < 50k LOC actives | Millions | ~30k LOC, cartographie MODULE_MAP |
| IA intégrée au scheduler | Tick 100 Hz | CFS classique | `ai_tick()` sur timer IRQ |
| Ops intégrées | kshell natif | bash + outils userspace | `kshell` noyau |
| Surface d’attaque initiale | Minimale | Large | Boot minimal réduit l’exposition |

## Axes où Linux reste devant (pour l’instant)

- Drivers matériels (GPU, WiFi, NVMe complet…)
- Compatibilité POSIX / glibc / Docker
- Fichiers (ext4, btrfs, NFS…)
- SMP / NUMA production
- Écosystème (apt, systemd, cloud…)

## Feuille de route « meilleur que Linux »

### Phase 1 — Honnêteté + mesure (fait / en cours)

- [x] Cartographie modules (full / partial / stub)
- [x] Boot minimal vs `init-full`
- [x] Scorecard noyau (`score` dans kshell)
- [ ] Tests automatiques sur chaque PR

### Phase 2 — Production sur un créneau

Choisir **un** déploiement cible, par ex. :

- **Appliance réseau** : virtio + DHCP + DNS + shell + AI sched
- **Lab OS / éducation** : boot rapide, code lisible
- **Bare-metal edge** : RTL8139 + TCP/IP minimal

### Phase 3 — Dépassement mesurable

- Boot < 200 ms sur QEMU (mesuré TSC/timer)
- AI scheduler : gain mesurable sur latence I/O (benchmark)
- Sécurité : lockdown activé par défaut sur `init-full`
- Userland : `sys_exec` + binaire `nettest` embarqué

## Commandes utiles

```text
make run
init-full
score
```

## Principe directeur

> **Mieux que Linux** = meilleur sur un objectif clair, prouvé par des chiffres — pas plus de badges « 100/100 » sur du code stub.
