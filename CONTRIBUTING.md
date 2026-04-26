# Contributing to EmuForge

Thank you for your interest in contributing to EmuForge! This document provides guidelines and instructions for contributing to the project.

## 📋 Code of Conduct

- **Respectful Communication** - Treat all contributors with respect
- **Constructive Feedback** - Focus on ideas, not individuals
- **Inclusive Environment** - Welcome contributors of all skill levels
- **Professional Conduct** - Maintain professional standards in all interactions

## 🚀 Getting Started

### 1. Fork the Repository

```bash
# Visit GitHub and click "Fork"
# Clone your fork
git clone https://github.com/YOUR_USERNAME/EmuForge.git
cd EmuForge
```

### 2. Create a Feature Branch

```bash
# Create descriptive branch name
git checkout -b feature/cpu-optimization
# or
git checkout -b fix/register-banking-bug
# or
git checkout -b docs/expand-architecture-guide
```

### 3. Set Up Development Environment

```bash
# Install dependencies (see BUILD.md)
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"  # or appropriate generator
cmake --build .
ctest --output-on-failure
```

## 📝 Commit Guidelines

### Commit Message Format

```
Type: Brief description (50 chars or less)

More detailed explanation if needed (70 chars per line).
Explain the why and what, not the how.

Fixes #123
```

### Commit Types

- **feat**: New feature
  ```
  feat: Implement ARMv7 coprocessor support
  ```

- **fix**: Bug fix
  ```
  fix: Correct register banking for IRQ mode
  ```

- **docs**: Documentation
  ```
  docs: Add coprocessor operation reference
  ```

- **test**: Test additions
  ```
  test: Add unit tests for condition codes
  ```

- **perf**: Performance improvement
  ```
  perf: Optimize instruction decoding with lookup table
  ```

- **refactor**: Code restructuring
  ```
  refactor: Extract instruction decoder to separate module
  ```

- **style**: Code formatting
  ```
  style: Format code to match style guide
  ```

### Good Commit Messages

✅ **Good**:
```
fix: Correct CPSR flags for ADD instruction

The ADD instruction was not properly setting the Z (zero) flag
when the result was 0. This fix ensures flags are updated
according to the ARMv6 specification.

Also improves handling of overflow flag for signed arithmetic.

Fixes #42
```

❌ **Bad**:
```
fixed stuff
```

## 🧪 Testing Requirements

### Before Submitting a Pull Request

1. **Write tests** for new functionality
2. **Run all tests**: `ctest --output-on-failure`
3. **Verify** tests pass: All tests green ✅
4. **Test coverage** > 90%

### Test Structure

```cpp
// Good test structure
void testMyFeature() {
    // Setup
    ARM11 cpu;
    
    // Test case 1
    cpu.setReg(0, 0x12345678);
    assert(cpu.getReg(0) == 0x12345678);
    
    // Test case 2 with different input
    cpu.setReg(1, 0xFFFFFFFF);
    assert(cpu.getReg(1) == 0xFFFFFFFF);
    
    // Verify side effects
    assert(cpu.getCPSR() == expectedCPSR);
    
    std::cout << "✓ testMyFeature passed\n";
}
```

### Coverage Guidelines

| Component | Min Coverage | Target |
|-----------|--------------|--------|
| Core CPU | 90% | 100% |
| New Instructions | 100% | 100% |
| Bug Fixes | 100% | 100% |
| Refactoring | Same as original | Same as original |

## 💻 Code Style Guide

### C++ Style

We follow **Google C++ Style Guide** with these modifications:

#### Naming Conventions

```cpp
// Classes and types: PascalCase
class MemoryController { };
struct ProcessorMode { };
typedef uint32_t Address;

// Functions and methods: camelCase
void executeInstruction();
uint32_t getRegister(int index);

// Constants: UPPER_CASE
const uint32_t MAX_REGISTERS = 16;
const int MEMORY_SIZE = 8 * 1024 * 1024;

// Member variables: camelCase with leading underscore (optional)
uint32_t _programCounter;
std::vector<uint32_t> registers;

// Local variables: camelCase
uint32_t result = a + b;
```

#### Formatting

```cpp
// Indentation: 4 spaces
if (condition) {
    doSomething();
    doSomethingElse();
}

// Braces: On same line (Allman for complex blocks)
class MyClass {
public:
    void myMethod() {
        // implementation
    }
};

// Line length: Max 100 characters
// Long lines: Break logically
uint32_t result = operand1 + operand2 
                + operand3 * operand4;

// Comments: Explain why, not what
// ✗ Bad: i++  // increment i
// ✓ Good: i++  // move to next instruction in sequence
```

#### Headers and Includes

```cpp
// Include guards (not #pragma once for portability)
#ifndef EMUFORGE_CPU_BASE_HPP
#define EMUFORGE_CPU_BASE_HPP

#include <cstdint>
#include <vector>
#include <functional>
#include <string>

// Project includes after std includes
#include "common/types.hpp"

namespace emuforge {
    // Code here
}

#endif  // EMUFORGE_CPU_BASE_HPP
```

#### Function Documentation

