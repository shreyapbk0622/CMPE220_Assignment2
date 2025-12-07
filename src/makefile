CC = gcc
CFLAGS = -std=c11 -Wall

ASSEMBLER = assembler
CPU = cpu
FACTORIAL = factorial
HEADER = call_demo.h
ASM_SOURCE = call_demo.asm
BIN_FILE = call_demo.bin

# Default target: compile all three programs
all: $(ASSEMBLER) $(FACTORIAL) $(CPU)

# Compilation and Assembly Targets
# 1. Build the assembler
$(ASSEMBLER): assembler.c
	@echo "--> Compiling custom assembler: $@"
	$(CC) $(CFLAGS) assembler.c -o $@

# 2. Build the standard C factorial program
$(FACTORIAL): factorial.c
	@echo "--> Compiling standard C program: $@"
	$(CC) $(CFLAGS) factorial.c -o $@

# 3. Rule to generate the machine code header (must run after assembler is built)
$(HEADER): $(ASM_SOURCE) $(ASSEMBLER)
	@echo "--> Assembling $(ASM_SOURCE) into machine code header: $@"
	./$(ASSEMBLER) $(ASM_SOURCE) $(HEADER)

# 4. Build the CPU emulator (depends on the generated header)
$(CPU): cpu.c $(HEADER)
	@echo "--> Compiling custom CPU emulator: $@"
	$(CC) $(CFLAGS) cpu.c -o $@


clean:
	rm -f $(ASSEMBLER) $(CPU) $(FACTORIAL) $(HEADER) $(BIN_FILE)
	@echo "Cleaned up all compiled and generated files."
