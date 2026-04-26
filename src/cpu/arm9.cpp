#include "arm9.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace n3ds {

ARM9::ARM9()
    : cpsr(0x13), spsr(0), fetched_instruction(0), decoded_instruction(0), thumb_mode(false) {
    registers.fill(0);
    cp15_regs.fill(0);
    
    // Set default CP15 registers
    // CP15 c0: MIDR (ARM9TDMI or compatible)
    cp15_regs[0] = 0x41059000;
    
    // CP15 c1: SCTLR
    cp15_regs[1] = 0x78;
}

// ===== Register Access =====

uint32_t ARM9::getReg(int index) const {
    if (index < 0 || index > 15) {
        throw std::out_of_range("Invalid register index");
    }
    return registers[index];
}

void ARM9::setReg(int index, uint32_t value) {
    if (index < 0 || index > 15) {
        throw std::out_of_range("Invalid register index");
    }
    if (index == 15) {
        registers[15] = value;
    } else {
        registers[index] = value;
    }
}

void ARM9::setCPSR(uint32_t value) {
    cpsr = value;
    PSR psr = PSR::fromU32(value);
    thumb_mode = (psr.IT != 0) || ((value >> 5) & 1);
}

uint32_t ARM9::getSPSR() const {
    return spsr;
}

void ARM9::setSPSR(uint32_t value) {
    spsr = value;
}

// ===== Execution =====

void ARM9::step() {
    // Simplified 3-stage pipeline
    if (!memRead) return;
    
    if (decoded_instruction != 0) {
        executeInstruction(decoded_instruction);
        decoded_instruction = 0;
    }
    
    if (fetched_instruction != 0) {
        decoded_instruction = fetched_instruction;
        fetched_instruction = 0;
    }
    
    uint32_t pc = registers[15];
    fetched_instruction = memRead(pc);
    registers[15] += thumb_mode ? 2 : 4;
}

void ARM9::executeInstruction(uint32_t opcode) {
    Condition cond = static_cast<Condition>((opcode >> 28) & 0xF);
    if (!evalCondition(cond)) return;
    
    uint32_t bits_27_25 = (opcode >> 25) & 0x7;
    
    switch (bits_27_25) {
        case 0b000:
        case 0b001:
            if ((opcode & 0x0F900090) == 0x01000090) {
                exec_Multiply(opcode);
            } else {
                exec_DataProcessing(opcode);
            }
            break;
        
        case 0b010:
        case 0b011:
            exec_SingleDataTransfer(opcode);
            break;
        
        case 0b100:
            exec_BlockDataTransfer(opcode);
            break;
        
        case 0b101:
            if (opcode & 0x01000000) {
                exec_BranchLink(opcode);
            } else {
                exec_Branch(opcode);
            }
            break;
        
        case 0b110:
        case 0b111:
            if ((opcode & 0x0F000000) == 0x0F000000) {
                exec_SoftwareInterrupt(opcode);
            } else {
                exec_Coprocessor(opcode);
            }
            break;
    }
}

// ===== Condition Evaluation =====

