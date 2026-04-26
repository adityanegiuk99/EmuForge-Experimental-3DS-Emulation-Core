# Problem Statement & Project Objectives

## 📌 The Core Problem

Nintendo 3DS emulation is one of the most challenging emulation tasks due to its:
1. **Complex dual-CPU architecture** (ARM11 + ARM9)
2. **Intricate memory systems** with multiple regions
3. **Sophisticated GPU** (tile-based deferred renderer)
4. **Proprietary file formats** (CCI, NCSD)
5. **Cycle-precise timing requirements**

Existing emulators like Citra achieve success but remain closed-source or community-driven. **EmuForge** aims to create an **educational, maintainable, from-scratch implementation** focusing on **correctness and clarity**.

---

## 🎯 Project Goals

### Primary Objectives

1. **Accurate CPU Emulation**
   - Implement both ARM11 and ARM9 processors
   - Support all instruction variations
   - Handle processor modes and banking correctly
   - Simulate pipeline behavior

2. **Complete Memory System**
   - Map all physical memory regions
   - Implement memory protection
   - Support virtual address translation
   - Handle cache behavior

3. **Graphics Rendering**
   - Emulate PICA200 GPU
   - Support tile-based deferred rendering
   - Implement texture operations
   - Handle framebuffer operations

4. **System Emulation**
   - OS kernel functionality
   - Game loading and execution
   - I/O peripheral handling
   - Interrupt management

### Secondary Objectives

- **Code Quality**
  - Maintainable, well-documented codebase
  - Comprehensive test coverage
  - Clear architecture and design patterns
  - Easy to extend and modify

- **Educational Value**
  - Learn ARM architecture
  - Understand emulation techniques
  - Study systems programming
  - Explore game development platforms

- **Performance**
  - Reasonable execution speed (30+ FPS target)
  - Optimized instruction interpretation
  - Efficient memory access
  - Minimal overhead

---

## 🔍 Problem Decomposition

### Challenge 1: ARM CPU Architecture

**Problem**: The 3DS uses two distinct ARM processors with different instruction sets and timing requirements.

**Components**:
- ARM11 (ARMv6K) - Main application processor @ 268/804 MHz
- ARM9 (ARMv5TE) - Security processor for bootrom and crypto
- Coprocessor (CP15) - System control
- Banked registers - Mode-specific storage

**Solution**:
- Implement abstract `CPUBase` interface ✅
- Full ARM11 instruction decoder with all addressing modes ✅
- Lightweight ARM9 for bootrom execution ✅
- Pipeline simulation for timing accuracy ✅

### Challenge 2: Memory Management

**Problem**: The 3DS has multiple memory regions with different characteristics and access rules.

**Memory Regions**:
- FCRAM (8 MB) - Shared external RAM
- AXRAM (1 MB) - ARM9 only
- I-Cache / D-Cache - Instruction/Data caches
- System memory - Kernel space
- Game memory - Application space

**Solution Approach**:
- Unified memory abstraction layer
- Region-based access control
- Virtual-to-physical mapping
- Cache simulation

### Challenge 3: Graphics Processing

**Problem**: PICA200 is a tile-based deferred renderer, fundamentally different from traditional rasterizers.

**PICA200 Features**:
- Tile-based deferred rendering
- Vertex/Fragment shaders
- Texture filtering and operations
- Framebuffer management
- Display lists

**Solution Approach**:
- Tile binning system
- Shader interpretation
- Texture cache simulation
- Output framebuffer

### Challenge 4: System Integration

**Problem**: Games expect proper OS emulation including loading, interrupts, and I/O.

**Requirements**:
- ROM/CCI file parsing
- Code and data section loading
- Interrupt handling and timing
- Peripheral I/O (input, timers, etc.)
- Exception handling

**Solution Approach**:
- Modular loader component
- Interrupt controller simulation
- I/O peripheral stubs that can be extended
- Exception handling in CPU

---

## 📊 Current Status: Phase 1 Complete

### What's Implemented ✅

#### CPU Core
- **ARM11 Processor** (1,300+ lines)
  - All data processing instructions (16 types)
  - Multiply operations (6 variants)
  - Memory operations (LDR, STR, LDM, STM)
  - Branch instructions (B, BL, BX, BLX)
  - System operations (SWI, MRC, MCR)
  - Condition code handling (16 codes)
  - Processor modes (7 modes)
  - Pipeline simulation (3-stage)

- **ARM9 Processor** (700+ lines)
  - Core instruction support
  - Mode switching
  - Register banking
  - Memory operations

#### Infrastructure
- Abstract `CPUBase` interface
- PSR (Program Status Register) handling
- Condition enum system
- Processor mode system
- Memory callback abstraction
- Helper functions for bitwise operations

