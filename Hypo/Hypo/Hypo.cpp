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
        E_NO_EOF = 0x08,
        E_INVALID_PC = 0x016
    };
    
    // Words are signed 32-bit and should accomodate 6 digits.
    typedef long word;

    // Memory.
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
                std::cout << "Program successfully loaded into memory.";
                return h_content;
            }
            else
            {
                if (AddressInRange(h_addr))
                {
                    std::cout << "Invalid address in program: " << h_addr;
                    return E_INVALID_ADDR;
                }
                else
                {
                    memory[h_addr] = h_content;
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
            if (!AddressInRange(r_pc))
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
            // ...
        }
    }
}


int main()
{
    Hypo::InitializeSystem();
    Hypo::AbsoluteLoader("../program.eom");

    return 0xFF;
}
