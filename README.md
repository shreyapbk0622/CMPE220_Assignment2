# CMPE220_Assignment2

cd src

# 1) Build assembler
gcc -std=c11 assembler.c -o assembler

# 2) Assemble call_demo.asm → call_demo.h + call_demo.bin
./assembler call_demo.asm call_demo.h

# 3) Build CPU emulator that runs call_demo
gcc -std=c11 cpu.c -o cpu

# 4) Run CPU emulator
./cpu

# 5) Build and run plain C recursion program (factorial.c)
gcc -std=c11 factorial.c -o factorial
./factorial
