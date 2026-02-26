# Kernel Benchmarks

This document describes the benchmark suite for measuring kernel performance.

## Available Benchmarks

### Memory Benchmarks

- **kmalloc/kfree**: Allocation/deallocation performance
- **Page allocation**: PMM allocation speed
- **Memory copy**: memcpy performance
- **Cache performance**: L1/L2/L3 cache hit rates

### Scheduler Benchmarks

- **Context switch**: Time per context switch
- **Process creation**: Fork/exec performance
- **Scheduler overhead**: CFS vs Deadline vs RT
- **Load balancing**: Multi-core scheduling

### I/O Benchmarks

- **Disk I/O**: Read/write throughput
- **Network I/O**: TCP/UDP throughput
- **File system**: VFS operations per second

### System Call Benchmarks

- **Syscall overhead**: Time per system call
- **Syscall throughput**: Calls per second

## Running Benchmarks

```bash
# Run all benchmarks
./scripts/benchmark.sh

# Run specific benchmark
./scripts/benchmark.sh --type memory

# Multiple iterations
./scripts/benchmark.sh --iterations 100
```

## Benchmark Results

Results are output in the kernel console and can be parsed for analysis.

### Expected Performance

- **Context switch**: < 1 microsecond
- **kmalloc**: < 100 nanoseconds
- **System call**: < 200 nanoseconds
- **Page fault**: < 5 microseconds

## Adding New Benchmarks

1. Create benchmark function in kernel
2. Register in benchmark system
3. Add to benchmark script
4. Document expected results
