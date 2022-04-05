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
    constexpr int H_OS_MODE = 1;
    constexpr int H_USER_MODE = 2;

    // MTOPS constants.
    constexpr int H_EOL = -1;
    constexpr int H_EOP = -1;
    constexpr int H_STACK_SIZE = 9;
    constexpr int H_START_SIZE_USER_FREE = 2000;
    constexpr int H_START_SIZE_OS_FREE = 5500;
    constexpr int H_PCBSIZE = 25;
    constexpr int H_DEFAULT_PRIORITY = 128;
    constexpr int H_TTL_EXP = 2;
    constexpr int H_HALT = 1;

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

        E_UNKOWN = -0xDEAD,

        // MTOPS specific errors.
        E_MTOPS_INVALID_PID = -0x800,
        E_MTOPS_INSUFFICIENT_MEM = -0x1000,
        E_MTOPS_NOT_MEM_BLOCK = -0x2000,
        E_MTOPS_INVALID_SYSCALL = -0x4000,
        E_MTOPS_QUEUE_FULL = -0x8000,
        E_MTOPS_INVALID_FS_NAME = -0x10000,
        E_MTOPS_INVALID_MEM_ADDR = -0x20000,
        E_MTOPS_REQ_MEM_TOO_SMALL = -0x40000,
        E_MTOPS_INVALID_MEM_RANGE = -0x80000,
        E_MTOPS_INVALID_SIZE = -0x100000
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

    enum H_INTS
    {
        INT_NO_OP = 0,
        INT_RUN_PROG = 1,
        INT_SHUTDOWN = 2,
        INT_IO_GETC = 3,
        INT_IO_PUTC = 4
    };

    enum SYSCALLS
    {
        PROCESS_CREATE = 1,
        PROCESS_DELETE = 2,
        PROCESS_INQ = 3,
        MEM_ALLOC = 4,
        MEM_FREE = 5,
        MSG_SEND = 6,
        MSG_RECV = 7,
        IO_GETC = 8,
        IO_PUTC = 9,
        TIME_GET = 10,
        TIME_SET = 11
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
    word mtops_pid = 1;

    // OS free list.
    word mtops_os_free_list = H_EOL;

    // User free list.
    word mtops_user_free_list = H_EOL;

    // Ready queue.
    word RQ = H_EOL;

    // Waiting queue.
    word WQ = H_EOL;

    // Should shutdown status (to process interrupts).
    bool shutdown_status = false;

    // Prototyping some methods.
    long CreateProcess(std::string* filename, word priority);
    word InsertIntoRQ(word pcb_ptr);

    bool OSAddressInRange(int addr)
    {
        if (addr > H_MAX_USER_FREE_ADDR && addr <= H_MAX_MEM_ADDR)
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

        mtops_user_free_list = H_MAX_PROGRAM_ADDR + 1;
        memory[mtops_user_free_list + I_NEXT_POINTER] = H_EOL;
        memory[mtops_user_free_list + 1] = H_START_SIZE_USER_FREE;

        mtops_os_free_list = H_MAX_USER_FREE_ADDR + 1;
        memory[mtops_os_free_list + I_NEXT_POINTER] = H_EOL;
        memory[mtops_os_free_list + 1] = H_START_SIZE_OS_FREE;

        std::string nullf = "../null.eom";
        std::string* nullfp = &nullf;
        CreateProcess(nullfp, 0);
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
                    std::cout << "Program [" + filename + "] successfully loaded into memory.";

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
    * word: FetchOperand
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

            if (UserFreeAddressInRange(*op_addr))
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

            if (UserFreeAddressInRange(*op_addr))
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

            if (UserFreeAddressInRange(*op_addr))
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

            if (UserFreeAddressInRange(*op_addr))
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
        memory[pcb_ptr + I_PID] = mtops_pid++;
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

        word c_ptr = mtops_os_free_list;
        word p_ptr = H_EOL;

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

        word c_ptr = mtops_user_free_list;
        word p_ptr = H_EOL;

        while (c_ptr != H_EOL)
        {
            if (memory[c_ptr + 1] == size)
            {
                if (c_ptr == mtops_user_free_list)
                {
                    mtops_user_free_list = memory[c_ptr];
                    memory[c_ptr] = H_EOL;
                    if (h_debug) { std::cout << "\nPointer returned [1]: " + c_ptr << std::endl; }
                    return c_ptr;
                }
                else
                {
                    memory[p_ptr] = memory[c_ptr];
                    memory[c_ptr] = H_EOL;
                    if (h_debug) { std::cout << "\nPointer returned [2]: " + c_ptr << std::endl; }
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
                    if (h_debug) { std::cout << "\nPointer returned [3]: " + c_ptr << std::endl; }
                    return c_ptr;
                }
                else
                {
                    memory[c_ptr + size] = memory[c_ptr];
                    memory[c_ptr + size + 1] = memory[c_ptr + 1] - size;
                    memory[p_ptr] = c_ptr + size;
                    memory[c_ptr] = H_EOL;
                    if (h_debug) { std::cout << "\nPointer returned [4]: " + c_ptr << std::endl; }
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

    word FreeUserMemory(word ptr, word size)
    {
        if (!UserFreeAddressInRange(ptr))
        {
            std::cout << "Memory address out of bounds for user free memory.";
            return E_MTOPS_NOT_MEM_BLOCK;
        }
        
        if (size < 2) //Size to User memory to free is too small, return error.
        {
            std::cout << "Memory size is too small, must be >= 2.";
            return E_MTOPS_REQ_MEM_TOO_SMALL;
        }
        else if ((ptr + size) > H_MAX_MEM_ADDR) //Trying to free elements in memory that pass its' limit, return error.
        {
            std::cout << "Requested size is too large and is out of bounds.";
            return E_MTOPS_INVALID_MEM_RANGE;
        }

        memory[ptr] = mtops_user_free_list; //Set the pointer of the released free block to point to the 'front' of the UserFreeList (the newly released block is taking it's place at the front).
        memory[ptr + 1] = size; //Set the size of this block in UserFreeList to the size given.
        mtops_user_free_list = ptr; //Set the pointer given to be the new front of the UserFreeList.
        return OK;
    }

    void TerminateProcess(word pcb_ptr)
    {
        FreeUserMemory(memory[pcb_ptr + I_STACK_START], memory[pcb_ptr + I_STACK_SIZE]); // Return stack memory using stack start address and stack size in the given PCB.

        FreeOSMemory(pcb_ptr, H_PCBSIZE); // Return PCB memory using the pcb_ptr.
    }

    void PrintPCB(word pcb_ptr)
    {
        using namespace std;

        cout << "\nPCB @ " << pcb_ptr << ":" << endl;

        //Prints PCB address, Next Pointer Address, PID, State, Priority, PC, and SP values of the PCB.
        cout << "PCB address = " << pcb_ptr << ", Next PCB Ptr = " << memory[pcb_ptr + I_NEXT_POINTER] << ", PID = " << memory[pcb_ptr + I_PID] << ", State = " << memory[pcb_ptr + I_STATE] << ", Reason for Waiting = " << memory[pcb_ptr + I_WAIT_REASON] << ", PC = " << memory[pcb_ptr + I_R_PC] << ", SP = " << memory[pcb_ptr + I_R_SP] << ", Priority = " << memory[pcb_ptr + I_PRIORITY] << ", STACK INFO: Starting Stack Address " << memory[pcb_ptr + I_STACK_START] << ", Stack Size = " << memory[pcb_ptr + I_STACK_SIZE] << endl;

        //Prints the GPR values of the PCB.
        cout << "GPRs:   GPR0: " << memory[pcb_ptr + I_GPR0] << "   GPR1: " << memory[pcb_ptr + I_GPR1] << "   GPR2: " << memory[pcb_ptr + I_GPR2] << "   GPR3: " << memory[pcb_ptr + I_GPR3] << "   GPR4: " << memory[pcb_ptr + I_GPR4] << "   GPR5: " << memory[pcb_ptr + I_GPR5] << "   GPR6: " << memory[pcb_ptr + I_GPR6] << "   GPR7: " << memory[pcb_ptr + I_GPR7] << "\n" << endl;
    }

    long CreateProcess(std::string *filename, word priority)
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
        InsertIntoRQ(pcb_ptr);

        return OK;
    }

    long PrintQueue(long queue_ptr)
    {
        long c_pcb_ptr = queue_ptr;

        if (c_pcb_ptr == H_EOL) //If the initial address is EndOfList, then the list itself is empty.
        {
            std::cout << "Empty list."; 
            return OK;
        }

        // Walk thru the queue.
        while (c_pcb_ptr != H_EOL)
        {
            PrintPCB(c_pcb_ptr);
            c_pcb_ptr = memory[c_pcb_ptr + I_NEXT_POINTER];
        }

        return OK;

    }

    word InsertIntoWQ(word pcb_ptr)
    {
        if (pcb_ptr < 0 || pcb_ptr > H_MAX_MEM_ADDR)
        {
            std::cout << "Invalid memory range.";
            return E_MTOPS_INVALID_MEM_RANGE;
        }

        memory[pcb_ptr + I_STATE] = H_WAITING_STATE; //Set the PCB's state to "waiting."
        memory[pcb_ptr + I_NEXT_POINTER] = WQ;
        WQ = pcb_ptr;

        return OK;

    }

    word InsertIntoRQ(word pcb_ptr)
    {
        word p_ptr = H_EOL;
        word c_ptr = RQ;

        if (pcb_ptr < 0 || pcb_ptr > H_MAX_MEM_ADDR)
        {
            std::cout << "Invalid memory range.";
            return E_MTOPS_INVALID_MEM_RANGE;
        }

        memory[pcb_ptr + I_STATE] = H_READY_STATE; //Set the PCB's state to "ready."
        memory[pcb_ptr + I_NEXT_POINTER] = H_EOL; //Set the PCB's Next Pointer value to EndOfList.

        if (RQ == H_EOL) //If RQ is equal to the value of EndOfList (-1), then RQ is empty.
        {
            RQ = pcb_ptr;
            return OK;
        }

        while (c_ptr != H_EOL)
        {
            if (memory[pcb_ptr + I_PRIORITY] > memory[c_ptr + I_PRIORITY]) //If the priority of the PCB we want to insert is higher than the priority of the current PCB...
            {
                if (p_ptr == H_EOL) //If p_ptr is EndOfList, then the priority of the PCB that we want to insert is higher than the highest priority PCB.
                {
                    memory[pcb_ptr + I_NEXT_POINTER] = RQ; //Change the current PCB's next pointer from EOL to the first PCB in the ready queue.
                    RQ = pcb_ptr; //Change the RQ value to the address of the PCB that we want to insert (because it is now the head of the queue).
                    return OK;
                }

                //If it isn't at the start of the RQ, then we're inserting this PCB into the middle of the list.
                memory[pcb_ptr + I_NEXT_POINTER] = memory[p_ptr + I_NEXT_POINTER]; //Set pcb_ptr's next pointer index to the previous pointer's next pointer index, because pcb_ptr is taking over the previous pointer's slot.
                memory[p_ptr + I_NEXT_POINTER] = pcb_ptr; //Set the previous pointer's next pointer index to the PCB address, completing the insertion.
                return OK;
            }

            else //PCB to be inserted has lower or equal priority as the current PCB in RQ, move on to the next PCB.
            {
                p_ptr = c_ptr;
                c_ptr = memory[c_ptr + I_NEXT_POINTER];
            }
        }

        //If it gets to this point in the InsertIntoRQ() function, than the PCB we want to insert has the lowest priority in the RQ. Insert the new PCB into the end of the RQ.
        memory[p_ptr + I_NEXT_POINTER] = pcb_ptr; //Change the previous pointer's next pointer index from EOL to the new PCB address. Note that the new PCB has a next pointer address of EOL.
        return OK;

    }

    word SearchAndRemovePCBfromWQ(word this_pid)
    {
        word currentpcb_ptr = WQ;
        word previouspcb_ptr = H_EOL;

        if (this_pid < 1) //PID cannot be zero or less than zero. Check for an incorrect PID.
        {
            std::cout << "Invalid PID.";
            return E_MTOPS_INVALID_PID;
        }

        //Search WQ for a PCB that has the given pid. If a match is found, remove it from WQ and return the PCB pointer
        while (currentpcb_ptr != H_EOL)
        {
            if (memory[currentpcb_ptr + I_PID] == this_pid) //If the current pointer's PID matches the PID we're looking for, then a match is found. Remove that process from WQ.
            {
                if (previouspcb_ptr == H_EOL) //First PCB in WQ is a match.
                {
                    WQ = memory[currentpcb_ptr + I_NEXT_POINTER]; //Set the starting point of WQ to the second PCB in WQ.
                }
                else //Match is somewhere in the middle of WQ.
                {
                    memory[previouspcb_ptr + I_NEXT_POINTER] = memory[currentpcb_ptr + I_NEXT_POINTER]; //Adjust the previous PCB's next pointer index to be the next pointer index of the PCB that's being removed from WQ.
                }

                memory[currentpcb_ptr + I_NEXT_POINTER] = H_EOL; //Adjust the returning PCB's next pointer index to be 'EndOfList'.
                return currentpcb_ptr; //Return matching PCB.
            }

            previouspcb_ptr = currentpcb_ptr; //Move on to the next PCB if there is no match.
            currentpcb_ptr = memory[currentpcb_ptr + I_NEXT_POINTER];
        }

        std::cout << "No process with ID " << this_pid << " could be found.";
        return E_MTOPS_INVALID_PID; // TODO: Replace with new error.
    }

    long SelectProcessFromRQ()
    {
        long pcb_ptr = RQ;

        if (RQ != H_EOL)
        {
            RQ = memory[RQ + I_NEXT_POINTER];
        }

        memory[pcb_ptr + I_NEXT_POINTER] = H_EOL; // This may be memory[-1].
        return pcb_ptr;
    }

    void SaveContext(long pcb_ptr)
    {
        memory[pcb_ptr + I_GPR0] = r_gpr[0];
        memory[pcb_ptr + I_GPR1] = r_gpr[1];
        memory[pcb_ptr + I_GPR2] = r_gpr[2];
        memory[pcb_ptr + I_GPR3] = r_gpr[3];
        memory[pcb_ptr + I_GPR4] = r_gpr[4];
        memory[pcb_ptr + I_GPR5] = r_gpr[5];
        memory[pcb_ptr + I_GPR6] = r_gpr[6];
        memory[pcb_ptr + I_GPR7] = r_gpr[7];

        memory[pcb_ptr + I_R_SP] = r_sp;
        memory[pcb_ptr + I_R_PC] = r_pc;
        memory[pcb_ptr + I_R_PSR] = r_psr;
    }

    void Dispatcher(long pcb_ptr)
    {
        r_gpr[0] = memory[pcb_ptr + I_GPR0];
        r_gpr[1] = memory[pcb_ptr + I_GPR1];
        r_gpr[2] = memory[pcb_ptr + I_GPR2];
        r_gpr[3] = memory[pcb_ptr + I_GPR3];
        r_gpr[4] = memory[pcb_ptr + I_GPR4];
        r_gpr[5] = memory[pcb_ptr + I_GPR5];
        r_gpr[6] = memory[pcb_ptr + I_GPR6];
        r_gpr[7] = memory[pcb_ptr + I_GPR7];
        r_sp = memory[pcb_ptr + I_R_SP];
        r_pc = memory[pcb_ptr + I_R_PC];
        r_psr = H_USER_MODE;
    }

    void ISRrunProgramInterrupt()
    {
        std::string programToRun;
        std::cout << "\nEnter filename: ";
        std::cin >> programToRun; //Prompt and read filename.

        std::string* fptr = &programToRun;
        CreateProcess(fptr, H_DEFAULT_PRIORITY); //Call Create Process passing filename and Default Priority as arguments.

    }

    void ISRinputCompletionInterrupt()
    {
        word PID;
        char i_char;

        std::cout << "ISR designed for input completion has begun running, please specify the PID of the process that the input is being completed for: ";
        std::cin >> PID; //Read the PID of the process we're completing input for.

        PID = (int) PID;

        word pcb_ptr = SearchAndRemovePCBfromWQ(PID); //Search WQ to find the PCB that has the given PID, return value is stored in pcb_ptr.
        if (pcb_ptr > 0) //Only performs this section with a valid PCB address.
        {
            std::cout << "Please enter a character to store: ";
            std::cin >> i_char; //Read one character from standard input device keyboard.
            memory[pcb_ptr + I_GPR1] = (int) i_char; //Store the character in the GPR in the PCB. Use typecasting from char to word data types.
            memory[pcb_ptr + I_STATE] = H_READY_STATE; //Set process state to Ready in the PCB.
            std::cout << "The character " << i_char << " was successfully INPUTTED.";
            InsertIntoRQ(pcb_ptr); //Insert PCB into ready queue.
        }
    } 

    void ISRoutputCompletionInterrupt()
    {
        word PID;
        char o_char;

        std::cout << "ISR designed for output completion has begun running, please specify the PID of the process that the output is being completed for: ";
        std::cin >> PID; //Read the PID of the process we're completing input for.

        PID = (int) PID;

        word pcb_ptr = SearchAndRemovePCBfromWQ(PID); //Search WQ to find the PCB that has the given PID, return value is stored in pcb_ptr.
        if (pcb_ptr > 0) //Only performs this section with a valid PCB address.
        {
            o_char = (char) memory[pcb_ptr + I_GPR1]; //Typecast the ascii code for the output character back into a character value. Store in output character.
            std::cout << "\nOUTPUT COMPLETED, CHARACTER DISPLAYED: " << o_char << std::endl; //Print the character that was in the PCB's GPR1 slot.
            memory[pcb_ptr + I_STATE] = H_READY_STATE; //Set process state to Ready in the PCB.
            InsertIntoRQ(pcb_ptr); //Insert PCB into ready queue.
        }

    }

    void ISRshutdownSystem()
    {
        //Terminate all processes in RQ one by one.
        word ptr = RQ; //Set ptr to first PCB pointed by RQ.

        while (ptr != H_EOL) //While there are still PCBs in the RQ...
        {
            RQ = memory[ptr + I_NEXT_POINTER]; //Set RQ to equal the next PCB in RQ.
            TerminateProcess(ptr); //Terminate the current process in the list.
            ptr = RQ; //Set ptr to the next PCB in RQ.
        }

        //Terminate all processes in WQ one by one.	
        ptr = WQ; //Set ptr to first PCB pointed by WQ.

        while (ptr != H_EOL) //While there are still PCBs in the WQ...
        {
            WQ = memory[ptr + I_NEXT_POINTER]; //Set RQ to equal the next PCB in RQ.
            TerminateProcess(ptr); //Terminate the current process in the list.
            ptr = WQ; //Set ptr to the next PCB in WQ.
        }
    }

    word CheckAndProcessInterrupt()
    {
        word i_id;

        std::cout << "\n Interrupts: \n 0: No interrupt. \n 1: Run program. \n 2: Shutdown system. \n 3: io_getc \n 4: io_putc \n Interrupt ID:";
        std::cin >> i_id;

        switch (i_id)
        {
        case INT_NO_OP:
            break;
        case INT_RUN_PROG:
            ISRrunProgramInterrupt();
            break;
        case INT_SHUTDOWN:
            ISRshutdownSystem();
            shutdown_status = true;
            break;
        case INT_IO_GETC:
            ISRinputCompletionInterrupt();
            break;
        case INT_IO_PUTC:
            ISRoutputCompletionInterrupt();
            break;
        default:
            std::cout << "Invalid interrupt signal. This is a no-op...";
            return INT_NO_OP;
        }

        return i_id;
    }

    word MemAllocSystemCall()
    {
        long size = r_gpr[2];

        if (size < 2 || size > H_START_SIZE_USER_FREE)
        {
            std::cout << "The size of memory requested was out of range.";
            return E_MTOPS_INVALID_SIZE;
        }

        r_gpr[1] = AllocateUserMemory(size); // Puts 2518, 18 over max addr, causing error

        if (r_gpr[1] < 0) // GPR1 reached an error status.
        {
            r_gpr[0] = r_gpr[1];
        }
        else
        {
            r_gpr[0] = 0; // BranchOnZero = OK
        }

        std::cout << "MemAllocSystemCall => GPR0: " << r_gpr[0] << " GPR1: " << r_gpr[1] << " GPR2: " << r_gpr[2] << std::endl;

        return r_gpr[0];
    }

    word MemFreeSystemCall()
    {
        long size = r_gpr[2];

        if (size < 2 || size > H_START_SIZE_USER_FREE)
        {
            std::cout << "The size of memory requested was out of range.";
            return E_MTOPS_INVALID_SIZE;
        }

        r_gpr[0] = FreeUserMemory(r_gpr[1], size);

        std::cout << "MemFreeSystemCall => GPR0: " << r_gpr[0] << " GPR1: " << r_gpr[1] << " GPR2: " << r_gpr[2] << std::endl;
    
        return r_gpr[0];
    }

    word io_getcSystemCall()
    {
        return INT_IO_GETC;
    }

    word io_putcSystemCall()
    {
        return INT_IO_PUTC;
    }

    /*
    * word: SystemCall
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
        r_psr = H_OS_MODE;

        word status = OK;

        switch (id)
        {
        case PROCESS_CREATE:
        {
            std::cout << "PROCESS_CREATE not implemented.";
            break;
        }
        case PROCESS_DELETE:
        {
            std::cout << "PROCESS_DELETE not implemented.";
            break;
        }
        case PROCESS_INQ:
        {
            std::cout << "PROCESS_INQ not implemented.";
            break;
        }
        case MEM_ALLOC:
        {
            status = MemAllocSystemCall();
            break;
        }
        case MEM_FREE:
        {
            status = MemFreeSystemCall();
            break;
        }
        case MSG_SEND:
        {
            std::cout << "MSG_SEND not implemented.";
            break;
        }
        case MSG_RECV:
        {
            std::cout << "MSG_RECV not implemented.";
            break;
        }
        case IO_GETC:
        {
            status = io_getcSystemCall();
            break;
        }
        case IO_PUTC:
        {
            status = io_putcSystemCall();
            break;
        }
        case TIME_GET:
        {
            std::cout << "TIME_GET not implemented.";
            break;
        }
        case TIME_SET:
        {
            std::cout << "TIME_SET not implemented.";
            break;
        }
        default:
        {
            std::cout << "Invalid syscall ID.";
            return E_MTOPS_INVALID_SYSCALL;
        }
        }

        r_psr = H_USER_MODE;

        return status;
    }

    /*
    * word: CPU
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
        word time_left = H_TTL;

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

            word _rem;
            
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
                        std::cout << "\nInvalid address for program counter on BRANCH_ON_MINUS: " << r_pc << std::endl;
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

                if (r_sp == memory[mtops_pcb_ptr + I_STACK_START] + H_STACK_SIZE)
                {
                    std::cout << "Stack is full, cannot push.";
                    return E_STACK_OVERFLOW;
                }
                else
                {
                    r_sp++;
                    memory[r_sp] = op1_value;
                }

                clock += 2;
                time_left -= 2;

                break;
            case H_OPCODE::POP: // Opcode 11, pop the latest value from the stack.
                
                status = FetchOperand(op1_mode, op1_gpr, &op1_addr, &op1_value);
                if (status < 0) { return status; }

                if (r_sp < memory[mtops_pcb_ptr + I_STACK_START])
                {
                    std::cout << "Stack is empty, cannot pop.";
                    return E_STACK_UNDERFLOW;
                }
                else
                {
                    std::cout << "Popping " << memory[r_sp] << " from the stack." << std::endl;;
                    op1_addr = memory[r_sp];
                    r_sp--;
                }

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
                    if (status == INT_IO_GETC || status == INT_IO_PUTC) { return status; }
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

        if (should_halt) { return H_HALT; }
        else if (time_left <= 0) { return H_TTL_EXP; }
        else { return E_UNKOWN; }
    }
}

// Begin Hypo process execution.
int main()
{
    Hypo::word status;

    Hypo::InitializeSystem();

    while (!Hypo::shutdown_status)
    {
        status = Hypo::CheckAndProcessInterrupt();
        if (status == Hypo::INT_SHUTDOWN) { break; }

        std::cout << "\nPre-CPU scheduling RQ: ";
        Hypo::PrintQueue(Hypo::RQ);

        std::cout << "\nPre-CPU scheduling WQ: ";
        Hypo::PrintQueue(Hypo::WQ);

        Hypo::DumpMemory("\nMemory pre-CPU scheduling: ", Hypo::H_MAX_PROGRAM_ADDR + 1, 249);

        Hypo::mtops_pcb_ptr = Hypo::SelectProcessFromRQ();

        Hypo::Dispatcher(Hypo::mtops_pcb_ptr);

        std::cout << "\nPost-process selection from RQ: ";
        Hypo::PrintQueue(Hypo::RQ);

        std::cout << "Dumping memory of running PCB: ";
        Hypo::PrintPCB(Hypo::mtops_pcb_ptr);

        std::cout << "\nCPU execution starting...\n";
        status = Hypo::CPU();
        std::cout << "\n --> CPU execution completed. Status code: " + status << std::endl;

        Hypo::DumpMemory("\nDynamic memory post-exeuction: ", Hypo::H_MAX_PROGRAM_ADDR + 1, 249);

        if (status == Hypo::H_TTL_EXP)
        {
            std::cout << "TTL has timed out, saving context and reinserting to RQ...";
            Hypo::SaveContext(Hypo::mtops_pcb_ptr);
            Hypo::InsertIntoRQ(Hypo::mtops_pcb_ptr);
            Hypo::mtops_pcb_ptr = Hypo::H_EOL;
        }

        else if (status == Hypo::H_HALT || status < 0)
        {
            std::cout << "Halt reached, terminating program...";
            Hypo::TerminateProcess(Hypo::mtops_pcb_ptr);
            Hypo::mtops_pcb_ptr = Hypo::H_EOL;
        }
        
        else if (status == Hypo::INT_IO_GETC)
        {
            std::cout << "\nIIO_GETC, enter interrupt for PID: " << Hypo::memory[Hypo::mtops_pcb_ptr + Hypo::I_PID];
            Hypo::SaveContext(Hypo::mtops_pcb_ptr); //Save CPU Context of running process in its PCB, because the running process is losing control of the CPU.
            Hypo::memory[Hypo::mtops_pcb_ptr + Hypo::I_WAIT_REASON] = Hypo::INT_IO_GETC;
            Hypo::InsertIntoWQ(Hypo::mtops_pcb_ptr); //Insert running process into WQ.
            Hypo::mtops_pcb_ptr = Hypo::H_EOL; 
        }

        else if (status == Hypo::INT_IO_PUTC)
        {
            std::cout << "\nIO_PUTC, enter interrupt for PID: " << Hypo::memory[Hypo::mtops_pcb_ptr + Hypo::I_PID];
            Hypo::SaveContext(Hypo::mtops_pcb_ptr); //Save CPU Context of running process in its PCB, because the running process is losing control of the CPU.
            Hypo::memory[Hypo::mtops_pcb_ptr + Hypo::I_WAIT_REASON] = Hypo::INT_IO_PUTC; //Set reason for waiting in the running PCB to 'Output Completion Event'.
            Hypo::InsertIntoWQ(Hypo::mtops_pcb_ptr); //Insert running process into WQ.
            Hypo::mtops_pcb_ptr = Hypo::H_EOL; //Set the running PCB ptr to the end of list.
        }

        else
        {
            std::cout << "\nUnknown error. (0xDEAD)";
            return Hypo::E_UNKOWN;
        }
    }

    std::cout << "System is shutting down.";
    return 0;
}
