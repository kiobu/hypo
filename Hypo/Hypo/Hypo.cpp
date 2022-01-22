#include <iostream>
#include <fstream>
#include <string>

namespace Hypo
{
    // Constants.
    constexpr int H_EOF = -1;
    constexpr int MAX_ADDR = 2499;

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
    word* r_mar;

    // Memory buffer register.
    word r_mbr;

    // General purpose registers.
    word r_gpr[8];

    // Instruction register.
    word r_ir;

    // Processor status register.
    word r_psr;

    // Stack pointer.
    word* r_sp;

    // Program counter.
    word* r_pc;

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

        int16_t h_addr, h_content;
        while (i_prog >> h_addr >> h_content)
        {
            std::cout << h_addr << "\n";
            if (h_addr == H_EOF)
            {
                std::cout << "Program successfully loaded into memory.";
                return h_content;
            }
            else
            {
                if (h_addr > MAX_ADDR || h_addr < -1)
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
}


int main()
{
    Hypo::InitializeSystem();
    Hypo::AbsoluteLoader("../program.txt");

    return 0xFF;
}