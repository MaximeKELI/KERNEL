# Quick Start Guide

## 🚀 Démarrage Rapide

### 1. Prérequis

```bash
# Ubuntu/Debian
sudo apt-get install build-essential nasm grub-pc-bin qemu-system-x86

# Fedora
sudo dnf install gcc nasm grub2-tools qemu-system-x86_64

# Arch
sudo pacman -S base-devel nasm grub qemu
```

### 2. Build

```bash
# Build simple
make

# Build avec tests
./scripts/build.sh --tests

# Build debug
./scripts/build.sh --type debug
```

### 3. Run

```bash
# QEMU
make run

# Ou avec script
./scripts/deploy.sh --target qemu
```

### 4. Test

```bash
# Run tests
make test

# Ou
./scripts/test.sh
```

### 5. Développement

```bash
# Check code quality
./scripts/check_code.sh

# Profile
./scripts/profiler.sh --type time

# Benchmark
./scripts/benchmark.sh
```

## 📚 Documentation

- **README.md** - Vue d'ensemble
- **[BUILD.md](BUILD.md)** - Instructions de build détaillées
- **[DEVELOPMENT.md](DEVELOPMENT.md)** - Guide développement complet
- **CONTRIBUTING.md** - Guide contributeurs
- **ALL_FEATURES.md** - Liste complète des fonctionnalités

## 🛠️ Outils

- **scripts/** - Scripts utilitaires
- **tools/** - Outils Python
- **examples/** - Exemples de code

## 🎯 Prochaines Étapes

1. Lire `README.md` pour comprendre l'architecture
2. Explorer `examples/` pour voir des exemples
3. Lire `DEVELOPMENT.md` pour le workflow
4. Contribuer avec `CONTRIBUTING.md`
