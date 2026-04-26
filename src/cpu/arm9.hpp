#pragma once

#include <cpu_base.hpp>
#include <cstdint>
#include <array>
#include <memory>

namespace n3ds {

/**
 * @class ARM9
 * @brief ARM9 security processor (ARMv5TE instruction set)
 * 
 * The 3DS includes a second ARM9 processor for:
 * - Running the bootrom (initializing hardware)
 * - Crypto operations (AES, SHA-1, RSA)
 * - NDMA (DSP) coordination
 * - Firmware protection
 * 
 * This implementation is a simplified interpreter supporting core ARMv5TE
 * instructions needed for OS initialization. Full crypto acceleration is
 * deferred to Phase 2-3 (HLE kernel would intercept crypto SVCs).
 * 
 * Note: ARM9 typically runs at lower frequency and handles less critical
 * operations than ARM11 in a full emulator.
 */
class ARM9 : public CPUBase {
public:
    ARM9();
    virtual ~ARM9() = default;

    // ===== Register Access =====
    uint32_t getReg(int index) const override;
    void setReg(int index, uint32_t value) override;
    uint32_t getCPSR() const override { return cpsr; }
    void setCPSR(uint32_t value) override;
    uint32_t getSPSR() const override;
    void setSPSR(uint32_t value) override;
    uint32_t getPC() const override { return registers[15]; }
    void setPC(uint32_t value) override { registers[15] = value; }

    // ===== Memory Interface =====
    using MemReadFn = std::function<uint32_t(uint32_t addr)>;
    using MemWriteFn = std::function<void(uint32_t addr, uint32_t value)>;
    void setMemoryCallbacks(MemReadFn read, MemWriteFn write) {
        memRead = read;
        memWrite = write;
    }

    // ===== Execution =====
    void step() override;

    // ===== Debugging =====
    std::string disassemble(uint32_t addr) const override;
    std::string getStateString() const override;

    // ===== Direct Instruction Execution (for testing) =====
    void executeInstruction(uint32_t opcode);

private:
    // ===== Register Banks =====
    std::array<uint32_t, 16> registers;
    uint32_t cpsr;
    
    // Simplified: Single SPSR (in real ARM, varies by mode)
    uint32_t spsr;

    // CP15 registers (ARM9 has fewer than ARM11)
    std::array<uint32_t, 16> cp15_regs;

    // Pipeline state
    uint32_t fetched_instruction;
    uint32_t decoded_instruction;
    bool thumb_mode;

    // Memory callbacks
    MemReadFn memRead;
    MemWriteFn memWrite;

    // ===== Helper Methods =====
    bool evalCondition(Condition cond) const;
    
    // Instruction implementations (subset of ARMv5TE)
    void exec_DataProcessing(uint32_t opcode);
    void exec_Multiply(uint32_t opcode);
    void exec_SingleDataTransfer(uint32_t opcode);
    void exec_BlockDataTransfer(uint32_t opcode);
    void exec_Branch(uint32_t opcode);
    void exec_BranchLink(uint32_t opcode);
    void exec_SoftwareInterrupt(uint32_t opcode);
    void exec_Coprocessor(uint32_t opcode);

    // Addressing and arithmetic
    uint32_t rotateBits(uint32_t value, uint32_t shift);
    uint32_t readMemory32(uint32_t addr);
    uint8_t readMemory8(uint32_t addr);
    void writeMemory32(uint32_t addr, uint32_t value);
    void writeMemory8(uint32_t addr, uint8_t value);
};

} // namespace n3ds
