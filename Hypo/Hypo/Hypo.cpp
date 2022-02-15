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
#include <cstdio>
#include <fstream>
#include <string>
#include <math.h>
#include <assert.h>
#include <vector>

namespace Hypo
{
    // ------ Debugging stuff. ------
    const bool h_debug = false;
    const std::string debug_opmode_descs[] 
        = { "no opmode", "register", "register deferred", "auto increment", "auto decrement", "direct", "immediate" };

    const std::string debug_opcode_descs[] 
        = { "halt", "add", "subtract", "multiply", "divide", "move", "branch", "branch on minus", "branch on plus", "branch on zero", "push", "pop", "syscall" };
    // ------ Debugging stuff. ------

    // Constants.
    constexpr int H_EOF = -1;
    constexpr int H_PROGRAM_ADDR = 0;
    constexpr int H_MAX_PROGRAM_ADDR = 2499;
    constexpr int H_MAX_USER_FREE_ADDR = 4499;
    constexpr int H_MAX_MEM_ADDR = 9999;
    constexpr int H_TTL = 2000;
    constexpr int H_TOTAL_USER_PROG = 99;

    // MTOPS constants.
    constexpr int H_EOL = -1;
    constexpr int H_EOP = -1;
    constexpr int H_STACK_SIZE = 9;
    constexpr int H_START_SIZE_USER_FREE = 2000;
    constexpr int H_START_SIZE_OS_FREE = 5500;
    constexpr int H_PCBSIZE = 25;
    constexpr int H_DEFAULT_PRIORITY = 128;

    // State constants.
    constexpr int H_READY_STATE = 1;
    constexpr int H_WAITING_STATE = 2;

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
        E_DIVIDE_BY_ZERO = -0x400,

        // MTOPS specific errors.
        E_MTOPS_INVALID_PID = -0x800,
        E_MTOPS_INSUFFICIENT_MEM = -0x1000,
        E_MTOPS_NOT_MEM_BLOCK = -0x2000,
        E_MTOPS_INVALID_SYSCALL = -0x4000,
        E_MTOPS_QUEUE_FULL = -0x8000,
        E_MTOPS_INVALID_FS_NAME = -0x10000,
        E_MTOPS_INVALID_MEM_ADDR = -0x20000,
        E_MTOPS_REQ_MEM_TOO_SMALL = -0x40000,
        E_MTOPS_INVALID_MEM_RANGE = -0x80000
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

    // PCB indicies.
    enum H_PCB_IDX
    {
        I_NEXT_POINTER = 0,
        I_STATE = 2,
        I_PID = 1,
        I_WAIT_REASON = 3,
        I_PRIORITY = 4,
        I_STACK_START = 5,
        I_STACK_SIZE = 6,
        I_GPR0 = 11,
        I_GPR1 = 12,
        I_GPR2 = 13,
        I_GPR3 = 14,
        I_GPR4 = 15,
        I_GPR5 = 16,
        I_GPR6 = 17,
        I_GPR7 = 18,
        I_R_SP = 19,
        I_R_PC = 20,
        I_R_PSR = 21
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

    // Running PCB pointer.
    word mtops_pcb_ptr = H_EOL;

    // Ready queue.
    word mtops_rq = H_EOL;

    // Waiting queue.
    word mtops_wq = H_EOL;

    // PID
    word pid = 1;

    // OS free list.
    word mtops_os_free_list = H_EOL;

    // User free list.
    word mtops_user_free_list = H_EOL;

    // Should shutdown status (to process interrupts).
    bool shutdown_status = false;

