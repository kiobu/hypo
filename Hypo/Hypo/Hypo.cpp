#include <iostream>
#include <fstream>
#include <string>

namespace Hypo
{
    enum H_ERROR
    {
        E_FS_CANT_OPEN = 0x01,
        E_INVALID_ADDR = 0x02,
        E_NO_EOF = 0x04,
        E_INVALID_PC = 0x08
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
        std::ifstream input_stream(filename);

        if (!input_stream)
        {
            std::cerr << E_FS_CANT_OPEN;
            return E_FS_CANT_OPEN;
        }

        return E_NO_EOF;
    }
}


int main()
{
    Hypo::InitializeSystem();
    Hypo::AbsoluteLoader("dummy.txt");
    return 0xFF;
}