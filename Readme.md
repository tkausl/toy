Simple TOY simulator in C as proposed by Princeton

See [Instruction Cheatsheet](https://introcs.cs.princeton.edu/java/62toy/cheatsheet.txt) for a short instruction manual

Usage:

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