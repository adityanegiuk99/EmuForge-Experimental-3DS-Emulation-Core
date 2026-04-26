#include <cpu_base.hpp>
#include "../src/cpu/arm11.hpp"
#include "../src/cpu/arm9.hpp"
#include <iostream>
#include <cassert>
#include <sstream>

using namespace n3ds;

/**
 * Test Suite for ARM11 and ARM9 Processors
 * 
 * This basic test suite verifies:
 * 1. Register read/write
 * 2. CPSR/SPSR manipulation
 * 3. Simple instruction execution
 * 4. Data processing operations (MOV, AND, OR, etc.)
 * 5. Arithmetic operations (ADD, SUB)
 */

void test_arm11_registers() {
    std::cout << "Testing ARM11 Register Access..." << std::endl;
    
    ARM11 cpu;
    
    // Test basic register write/read
    for (int i = 0; i < 16; ++i) {
        uint32_t value = 0x12345600 + i;
        cpu.setReg(i, value);
        assert(cpu.getReg(i) == value);
    }
    
    // Test CPSR
    uint32_t cpsr_val = 0xF0000013;  // Supervisor mode, all flags set
    cpu.setCPSR(cpsr_val);
    assert(cpu.getCPSR() == cpsr_val);
    
    std::cout << "  ✓ ARM11 registers OK" << std::endl;
}

void test_arm11_psr_structure() {
    std::cout << "Testing ARM11 PSR Structure..." << std::endl;
    
    PSR psr;
    psr.N = 1;
    psr.Z = 0;
    psr.C = 1;
    psr.V = 0;
    psr.M = 0x13;  // Supervisor mode
    
    uint32_t val = psr.toU32();
    PSR psr2 = PSR::fromU32(val);
    
    assert(psr2.N == psr.N);
    assert(psr2.Z == psr.Z);
    assert(psr2.C == psr.C);
    assert(psr2.V == psr.V);
    assert(psr2.M == psr.M);
    
    std::cout << "  ✓ ARM11 PSR structure OK" << std::endl;
}

void test_arm11_condition_codes() {
    std::cout << "Testing ARM11 Condition Evaluation..." << std::endl;
    
    ARM11 cpu;
    
    // Set flags: N=1, Z=0, C=1, V=0
    uint32_t cpsr = 0xA0000000;
    cpu.setCPSR(cpsr);
    
    // Test condition evaluation (we check via instruction execution)
    // MOV R0, #5 (AL condition, should always execute)
    uint32_t mov_r0_5 = 0xE3A00005;  // MOV R0, #5
    cpu.executeInstruction(mov_r0_5);
    assert(cpu.getReg(0) == 5);
    
    std::cout << "  ✓ ARM11 condition codes OK" << std::endl;
}

void test_arm11_data_processing() {
    std::cout << "Testing ARM11 Data Processing Instructions..." << std::endl;
    
    ARM11 cpu;
    
    // MOV R0, #42
    uint32_t mov_r0_42 = 0xE3A0002A;  // MOV R0, #42
    cpu.executeInstruction(mov_r0_42);
    assert(cpu.getReg(0) == 42);
    
    // MOV R1, #10
    uint32_t mov_r1_10 = 0xE3A0100A;  // MOV R1, #10
    cpu.executeInstruction(mov_r1_10);
    assert(cpu.getReg(1) == 10);
    
    // ADD R2, R0, R1  (R2 = R0 + R1 = 52)
    uint32_t add_r2_r0_r1 = 0xE0822001;  // ADD R2, R0, R1
    cpu.executeInstruction(add_r2_r0_r1);
    assert(cpu.getReg(2) == 52);
    
    // SUB R3, R0, R1  (R3 = R0 - R1 = 32)
    uint32_t sub_r3_r0_r1 = 0xE0433001;  // SUB R3, R0, R1
    cpu.executeInstruction(sub_r3_r0_r1);
    assert(cpu.getReg(3) == 32);
    
    std::cout << "  ✓ ARM11 data processing OK" << std::endl;
}

