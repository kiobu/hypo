# Hypo Opcodes

| Opcode	| Mnemonic	 | Operands	      | Description                             |
|---------|------------|----------------|-----------------------------------------|
| 0		    | Halt		   | None		        | Stop program execution                  |
| 1		    |	Add		     | Op1,Op2	      | Op1 = Op1 + Op2                         |
| 2		    |	Subtract	 | Op1,Op2	      |	Op1 = Op1 – Op2                         |
| 3		    | Multiply	 | Op1,Op2	      |	Op1 = Op1 * Op2                         |
| 4		    |	Divide		 | Op1,Op2	      |	Op1 = Op1 / Op2  (integer division)     |
| 5		    |	Move		   | Op1,Op2	      |	Op1 = Op2                               |
| 6		    |	Branch	 	 | Address	      |	PC = Address                            |
| 7		    |	BrOnMinus	 | Op1,Address	  |	if (Op1 < 0), PC = Address, else PC++   |
| 8		    |	BrOnPlus	 | Op1,Address	  |	if (Op1 > 0), PC = Address else PC++    |
| 9		    |	BrOnZero	 | Op1,Address	  |	if (Op1 = 0), PC = Address, else PC++   |
| 10		  |	Push		   | Op1	          |	SP++ then Memory[SP] = Op1              |
| 11		  |	Pop		     | Op1	          |	Op1 = Memory[SP], then SP--             |
| 12		  |	SystemCall | Op1	          |	Op1 is the System Call Identifier       |
