#include <stdio.h>
#include <stdint.h>

#define WORD_SIZE 16
#define STACK_SIZE 1000
#define MMIO_CHAR_OUT 32

typedef uint16_t word_t;

enum {
    NOP, MOV, ADD, SUB, AND, OR, MUL, DIV,
    JMP, JZ, CALL, RET, HALT, LOAD, STORE
};

const char *OPCODE_STRINGS[] = {
    "NOP","MOV","ADD","SUB","AND","OR","MUL","DIV",
    "JMP","JZ","CALL","RET","HALT","LOAD","STORE"
};

struct ALUFlags {
    uint8_t zx, nx, zy, ny, f, no;
    uint8_t zr, ng, ov, cy;
};

struct ALU {
    word_t x, y, out;
    struct ALUFlags flags;
};

struct Memory {
    word_t mem[400];
};

struct GPR {
    word_t reg[8];
};

struct SPR {
    word_t SP;
};

struct CU {
    word_t IP, IR;
    struct ALUFlags aluflags;
};

struct CPU {
    struct Memory mainMemory;
    struct GPR gpr;
    struct SPR spr;
    struct CU cu;
    struct ALU alu;
    int running;
    word_t static_counter;
};

/* ---------------- N-bit adder ---------------- */

static word_t add_nbit(word_t a, word_t b, uint8_t *carry_out) {
    word_t sum = 0;
    word_t carry = 0;

    for (int i = 0; i < WORD_SIZE; i++) {
        word_t bit_a = (a >> i) & 1u;
        word_t bit_b = (b >> i) & 1u;

        word_t sum_bit = bit_a ^ bit_b ^ carry;
        carry = (bit_a & bit_b) | (bit_a & carry) | (bit_b & carry);
        sum |= (sum_bit << i);
    }

    *carry_out = (uint8_t)carry;
    return sum;
}

/* ---------------- ALU core (ADD / SUB) ---------------- */

static void alu_compute(struct ALU *alu) {
    word_t x = alu->x;
    word_t y = alu->y;

    alu->flags.cy = 0;
    alu->flags.ov = 0;

    if (alu->flags.zx) x = 0;
    if (alu->flags.zy) y = 0;
    if (alu->flags.nx) x = (word_t)~x;
    if (alu->flags.ny) y = (word_t)~y;

    uint8_t d = (uint8_t)((alu->flags.zx << 5) |
                          (alu->flags.nx << 4) |
                          (alu->flags.zy << 3) |
                          (alu->flags.ny << 2) |
                          (alu->flags.f  << 1) |
                           alu->flags.no);

    if (d == 0b000010) {          // x + y
        uint8_t carry;
        alu->out = add_nbit(x, y, &carry);
        alu->flags.cy = carry;

        int16_t sx   = (int16_t)x;
        int16_t sy   = (int16_t)y;
        int16_t sres = (int16_t)alu->out;

        alu->flags.ov = ((sx > 0 && sy > 0 && sres < 0) ||
                         (sx < 0 && sy < 0 && sres > 0));
    } else if (d == 0b010011) {   // x - y
        int16_t sx   = (int16_t)x;
        int16_t sy   = (int16_t)y;
        int16_t sres = (int16_t)(sx - sy);

        alu->out = (word_t)sres;
        alu->flags.cy = (sx < sy);
        alu->flags.ov = ((sx > 0 && sy < 0 && sres < 0) ||
                         (sx < 0 && sy > 0 && sres > 0));
    } else {
        alu->out = 0;
    }

    alu->flags.zr = (alu->out == 0);
    alu->flags.ng = ((alu->out & (1u << (WORD_SIZE - 1))) != 0);
}

/* ---------------- Encoding & helpers ---------------- */

static word_t encodeI(uint8_t op, uint8_t r1, uint8_t r2, uint8_t imm) {
    return (word_t)(((op & 0xF) << 12) |
                    ((r1 & 0x7) << 9)  |
                    ((r2 & 0x7) << 6)  |
                     (imm & 0x3F));
}

static void load_program(struct CPU *cpu, word_t *program, int size) {
    for (int i = 0; i < size; i++) {
        cpu->mainMemory.mem[i] = program[i];
    }
    cpu->cu.IP = 0;
    cpu->spr.SP = 399;
}

static void dump_registers(struct CPU *cpu) {
    printf("  Registers: ");
    for (int j = 0; j < 8; j++) {
        printf("R%d=%d ", j, cpu->gpr.reg[j]);
    }
    printf("| IP=%d ", cpu->cu.IP);
    printf("| Flags: ZR=%d\n", cpu->cu.aluflags.zr);
}

/* ---------------- Fetch / Decode / Execute ---------------- */

