#include "arm11.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <functional>

namespace n3ds {

ARM11::ARM11() 
    : cpsr(0x13), fetched_instruction(0), decoded_instruction(0), thumb_mode(false) {
    // Initialize register file
    registers.fill(0);
    cp15_regs.fill(0);
    
    // Initialize banked registers for each mode
    for (int mode = 0; mode <= 0x1F; ++mode) {
        ARMMode m = static_cast<ARMMode>(mode);
        spsr_bank[m] = 0;
        r13_bank[m] = 0;
        r14_bank[m] = 0;
    }
    
    // Set default CP15 registers
    // CP15 c0: MIDR (Main ID Register) — ARM1176JZ-S (3DS ARM11)
    cp15_regs[0] = 0x410FB766;
    
    // CP15 c1: SCTLR (System Control Register)
    cp15_regs[1] = 0x78;  // MMU disabled, caches disabled (typical boot state)
    
    // Set Supervisor mode (0x13)
    setCurrentMode(ARMMode::Supervisor);
}

// ===== Register Access =====

uint32_t ARM11::getReg(int index) const {
    if (index < 0 || index > 15) {
        throw std::out_of_range("Invalid register index");
    }
    return registers[index];
}

void ARM11::setReg(int index, uint32_t value) {
    if (index < 0 || index > 15) {
        throw std::out_of_range("Invalid register index");
    }
    if (index == 15) {
        // PC written — branch with pipeline flush
        registers[15] = value;
    } else {
        registers[index] = value;
    }
}

void ARM11::setCPSR(uint32_t value) {
    PSR psr = PSR::fromU32(value);
    cpsr = value;
    
    // Update mode-dependent banked registers if mode changed
    ARMMode old_mode = getCurrentMode();
    ARMMode new_mode = static_cast<ARMMode>(psr.M);
    
    if (old_mode != new_mode) {
        // Save old mode's R13 and R14
        r13_bank[old_mode] = registers[13];
        r14_bank[old_mode] = registers[14];
        
        // Load new mode's R13 and R14
        registers[13] = r13_bank[new_mode];
        registers[14] = r14_bank[new_mode];
    }
    
    thumb_mode = (psr.IT != 0) || ((value >> 5) & 1);  // T bit in Thumb
}

uint32_t ARM11::getSPSR() const {
    ARMMode mode = getCurrentMode();
    auto it = spsr_bank.find(mode);
    if (it != spsr_bank.end()) {
        return it->second;
    }
    return 0;
}

void ARM11::setSPSR(uint32_t value) {
    ARMMode mode = getCurrentMode();
    spsr_bank[mode] = value;
}

// ===== Mode Management =====

ARMMode ARM11::getCurrentMode() const {
    return static_cast<ARMMode>(cpsr & 0x1F);
}

void ARM11::setCurrentMode(ARMMode mode) {
    PSR psr = PSR::fromU32(cpsr);
    psr.M = static_cast<uint8_t>(mode);
    cpsr = psr.toU32();
}

// ===== Condition Evaluation =====

bool ARM11::evalCondition(Condition cond) const {
    PSR psr = PSR::fromU32(cpsr);
    
    switch (cond) {
        case Condition::EQ: return psr.Z;
        case Condition::NE: return !psr.Z;
        case Condition::CS: return psr.C;
        case Condition::CC: return !psr.C;
        case Condition::MI: return psr.N;
        case Condition::PL: return !psr.N;
        case Condition::VS: return psr.V;
        case Condition::VC: return !psr.V;
        case Condition::HI: return psr.C && !psr.Z;
        case Condition::LS: return !psr.C || psr.Z;
        case Condition::GE: return psr.N == psr.V;
        case Condition::LT: return psr.N != psr.V;
        case Condition::GT: return !psr.Z && (psr.N == psr.V);
        case Condition::LE: return psr.Z || (psr.N != psr.V);
        case Condition::AL: return true;
        case Condition::NV: return false;
        default: return false;
    }
}

// ===== Execution =====

void ARM11::step() {
    // Simplified 3-stage pipeline: Fetch -> Decode -> Execute
    
    if (!memRead) {
        // No memory callback set — cannot fetch
        return;
    }
    
    // Execute previously decoded instruction
    if (decoded_instruction != 0) {
        executeInstruction(decoded_instruction);
        decoded_instruction = 0;
    }
    
    // Decode fetched instruction
    if (fetched_instruction != 0) {
        decoded_instruction = fetched_instruction;
        fetched_instruction = 0;
    }
    
    // Fetch next instruction from PC
    uint32_t pc = registers[15];
    fetched_instruction = memRead(pc);
    
    // Increment PC (ARM: 4 bytes, Thumb: 2 bytes)
    registers[15] += thumb_mode ? 2 : 4;
}

void ARM11::executeInstruction(uint32_t opcode) {
    // Check condition code (bits 31:28)
    Condition cond = static_cast<Condition>((opcode >> 28) & 0xF);
    if (!evalCondition(cond)) {
        return;  // Instruction skipped
    }
    
    // Decode instruction class (bits 27:25)
    uint32_t bits_27_25 = (opcode >> 25) & 0x7;
    
    switch (bits_27_25) {
        case 0b000:
            // Data processing, Multiply, or Undefined
            if ((opcode & 0x0F900090) == 0x01000090) {
                exec_Multiply(opcode);
            } else if ((opcode & 0x0F800090) == 0x00800090) {
                exec_MultiplyLong(opcode);
            } else {
                exec_DataProcessing(opcode);
            }
            break;
        
        case 0b001:
            // Data processing (immediate)
            exec_DataProcessing(opcode);
            break;
        
        case 0b010:
        case 0b011:
            // Single Data Transfer (LDR/STR)
            exec_SingleDataTransfer(opcode);
            break;
        
        case 0b100:
            // Block Data Transfer (LDM/STM)
            exec_BlockDataTransfer(opcode);
            break;
        
        case 0b101:
            // Branch and Branch with Link
            if (opcode & 0x01000000) {
                exec_BranchLink(opcode);
            } else {
                exec_Branch(opcode);
            }
            break;
        
        case 0b110:
        case 0b111:
            // Coprocessor Data Transfer, Coprocessor Data Operation, SWI
            if ((opcode & 0x0F000000) == 0x0F000000) {
                exec_SoftwareInterrupt(opcode);
            } else {
                exec_Coprocessor(opcode);
            }
            break;
        
        default:
            exec_Undefined(opcode);
            break;
    }
}

// ===== Data Processing =====

void ARM11::exec_DataProcessing(uint32_t opcode) {
    // Bits: cond(4) 00I(3) opcode(4) S(1) Rn(4) Rd(4) operand2(8)
    
    uint32_t opcode_bits = (opcode >> 21) & 0xF;
    bool S = (opcode >> 20) & 1;  // Set flags
    uint32_t Rd = (opcode >> 12) & 0xF;
    uint32_t Rn = (opcode >> 16) & 0xF;
    
    // Calculate operand2
    uint32_t operand2;
    bool carry_out = false;
    
    if (opcode & 0x02000000) {
        // Immediate operand: 8-bit with ROR
        uint32_t imm = opcode & 0xFF;
        uint32_t shift = ((opcode >> 8) & 0xF) * 2;
        operand2 = rotateBits(imm, shift);
        carry_out = (shift > 0) && ((imm >> (shift - 1)) & 1);
    } else {
        // Register operand
        uint32_t Rm = opcode & 0xF;
        uint32_t shift_type = (opcode >> 5) & 0x3;
        uint32_t shift_amt;
        
        if (opcode & 0x00000010) {
            // Shift amount in register (Rs)
            shift_amt = registers[(opcode >> 8) & 0xF] & 0xFF;
        } else {
            // Shift amount is immediate
            shift_amt = (opcode >> 7) & 0x1F;
        }
        
        operand2 = registers[Rm];
        carry_out = (cpsr >> 29) & 1;  // Carry in
        
        // Apply shift
        switch (shift_type) {
            case 0b00:  // LSL
                if (shift_amt < 32) {
                    carry_out = (shift_amt > 0) && ((operand2 >> (32 - shift_amt)) & 1);
                    operand2 <<= shift_amt;
                } else if (shift_amt == 32) {
                    carry_out = operand2 & 1;
                    operand2 = 0;
                } else {
                    carry_out = false;
                    operand2 = 0;
                }
                break;
            case 0b01:  // LSR
                if (shift_amt < 32) {
                    carry_out = (operand2 >> (shift_amt - 1)) & 1;
                    operand2 >>= shift_amt;
                } else if (shift_amt == 32) {
                    carry_out = (operand2 >> 31) & 1;
                    operand2 = 0;
                } else {
                    carry_out = false;
                    operand2 = 0;
                }
                break;
            case 0b10:  // ASR
                if (shift_amt < 32) {
                    carry_out = (operand2 >> (shift_amt - 1)) & 1;
                    operand2 = static_cast<uint32_t>(static_cast<int32_t>(operand2) >> shift_amt);
                } else {
                    carry_out = (operand2 >> 31) & 1;
                    operand2 = static_cast<uint32_t>(static_cast<int32_t>(operand2) >> 31);
                }
                break;
            case 0b11:  // ROR
                carry_out = (operand2 >> (shift_amt - 1)) & 1;
                operand2 = rotateBits(operand2, shift_amt);
                break;
        }
    }
    
    uint32_t Rn_val = registers[Rn];
    uint32_t result = 0;
    bool new_carry = carry_out;
    bool overflow = false;
    
    switch (opcode_bits) {
        case 0b0000:  // AND
            result = Rn_val & operand2;
            new_carry = carry_out;
            break;
        
        case 0b0001:  // EOR (XOR)
            result = Rn_val ^ operand2;
            new_carry = carry_out;
            break;
        
        case 0b0010:  // SUB
            result = Rn_val - operand2;
            new_carry = !(Rn_val < operand2);
            overflow = ((Rn_val ^ operand2) & (Rn_val ^ result)) >> 31;
            break;
        
        case 0b0011:  // RSB (Reverse Subtract)
            result = operand2 - Rn_val;
            new_carry = !(operand2 < Rn_val);
            overflow = ((operand2 ^ Rn_val) & (operand2 ^ result)) >> 31;
            break;
        
        case 0b0100:  // ADD
            result = Rn_val + operand2;
            new_carry = (result < Rn_val);
            overflow = !((Rn_val ^ operand2) >> 31) && ((Rn_val ^ result) >> 31);
            break;
        
        case 0b0101:  // ADC (Add with Carry)
            {
                uint64_t sum = static_cast<uint64_t>(Rn_val) + operand2 + ((cpsr >> 29) & 1);
                result = static_cast<uint32_t>(sum);
                new_carry = sum >> 32;
                overflow = !((Rn_val ^ operand2) >> 31) && ((Rn_val ^ result) >> 31);
            }
            break;
        
        case 0b0110:  // SBC (Subtract with Carry)
            {
                uint64_t diff = static_cast<uint64_t>(Rn_val) - operand2 - (1 - ((cpsr >> 29) & 1));
                result = static_cast<uint32_t>(diff);
                new_carry = !(diff >> 32);
                overflow = ((Rn_val ^ operand2) & (Rn_val ^ result)) >> 31;
            }
            break;
        
        case 0b0111:  // RSC (Reverse Subtract with Carry)
            {
                uint64_t diff = static_cast<uint64_t>(operand2) - Rn_val - (1 - ((cpsr >> 29) & 1));
                result = static_cast<uint32_t>(diff);
                new_carry = !(diff >> 32);
                overflow = ((operand2 ^ Rn_val) & (operand2 ^ result)) >> 31;
            }
            break;
        
        case 0b1000:  // TST (Test)
            result = Rn_val & operand2;
            new_carry = carry_out;
            break;
        
        case 0b1001:  // TEQ (Test Equivalence)
            result = Rn_val ^ operand2;
            new_carry = carry_out;
            break;
        
        case 0b1010:  // CMP (Compare)
            result = Rn_val - operand2;
            new_carry = !(Rn_val < operand2);
            overflow = ((Rn_val ^ operand2) & (Rn_val ^ result)) >> 31;
            break;
        
        case 0b1011:  // CMN (Compare Negative)
            result = Rn_val + operand2;
            new_carry = (result < Rn_val);
            overflow = !((Rn_val ^ operand2) >> 31) && ((Rn_val ^ result) >> 31);
            break;
        
        case 0b1100:  // ORR (OR)
            result = Rn_val | operand2;
            new_carry = carry_out;
            break;
        
        case 0b1101:  // MOV
            result = operand2;
            new_carry = carry_out;
            break;
        
        case 0b1110:  // BIC (Bit Clear)
            result = Rn_val & ~operand2;
            new_carry = carry_out;
            break;
        
        case 0b1111:  // MVN (Move NOT)
            result = ~operand2;
            new_carry = carry_out;
            break;
    }
    
    // Write result to Rd
    if (Rd != 15) {
        registers[Rd] = result;
    } else {
        // PC written — branch
        registers[15] = result;
        fetched_instruction = 0;
        decoded_instruction = 0;
    }
    
    // Update CPSR if S bit set
    if (S) {
        PSR psr = PSR::fromU32(cpsr);
        psr.N = (result >> 31) & 1;
        psr.Z = (result == 0) ? 1 : 0;
        psr.C = new_carry ? 1 : 0;
        psr.V = overflow ? 1 : 0;
        cpsr = psr.toU32();
    }
}

// ===== Multiply =====

void ARM11::exec_Multiply(uint32_t opcode) {
    // MUL/MLA: cond(4) 000000A(3) S(1) Rd(4) Rn(4) Rs(4) 1001(4) Rm(4)
    uint32_t Rd = (opcode >> 16) & 0xF;
    uint32_t Rn = (opcode >> 12) & 0xF;
    uint32_t Rs = (opcode >> 8) & 0xF;
    uint32_t Rm = opcode & 0xF;
    bool A = (opcode >> 21) & 1;  // Accumulate
    bool S = (opcode >> 20) & 1;  // Set flags
    
    uint64_t result = static_cast<uint64_t>(registers[Rm]) * registers[Rs];
    
    if (A) {
        result += registers[Rn];
    }
    
    registers[Rd] = static_cast<uint32_t>(result);
    
    if (S) {
        PSR psr = PSR::fromU32(cpsr);
        uint32_t res32 = static_cast<uint32_t>(result);
        psr.N = (res32 >> 31) & 1;
        psr.Z = (res32 == 0) ? 1 : 0;
        cpsr = psr.toU32();
    }
}

// ===== Multiply Long =====

void ARM11::exec_MultiplyLong(uint32_t opcode) {
    // MULL/MLAL: cond(4) 00001A(3) S(1) RdHi(4) RdLo(4) Rs(4) 1001(4) Rm(4)
    uint32_t RdHi = (opcode >> 16) & 0xF;
    uint32_t RdLo = (opcode >> 12) & 0xF;
    uint32_t Rs = (opcode >> 8) & 0xF;
    uint32_t Rm = opcode & 0xF;
    bool A = (opcode >> 21) & 1;
    bool U = (opcode >> 22) & 1;  // Unsigned
    bool S = (opcode >> 20) & 1;
    
    uint64_t result;
    
    if (U) {
        // Unsigned multiply
        uint64_t op1 = registers[Rm];
        uint64_t op2 = registers[Rs];
        result = op1 * op2;
    } else {
        // Signed multiply
        int64_t op1 = static_cast<int32_t>(registers[Rm]);
        int64_t op2 = static_cast<int32_t>(registers[Rs]);
        result = static_cast<uint64_t>(op1 * op2);
    }
    
    if (A) {
        uint64_t acc = (static_cast<uint64_t>(registers[RdHi]) << 32) | registers[RdLo];
        result += acc;
    }
    
    registers[RdLo] = static_cast<uint32_t>(result);
    registers[RdHi] = static_cast<uint32_t>(result >> 32);
    
    if (S) {
        PSR psr = PSR::fromU32(cpsr);
        psr.N = (registers[RdHi] >> 31) & 1;
        psr.Z = (registers[RdHi] == 0 && registers[RdLo] == 0) ? 1 : 0;
        cpsr = psr.toU32();
    }
}

// ===== Single Data Transfer =====

void ARM11::exec_SingleDataTransfer(uint32_t opcode) {
    // LDR/STR: cond(4) 01IUBP(6) Rn(4) Rd(4) addr(12)
    bool I = (opcode >> 25) & 1;  // Immediate offset
    bool U = (opcode >> 23) & 1;  // Up/Down
    bool B = (opcode >> 22) & 1;  // Byte/Word
    bool P = (opcode >> 24) & 1;  // Pre/Post-index
    bool L = (opcode >> 20) & 1;  // Load/Store
    
    uint32_t Rn = (opcode >> 16) & 0xF;
    uint32_t Rd = (opcode >> 12) & 0xF;
    
    uint32_t addr = registers[Rn];
    uint32_t offset;
    
    if (I) {
        offset = opcode & 0xFFF;
    } else {
        offset = registers[opcode & 0xF];
    }
    
    if (U) {
        addr += offset;
    } else {
        addr -= offset;
    }
    
    if (!memRead || !memWrite) {
        // Memory callbacks not set
        return;
    }
    
    if (L) {
        // Load
        if (B) {
            registers[Rd] = readMemory8(addr);
        } else {
            registers[Rd] = readMemory32(addr);
        }
    } else {
        // Store
        if (B) {
            writeMemory8(addr, registers[Rd] & 0xFF);
        } else {
            writeMemory32(addr, registers[Rd]);
        }
    }
    
    if (P) {
        // Post-index: update Rn after transfer
        if (U) {
            registers[Rn] += offset;
        } else {
            registers[Rn] -= offset;
        }
    }
}

// ===== Block Data Transfer =====

void ARM11::exec_BlockDataTransfer(uint32_t opcode) {
    // LDM/STM: cond(4) 100PUBW(5) Rn(4) register_list(16)
    bool P = (opcode >> 24) & 1;
    bool U = (opcode >> 23) & 1;
    bool S = (opcode >> 22) & 1;
    bool W = (opcode >> 21) & 1;
    bool L = (opcode >> 20) & 1;
    
    uint32_t Rn = (opcode >> 16) & 0xF;
    uint16_t reg_list = opcode & 0xFFFF;
    
    if (!memRead || !memWrite) return;
    
    uint32_t addr = registers[Rn];
    int count = __builtin_popcount(reg_list);
    
    if (U) {
        // Ascending: start at base address
        if (P) addr += 4;  // Pre-index
    } else {
        // Descending: start at base address - 4*count
        addr -= 4 * count;
        if (P) addr += 4;
    }
    
    uint32_t base = addr;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            if (L) {
                registers[i] = readMemory32(addr);
            } else {
                writeMemory32(addr, registers[i]);
            }
            addr += 4;
        }
    }
    
    // Write back base register if W bit set
    if (W) {
        if (U) {
            registers[Rn] += 4 * count;
        } else {
            registers[Rn] -= 4 * count;
        }
    }
}

