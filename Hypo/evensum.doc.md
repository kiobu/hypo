# HYPO Program 1: EvenSum

## Assembly Code

## Symbol Table

| Symbol               | Addr |
|----------------------|------|
| StartOfProgram       | 2    |
| LoopStart            | 17   |
| LoopEnd              | 22   |
| ProgramEnd           | 27   |


## Machine Code

| Address | Content   | Comment                                        |
|---------|-----------|------------------------------------------------|
| 0       | 0         |
| 1       | 5         |
| 2       | 51060     | Move the below value to GPR0.
| 3       | 2         | Immediate `2` for repeated addition.
| 4       | 51360     | Move the below value to GPR3.
| 5       | 550       | Immediate `550` used to subtract.
| 6       | 51460     | Move the below value to GPR4.
| 7       | 100       | Immediate `100` used as a dividend.
| 8       | 51560     | Move the below value to GPR5.
| 9       | 5         | Immediate `5` to multiply.
| 10      |  51660    | Move the below value to GPR6.
| 11      |  50       | Loop counter value.
| 12      |  51760    | Move the below value to GPR7.
| 13      |  1        | Number to decrement the loop counter.
| 14      |  11110    | Adds value of GPR0 + value of GPR1.
| 15      |  11211    | Adds value of GPR1 + value of GPR2 (even sum).
| 16      |  21617    | Adds value of GPR6 - value of GPR7.
| 17      |  81600    | Branch to the below instruction.
| 18      |  14       | Instruction 14.
| 19      |  21312    | Subtracts value of GPR3 - value of GPR2.
| 20      |  41314    | Divides value of GPR3 / value of GPR4.
| 21      |  31315    | Multiplies value of GPR3 * value of GPR5.
| 22      |  71300    | Branch-on-minus to the below instruction (loop is complete.)
| 23      |  26       | Instruction 26.
| 24      |  81300    | Branch-on-plus to the below instruction (one more iteration.)
| 25      |  28       | Instruction 28.
| 26      |  11617    | Adds value of GPR6 + value of GPR7.
| 27      |  01010    | Halt.
| 28      |  11617    | Adds value of GPR6 + value of GPR7.
| 29      |  11617    | Repeats above instruction.
| -1      |  2        | PC is set to instruction 2.