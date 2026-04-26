#pragma once

#include <cpu_base.hpp>
#include <cstdint>
#include <array>
#include <memory>
#include <map>

namespace n3ds {

/**
 * @class ARM11
 * @brief ARM11 MPCore processor emulator (ARMv6K instruction set)
 * 
 * The ARM11 is the primary application processor on 3DS.
 * - New 3DS: 2x cores @ ~804 MHz
 * - Old 3DS: 1x core @ ~268 MHz
 * 
 * This implementation is a single-core interpreter supporting:
 * - Full ARMv6K instruction set (ARM and Thumb modes)
 * - All addressing modes
 * - Condition code evaluation
 * - CP15 (system control coprocessor)
 * - Basic pipeline simulation
 * 
 * Note: Multi-core synchronization is deferred to Phase 3.
 */
class ARM11 : public CPUBase {
public:
    ARM11();
    virtual ~ARM11() = default;

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
    /** Set external memory read callback (for memory phase) */
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
    // General-purpose registers R0-R15
    std::array<uint32_t, 16> registers;

    // CPSR (Current Program Status Register)
    uint32_t cpsr;

    // Banked registers for different modes (SPSR, R13_banked, R14_banked)
    std::map<ARMMode, uint32_t> spsr_bank;
    std::map<ARMMode, uint32_t> r13_bank;
    std::map<ARMMode, uint32_t> r14_bank;

    // CP15 registers (simplified)
    std::array<uint32_t, 16> cp15_regs;

    // Pipeline state (simplified 3-stage)
    uint32_t fetched_instruction;
    uint32_t decoded_instruction;
    bool thumb_mode;

    // Memory callbacks
    MemReadFn memRead;
    MemWriteFn memWrite;

    // ===== Helper Methods =====
    ARMMode getCurrentMode() const;
    void setCurrentMode(ARMMode mode);

    // Condition evaluation
    bool evalCondition(Condition cond) const;
    
    // Instruction decode & execute
    void decodeARM(uint32_t opcode);
    void decodeThumb(uint16_t opcode);
    
    // ARM instruction implementations
    void exec_DataProcessing(uint32_t opcode);
    void exec_Multiply(uint32_t opcode);
    void exec_MultiplyLong(uint32_t opcode);
    void exec_SingleDataTransfer(uint32_t opcode);
    void exec_BlockDataTransfer(uint32_t opcode);
    void exec_Branch(uint32_t opcode);
    void exec_BranchLink(uint32_t opcode);
    void exec_SoftwareInterrupt(uint32_t opcode);
    void exec_Coprocessor(uint32_t opcode);
    void exec_Undefined(uint32_t opcode);

    // Addressing mode calculations
    uint32_t calcAddressMode2(uint32_t opcode); // Load/Store addressing
    uint32_t calcAddressMode4(uint32_t opcode); // Block transfer addressing

    // Flag updates
    void updateFlags(uint32_t result, bool carry, bool overflow);
    uint32_t rotateBits(uint32_t value, uint32_t shift);

    // Memory access helpers (for when memRead/memWrite are available)
    uint32_t readMemory32(uint32_t addr);
    uint16_t readMemory16(uint32_t addr);
    uint8_t readMemory8(uint32_t addr);
    void writeMemory32(uint32_t addr, uint32_t value);
    void writeMemory16(uint32_t addr, uint16_t value);
    void writeMemory8(uint32_t addr, uint8_t value);
};

} // namespace n3ds