// ===== Branch =====

void ARM11::exec_Branch(uint32_t opcode) {
    // B: cond(4) 1010(4) offset(24)
    int32_t offset = signExtend(opcode & 0xFFFFFF, 24) << 2;
    registers[15] += offset;
    
    // Flush pipeline
    fetched_instruction = 0;
    decoded_instruction = 0;
}

void ARM11::exec_BranchLink(uint32_t opcode) {
    // BL: cond(4) 1011(4) offset(24)
    int32_t offset = signExtend(opcode & 0xFFFFFF, 24) << 2;
    registers[14] = registers[15];  // Link return address
    registers[15] += offset;
    
    // Flush pipeline
    fetched_instruction = 0;
    decoded_instruction = 0;
}

// ===== Software Interrupt =====

void ARM11::exec_SoftwareInterrupt(uint32_t opcode) {
    // SWI: cond(4) 1111(4) 24-bit comment
    
    // Transition to Supervisor mode
    PSR psr = PSR::fromU32(cpsr);
    
    // Save current CPSR to SPSR_svc
    spsr_bank[ARMMode::Supervisor] = cpsr;
    
    // Set CPSR to Supervisor mode, IRQ disabled
    psr.M = static_cast<uint8_t>(ARMMode::Supervisor);
    psr.N = 0; psr.Z = 0; psr.C = 0; psr.V = 0;
    cpsr = psr.toU32();
    
    // Set PC to SVC vector (0x08)
    registers[15] = 0x08;
    registers[14] = registers[15] - 4;  // Return address (before SWI)
    
    fetched_instruction = 0;
    decoded_instruction = 0;
}