```cpp
/// @brief Execute one CPU cycle
/// @details Performs fetch-decode-execute cycle with proper
///          pipeline simulation and timing behavior.
/// @return void
virtual void step() = 0;

/// @brief Read a 32-bit value from register
/// @param index Register index (0-15)
/// @return Register value, or 0 if invalid index
/// @throws std::out_of_range If index >= 16
virtual uint32_t getReg(int index) const = 0;
```

## 📊 Documentation Requirements

### Code Comments

- **Class-level**: Explain purpose and key methods
- **Method-level**: Explain complex logic
- **Inline**: Use sparingly, explain why not what

### Documentation Files

For significant changes:
1. Update relevant .md files
2. Add examples if applicable
3. Update table of contents
4. Keep consistency with existing docs

### Example Documentation

```markdown
## New Feature: XYZ

### Overview
Brief description of feature and why it's useful.

### Usage
```cpp
// Code example
```

### Implementation Details
Technical details for maintainers.

### Testing
How to verify the feature works.
```

## 🔄 Pull Request Process

### Before Creating PR

1. ✅ Ensure code compiles without warnings
2. ✅ All tests pass: `ctest --output-on-failure`
3. ✅ Code follows style guide
4. ✅ Commits have good messages
5. ✅ Rebase on latest main: `git rebase origin/main`

### Creating PR

1. Push to your fork:
   ```bash
   git push origin feature/my-feature
   ```

2. Create PR with:
   - **Title**: Clear, concise description
   - **Description**: Explain what, why, and how
   - **Checklist**: Confirm items from "Before Creating PR"
   - **Related Issues**: Link to #123 if applicable

### PR Template

```markdown
## Description
Brief description of changes.

## Type of Change
- [ ] New feature
- [ ] Bug fix
- [ ] Documentation
- [ ] Performance improvement
- [ ] Code refactoring

## Related Issues
Fixes #123

## Testing
- [ ] Added/updated tests
- [ ] All tests pass
- [ ] Coverage > 90%

## Documentation
- [ ] Updated README if needed
- [ ] Updated Architecture if needed
- [ ] Added code comments
- [ ] Updated CHANGELOG

## Checklist
- [ ] Code follows style guide
- [ ] No compiler warnings
- [ ] Commits have good messages
- [ ] Changes are minimal and focused
```

### Code Review

- Respond to all feedback respectfully
- Request re-review after changes
- Maintain professional tone
- Thank reviewers for time

## 🐛 Bug Reports

### Before Reporting

1. Check existing issues
2. Update to latest code
3. Verify the bug reproduces

### Bug Report Template

```markdown
## Description
Brief description of bug.

## Reproduction Steps
1. Step 1
2. Step 2
3. Step 3

## Expected Behavior
What should happen.

## Actual Behavior
What actually happens.

## Environment
- OS: Windows 10 / Linux / macOS
- Compiler: MSVC 2019 / GCC 9 / Clang 10
- CMake version: 3.20

## Additional Context
Screenshots, error messages, etc.
```

## 💡 Feature Requests

### Before Requesting

1. Check existing feature requests
2. Verify it aligns with project goals
3. Consider implementation complexity

### Feature Request Template

```markdown
## Description
Clear description of desired feature.

## Use Case
Why is this feature useful?

## Proposed Solution
Your suggested implementation.

## Alternatives
Other possible approaches.

## Additional Context
References, examples, etc.
```

## 📚 Development Workflow Example

### Adding a New Instruction (e.g., BIT Operation)

1. **Create feature branch**:
   ```bash
   git checkout -b feature/bit-instruction
   ```

2. **Implement instruction**:
   - Update instruction enum
   - Add decoder logic
   - Implement execution
   - Add documentation

3. **Write tests**:
   ```cpp
   void testBitInstruction() {
       ARM11 cpu;
       cpu.setReg(0, 0xFF00FF00);
       // Execute BIT #0xFF, R0
       // Verify CPSR flags
   }
   ```

4. **Verify**:
   ```bash
   cmake --build . --config Release
   ctest --output-on-failure
   ```

5. **Commit**:
   ```bash
   git add -A
   git commit -m "feat: Implement BIT instruction
   
   Adds support for BIT test instruction following ARMv6
   specification. Properly sets Z and N flags based on
   AND result.
   
   - Adds BIT to instruction decoder
   - Implements bit-test logic
   - Adds comprehensive unit tests
   - Updates documentation"
   ```

6. **Push and Create PR**:
   ```bash
   git push origin feature/bit-instruction
   ```
   Then create PR on GitHub

## 🎓 Learning Resources

### ARM Architecture
- [ARM Developer Resources](https://developer.arm.com/)
- [ARM Instruction Set Reference](https://www.arm.com/)

### 3DS Hardware
- [3DBrew Hardware Documentation](https://www.3dbrew.org/)

### Emulation
- [Emulation Development Guide](https://emulation.gametechwiki.com/)

### C++
- [Modern C++ Best Practices](https://isocpp.org/)

## ❓ Questions?

- Check [README.md](README.md) for overview
- Check [ARCHITECTURE.md](ARCHITECTURE.md) for technical details
- Open an issue with `[QUESTION]` tag
- Ask in discussions

## 🏆 Contributor Recognition

Contributors will be recognized in:
- CONTRIBUTORS.md file
- GitHub repository
- Release notes

---

Thank you for contributing to EmuForge! 🚀

**Last Updated**: April 26, 2026