bool ARM9::evalCondition(Condition cond) const {
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

// ===== Data Processing (Simplified) =====

void ARM9::exec_DataProcessing(uint32_t opcode) {
    // Simplified version — focuses on core operations
    uint32_t opcode_bits = (opcode >> 21) & 0xF;
    bool S = (opcode >> 20) & 1;
    uint32_t Rd = (opcode >> 12) & 0xF;
    uint32_t Rn = (opcode >> 16) & 0xF;
    
    uint32_t operand2;
    bool carry_out = false;
    
    if (opcode & 0x02000000) {
        // Immediate
        uint32_t imm = opcode & 0xFF;
        uint32_t shift = ((opcode >> 8) & 0xF) * 2;
        operand2 = rotateBits(imm, shift);
        carry_out = (shift > 0) && ((imm >> (shift - 1)) & 1);
    } else {
        // Register
        operand2 = registers[opcode & 0xF];
        carry_out = (cpsr >> 29) & 1;
    }
    
    uint32_t Rn_val = registers[Rn];
    uint32_t result = 0;
    bool overflow = false;
    
    switch (opcode_bits) {
        case 0b0000: result = Rn_val & operand2; break;  // AND
        case 0b0001: result = Rn_val ^ operand2; break;  // EOR
        case 0b0010: result = Rn_val - operand2; break;  // SUB
        case 0b0011: result = operand2 - Rn_val; break;  // RSB
        case 0b0100: result = Rn_val + operand2; break;  // ADD
        case 0b1100: result = Rn_val | operand2; break;  // ORR
        case 0b1101: result = operand2; break;           // MOV
        case 0b1110: result = Rn_val & ~operand2; break; // BIC
        case 0b1111: result = ~operand2; break;          // MVN
        default: result = 0; break;
    }
    
    registers[Rd] = result;
    
    if (S) {
        PSR psr = PSR::fromU32(cpsr);
        psr.N = (result >> 31) & 1;
        psr.Z = (result == 0) ? 1 : 0;
        psr.C = carry_out ? 1 : 0;
        psr.V = overflow ? 1 : 0;
        cpsr = psr.toU32();
    }
}

// ===== Multiply =====

void ARM9::exec_Multiply(uint32_t opcode) {
    uint32_t Rd = (opcode >> 16) & 0xF;
    uint32_t Rn = (opcode >> 12) & 0xF;
    uint32_t Rs = (opcode >> 8) & 0xF;
    uint32_t Rm = opcode & 0xF;
    bool A = (opcode >> 21) & 1;
    bool S = (opcode >> 20) & 1;
    
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

// ===== Single Data Transfer =====

void ARM9::exec_SingleDataTransfer(uint32_t opcode) {
    bool I = (opcode >> 25) & 1;
    bool U = (opcode >> 23) & 1;
    bool B = (opcode >> 22) & 1;
    bool P = (opcode >> 24) & 1;
    bool L = (opcode >> 20) & 1;
    
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
    
    if (!memRead || !memWrite) return;
    
    if (L) {
        if (B) {
            registers[Rd] = readMemory8(addr);
        } else {
            registers[Rd] = readMemory32(addr);
        }
    } else {
        if (B) {
            writeMemory8(addr, registers[Rd] & 0xFF);
        } else {
            writeMemory32(addr, registers[Rd]);
        }
    }
    
    if (P) {
        if (U) {
            registers[Rn] += offset;
        } else {
            registers[Rn] -= offset;
        }
    }
}

// ===== Block Data Transfer =====

void ARM9::exec_BlockDataTransfer(uint32_t opcode) {
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
        if (P) addr += 4;
    } else {
        addr -= 4 * count;
        if (P) addr += 4;
    }
    
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
    
    if (W) {
        if (U) {
            registers[Rn] += 4 * count;
        } else {
            registers[Rn] -= 4 * count;
        }
    }
}

// ===== Branch =====

void ARM9::exec_Branch(uint32_t opcode) {
    int32_t offset = CPUBase::signExtend(opcode & 0xFFFFFF, 24) << 2;
    registers[15] += offset;
    
    fetched_instruction = 0;
    decoded_instruction = 0;
}

void ARM9::exec_BranchLink(uint32_t opcode) {
    int32_t offset = CPUBase::signExtend(opcode & 0xFFFFFF, 24) << 2;
    registers[14] = registers[15];
    registers[15] += offset;
    
    fetched_instruction = 0;
    decoded_instruction = 0;
}

// ===== Software Interrupt =====

void ARM9::exec_SoftwareInterrupt(uint32_t opcode) {
    // Transition to Supervisor mode
    PSR psr = PSR::fromU32(cpsr);
    spsr = cpsr;
    psr.M = static_cast<uint8_t>(ARMMode::Supervisor);
    cpsr = psr.toU32();
    
    registers[15] = 0x08;
    registers[14] = registers[15] - 4;
    
    fetched_instruction = 0;
    decoded_instruction = 0;
}

// ===== Coprocessor =====

void ARM9::exec_Coprocessor(uint32_t opcode) {
    uint32_t cp = (opcode >> 8) & 0xF;
    
    if (cp == 15) {
        uint32_t CRn = (opcode >> 16) & 0xF;
        uint32_t Rd = (opcode >> 12) & 0xF;
        bool L = (opcode >> 20) & 1;
        
        if (L) {
            registers[Rd] = cp15_regs[CRn];
        } else {
            cp15_regs[CRn] = registers[Rd];
        }
    }
}

// ===== Helper Functions =====

uint32_t ARM9::rotateBits(uint32_t value, uint32_t shift) {
    shift &= 0x1F;
    if (shift == 0) return value;
    return (value >> shift) | (value << (32 - shift));
}

uint32_t ARM9::readMemory32(uint32_t addr) {
    return memRead(addr);
}

uint8_t ARM9::readMemory8(uint32_t addr) {
    uint32_t word = memRead(addr & ~3);
    int shift = (addr & 3) * 8;
    return (word >> shift) & 0xFF;
}

void ARM9::writeMemory32(uint32_t addr, uint32_t value) {
    memWrite(addr, value);
}

void ARM9::writeMemory8(uint32_t addr, uint8_t value) {
    uint32_t word = memRead(addr & ~3);
    int shift = (addr & 3) * 8;
    word = (word & ~(0xFFu << shift)) | (static_cast<uint32_t>(value) << shift);
    memWrite(addr & ~3, word);
}

// ===== Debugging =====

std::string ARM9::disassemble(uint32_t addr) const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(8) << addr << ": ";
    oss << "[disassembly not yet implemented]";
    return oss.str();
}

std::string ARM9::getStateString() const {
    std::ostringstream oss;
    oss << "=== ARM9 Processor State ===\n";
    oss << "PC (R15): 0x" << std::hex << std::setfill('0') << std::setw(8) << registers[15] << "\n";
    oss << "CPSR: 0x" << std::setw(8) << cpsr << "\n";
    oss << "Registers:\n";
    for (int i = 0; i < 16; ++i) {
        oss << "  R" << std::dec << i << ": 0x" << std::hex << std::setfill('0') << std::setw(8) << registers[i] << "\n";
    }
    return oss.str();
}

} // namespace n3ds