// ===== Coprocessor =====

void ARM11::exec_Coprocessor(uint32_t opcode) {
    // CP15: cond(4) 1110(4) ...
    uint32_t cp = (opcode >> 8) & 0xF;
    
    if (cp == 15) {
        // CP15 (System Control Coprocessor)
        uint32_t op1 = (opcode >> 21) & 0x7;
        uint32_t CRn = (opcode >> 16) & 0xF;
        uint32_t Rd = (opcode >> 12) & 0xF;
        uint32_t CRm = opcode & 0xF;
        uint32_t op2 = (opcode >> 5) & 0x7;
        bool L = (opcode >> 20) & 1;
        
        // Simplified: just read/write to cp15_regs
        if (L) {
            // MRC: Move from Coprocessor to Register
            registers[Rd] = cp15_regs[CRn];
        } else {
            // MCR: Move from Register to Coprocessor
            cp15_regs[CRn] = registers[Rd];
        }
    }
}

// ===== Undefined =====

void ARM11::exec_Undefined(uint32_t opcode) {
    // Undefined instruction exception
    throw std::runtime_error("Undefined instruction");
}

// ===== Helper Functions =====

uint32_t ARM11::rotateBits(uint32_t value, uint32_t shift) {
    shift &= 0x1F;
    if (shift == 0) return value;
    return (value >> shift) | (value << (32 - shift));
}