static void fetch_decode_execute(struct CPU *cpu) {
    if (cpu->running == 0) return;

    cpu->cu.IR = cpu->mainMemory.mem[cpu->cu.IP++];
    word_t instr = cpu->cu.IR;

    uint8_t op  = (instr >> 12) & 0xF;
    uint8_t r1  = (instr >> 9)  & 0x7;
    uint8_t r2  = (instr >> 6)  & 0x7;
    uint8_t imm = instr & 0x3F;

    printf("[Cycle %d] FETCH: IP=%d, IR=0x%04X\n",
           cpu->cu.IP - 1, cpu->cu.IP - 1, instr);
    printf("          DECODE: OP=%s, R1=%d, R2=%d, IMM=%d\n", 
           OPCODE_STRINGS[op], r1, r2, imm);

    if (op == NOP) {
        // No operation

    } else if (op == MOV) {
        printf("          EXECUTE: R%d = %d\n", r1, imm);
        cpu->gpr.reg[r1] = imm;

    } else if (op == ADD) {
        word_t old_val = cpu->gpr.reg[r1];
        cpu->alu.x = cpu->gpr.reg[r1];
        cpu->alu.y = imm;
        cpu->alu.flags = (struct ALUFlags){0,0,0,0,0,0,0,0,0,0};
        alu_compute(&cpu->alu);
        cpu->gpr.reg[r1] = cpu->alu.out;
        cpu->cu.aluflags = cpu->alu.flags;
        printf("          EXECUTE: R%d = %d + %d = %d\n",
               r1, old_val, imm, cpu->gpr.reg[r1]);

    } else if (op == SUB) {
        word_t old_val = cpu->gpr.reg[r1];
        cpu->alu.x = cpu->gpr.reg[r1];
        cpu->alu.y = imm;
        cpu->alu.flags = (struct ALUFlags){0,0,1,0,1,1,0,0,0,0};
        alu_compute(&cpu->alu);
        cpu->gpr.reg[r1] = cpu->alu.out;
        cpu->cu.aluflags = cpu->alu.flags;
        printf("          EXECUTE: R%d = %d - %d = %d, ZR=%d\n", 
               r1, old_val, imm, cpu->gpr.reg[r1], cpu->cu.aluflags.zr);

    } else if (op == JMP) {
        printf("          EXECUTE: Jump to address %d\n", imm);
        cpu->cu.IP = imm;

    } else if (op == JZ) {
        if (cpu->cu.aluflags.zr) {
            printf("          EXECUTE: Jump to address %d (ZR=1)\n", imm);
            cpu->cu.IP = imm;
        } else {
            printf("          EXECUTE: No jump (ZR=0)\n");
        }

    } else if (op == HALT) {
        cpu->running = 0;
        printf("          EXECUTE: HALT\n");
        printf("\n=== Program Complete ===\n");
        return;
    }

    printf("          STORE: ");
    dump_registers(cpu);
    printf("\n");
}

static void run_cpu(struct CPU *cpu) {
    cpu->running = 1;
    while (cpu->running) {
        fetch_decode_execute(cpu);
    }
}

/* ---------------- TIMER PROGRAM ---------------- */
/*
   This program demonstrates a simple counter from 1 to 10 in R0.

   Pseudocode:
       R0 = 0        ; counter
       R1 = 10       ; limit (not strictly needed)
   loop:
       R0 = R0 + 1
       tmp = R0 - 10
       if tmp == 0: HALT
       else restore R0 (add 10 back) and continue

   We implement this with immediate ADD/SUB and JZ.
*/

int main(void) {
    struct CPU cpu = {0};
    
    printf("=======================================================\n");
    printf("  TIMER PROGRAM\n");
    printf("  Demonstrates Fetch-Decode-Execute-Store Cycle\n");
    printf("=======================================================\n\n");

    int prog_size = 0;
    word_t timer_program[100];

    // Initialize R0 = 0 (counter), R1 = 10 (limit - just for reference)
    timer_program[prog_size++] = encodeI(MOV, 0, 0, 0);   // R0 = 0
    timer_program[prog_size++] = encodeI(MOV, 1, 0, 10);  // R1 = 10

    // loop starts at address 2
    // R0 = R0 + 1
    timer_program[prog_size++] = encodeI(ADD, 0, 0, 1);   // [2] R0 = R0 + 1

    // R0 = R0 - 10   (for comparison)
    timer_program[prog_size++] = encodeI(SUB, 0, 0, 10);  // [3] R0 = R0 - 10

    // If ZR == 1, we've reached 10 → HALT (jump to HALT at addr 7)
    timer_program[prog_size++] = encodeI(JZ, 0, 0, 7);    // [4] if R0-10 == 0, jump to 7

    // Otherwise restore R0: R0 = (R0 - 10) + 10 = original R0
    timer_program[prog_size++] = encodeI(ADD, 0, 0, 10);  // [5] restore R0

    // Jump back to loop start (addr 2)
    timer_program[prog_size++] = encodeI(JMP, 0, 0, 2);   // [6] loop

    // HALT
    timer_program[prog_size++] = encodeI(HALT, 0, 0, 0);  // [7] HALT

    printf("Initial State:\n");
    dump_registers(&cpu);
    printf("\n");
    printf("Starting Execution...\n");
    printf("=======================================================\n\n");

    load_program(&cpu, timer_program, prog_size);
    run_cpu(&cpu);

    printf("\nFinal Register State:\n");
    dump_registers(&cpu);
    printf("\n(TIMER) Final R0 value (last count): %d\n", cpu.gpr.reg[0]);

    return 0;
}