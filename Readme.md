Simple TOY simulator in C as proposed by Princeton

See [Instruction Cheatsheet](https://introcs.cs.princeton.edu/java/62toy/cheatsheet.txt) for a short instruction manual

Usage: ./toy <filename>

If no filename is passed, starts in interactive mode. If a filename is passed, the state is loaded from the file and the machine runs to completion (a HALT instruction), then prints out the resulting state.


The following commands are available in interactive mode:

    h
      prints this help message

    m
      displays the memory

    m <addr> <value> [<value>...]
      sets one or more values, beginning at <addr>

    r
      prints the state of the registers

    r <addr> <value>
      sets the state of register <addr> to <value>

    p
      prints the PC

    p <value>
      sets the PC

    s
      executes one instruction from M[PC] and increments the PC

    s <num>
      executes <num> instructions. Stops at HALT

    store <filename>
      store the current state in <filename>

    load <filename>
      load the state stored in <filename>
      

      
Assembler:

asm.c contains a _very_ primitive assembler which assembles instructions to a machine state. Instructions are written using a syntax similar to intel assembly syntax.

Usage: ./asm <source file>

Lines starting with a `;` are considered comments and ignored.

Instructions are of the form `<op> <destination> <source> (<source2>)` where `<op>` is one of `add`, `sub`, `and`, `xor`, `shl`, `shr`, `hlt`, `brz`, `brp`, `jmp`, `call` or `mov` and `<destination>`, `<source>` and `<source2>` are arguments to the instruction.

Depending on the operation, the arguments can be immediate values written as HEX-digits, registers written as `rX` where `X` is one hex digit,
immediate memory locations written as `[XX]` where `X` is a hex digit, or indirect memory locations written as `[rX]` where `rX` is the register containing the memory location.

Currently, the following instructions are supported:

`add` for addition, `sub` for subtraction, `and` for binary AND, `xor` for binary exclusive OR, `shl` for left-shift and `shr` for right-shift all take three arguments, `destination`, `source1` and `source2`.

`hlt` is the HALT instruction and takes no arguments

`brz` and `brp`, branch if zero and branch if positive, take one register argument which contains the value to compare and the target address as immediate argument.

`jmp` takes one register argument and jumps to the stored value

`call` requires one register and one immediate argument. The instruction stores the current PC in the register and then jumps unconditionally to the immediate value

`mov` is used for most load and store operations. It takes two arguments, the destination and the source. The arguments can be direct or indirect memory arguments,
registers or immediate values, however exactly one of both arguments must be an register argument and the destination cannot be an immediate value.

Additionally, the assembler supports non-instruction directives which have a dot `.` as prefix. The following directives are supported:

`.offset XX` where `X` is a hex digit sets the memory location for the following instructions or immediate values

`.pc XX` sets the initial instruction pointer

`.rX XXXX` where `X` is a hex digit sets the initial value of a register

`.immediate XXXX (XXXX ...)` where `X` is a hex digit sets one or more literal values for data or instructions which can't be assembled currently

Example source code:

    ; a demo program
    ; default offset is 0, so below line sets the first 16 memory values to 0 to f 
    .immediate 0 1 2 3 4 5 6 7 8 9 a b c d e f
    ; above line incremented offset automatically, but just to be sure set it to 10 again
    .offset 10
    .pc 10
    ; our "code" begins at 10 so set the initial PC to 10

    mov r1, 1
    mov rb, [FF]
    mov ra, 80
    mov r9, 0
    sub r2, rb, r9
    brz r2, 1B
    add rc, ra, r9
    mov rd, [FF]
    mov [rc], rd
    add r9, r9, r1
    brz r0, 14
