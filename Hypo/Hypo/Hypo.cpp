#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <math.h>
#include <assert.h>

namespace Hypo
{
    // Constants.
    constexpr int H_EOF = -1;
    constexpr int H_MAX_PROGRAM_ADDR = 2499;
    constexpr int H_MAX_USER_FREE_ADDR = 4499;
    constexpr int H_MAX_MEM_ADDR = 9999;
    constexpr int H_TTL = 200;

    enum H_ERROR_CODE
    {
        OK = 0x01,
        E_FS_CANT_OPEN = -0x01,
        E_INVALID_ADDR_IN_PROGRAM = -0x02,
        E_INVALID_PC = -0x04,
        E_NO_EOF = -0x08,
        E_INVALID_MODE = -0x10,
        E_INVALID_GPR = -0x20,
        E_INVALID_OPCODE = -0x40,
        E_INVALID_ADDR_IN_GPR = -0x80
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

    enum H_OPMODE
    {
        NO_OP = 0,
        REGISTER = 1,
        REGISTER_DEF = 2,
        AUTO_INC = 3,
        AUTO_DEC = 4,
        DIRECT = 5,
        IMMEDIATE = 6
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

    bool UserFreeAddressInRange(int addr)
    {
        if (addr > H_MAX_PROGRAM_ADDR && addr <= H_MAX_USER_FREE_ADDR)
        {
            return true;
        }
        else 
        {
            return false;
        }
    }

    bool ProgramAddressInRange(int addr)
    {
        if (addr > H_MAX_PROGRAM_ADDR || addr < -1)
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

                if (ProgramAddressInRange(entrypoint))
                {
                    std::cout << "Program successfully loaded into memory.";
                    return h_content;
                }
                else
                {
                    std::cout << "Invalid address for program counter: " << h_content;
                    return E_INVALID_PC;
                }
            }
            else
            {
                if (ProgramAddressInRange(h_addr))
                {
                    memory[h_addr] = h_content;
                }
                else
                {
                    std::cout << "Invalid address in program: " << h_addr;
                    return E_INVALID_ADDR_IN_PROGRAM;
                }
            }
        }
        return E_NO_EOF;
    }

    void DumpMemory(std::string str, word start_addr, word size)
    {
        using namespace std;

        cout << str << endl;

        // Checks for invalid starting location, ending location, or size. Checks for valid memory dump range between 0-9999.
        if (start_addr < 0 || start_addr > H_MAX_MEM_ADDR || size < 1 || start_addr + size > H_MAX_MEM_ADDR)
        {
            cout << "Invalid parameter.";
        }
        else
        {
            cout << setw(12) << "\nGPRs: " << setw(7) << "G0" << setw(7) << "G1" << setw(7) << "G2" << setw(7) << "G3" << setw(7) << "G4" << setw(7) << "G5" << setw(7) << "G6" << setw(7) << "G7" << setw(7) << "SP" << setw(7) << "PC" << endl; //Display GPR header.
            cout << left << setfill(' ') << setw(11) << " " << setw(7) << r_gpr[0] << setw(7) << r_gpr[1] << setw(7) << r_gpr[2] << setw(7) << r_gpr[3] << setw(7) << r_gpr[4] << setw(7) << r_gpr[5] << setw(7) << r_gpr[6] << setw(7) << r_gpr[7] << setw(7) << r_sp << setw(7) << r_pc << endl; //Display GPR, SP, and PC.

            cout << left << setw(12) << "\nAddress: " << setw(7) << "+0" << setw(7) << "+1" << setw(7) << "+2" << setw(7) << "+3" << setw(7) << "+4" << setw(7) << "+5" << setw(7) << "+6" << setw(7) << "+7" << setw(7) << "+8" << setw(7) << "+9" << endl; //Display memory header.

            int addr = start_addr;
            int end_address = start_addr + size;

            while (addr <= end_address)
            {
                // Print starting memory location in the row.
                cout << left << setw(11) << addr;

                // Prints all values at the desired memory location until the end address is reached.
                for (int i = 0; i < 10; i++)
                {
                    if (addr <= end_address)
                    {
                        cout << setw(7) << memory[addr];
                        addr++;
                    }
                    else
                    {
                        // No more values to print, break out of the loop.
                        break;
                    }
                }
                cout << "\n";
            }

            cout << "Clock: " << clock << endl;
            cout << "PSR: " << r_psr << endl;
        }
    }