void test_arm11_logical_operations() {
    std::cout << "Testing ARM11 Logical Operations..." << std::endl;
    
    ARM11 cpu;
    
    // MOV R0, #0xFF
    cpu.executeInstruction(0xE3A000FF);
    assert(cpu.getReg(0) == 0xFF);
    
    // MOV R1, #0xAA
    cpu.executeInstruction(0xE3A010AA);
    assert(cpu.getReg(1) == 0xAA);
    
    // AND R2, R0, R1  (R2 = 0xFF & 0xAA = 0xAA)
    uint32_t and_r2_r0_r1 = 0xE0022001;  // AND R2, R0, R1
    cpu.executeInstruction(and_r2_r0_r1);
    assert(cpu.getReg(2) == (0xFF & 0xAA));
    
    // ORR R3, R0, R1  (R3 = 0xFF | 0xAA = 0xFF)
    uint32_t orr_r3_r0_r1 = 0xE1833001;  // ORR R3, R0, R1
    cpu.executeInstruction(orr_r3_r0_r1);
    assert(cpu.getReg(3) == (0xFF | 0xAA));
    
    // EOR R4, R0, R1  (R4 = 0xFF ^ 0xAA = 0x55)
    uint32_t eor_r4_r0_r1 = 0xE0244001;  // EOR R4, R0, R1
    cpu.executeInstruction(eor_r4_r0_r1);
    assert(cpu.getReg(4) == (0xFF ^ 0xAA));
    
    std::cout << "  ✓ ARM11 logical operations OK" << std::endl;
}

void test_arm9_registers() {
    std::cout << "Testing ARM9 Register Access..." << std::endl;
    
    ARM9 cpu;
    
    for (int i = 0; i < 16; ++i) {
        uint32_t value = 0x87654300 + i;
        cpu.setReg(i, value);
        assert(cpu.getReg(i) == value);
    }
    
    uint32_t cpsr_val = 0xF0000013;
    cpu.setCPSR(cpsr_val);
    assert(cpu.getCPSR() == cpsr_val);
    
    std::cout << "  ✓ ARM9 registers OK" << std::endl;
}

void test_arm9_data_processing() {
    std::cout << "Testing ARM9 Data Processing..." << std::endl;
    
    ARM9 cpu;
    
    // MOV R0, #25
    cpu.executeInstruction(0xE3A00019);
    assert(cpu.getReg(0) == 25);
    
    // MOV R1, #15
    cpu.executeInstruction(0xE3A0100F);
    assert(cpu.getReg(1) == 15);
    
    // ADD R2, R0, R1
    cpu.executeInstruction(0xE0822001);
    assert(cpu.getReg(2) == 40);
    
    std::cout << "  ✓ ARM9 data processing OK" << std::endl;
}

void test_debugging_output() {
    std::cout << "Testing Debugging Output..." << std::endl;
    
    ARM11 arm11;
    ARM9 arm9;
    
    // Set some state
    arm11.setReg(0, 0x12345678);
    arm11.setPC(0x00008000);
    
    std::string state = arm11.getStateString();
    assert(state.find("ARM11") != std::string::npos);
    assert(state.find("0x8000") != std::string::npos);
    
    state = arm9.getStateString();
    assert(state.find("ARM9") != std::string::npos);
    
    std::cout << "  ✓ Debugging output OK" << std::endl;
}

int main() {
    std::cout << "\n=== N3DS Emulator Phase 1 Test Suite ===\n" << std::endl;
    
    try {
        test_arm11_registers();
        test_arm11_psr_structure();
        test_arm11_condition_codes();
        test_arm11_data_processing();
        test_arm11_logical_operations();
        test_arm9_registers();
        test_arm9_data_processing();
        test_debugging_output();
        
        std::cout << "\n✓ All tests passed!\n" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
