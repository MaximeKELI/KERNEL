# Kernel Examples

This directory contains example code for extending the kernel.

## Examples

### Driver Example (`driver_example.c`)

Shows how to create a new device driver:
- Device registration
- Driver operations (open, close, read, write)
- Driver initialization and cleanup
- Proper error handling

**Usage**:
```c
// Initialize driver
example_driver_init();

// Use device
device_t* dev = device_find("example");
device_open(dev);
device_read(dev, buf, size);
device_close(dev);
```

### System Call Example (`syscall_example.c`)

Shows how to add new system calls:
- System call implementation
- Parameter validation
- User space interaction
- System call registration

**Usage**:
```c
// Register system calls
example_syscalls_register();

// System calls available to user space
long version_len = syscall(SYS_GET_KERNEL_VERSION, buf, len);
syscall(SYS_KERNEL_INFO, &info);
```

## Adding Your Own Code

1. Copy the example file
2. Modify for your needs
3. Register in appropriate initialization function
4. Add to Makefile if needed
5. Test thoroughly

## Best Practices

- Always validate pointers with `VALIDATE_PTR_RET`
- Use proper error handling
- Add debug logging with `DEBUG_INFO`
- Follow kernel coding style
- Add unit tests for new features