    word FetchOperand(word op_mode, word op_reg, word* op_addr, word* op_value)
    {
        switch (op_mode)
        {
        case H_OPMODE::REGISTER:
            *op_addr = -2;
            *op_value = r_gpr[op_reg];

            break;
        case H_OPMODE::REGISTER_DEF:
            *op_addr = r_gpr[op_reg];

            if (UserFreeAddressInRange(*op_addr))
            {
                *op_value = memory[*op_addr];
            }
            else
            {
                std::cout << "Invalid address in GPR: " << op_reg;
                std::cout << "\n-- Address: " << *op_addr;
                return E_INVALID_ADDR_IN_GPR;
            }

            break;
        case H_OPMODE::AUTO_INC:
            *op_addr = r_gpr[op_reg];

            if (UserFreeAddressInRange(*op_addr))
            {
                *op_value = memory[*op_addr];
            }
            else
            {
                std::cout << "Invalid address in GPR: " << op_reg;
                std::cout << "\n-- Address: " << *op_addr;
                return E_INVALID_ADDR_IN_GPR;
            }

            r_gpr[op_reg]++;

            break;
        case H_OPMODE::AUTO_DEC:
            --r_gpr[op_reg];
            *op_addr = r_gpr[op_reg];

            if (UserFreeAddressInRange(*op_addr))
            {
                *op_value = memory[*op_addr];
            }
            else
            {
                std::cout << "Invalid address in GPR: " << op_reg;
                std::cout << "\n-- Address: " << *op_addr;
                return E_INVALID_ADDR_IN_GPR;
            }

            break;
        case H_OPMODE::DIRECT:
            *op_addr = memory[r_pc++];

            if (UserFreeAddressInRange(*op_addr))
            {
                *op_value = memory[*op_addr];
            }
            else
            {
                std::cout << "Invalid address in GPR: " << op_reg;
                std::cout << "\n-- Address: " << *op_addr;
                return E_INVALID_ADDR_IN_GPR;
            }

            break;

        case H_OPMODE::IMMEDIATE:
            if (ProgramAddressInRange(r_pc))
            {
                *op_addr = -2;
                *op_value = memory[r_pc++];
            }
            else
            {
                std::cout << "Invalid address in PC: " << op_reg;
                return E_INVALID_ADDR_IN_GPR;
            }

            break;
        default:
            std::cout << "Invalid opmode: " << op_mode;
            return E_INVALID_MODE;
        }

        return OK;
    }

    word CPU()
    {
        word opcode, op1_mode, op1_gpr, op2_mode, op2_gpr, op1_addr, op1_value, op2_addr, op2_value, result;
        long time_left = H_TTL;
        bool should_halt = false;

        word status;

        while (!should_halt && time_left > 0)
        {
            if (ProgramAddressInRange(r_pc))
            {
                r_mar = r_pc++;
                r_mbr = memory[r_mar];
            }
            else
            {
                std::cout << "Invalid address for program counter: " << r_pc;
                return E_INVALID_PC;
            }

            r_ir = r_mbr;

            std::cout << "r_ir: " << r_ir << std::endl;

            long _rem;
            
            // Break EOM format down into opcodes, operand modes, and GPR dests.
            opcode = r_ir / 10000;
            _rem = r_ir % 10000;

            std::cout << "Opcode: " << opcode << std::endl;

            op1_mode = _rem / 1000;
            _rem = _rem % 1000;

            std::cout << "op1 mode: " << op1_mode << std::endl;

            op1_gpr = _rem / 100;
            _rem = _rem % 100;

            std::cout << "op1 GPR: " << op1_gpr << std::endl;

            op2_mode = _rem / 10;
            _rem = _rem % 10;

            std::cout << "op2 mode: " << op2_mode << std::endl;

            op2_gpr = _rem;

            std::cout << "op2 GPR: " << op2_gpr << std::endl;

            if (op1_mode < H_OPMODE::NO_OP || op1_mode > H_OPMODE::IMMEDIATE || op2_mode < H_OPMODE::NO_OP || op2_mode > H_OPMODE::IMMEDIATE)
            {
                std::cout << "Invalid mode for operand.\n" << "-- First operand mode: " << op1_mode << "\n-- Second operand mode: " << op2_mode;
                return E_INVALID_MODE;
            }

            size_t _gpr_len = (sizeof(r_gpr) / sizeof(r_gpr[0])) - 1;

            if (op1_gpr < 0 || op1_gpr > _gpr_len || op2_gpr < 0 || op2_gpr > _gpr_len)
            {
                std::cout << "Invalid GPR for operand.\n" << "-- First operand GPR: " << op1_gpr << "\n-- Second operand GPR: " << op2_gpr;
                return E_INVALID_GPR;
            }

            switch (opcode)
            {
            case H_OPCODE::HALT:
                std::cout << "Halt" << std::endl;

                should_halt = true;
                clock += 12;
                time_left -= 12;

                break;

            case H_OPCODE::ADD:
                std::cout << "Add" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                result = op1_value + op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; }

                clock += 3;
                time_left -= 3;

                break;
            case H_OPCODE::SUBTRACT:
                std::cout << "Subtract" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                result = op1_value - op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; }

                clock += 3;
                time_left -= 3;

                break;
            case H_OPCODE::MULTIPLY:
                std::cout << "Mult" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                result = op1_value * op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; }

