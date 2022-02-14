## Assembly Code

| Label | Mnemonic    | Operands      | Description
|-------|-------------|---------------|---------------------------------------------------|
|  main	|  Method     |		          | Start of main function
|  sum 	|  long	      | 0		      | Initialize sum to 0
|  even	|  long	      | 2		      | Set even counter val to 2
|  count|  long       | 0		      | Set value of count to 0
|  n1	|  long       | 550	          | Value for arithmetic set to 550
|  n2	|  long	      | 100	          | Value for arithmetic set to 100
|  n3	|  long	      | 5		      | Value for arithmetic set to 5
|decrem	|  long	      | 1		      | number used for loop decrement
|Counter|  long	      | 50		      | loop counter set to 50
|  Start|  Move	      | GPR2,0	      | move 0 to GPR2
|  Loop	|  Add	      | count,even	  | add 2 to get next even number
|       |  Add	      | GPR2,count	  | add next even number to sum
|       |  Subtract   | Counter,dec	  | subtract one from loop counter
|       |  BrOnPlus   | Counter,Loop  | Branch on plus if counter > 0
|       |  move	      | sum,GPR2	  | move value of r2 to sum
|  temp	|  long	      | 0		      | Temp set to 0
|       |  subtract   | n1,sum	      |	Subtract sum from n1 (550)
|       |  divide     | n1,n2	      | Divide n1 / 100
|       |  multiply   | n1,n3	      | multiply n1 * 5
|       |  move	      | temp,num	  | Move num -> temp
|       |  BrOnMinus  | Temp,check1   |	Branch on minus if check1 < 0
|       |  BrOnPlus   | Temp,Check2	  | Branch on plus if check2 > 0
| Check1| move 		  | sum,2	      |	Move value of 2 to sum
|       | Halt		  |	              |
| Check2| move		  | sum,1	      |	Move value of 1 to sum
|  	    | Halt		  |	              |
|  	    | end         | Start         |



## Symbol Table

| Symbol               | Addr |
|----------------------|------|
| StartOfProgram       | 2    |
| LoopStart            | 17   |
| LoopEnd              | 22   |
| CheckValue1          | 26   |
| ProgramEnd           | 27   |
| CheckValue2          | 28   |


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