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

### Phase 1 — Honnêteté + mesure ✅

- [x] Cartographie modules (full / partial / stub)
- [x] Boot minimal vs `init-full` + `make boot-bench`
- [x] Scorecard noyau (`score` dans kshell)
- [ ] Tests automatiques sur chaque PR

### Phase 2 — Créneau appliance réseau ✅

Voir [NICHE_NETWORK_APPLIANCE.md](NICHE_NETWORK_APPLIANCE.md).

### Phase 3 — Dépassement mesurable ✅ (base)

- [x] Boot cible < 200 ms (`boot_profiler` + `make boot-bench`)
- [x] `sys_exec` + `nettest` embarqué + `/boot/nettest` sur ISO
- [x] DHCP DORA (DISCOVER → OFFER → REQUEST → ACK) + `appliance` (ping 10.0.2.2)
- [x] `bench-ai` — latence scheduler I/O avec/sans IA

## Commandes utiles

```text
make run
init-full
score
```

## Principe directeur

> **Mieux que Linux** = meilleur sur un objectif clair, prouvé par des chiffres — pas plus de badges « 100/100 » sur du code stub.