                clock += 6;
                time_left -= 6;

                break;
            case H_OPCODE::DIVIDE:
                std::cout << "Divide" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                result = op1_value * op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; }

                clock += 6;
                time_left -= 6;

                break;
            case H_OPCODE::MOVE:
                std::cout << "Mov" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                result = op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; }

                clock += 2;
                time_left -= 2;

                break;
            case H_OPCODE::BRANCH:
                std::cout << "Branch" << std::endl;
                
                if (ProgramAddressInRange(r_pc))
                {
                    r_pc = memory[r_pc];
                }
                else
                {
                    std::cout << "Invalid address for program counter on BRANCH: " << r_pc;
                    return E_INVALID_PC;
                }

                clock += 2;
                time_left -= 2;

                break;
            case H_OPCODE::BRANCH_ON_MINUS:
                std::cout << "BOM" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                if (op1_value < 0)
                {
                    if (ProgramAddressInRange(r_pc))
                    {
                        r_pc = memory[r_pc];
                    }
                    else
                    {
                        std::cout << "Invalid address for program counter on BRANCH_ON_MINUS: " << r_pc;
                        return E_INVALID_PC;
                    }
                }
                else
                {
                    r_pc++; // Skip branch instruction.
                }

                clock += 4;
                time_left -= 4;

                break;
            case H_OPCODE::BRANCH_ON_PLUS:
                std::cout << "BOP" << std::endl;

                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                if (op1_value > 0)
                {
                    if (ProgramAddressInRange(r_pc))
                    {
                        r_pc = memory[r_pc];
                    }
                    else
                    {
                        std::cout << "Invalid address for program counter on BRANCH_ON_PLUS: " << r_pc;
                        return E_INVALID_PC;
                    }
                }
                else
                {
                    r_pc++; // Skip branch instruction.
                }

                clock += 4;
                time_left -= 4;

                break;
            case H_OPCODE::BRANCH_ON_ZERO:
                std::cout << "BO0" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                if (op1_value == 0)
                {
                    if (ProgramAddressInRange(r_pc))
                    {
                        r_pc = memory[r_pc];
                    }
                    else
                    {
                        std::cout << "Invalid address for program counter on BRANCH_ON_ZERO: " << r_pc;
                        return E_INVALID_PC;
                    }
                }
                else
                {
                    r_pc++; // Skip branch instruction.
                }

                clock += 4;
                time_left -= 4;

                break;
            case H_OPCODE::PUSH:
                std::cout << "Push" << std::endl;
                // ...

                break;
            case H_OPCODE::POP:
                std::cout << "Pop" << std::endl;
                // ...

                break;
            case H_OPCODE::SYSCALL:
                std::cout << "Syscall" << std::endl;
                
                if (ProgramAddressInRange(r_pc))
                {
                    status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                    if (status < 0) { return status; }

                    // TODO: Implement syscalls.
                    status = 100;
                }
                else
                {
                    std::cout << "Invalid address for program counter on SYSCALL: " << r_pc;
                    return E_INVALID_PC;
                }

                clock += 12;
                time_left -= 12;

                break;
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
    Hypo::word eom_entrypoint = Hypo::AbsoluteLoader("../program.eom");

    Hypo::r_pc = eom_entrypoint;

    Hypo::CPU();

    return 0xFF;
}