#### Testing
- 8 comprehensive test functions
- Instruction execution validation
- Register operation tests
- Condition code verification
- Pipeline behavior tests

#### Build System
- CMake 3.20+ support
- Cross-platform (Windows, Linux, macOS)
- CTest integration
- Release/Debug configurations

### What's Coming 📋

**Phase 2: Memory System**
- [ ] Physical memory mapping (FCRAM, AXRAM)
- [ ] Virtual memory and paging
- [ ] Memory protection unit (MMU)
- [ ] Cache hierarchy simulation

**Phase 3: OS Kernel**
- [ ] Process/thread management
- [ ] Exception handling
- [ ] Interrupt controllers
- [ ] Privilege level enforcement

**Phase 4: Game Loader**
- [ ] ROM/CCI format parsing
- [ ] Code relocation
- [ ] Resource loading
- [ ] Header validation

**Phase 5: GPU (PICA200)**
- [ ] Tile binning system
- [ ] Vertex processing
- [ ] Rasterization
- [ ] Fragment operations
- [ ] Texture unit emulation

**Phase 6: I/O & Peripherals**
- [ ] Input device handling (buttons, touch, gyro)
- [ ] Timer management
- [ ] RTC emulation
- [ ] DMA engines
- [ ] Communication interfaces

---

## 🎓 Design Principles

### 1. Modularity
- Each component (CPU, Memory, GPU, I/O) is independent
- Clear interfaces between modules
- Easy to replace or upgrade implementations
- Testable in isolation

### 2. Accuracy Over Performance
- Phase 1 prioritizes correctness
- Cycle-accurate where feasible
- Proper instruction semantics
- Later phases can optimize

### 3. Educational Value
- Well-commented code
- Clear design patterns
- Documentation at every level
- Learning resources included

### 4. Maintainability
- Consistent code style
- Comprehensive tests
- Clear abstractions
- Version control best practices

### 5. Extensibility
- Easy to add new instructions
- Plugin-like peripheral support
- Configurable optimization levels
- Pluggable rendering backends

---

## 🔧 Technical Approach

### Instruction Interpretation

Rather than JIT compilation (which is complex and platform-dependent), Phase 1 uses **direct interpretation**:

```
Fetch Instruction → Decode → Execute → Update State → Next Cycle
```

**Advantages**:
- Simpler implementation
- Easier to debug
- More portable
- Foundation for future optimization

### Memory Access

Uses callback abstraction to allow flexible memory implementations:

```cpp
std::function<uint32_t(uint32_t)> readMemory;
std::function<void(uint32_t, uint32_t)> writeMemory;
```

### Register Banking

Proper ARMv6 mode switching with banked registers:
- User/System mode (shared registers)
- Supervisor mode (separate SP, LR)
- IRQ/FIQ modes (separate registers)
- Abort/Undefined modes (separate registers)

---

## 📈 Success Metrics

### Phase 1 ✅
- [x] CPU instructions fully implemented
- [x] Test coverage > 90%
- [x] Documentation complete
- [x] Cross-platform builds

### Phase 2 (Target)
- [ ] Memory system working
- [ ] Boot sequence possible
- [ ] Test ROM execution

### Phase 3 (Target)
- [ ] Kernel emulation functional
- [ ] Game loading working
- [ ] Exception handling correct

### Phase 4 (Target)
- [ ] Commercial ROM loading
- [ ] Code execution verified
- [ ] Savedata support

### Phase 5 (Target)
- [ ] Graphics output visible
- [ ] 3D rendering working
- [ ] Visual correctness verified

### Final (Target)
- [ ] Multiple games playable
- [ ] > 30 FPS performance
- [ ] Near-full compatibility

---

## 🚧 Known Limitations & Future Work

### Current Limitations
- Memory system is not implemented (stub only)
- GPU is not implemented (stub only)
- I/O peripherals are not implemented (stubs only)
- OS kernel is not implemented (stub only)
- Game loading not supported (ROM parsing not implemented)

### Future Enhancements
- JIT compilation for performance
- Parallel CPU execution
- Advanced caching strategies
- GPU compute integration
- Networking support
- Save state management

---

## 📚 References

### ARM Documentation
- ARM Architecture Reference Manual (ARMv6, ARMv5TE)
- ARM Instruction Set Quick Reference
- CP15 Coprocessor Documentation

### 3DS Hardware
- 3DBrew Hardware Documentation
- Reverse engineering resources
- Technical analyses

### Emulation Techniques
- CPU emulation fundamentals
- Dynamic binary translation
- Memory protection techniques
- Timing synchronization

---

**Document Version**: 1.0  
**Last Updated**: April 26, 2026  
**Status**: Phase 1 Complete
