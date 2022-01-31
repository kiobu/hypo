#include <iostream>
#include <fstream>
#include <string>

namespace Hypo
{
    // Constants.
    constexpr int H_EOF = -1;
    constexpr int H_MAX_ADDR = 2499;
    constexpr int H_TTL = 200;

    enum H_ERROR_CODE
    {
        OK = 0x01,
        E_FS_CANT_OPEN = 0x02,
        E_INVALID_ADDR = 0x04,
        E_INVALID_PC = 0x08,
        E_NO_EOF = 0x10,
        E_INVALID_MODE = 0x20,
        E_INVALID_GPR = 0x40,
        E_INVALID_OPCODE = 0x80
    };

    enum H_OPCODE
    {
        HALT = 0,
        ADD = 1,
        SUBTRACT = 2,
        MULTIPLY = 3,
        DIVIDE = 4,
        MOVE = 5,
        BRANCH = 6,
        BRANCH_ON_MINUS = 7,
        BRANCH_ON_PLUS = 8,
        BRANCH_ON_ZERO = 9,
        PUSH = 10,
        POP = 11,
        SYSCALL = 12
    };
    
    // Words are signed 32-bit and should accomodate 6 digits.
    typedef long word;

    // Memory, addresses are simply integers 1-5000.
    word memory[10000];

    // Clock time in ms.
    word clock;

    // Memory address register.
    word r_mar;

    // Memory buffer register.
    word r_mbr;

    // General purpose registers.
    word r_gpr[8];

    // Instruction register.
    word r_ir;

    // Processor status register.
    word r_psr;

    // Stack pointer.
    word r_sp;

    // Program counter.
    word r_pc;

    bool AddressInRange(int addr)
    {
        if (addr > H_MAX_ADDR || addr < -1)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    // Initializes Hypo machine.
    void InitializeSystem()
    {
        // Initialize clock to 0.
        clock = 0;

        // Initalize memory to 0.
        memset(memory, 0, 10000);

        // Initialize MAR and MBR to 0.
        r_mar = 0;
        r_mbr = 0;

        // Initialize GPRs to 0.
        memset(r_gpr, 0, 8);
        
        // Initialize IR, PSR, SP, PC to 0.
        r_ir = 0;
        r_psr = 0;
        r_sp = 0;
        r_pc = 0;
    }

    // Loads a Hypo script.
    int AbsoluteLoader(std::string filename)
    {
        std::ifstream i_prog(filename);

        if (!i_prog)
        {
            std::cerr << "Cannot open file: " << filename;
            return E_FS_CANT_OPEN;
        }

        int32_t h_addr, h_content;
        while (i_prog >> h_addr >> h_content)
        {
            if (h_addr == H_EOF)
            {
                // The operand for opcode -1 is the first address to be
                // executed from the EOM, so we should make sure it is 
                // pointing to a valid address.

                auto& entrypoint = h_content;

                if (AddressInRange(entrypoint))
                {
                    std::cout << "Program successfully loaded into memory.";
                    return h_content;
                }
                else
                {
                    std::cout << "Invalid program counter: " << h_content;
                    return E_INVALID_PC;
                }
            }
            else
            {
                if (AddressInRange(h_addr))
                {
                    memory[h_addr] = h_content;
                }
                else
                {
                    std::cout << "Invalid address in program: " << h_addr;
                    return E_INVALID_ADDR;
                }
            }
        }
        return E_NO_EOF;
    }

    word CPU()
    {
        word opcode, op1_mode, op1_gpr, op2_mode, op2_gpr, op1_addr, op1_val, op2_addr, op2_val, result;
        long time_left = H_TTL;
        bool should_halt = false;

        while (!should_halt && time_left > 0)
        {
            if (AddressInRange(r_pc))
            {
                r_mar = r_pc++;
                r_mbr = memory[r_mar];
            }
            else
            {
                std::cout << "Invalid address for program counter: " << r_pc;
                return E_INVALID_ADDR;
            }

            r_ir = r_mbr;

            long _rem;
            
            // Break EOM format down into opcodes, operand modes, and GPR dests.
            opcode = r_ir / 10000;
            _rem = r_ir % 10000;

            op1_mode = _rem / 1000;
            _rem = _rem % 1000;

            op1_gpr = _rem / 100;
            _rem = _rem % 100;

            op2_mode = _rem / 10;
            _rem = _rem % 10;

            op2_gpr = _rem;

            if (op1_mode < 0 || op1_mode > 6 || op2_mode < 0 || op2_mode > 6)
            {
                std::cout << "Invalid mode for operand.\n" << "-- First operand GPR: " << op1_gpr << "\n-- Second operand GPR: " << op2_gpr;
                return E_INVALID_MODE;
            }

            if (op1_gpr < 0 || op1_gpr > 7 || op2_gpr < 0 || op2_gpr > 7)
            {
                std::cout << "Invalid GPR for operand.\n" << "-- First operand GPR: " << op1_gpr << "\n-- Second operand GPR: " << op2_gpr;
                return E_INVALID_GPR;
            }

            switch (opcode)
            {
            case H_OPCODE::HALT:
                should_halt = true;
                clock += 12;
                time_left -= 12;
                break;

            case H_OPCODE::ADD:
                // ...

            case H_OPCODE::SUBTRACT:
                // ...

            case H_OPCODE::MULTIPLY:
                // ...

            case H_OPCODE::DIVIDE:
                // ...

            case H_OPCODE::MOVE:
                // ...

            case H_OPCODE::BRANCH:
                // ...

            case H_OPCODE::BRANCH_ON_MINUS:
                // ...

            case H_OPCODE::BRANCH_ON_PLUS:
                // ...

            case H_OPCODE::BRANCH_ON_ZERO:
                // ...

            case H_OPCODE::PUSH:
                // ...

            case H_OPCODE::POP:
                // ...

            case H_OPCODE::SYSCALL:
                // ...

            default:
                std::cout << "Invalid opcode: " << opcode;
                return E_INVALID_OPCODE;
            }
        }
    }
}


int main()
{
    Hypo::InitializeSystem();
    Hypo::AbsoluteLoader("../program.eom");
    Hypo::CPU();
    return 0xFF;
}
