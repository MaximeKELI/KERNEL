# Architecture IA du noyau (v3)

Sous-système **heuristique et mesurable** — pas de ML lourd : moyennes mobiles (EMA), classification de processus, politiques prédéfinies, journal des décisions.

## Modules

| Module | Rôle |
|--------|------|
| `ai_manager` | Init, `ai_tick()` orchestré par phases |
| `ai_monitor` | Métriques CPU, mémoire, I/O, réseau, ctx switches |
| `ai_predict` | EMA + prédiction court terme + tendance |
| `ai_process` | Profil par processus (`ai_class`, wait/run ticks) |
| `ai_policy` | Modes : balanced, latency, throughput, powersave |
| `ai_optimizer` | Scheduler, mémoire, anomalies |
| `ai_network` | Boost tâches réseau sous charge RX |
| `ai_log` | Anneau 32 décisions (audit / debug) |
| `ai_shell` | Interface kshell |
| `ai_sysfs` | Interface sysfs (config) |
| `ai_controller` | Auto-policy (net→latency, idle→powersave) |
| `ai_learn` | Poids adaptatifs par type d'action (renforcement léger) |
| `ai_history` | Anneau 24 échantillons + sparklines kshell |
| `ai_bench` | Benchmark scheduler on/off |

## Tick timer (100 Hz)

Chaque tick :

1. Mise à jour métriques
2. Alimentation prédicteur EMA
3. Mise à jour profils processus

Selon la politique active :

- Scheduler adaptatif (diviseur 1–8)
- Réseau (boost NET class)
- Mémoire (cache_sync si pression)
- Anomalies (toutes les 50 ticks)

## Classes processus

- `IDLE` — peu d’activité
- `CPU` — beaucoup de runtime, peu d’attente
- `IO` — souvent `BLOCKED`
- `NET` — bloqué + trafic réseau élevé
- `MIXED` — intermédiaire

## Politiques

| Mode | Usage |
|------|--------|
| `balanced` | Défaut appliance |
| `latency` | Réactivité I/O / réseau |
| `throughput` | Timeslices larges si idle |
| `powersave` | Moins d’interventions IA |

## kshell

```text
init-full
ai                  # statut complet
ai policy latency   # mode faible latence
ai log              # dernières décisions
ai bench            # benchmark
ai tune cpu 70      # seuil CPU
ai off / ai on
```

## Différenciation vs Linux CFS

Linux CFS est équitable et statique par nice ; ce noyau ajoute une **couche adaptative** optionnelle qui réagit aux métriques globales et au profil heuristique des tâches — utile pour appliance réseau et lab, pas un remplacement production de CFS.