    bool OSAddressInRange(int addr)
    {
        if (addr > H_MAX_USER_FREE_ADDR && addr < H_MAX_MEM_ADDR)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

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
        if (addr > H_MAX_PROGRAM_ADDR || addr < H_PROGRAM_ADDR)
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
    * void: InitializePCB
    * 
    * Initializes the simulated hardware for our PCB.
    * 
    * @param pcb_ptr The address in memory for the PCB.
    * 
    */
    void InitializePCB(word pcb_ptr)
    {
        for (int pcb_idx = 0; pcb_idx < H_PCBSIZE; pcb_idx++)
        {
            memory[pcb_ptr + pcb_idx] = 0;
        }

        memory[pcb_ptr + I_NEXT_POINTER] = H_EOL;
        memory[pcb_ptr + I_PID] = pid++;
        memory[pcb_ptr + I_STATE] = H_READY_STATE;
        memory[pcb_ptr + I_PRIORITY] = H_DEFAULT_PRIORITY;
    }

    word AllocateOSMemory(word size)
    {
        if (mtops_os_free_list == H_EOL)
        {
            std::cout << "No memory available to allocate.";
            return E_MTOPS_INSUFFICIENT_MEM;
        }

        if (size <= 1)
        {
            std::cout << "Requested memory is too small. Must be >= 2.";
            return E_MTOPS_REQ_MEM_TOO_SMALL;
        }

        long c_ptr = mtops_os_free_list;
        long p_ptr = H_EOL;

        while (c_ptr != H_EOL)
        {
            if (memory[c_ptr + 1] == size)
            {
                if (c_ptr == mtops_os_free_list)
                {
                    mtops_os_free_list = memory[c_ptr];
                    memory[c_ptr] = H_EOL;
                    return c_ptr;
                }
                else
                {
                    memory[p_ptr] = memory[c_ptr];
                    memory[c_ptr] = H_EOL;
                    return c_ptr;
                }
            }
            else if (memory[c_ptr + 1] > size)
            {
                if (c_ptr == mtops_os_free_list)
                {
                    memory[c_ptr + size] = memory[c_ptr];
                    memory[c_ptr + size + 1] = memory[c_ptr + 1] - size;
                    mtops_os_free_list = c_ptr + size;
                    return c_ptr;
                }
                else
                {
                    memory[c_ptr + size] = memory[c_ptr];
                    memory[c_ptr + size + 1] = memory[c_ptr + 1] - size;
                    memory[p_ptr] = c_ptr + size;
                    memory[c_ptr] = H_EOL;
                    return c_ptr;
                }
            }
            else
            {
                p_ptr = c_ptr;
                c_ptr = memory[c_ptr];
            }
        }

        std::cout << "No memory blocks were large enough for the requested allocation size.";
        return E_MTOPS_INSUFFICIENT_MEM;
    }

    word AllocateUserMemory(word size)
    {
        if (mtops_user_free_list == H_EOL)
        {
            std::cout << "No memory available to allocate.";
            return E_MTOPS_INSUFFICIENT_MEM;
        }

        if (size <= 1)
        {
            std::cout << "Requested memory is too small. Must be >= 2.";
            return E_MTOPS_REQ_MEM_TOO_SMALL;
        }

        long c_ptr = mtops_user_free_list;
        long p_ptr = H_EOL;

        while (c_ptr != H_EOL)
        {
            if (memory[c_ptr + 1] == size)
            {
                if (c_ptr == mtops_user_free_list)
                {
                    mtops_user_free_list = memory[c_ptr];
                    memory[c_ptr] = H_EOL;
                    return c_ptr;
                }
                else
                {
                    memory[p_ptr] = memory[c_ptr];
                    memory[c_ptr] = H_EOL;
                    return c_ptr;
                }
            }
            else if (memory[c_ptr + 1] > size)
            {
                if (c_ptr == mtops_user_free_list)
                {
                    memory[c_ptr + size] = memory[c_ptr];
                    memory[c_ptr + size + 1] = memory[c_ptr + 1] - size;
                    mtops_user_free_list = c_ptr + size;
                    return c_ptr;
                }
                else
                {
                    memory[c_ptr + size] = memory[c_ptr];
                    memory[c_ptr + size + 1] = memory[c_ptr + 1] - size;
                    memory[p_ptr] = c_ptr + size;
                    memory[c_ptr] = H_EOL;
                    return c_ptr;
                }
            }
            else
            {
                p_ptr = c_ptr;
                c_ptr = memory[c_ptr];
            }
        }

        std::cout << "No memory blocks were large enough for the requested allocation size.";
        return E_MTOPS_INSUFFICIENT_MEM;
    }

    word FreeOSMemory(word ptr, word size)
    {
        if (OSAddressInRange(ptr))
        {
            if (size <= 1)
            {
                std::cout << "Requested memory is too small. Must be >= 2.";
                return E_MTOPS_REQ_MEM_TOO_SMALL;
            }
            else
            {
                if ((ptr + size) > H_MAX_MEM_ADDR)
                {
                    std::cout << "The requested memory size was too large.";
                    return E_MTOPS_INVALID_MEM_RANGE;
                }
                else
                {
                    memory[ptr] = mtops_os_free_list;
                    memory[ptr + 1] = size;
                    mtops_os_free_list = ptr;
                    return OK;
                }
            }
        }
        else
        {
            std::cout << "Pointer address is outside of the OS memory.";
            return E_MTOPS_NOT_MEM_BLOCK;
        }
    }

    void PrintPCB(word PCBptr)
    {
        using namespace std;

        cout << "\nPCB @ " << PCBptr << ":" << endl;

        //Prints PCB address, Next Pointer Address, PID, State, Priority, PC, and SP values of the PCB.
        cout << "PCB address = " << PCBptr << ", Next PCB Ptr = " << memory[PCBptr + I_NEXT_POINTER] << ", PID = " << memory[PCBptr + I_PID] << ", State = " << memory[PCBptr + I_STATE] << ", Reason for Waiting = " << memory[PCBptr + I_WAIT_REASON] << ", PC = " << memory[PCBptr + I_R_PC] << ", SP = " << memory[PCBptr + I_R_SP] << ", Priority = " << memory[PCBptr + I_PRIORITY] << ", STACK INFO: Starting Stack Address " << memory[PCBptr + I_STACK_START] << ", Stack Size = " << memory[PCBptr + I_STACK_SIZE] << endl;

        //Prints the GPR values of the PCB.
        cout << "GPRs:   GPR0: " << memory[PCBptr + I_GPR0] << "   GPR1: " << memory[PCBptr + I_GPR1] << "   GPR2: " << memory[PCBptr + I_GPR2] << "   GPR3: " << memory[PCBptr + I_GPR3] << "   GPR4: " << memory[PCBptr + I_GPR4] << "   GPR5: " << memory[PCBptr + I_GPR5] << "   GPR6: " << memory[PCBptr + I_GPR6] << "   GPR7: " << memory[PCBptr + I_GPR7] << "\n" << endl;
    }

    word CreateProcess(std::string *filename, word priority)
    {
        word pcb_ptr = AllocateOSMemory(H_PCBSIZE);
        if (pcb_ptr < 0) { return pcb_ptr; }

        InitializePCB(pcb_ptr);

        word status = AbsoluteLoader(*filename);
        if (status < 0) { return status; }

        memory[pcb_ptr + I_R_PC] = status;

        word u_ptr = AllocateUserMemory(H_STACK_SIZE);
        if (pcb_ptr < 0) { FreeOSMemory(pcb_ptr, H_PCBSIZE); return pcb_ptr; }

        memory[pcb_ptr + I_STACK_START] = u_ptr;
        memory[pcb_ptr + I_R_SP] = u_ptr - 1;
        memory[pcb_ptr + I_STACK_SIZE] = H_STACK_SIZE;
        memory[pcb_ptr + I_PRIORITY] = priority;

        DumpMemory("\n User Program Area \n", H_PROGRAM_ADDR, H_TOTAL_USER_PROG);

        PrintPCB(pcb_ptr);
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

            if (h_debug) { std::cout << std::endl << "instruction: " << r_ir << std::endl; }
            if (h_debug) { std::cout << "opcode: " << opcode << " = " << debug_opcode_descs[opcode] << std::endl; }

            op1_mode = _rem / 1000;
            _rem = _rem % 1000;

            if (h_debug) { std::cout << "op1 mode: " << op1_mode << " = " << debug_opmode_descs[op1_mode] << std::endl; }

            op1_gpr = _rem / 100;
            _rem = _rem % 100;

            if (h_debug) { std::cout << "op1 gpr: " << op1_gpr << std::endl; }

            op2_mode = _rem / 10;
            _rem = _rem % 10;

            if (h_debug) { std::cout << "op2 mode: " << op2_mode << " = " << debug_opmode_descs[op2_mode] << std::endl; }

            op2_gpr = _rem;

            if (h_debug) { std::cout << "op2 gpr: " << op2_gpr << std::endl; }

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

                // Halt execution.
                should_halt = true;

                clock += 12;
                time_left -= 12;

                break;

            case H_OPCODE::ADD: // Opcode 1, add operands.
                
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
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                // ...

                clock += 2;
                time_left -= 2;

                break;
            case H_OPCODE::POP: // Opcode 11, pop the latest value from the stack.
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                // ...

                clock += 2;
                time_left -= 2;

                break;
            case H_OPCODE::SYSCALL: // Opcode 12, perform a system function call.
                
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

    Hypo::DumpMemory("BEGINNING OF CPU CYCLE", 0, 100);

    Hypo::r_pc = eom_entrypoint;

    Hypo::CPU();

    Hypo::DumpMemory("END OF CPU CYCLE", 0, 100);
}
