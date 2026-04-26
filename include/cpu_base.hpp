#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <memory>

namespace n3ds {

/**
 * @class CPUBase
 * @brief Abstract base class for ARM processors in the 3DS
 * 
 * The 3DS contains two ARM processors:
 * - ARM11 MPCore (main application processor, ARMv6K, 2x cores on New3DS)
 * - ARM9 (security processor, ARMv5TE, runs bootrom & crypto)
 */
class CPUBase {
public:
    virtual ~CPUBase() = default;

    // ===== Register Access =====
    /** Get a general-purpose register (R0-R15) */
    virtual uint32_t getReg(int index) const = 0;
    
    /** Set a general-purpose register (R0-R15) */
    virtual void setReg(int index, uint32_t value) = 0;
    
    /** Get the Current Program Status Register */
    virtual uint32_t getCPSR() const = 0;
    
    /** Set the CPSR */
    virtual void setCPSR(uint32_t value) = 0;
    
    /** Get the Saved Program Status Register */
    virtual uint32_t getSPSR() const = 0;
    
    /** Set the SPSR */
    virtual void setSPSR(uint32_t value) = 0;

    // ===== Program Counter Access =====
    virtual uint32_t getPC() const = 0;
    virtual void setPC(uint32_t value) = 0;

    // ===== Execution =====
    /** Execute a single instruction cycle */
    virtual void step() = 0;
    
    /** Execute N instruction cycles */
    virtual void executeFor(int cycles) {
        for (int i = 0; i < cycles; ++i) {
            step();
        }
    }

    // ===== State Query =====
    virtual std::string disassemble(uint32_t addr) const = 0;
    virtual std::string getStateString() const = 0;

protected:
    // Helper: Extract bits from a value
    static inline uint32_t bits(uint32_t value, int start, int count) {
        return (value >> start) & ((1u << count) - 1);
    }

    // Helper: Set bits in a value
    static inline uint32_t setBits(uint32_t value, int start, int count, uint32_t bits_val) {
        uint32_t mask = ((1u << count) - 1) << start;
        return (value & ~mask) | ((bits_val & ((1u << count) - 1)) << start);
    }

    // Helper: Sign extend a value
    static inline int32_t signExtend(uint32_t value, int bits_count) {
        int32_t sign_bit = 1 << (bits_count - 1);
        return (static_cast<int32_t>(value) << (32 - bits_count)) >> (32 - bits_count);
    }
};

/**
 * @enum Condition
 * @brief ARM instruction condition codes (bits 31:28 of instruction)
 */
enum class Condition : uint8_t {
    EQ = 0x0,  // Z set
    NE = 0x1,  // Z clear
    CS = 0x2,  // C set (unsigned higher or same)
    CC = 0x3,  // C clear (unsigned lower)
    MI = 0x4,  // N set (negative)
    PL = 0x5,  // N clear (positive or zero)
    VS = 0x6,  // V set (overflow)
    VC = 0x7,  // V clear (no overflow)
    HI = 0x8,  // C set and Z clear (unsigned higher)
    LS = 0x9,  // C clear or Z set (unsigned lower or same)
    GE = 0xA,  // N == V (signed >=)
    LT = 0xB,  // N != V (signed <)
    GT = 0xC,  // Z clear and N == V (signed >)
    LE = 0xD,  // Z set or N != V (signed <=)
    AL = 0xE,  // Always
    NV = 0xF   // Never (reserved)
};

/**
 * @struct PSR
 * @brief Program Status Register layout (CPSR/SPSR)
 */
struct PSR {
    // Flags (bits 31:28)
    bool N;  // Negative/Sign (bit 31)
    bool Z;  // Zero (bit 30)
    bool C;  // Carry/Borrow (bit 29)
    bool V;  // Overflow (bit 28)

    // Control bits
    bool Q;  // Cumulative saturation (bit 27) — ARMv6K
    bool J;  // Jazelle (bit 24) — legacy, unused on 3DS
    
    uint8_t GE;      // Greater-than-or-Equal (bits 19:16) — SIMD
    uint8_t IT;      // If-Then state (bits 15:10) — Thumb mode
    bool E;          // Big-endian (bit 9)
    uint8_t M;       // Mode (bits 4:0)

    // Extract from raw CPSR/SPSR value
    static PSR fromU32(uint32_t val) {
        PSR psr;
        psr.N = (val >> 31) & 1;
        psr.Z = (val >> 30) & 1;
        psr.C = (val >> 29) & 1;
        psr.V = (val >> 28) & 1;
        psr.Q = (val >> 27) & 1;
        psr.J = (val >> 24) & 1;
        psr.GE = (val >> 16) & 0xF;
        psr.IT = (val >> 10) & 0x3F;
        psr.E = (val >> 9) & 1;
        psr.M = val & 0x1F;
        return psr;
    }

    // Convert to raw 32-bit value
    uint32_t toU32() const {
        uint32_t val = 0;
        val |= (N << 31) | (Z << 30) | (C << 29) | (V << 28);
        val |= (Q << 27) | (J << 24);
        val |= (GE << 16) | (IT << 10);
        val |= (E << 9) | M;
        return val;
    }
};

/**
 * @enum ARMMode
 * @brief ARM processor modes
 */
enum class ARMMode : uint8_t {
    User = 0x10,
    FIQ = 0x11,
    IRQ = 0x12,
    Supervisor = 0x13,
    Abort = 0x17,
    Undefined = 0x1B,
    System = 0x1F
};

} // namespace n3ds