uint32_t ARM11::readMemory32(uint32_t addr) {
    return memRead(addr);
}

uint16_t ARM11::readMemory16(uint32_t addr) {
    uint32_t word = memRead(addr & ~3);
    int shift = (addr & 2) * 8;
    return (word >> shift) & 0xFFFF;
}

uint8_t ARM11::readMemory8(uint32_t addr) {
    uint32_t word = memRead(addr & ~3);
    int shift = (addr & 3) * 8;
    return (word >> shift) & 0xFF;
}

void ARM11::writeMemory32(uint32_t addr, uint32_t value) {
    memWrite(addr, value);
}

void ARM11::writeMemory16(uint32_t addr, uint16_t value) {
    uint32_t word = memRead(addr & ~3);
    int shift = (addr & 2) * 8;
    word = (word & ~(0xFFFFu << shift)) | (static_cast<uint32_t>(value) << shift);
    memWrite(addr & ~3, word);
}

void ARM11::writeMemory8(uint32_t addr, uint8_t value) {
    uint32_t word = memRead(addr & ~3);
    int shift = (addr & 3) * 8;
    word = (word & ~(0xFFu << shift)) | (static_cast<uint32_t>(value) << shift);
    memWrite(addr & ~3, word);
}

// ===== Debugging =====

std::string ARM11::disassemble(uint32_t addr) const {
    // Placeholder disassembler
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(8) << addr << ": ";
    oss << "[disassembly not yet implemented]";
    return oss.str();
}

std::string ARM11::getStateString() const {
    std::ostringstream oss;
    oss << "=== ARM11 Processor State ===\n";
    oss << "PC (R15): 0x" << std::hex << std::setfill('0') << std::setw(8) << registers[15] << "\n";
    oss << "CPSR: 0x" << std::setw(8) << cpsr << "\n";
    oss << "Registers:\n";
    for (int i = 0; i < 16; ++i) {
        oss << "  R" << std::dec << i << ": 0x" << std::hex << std::setfill('0') << std::setw(8) << registers[i] << "\n";
    }
    return oss.str();
}

} // namespace n3ds
