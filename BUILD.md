# Build Instructions for N3DS Emulator Phase 1

## Prerequisites

You need to install these tools first:

### Windows

1. **Visual Studio 2019 or 2022** (Community Edition is free)
   - https://visualstudio.microsoft.com/downloads/
   - During installation, select "Desktop development with C++" workload
   
2. **CMake 3.20+**
   - https://cmake.org/download/
   - Choose "Windows x64 Installer"
   - During installation, select "Add CMake to the system PATH"

### macOS

```bash
# Install via Homebrew
brew install cmake
```

### Linux (Ubuntu/Debian)

```bash
sudo apt-get install build-essential cmake
```

## Building

Once prerequisites are installed:

### Windows (Visual Studio)

```bash
cd Custom-3DS
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
ctest --output-on-failure
```

### Windows (MinGW)

```bash
cd Custom-3DS
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
ctest --output-on-failure
```

### macOS / Linux

```bash
cd Custom-3DS
mkdir build
cd build
cmake ..
make
ctest --output-on-failure
```

## Troubleshooting

**"CMake not found"**
- Ensure CMake is installed and added to PATH
- Restart your terminal after installing

**"C++ compiler not found"**
- Install Visual Studio with C++ tools (Windows)
- Install build-essential (Linux)
- Xcode Command Line Tools (macOS): `xcode-select --install`

**Build errors with MSVC**
- Ensure you're using C++17 or later
- Try: `cmake .. -G "Visual Studio 17 2022" -DCMAKE_CXX_STANDARD=17`

## Verification

After building, you should see:

```
✓ All tests passed!
```

This confirms:
- ARM11 register operations work
- ARM9 register operations work
- Data processing instructions execute correctly
- PSR structure is correctly encoded/decoded

## Next Steps

Once built successfully:

1. Review [README.md](../README.md) for architecture details
2. Proceed to Phase 2 (Memory System) when ready
3. Examine the generated executables:
   - `test_cpu` or `test_cpu.exe` — Unit test suite
