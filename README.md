# Architecture
Condition Codes
1. Cmpq: compare
This instruction compares the value in the two given registers. Basically, it performs the subtraction of two
register. It is usually used with one of the set instructions.
Syntax: cmpq [rs],[rd]
Example: cmpq r1, r2
2. Setne: set if not equal
This instruction sets a given register based on the value computed from compare instruction.
Syntax : setne [rd]
Example: setne r3  if r1!=r2, r3 value will be set as 1
3. Sete: set if equal
This instruction the store the value ‘1’ or ‘0’ on given register depends on the value computed from compare
instruction.
Syntax : sete [rd]
Example: setne r3  if r1=r2, r3 value will be set as 1
4. setl : Set Less Than
This instruction will set the indicated register to '1' if the comparison of 2 registers (performed prior) is true for
r1<r2.
Syntax: setl [rd]
5. setg : Set Greater Than
This instruction will set the indicated register to '1' if the 1st operand is greater than 2nd
Syntax: setg [rd]
6. beq : Branch If Equal
This instruction compares the value of given two register and if found equal then jumps to the given label or
else continues to next instruction.
Syntax: beq [rs],[rd],[label]
7. bne : Branch if Not Equal
This instruction compares the value of given two register and if found not equal then jumps to the given label
or else continues to next instruction.
Syntax: bne [rs],[rd],[label]

Examples with Flags
1. Zero Flag
compare function performs subtraction of r1 and r2 resulting in value 0. Thus, setting zero flag.
3. Sign Flag
Comparing values -21474836 and 2 in r1 and r2 register. So, subtraction of r1 and r2 sets sign flag.

Jump Instruction
1. Jump to a label
Syntax: j [label]
2. Jump and link
jump to the specified instruction after storing the PC value in return address register.
Syntax: jl v:[value] -> jump and link to the specified value
jl [label] -> jump and link to the label
jl r:[reg] -> jump and link to value in register
Example: jl v:20 , jl function, jl r:r4
3. Jump to a register value
jump to the specified value stored in a register.
Syntax: jr [reg] -> jump to value in register
Example: jr r4

LEA Instruction
Load Effective Address Instruction is used to calculate and load the memory location address in the destination
address. This is used in manipulating stack pointer.
Syntax: lea rd,rt:rs:Scale:displacement
Example: lea rsp,r1,r2:*:*:*, lea rsp,r2,rz,4,5

Function Calls
Implemented Simple addition function call.
