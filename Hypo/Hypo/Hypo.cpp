/*
* 
* --------------------------------
* | Hypothetical Decimal Machine |
* --------------------------------
* 
* Author: C.D.
* ID: 301020137
* HW: #1
* Date: February 1st, 2022
* 
* The hypothetical decimal machine is a simulation of a decimal-based processor that
* simulates hardware by executing instructions from a proprietary executable object
* module format comprised of opcodes, opmodes, and simualated memory hardware addresses.
* 
* The Hypo simulator will be used to run MTOPS real-time executions.
* 
*/

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
    constexpr int H_START_STACK_ADDR = 4500;
    constexpr int H_STACK_SIZE = 9;
    constexpr int H_MAX_MEM_ADDR = 9999;
    constexpr int H_TTL = 2000;

    // Error bitflags to be returned by Hypo methods.
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
        E_INVALID_ADDR_IN_GPR = -0x80,
        E_STACK_OVERFLOW = -0x100,
        E_STACK_UNDERFLOW = -0x200,
        E_DIVIDE_BY_ZERO = -0x400
    };

    // Hypo opcodes.
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

    // Hypo opmodes.
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

    /*
    * bool: UserFreeAddressInRange
    *
    * Check if a given address in memory is in the range for the user-free list area.
    *
    * @param addr Address in memory.
    * 
    * @return true if in range, false if not in range.
    * 
    */
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

    /*
    * bool: ProgramAddressInRange
    *
    * Check if a given address in memory is in the range for the program area.
    *
    * @param addr Address in memory.
    * 
    * @return true if in range, false if not in range.
    * 
    */
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

    /*
    * void: InitializeSystem
    *
    * Initialize the hypo machine. This will set all global Hypo fields to 0.
    * 
    */
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

    /*
    * int: AbsoluteLoader
    *
    * Loads the given EOM file and loads it into memory.
    * 
    * @param filename The EOM file to load.
    * 
    * @return One of the following status codes:
    *   -1 = Cannot open file.
    *   -2 = Invalid address in EOM.
    *   -4 = Invalid PC.
    *   -8 = No end-of-file marker in EOM.
    * 
    *   OR
    * 
    *   Any value over 0, which is to be the first instruction to be executed.
    * 
    */
    int AbsoluteLoader(std::string filename)
    {
        // Open an ifstream.
        std::ifstream i_prog(filename);

        // Check for file buffer validity.
        if (!i_prog)
        {
            std::cerr << "Cannot open file: " << filename;
            return E_FS_CANT_OPEN;
        }

        int32_t h_addr, h_content;
        
        // Loops through columns of the EOM.
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

                    // Return the first instruction to execute.
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
                    // Store the instruction in memory.
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

    /*
    * void: DumpMemory
    *
    * Dumps the running memory to the screen.
    *
    * @param str The header string to display.
    * @param start_addr The address to start dumping from
    * @param size How many values should be displayed past the start address.
    * 
    */
    void DumpMemory(std::string str, word start_addr, word size)
    {
        using namespace std;

        cout << endl << str << endl;

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

    /*
    * long: FetchOperand
    *
    * Gets the operand from a given sliced EOM instruction from memory.
    *
    * @param op_mode The operand mode to process the operand with.
    * @param op_reg The operand register for modes that access the GPRs.
    * @param op_addr The address in memory to fetch.
    * @param op_value The final value fetched from the instruction.
    *
    * @return A status code corresponding to H_ERROR_CODE.
    * 
    */
    word FetchOperand(word op_mode, word op_reg, word* op_addr, word* op_value)
    {
        switch (op_mode)
        {

        // ------ Register mode ------ Operand value is in the GPR.
        case H_OPMODE::REGISTER:

            // Set the address to a negative value, since our value is in a GPR.
            *op_addr = -2;

            *op_value = r_gpr[op_reg];

            break;

        // ------ Register deferred mode ------ Operand address is in a GPR & value is in memory.
        case H_OPMODE::REGISTER_DEF:
            *op_addr = r_gpr[op_reg];

            if (ProgramAddressInRange(*op_addr))
            {
                *op_value = memory[*op_addr];
            }
            else
            {
                std::cout << "Invalid address in GPR: " << op_reg;
                std::cout << "\n-- Address: " << *op_addr;
                std::cout << "\n-- PC: " << r_pc;
                return E_INVALID_ADDR_IN_GPR;
            }

            break;

        // ------ Auto-increment mode ------ Operand address is in a GPR & operand value is in memory; auto-increment the register value.
        case H_OPMODE::AUTO_INC:
            *op_addr = r_gpr[op_reg];

            if (ProgramAddressInRange(*op_addr))
            {
                *op_value = memory[*op_addr];
            }
            else
            {
                std::cout << "Invalid address in GPR: " << op_reg;
                std::cout << "\n-- Address: " << *op_addr;
                std::cout << "\n-- PC: " << r_pc;
                return E_INVALID_ADDR_IN_GPR;
            }

            r_gpr[op_reg]++; // Increment register value +1.

            break;

        // ------ Auto-decrement mode ------ Operand address is in a GPR & operand value is in memory; auto-decrement the register value.
        case H_OPMODE::AUTO_DEC:

            // Decrement register value -1.
            --r_gpr[op_reg];

            *op_addr = r_gpr[op_reg];

            if (ProgramAddressInRange(*op_addr))
            {
                *op_value = memory[*op_addr];
            }
            else
            {
                std::cout << "Invalid address in GPR: " << op_reg;
                std::cout << "\n-- Address: " << *op_addr;
                std::cout << "\n-- PC: " << r_pc;
                return E_INVALID_ADDR_IN_GPR;
            }

            break;

        // ------ Direct mode ------ Operand address is in the instruction, pointed to by r_pc.
        case H_OPMODE::DIRECT:

            // Get address from r_pc.
            *op_addr = memory[r_pc++];

            if (ProgramAddressInRange(*op_addr))
            {
                *op_value = memory[*op_addr];
            }
            else
            {
                std::cout << "Invalid address in GPR: " << op_reg;
                std::cout << "\n-- Address: " << *op_addr;
                std::cout << "\n-- PC: " << r_pc;

                return E_INVALID_ADDR_IN_GPR;
            }

            break;

        // ------ Immediate mode ------ Operand value is in the instruction.
        case H_OPMODE::IMMEDIATE:
            if (ProgramAddressInRange(r_pc))
            {
                // Set the address to a negative value, since our value is in a GPR.
                *op_addr = -2;

                *op_value = memory[r_pc++];
            }
            else
            {
                std::cout << "Invalid address in PC: " << op_reg;
                return E_INVALID_ADDR_IN_GPR;
            }

            break;
        // ------ Invalid opmode ------
        default:
            std::cout << "Invalid opmode: " << op_mode;
            return E_INVALID_MODE;
        }

        return OK;
    }

    /*
    * long: SystemCall
    *
    * Performs a system call (unimplemented).
    *
    * @param id The syscall ID to execute.
    *
    * @return OK
    * 
    */
    word SystemCall(word id)
    {
        // TODO: Implement syscalls.
        return OK;
    }

    /*
    * long: CPU
    *
    * Simulate the Hypo CPU.
    *
    * @return A status code corresponding to H_ERROR_CODE.
    * 
    */
    word CPU()
    {
        // Instruction parameters.
        word opcode, op1_mode, op1_gpr, op2_mode, op2_gpr, op1_addr, op1_value, op2_addr, op2_value, result;

        // Time left before CPU times out.
        long time_left = H_TTL;

        // Whether or not the CPU should halt execution.
        bool should_halt = false;

        // The status of execution based on FetchOperand.
        word status;

        while (!should_halt && time_left > 0)
        {
            if (ProgramAddressInRange(r_pc))
            {
                // Set r_mar to r_pc and increment r_pc to get the next word.
                r_mar = r_pc++;

                r_mbr = memory[r_mar];
            }
            else
            {
                std::cout << "Invalid address for program counter: " << r_pc;
                return E_INVALID_PC;
            }

            // Copy r_mbr to r_ir to process instruction.
            r_ir = r_mbr;

            long _rem;
            
            // Slice EOM instruction down into opcodes, operand modes, and GPR dests.
            opcode = r_ir / 10000;
            _rem = r_ir % 10000;

            op1_mode = _rem / 1000;
            _rem = _rem % 1000;

            op1_gpr = _rem / 100;
            _rem = _rem % 100;

            op2_mode = _rem / 10;
            _rem = _rem % 10;

            op2_gpr = _rem;

            // Check validity of operand mode.
            if (op1_mode < H_OPMODE::NO_OP || op1_mode > H_OPMODE::IMMEDIATE || op2_mode < H_OPMODE::NO_OP || op2_mode > H_OPMODE::IMMEDIATE)
            {
                std::cout << "Invalid mode for operand.\n" << "-- First operand mode: " << op1_mode << "\n-- Second operand mode: " << op2_mode;
                return E_INVALID_MODE;
            }

            size_t _gpr_len = (sizeof(r_gpr) / sizeof(r_gpr[0])) - 1;

            // Check if the GPR exists (0 to sizeof(gprs)).
            if (op1_gpr < 0 || op1_gpr > _gpr_len || op2_gpr < 0 || op2_gpr > _gpr_len)
            {
                std::cout << "Invalid GPR for operand.\n" << "-- First operand GPR: " << op1_gpr << "\n-- Second operand GPR: " << op2_gpr;
                return E_INVALID_GPR;
            }

            switch (opcode)
            {
            case H_OPCODE::HALT: // Opcode 0, halt execution.
                //std::cout << "Halt" << std::endl;

                // Halt execution.
                should_halt = true;

                clock += 12;
                time_left -= 12;

                break;

            case H_OPCODE::ADD: // Opcode 1, add operands.
                //std::cout << "Add" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                // Add the values.
                result = op1_value + op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; }

                clock += 3;
                time_left -= 3;

                break;
            case H_OPCODE::SUBTRACT: // Opcode 2, subtract operands.
                //std::cout << "Subtract" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                // Subtract the values.
                result = op1_value - op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; }

                clock += 3;
                time_left -= 3;

                break;
            case H_OPCODE::MULTIPLY: // Opcode 3, multiply operands.
                //std::cout << "Mult" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                // Multiply the values.
                result = op1_value * op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; }

                clock += 6;
                time_left -= 6;

                break;
            case H_OPCODE::DIVIDE: // Opcode 4, divide operands.
                //std::cout << "Divide" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                // x/0 is undefined.
                if (op2_value == 0)
                {
                    std::cout << "Cannot divide by zero.";
                    return E_DIVIDE_BY_ZERO;
                }

                // Divide the values.
                result = op1_value / op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; }

                clock += 6;
                time_left -= 6;

                break;
            case H_OPCODE::MOVE: // Opcode 5, move/reassign memory address to value.
                //std::cout << "Mov" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                status = FetchOperand(op2_mode, op2_gpr, &op2_addr, &op2_value);
                if (status < 0) { return status; }

                // Get result from operand 2.
                result = op2_value;

                if (op1_mode == H_OPMODE::IMMEDIATE) { std::cout << "Cannot store value in immediate mode."; return H_ERROR_CODE::E_INVALID_MODE; }
                if (op1_mode == H_OPMODE::REGISTER) { r_gpr[op1_gpr] = result; }
                else { memory[op1_addr] = result; } // Move result to memory address from operand 1.

                clock += 2;
                time_left -= 2;

                break;
            case H_OPCODE::BRANCH: // Opcode 6, branch/`goto` another memory address to continue execution.
                //std::cout << "Branch" << std::endl;
                
                if (ProgramAddressInRange(r_pc))
                {
                    // Get next instruction from current instruction.
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
            case H_OPCODE::BRANCH_ON_MINUS: // Opcode 7, branch if the value in operand 0 is negative.
                //std::cout << "BOM" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                // Check if operand 1 is negative.
                if (op1_value < 0)
                {
                    if (ProgramAddressInRange(r_pc))
                    {
                        // Get next instruction from current instruction.
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
            case H_OPCODE::BRANCH_ON_PLUS: // Opcode 8, branch if the value in operand 0 is positive.
                //std::cout << "BOP" << std::endl;

                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                // Check if operand 1 is positive.
                if (op1_value > 0)
                {
                    if (ProgramAddressInRange(r_pc))
                    {
                        // Get next instruction from current instruction.
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
            case H_OPCODE::BRANCH_ON_ZERO: // Opcode 9, branch if the value in operand 0 is equal to zero.
                //std::cout << "BO0" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                // Check if operand 1 is zero.
                if (op1_value == 0)
                {
                    if (ProgramAddressInRange(r_pc))
                    {
                        // Get next instruction from current instruction.
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
            case H_OPCODE::PUSH: // Opcode 10, push the value of operand 1 to the stack.
                //std::cout << "Push" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                // Check for a stack overflow.
                if (r_sp == memory[H_START_STACK_ADDR] + H_STACK_SIZE)
                {
                    std::cout << "Stack overflow.";
                    return E_STACK_OVERFLOW;
                }
                else
                {
                    // Adjust stack pointer.
                    r_sp++;

                    // Push the value.
                    memory[r_sp] = op1_value;


                    std::cout << "\nValue: " << memory[r_sp] << " pushed to the stack.\n";
                }

                clock += 2;
                time_left -= 2;

                break;
            case H_OPCODE::POP: // Opcode 11, pop the latest value from the stack.
                //std::cout << "Pop" << std::endl;
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                // Check for a stack underflow.
                if (r_sp < memory[H_STACK_SIZE])
                {
                    std::cout << "Stack underflow.";
                    return E_STACK_UNDERFLOW;
                }
                else
                {
                    // Pop the value.
                    op1_addr = memory[r_sp];

                    std::cout << "\nValue: " << memory[r_sp] << " popped from the stack.\n";

                    // Adjust stack pointer.
                    r_sp--;
                }

                clock += 2;
                time_left -= 2;

                break;
            case H_OPCODE::SYSCALL: // Opcode 12, perform a system function call.
                //std::cout << "Syscall" << std::endl;
                
                if (ProgramAddressInRange(r_pc))
                {
                    status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                    if (status < 0) { return status; }

                    // Execute the system call.
                    status = SystemCall(op1_value);
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

// Begin Hypo process execution.
int main()
{
    Hypo::InitializeSystem();
    Hypo::word eom_entrypoint = Hypo::AbsoluteLoader("../evensum.eom");

    Hypo::r_pc = eom_entrypoint;

    Hypo::CPU();

    Hypo::DumpMemory("END OF CPU CYCLE", 0, 100);
}
