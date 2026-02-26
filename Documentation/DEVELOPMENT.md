# Development Guide

## 🛠️ Development Tools

### Scripts Utilitaires

Le projet inclut plusieurs scripts pour faciliter le développement :

#### Build Script (`scripts/build.sh`)
```bash
# Build release
./scripts/build.sh --type release

# Build debug
./scripts/build.sh --type debug --tests

# Build with documentation
./scripts/build.sh --docs

# Clean build
./scripts/build.sh --clean
```

#### Test Script (`scripts/test.sh`)
```bash
# Run all tests
./scripts/test.sh

# Run specific suite
./scripts/test.sh --suite memory

# Verbose output
./scripts/test.sh --verbose
```

#### Benchmark Script (`scripts/benchmark.sh`)
```bash
# Run all benchmarks
./scripts/benchmark.sh

# Specific benchmark
./scripts/benchmark.sh --type memory --iterations 100
```

#### Deploy Script (`scripts/deploy.sh`)
```bash
# Deploy to QEMU
./scripts/deploy.sh --target qemu

# Create USB bootable
./scripts/deploy.sh --target usb --device /dev/sdX

# Just create ISO
./scripts/deploy.sh --target iso
```

#### Code Quality Checker (`scripts/check_code.sh`)
```bash
# Check code quality
./scripts/check_code.sh

# Check specific type
./scripts/check_code.sh --type style
```

#### Profiler (`scripts/profiler.sh`)
```bash
# Profile time
./scripts/profiler.sh --type time --duration 30

# Profile memory
./scripts/profiler.sh --type memory
```

#### Release Generator (`scripts/gen_release.sh`)
```bash
# Generate release
./scripts/gen_release.sh --version 1.0.0 --type stable
```

### Outils Python

#### Kernel Debugger (`tools/kernel_debug.py`)
```bash
# Analyze crash log
python3 tools/kernel_debug.py --analyze --log kernel.log

# Add breakpoint
python3 tools/kernel_debug.py --breakpoint 0x100000

# List breakpoints
python3 tools/kernel_debug.py --list-bp
```

#### Kernel Statistics (`tools/kernel_stats.py`)
```bash
# Analyze kernel statistics
python3 tools/kernel_stats.py kernel.log

# Summary only
python3 tools/kernel_stats.py kernel.log --summary
```

## 🔧 Workflow de Développement

### 1. Setup Initial
```bash
# Clone repository
git clone <repo>
cd kernel

# Install dependencies
sudo apt-get install build-essential nasm grub-pc-bin qemu-system-x86

# Build kernel
make
```

### 2. Développement
```bash
# Build debug version
./scripts/build.sh --type debug

# Run tests
./scripts/test.sh

# Check code quality
./scripts/check_code.sh

# Profile performance
./scripts/profiler.sh --type time
```

### 3. Tests
```bash
# Run all tests
make test

# Run specific test suite
./scripts/test.sh --suite memory

# Run with coverage
./scripts/test.sh --coverage
```

### 4. Benchmark
```bash
# Run benchmarks
make benchmark

# Specific benchmark
./scripts/benchmark.sh --type scheduler
```

### 5. Release
```bash
# Generate release
./scripts/gen_release.sh --version 1.1.0

# Deploy
./scripts/deploy.sh --target iso
```

## 📊 CI/CD

Le projet inclut GitHub Actions pour l'intégration continue :

- **Build automatique** sur push/PR
- **Tests automatiques**
- **Génération documentation**
- **Vérification erreurs**

Voir `.github/workflows/ci.yml` pour configuration.

## 🎯 Exemples de Code

Le répertoire `examples/` contient :
- `driver_example.c` - Exemple de driver
- `syscall_example.c` - Exemple de system call

Voir `examples/README.md` pour détails.

## 📝 Documentation

### Générer Documentation API
```bash
make docs
# Documentation dans docs/html/
```

### Documentation Disponible
- `README.md` - Vue d'ensemble
- `BUILD.md` - Instructions de build
- `CONTRIBUTING.md` - Guide contributeurs
- `ALL_FEATURES.md` - Liste complète des fonctionnalités
- `DEVELOPMENT.md` - Ce document

## 🐛 Debugging

### QEMU avec GDB
```bash
# Terminal 1: QEMU avec GDB
qemu-system-x86_64 -cdrom build/kernel.iso -m 512M -s -S

# Terminal 2: GDB
gdb build/kernel.elf
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
```

### Analyse de Logs
```bash
# Analyser crash
python3 tools/kernel_debug.py --analyze --log kernel.log

# Statistiques
python3 tools/kernel_stats.py kernel.log
```

## 🚀 Performance

### Profiling
```bash
# Profile time
./scripts/profiler.sh --type time --duration 60

# Profile memory
./scripts/profiler.sh --type memory

# Profile cache
./scripts/profiler.sh --type cache
```

### Benchmarks
```bash
# Run all benchmarks
make benchmark

# Specific benchmark
./scripts/benchmark.sh --type memory --iterations 1000
```

## ✅ Checklist Avant Commit

- [ ] Code compile sans warnings
- [ ] Tests passent (`make test`)
- [ ] Code quality check passe (`./scripts/check_code.sh`)
- [ ] Documentation mise à jour
- [ ] Changelog mis à jour
- [ ] Pas de memory leaks
- [ ] Validation input complète
