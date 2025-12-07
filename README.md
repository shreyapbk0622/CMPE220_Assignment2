# CMPE 220 Project 2: Software CPU Recursion Demo

This repository contains the source code for a Software CPU emulator, its custom assembler, and a demonstration of function call handling and recursion implemented in custom assembly language (`call_demo.asm`).

## ⚙️ Prerequisites

To build and run this project, you will need:
* **Git:** For cloning the repository.
* **GCC (GNU Compiler Collection):** The `Makefile` uses the `gcc` command.
* **Make:** The build utility.

## ⬇️ Setup and Compilation

All build and run operations must be performed from the **`src/` directory**.

1.  **Clone the Repository:**
    ```bash
    git clone [https://github.com/shreyapbk0622/CMPE220_Assignment2.git](https://github.com.shreyapbk0622/CMPE220_Assignment2.git)
    ```

2.  **Navigate to the Source Directory:**
    ```bash
    cd CMPE220_Assignment2/src
    ```

3.  **Compile All Programs:**
    The `make all` command executes the entire build process based on the dependencies defined in the `Makefile`:
    * Compiles `assembler.c` -> `./assembler`
    * Assembles `call_demo.asm` using `./assembler` -> `call_demo.h` and `call_demo.bin`
    * Compiles `cpu.c` (including `call_demo.h`) -> `./cpu`
    * Compiles `factorial.c` -> `./factorial`

    Run the following command:
    ```bash
    make all
    ```

    Alternatively you can run these set of commands in the following order:
    ```bash
    1) gcc -std=c11 -Wall assembler.c -o assembler
    2) ./assembler call_demo.asm call_demo.h
    3) gcc -std=c11 -Wall cpu.c -o cpu
    4) gcc -std=c11 -Wall factorial.c -o factorial
    ```

## 🚀 Running the Demos

The execution steps directly correspond to the project demonstration requirements.

### 1. Run Custom CPU Recursion Demo (Steps 4 & 5)

This sequence runs the assembled program (`call_demo.asm`) on the custom CPU emulator. The output will show the **Instruction Pointer (IP)**, **Stack Pointer (SP)**, and register changes, providing the evidence of correct recursion and stack handling.

1.  **Run the CPU Emulator:**
    ```bash
    ./cpu
    ```

### 2. Run Standard C Recursion Demo (Steps 6 & 7)

This sequence runs the standard C version of the recursive program, demonstrating the expected high-level result for comparison.

1.  **Run the Factorial Program:**
    ```bash
    ./factorial
    ```
    (You will be prompted to enter a number, e.g., `8`).

### 3. Clean Up

To remove all generated executables and machine code files:

```bash
make clean

