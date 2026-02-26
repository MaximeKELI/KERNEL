# Contributing Guide

Thank you for your interest in contributing to the kernel project!

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Code Style](#code-style)
- [Testing](#testing)
- [Documentation](#documentation)
- [Submitting Changes](#submitting-changes)

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Follow the project's coding standards
- Test your changes thoroughly

## Getting Started

### Prerequisites

- GCC (x86_64)
- NASM
- GNU Make
- GRUB2 tools
- Doxygen (for documentation)

### Setting Up Development Environment

```bash
# Clone the repository
git clone <repository-url>
cd kernel

# Build the kernel
make

# Run tests
make test

# Generate documentation
make docs
```

## Development Workflow

### 1. Create a Branch

```bash
git checkout -b feature/my-feature
```

### 2. Make Changes

- Follow the code style guidelines
- Add tests for new features
- Update documentation

### 3. Test Your Changes

```bash
# Compile with tests
make CFLAGS+="-DRUN_TESTS"

# Run specific test suite
test_run_suite("my_suite");
```

### 4. Commit Changes

```bash
git add .
git commit -m "Add feature: description"
```

## Code Style

### General Guidelines

- Use 4 spaces for indentation (no tabs)
- Maximum line length: 100 characters
- Use descriptive variable and function names
- Comment complex logic
- Follow existing code patterns

### C Code Style

```c
// Function naming: lowercase with underscores
int my_function(void* param) {
    // Always validate pointers
    VALIDATE_PTR_RET(param, -1);
    
    // Use braces even for single statements
    if (condition) {
        do_something();
    }
    
    return 0;
}

// Type definitions: use kernel types
u32 count;
u64 address;
bool enabled;
```

### Header Files

- Include guards: `#ifndef HEADER_H` / `#define HEADER_H`
- Include order: system headers, then project headers
- Document all public functions

### Memory Management

```c
// Always check allocations
void* ptr = kmalloc(size);
if (!ptr) {
    DEBUG_ERROR("Allocation failed");
    return -1;
}

// Always free allocated memory
kfree(ptr);
```

### Error Handling

```c
// Use validation macros
VALIDATE_PTR_RET(ptr, -1);
VALIDATE_SIZE(size);
VALIDATE_RANGE(value, min, max);

// Log errors
DEBUG_ERROR("Operation failed: %s", reason);
```

## Testing

### Writing Tests

```c
// kernel/test/tests_myfeature.c
#include "test.h"
#include "myfeature.h"

static test_result_t test_myfeature_basic(void) {
    // Test implementation
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQ(value, expected);
    return TEST_PASS;
}

void register_myfeature_tests(void) {
    test_register("myfeature", "basic", test_myfeature_basic);
}
```

### Test Requirements

- All new features must include tests
- Tests must cover edge cases
- Tests must be deterministic
- Tests must clean up resources

### Running Tests

```bash
# Run all tests
test_run_all();

# Run specific suite
test_run_suite("myfeature");
```

## Documentation

### Code Documentation

```c
/**
 * @brief Brief description of function
 * 
 * @param param1 Description of parameter 1
 * @param param2 Description of parameter 2
 * @return Description of return value
 * 
 * @note Additional notes
 * @warning Warnings
 */
int my_function(void* param1, u32 param2);
```

### README Updates

- Update README.md for user-facing changes
- Add examples for new features
- Update feature list if needed

### API Documentation

- Document all public APIs
- Include usage examples
- Explain parameters and return values

## Submitting Changes

### Before Submitting

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] Code follows style guidelines
- [ ] Documentation is updated
- [ ] No memory leaks
- [ ] Validation is complete

### Commit Messages

```
Format: <type>: <description>

Types:
- feat: New feature
- fix: Bug fix
- docs: Documentation changes
- test: Test additions/changes
- refactor: Code refactoring
- perf: Performance improvements
- security: Security fixes
```

Examples:
```
feat: Add new filesystem driver
fix: Memory leak in scheduler
docs: Update API documentation
test: Add tests for memory allocator
```

### Pull Request Checklist

- [ ] Description of changes
- [ ] Related issues (if any)
- [ ] Tests added/updated
- [ ] Documentation updated
- [ ] Code reviewed
- [ ] All checks pass

## Code Review Process

1. **Automated Checks**: Code must pass all automated checks
2. **Review**: At least one reviewer must approve
3. **Tests**: All tests must pass
4. **Documentation**: Documentation must be updated

## Areas for Contribution

### High Priority

- Additional unit tests
- Performance optimizations
- Security improvements
- Documentation enhancements
- Bug fixes

### Medium Priority

- New drivers
- Filesystem improvements
- Networking features
- Debugging tools

### Low Priority

- Code cleanup
- Refactoring
- Style improvements

## Getting Help

- Check existing documentation
- Review similar code in the project
- Ask questions in issues
- Review test examples

## License

By contributing, you agree that your contributions will be licensed under the same license as the project.

---

Thank you for contributing! 🎉
