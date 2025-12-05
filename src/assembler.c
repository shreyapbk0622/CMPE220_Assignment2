#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_LABELS 100
#define MAX_LINE_LENGTH 256
#define MAX_INSTRUCTIONS 400

typedef uint16_t word_t;

// Label structure for symbol table
typedef struct {
    char name[64];
    int address;
} Label;

// Instruction structure
typedef struct {
    word_t code;
    int line_number;
    char original[MAX_LINE_LENGTH];
} Instruction;

Label labels[MAX_LABELS];
int label_count = 0;

Instruction instructions[MAX_INSTRUCTIONS];
int instruction_count = 0;

// Opcode mapping
typedef struct {
    const char *mnemonic;
    int opcode;
} OpcodeMap;

OpcodeMap opcodes[] = {
    {"NOP", 0x0}, {"MOV", 0x1}, {"ADD", 0x2}, {"SUB", 0x3},
    {"AND", 0x4}, {"OR", 0x5},  {"MUL", 0x6}, {"DIV", 0x7},
    {"JMP", 0x8}, {"JZ", 0x9},  {"CALL", 0xA}, {"RET", 0xB},
    {"HALT", 0xC}
};

// Helper: Find opcode for mnemonic
int get_opcode(const char *mnemonic) {
    for (int i = 0; i < (int)(sizeof(opcodes) / sizeof(opcodes[0])); i++) {
        if (strcasecmp(opcodes[i].mnemonic, mnemonic) == 0) {
            return opcodes[i].opcode;
        }
    }
    return -1;
}

// Helper: Parse register number (e.g., "R0" -> 0)
int parse_register(const char *token) {
    if (token[0] == 'R' || token[0] == 'r') {
        int reg = atoi(token + 1);
        if (reg >= 0 && reg <= 7) {
            return reg;
        }
    }
    return -1;
}

// Helper: Parse immediate value or label reference
int parse_immediate(const char *token, int *is_label) {
    *is_label = 0;

    if (isdigit((unsigned char)token[0]) || token[0] == '-') {
        return atoi(token);
    }

    // It's a label
    *is_label = 1;
    return 0; // Will be resolved in second pass
}

// Helper: Find label address
int find_label(const char *name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) {
            return labels[i].address;
        }
    }
    return -1;
}

// Helper: Add label to symbol table
void add_label(const char *name, int address) {
    if (label_count < MAX_LABELS) {
        strncpy(labels[label_count].name, name, sizeof(labels[label_count].name) - 1);
        labels[label_count].name[sizeof(labels[label_count].name) - 1] = '\0';
        labels[label_count].address = address;
        label_count++;
    }
}

// Helper: Encode instruction
word_t encode_instruction(int op, int r1, int r2, int imm) {
    return (word_t)(((op & 0xF) << 12) |
                    ((r1 & 0x7) << 9)  |
                    ((r2 & 0x7) << 6)  |
                     (imm & 0x3F));
}

// Helper: Remove comments and trim whitespace
void clean_line(char *line) {
    // Remove comments (semicolon to end of line)
    char *comment = strchr(line, ';');
    if (comment) *comment = '\0';

    // Trim trailing whitespace
    int len = (int)strlen(line);
    while (len > 0 && isspace((unsigned char)line[len - 1])) {
        line[--len] = '\0';
    }

    // Trim leading whitespace
    char *start = line;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != line) {
        memmove(line, start, strlen(start) + 1);
    }
}

// First pass: Collect labels
void first_pass(FILE *fp) {
    char line[MAX_LINE_LENGTH];
    int address = 0;
    int line_number = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_number++;
        clean_line(line);

        if (strlen(line) == 0) continue;

        // Check for label (ends with colon)
        char *colon = strchr(line, ':');
        if (colon) {
            char label[64];
            sscanf(line, "%63[^:]:", label);
            add_label(label, address);

            // Check if there's an instruction on the same line
            char *after_label = colon + 1;
            clean_line(after_label);
            if (strlen(after_label) > 0) {
                address++;
            }
        } else {
            // Regular instruction
            address++;
        }
    }

    rewind(fp);
}

// Second pass: Generate machine code
void second_pass(FILE *fp) {
    char line[MAX_LINE_LENGTH];
    int line_number = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_number++;
        strncpy(instructions[instruction_count].original, line,
                sizeof(instructions[instruction_count].original) - 1);
        instructions[instruction_count].original[
            sizeof(instructions[instruction_count].original) - 1] = '\0';
        instructions[instruction_count].line_number = line_number;

        clean_line(line);
        if (strlen(line) == 0) continue;

        // Skip labels, but check for instruction after label
        char *colon = strchr(line, ':');
        if (colon) {
            char *after_label = colon + 1;
            clean_line(after_label);
            if (strlen(after_label) == 0) continue;
            strcpy(line, after_label);
        }

        // Parse instruction
        char mnemonic[16];
        char operands[MAX_LINE_LENGTH] = {0};

        int items = sscanf(line, "%15s %[^\n]", mnemonic, operands);

        int op = get_opcode(mnemonic);
        if (op == -1) {
            fprintf(stderr, "Error on line %d: Unknown instruction '%s'\n",
                    line_number, mnemonic);
            continue;
        }

        int r1 = 0, r2 = 0, imm = 0;

        if (items == 2) {
            // Parse operands
            char *token = strtok(operands, ",");
            char tokens[3][64];
            int token_count = 0;

            while (token && token_count < 3) {
                while (*token && isspace((unsigned char)*token)) token++;
                char *end = token + strlen(token) - 1;
                while (end > token && isspace((unsigned char)*end)) *end-- = '\0';

                strncpy(tokens[token_count], token, sizeof(tokens[token_count]) - 1);
                tokens[token_count][sizeof(tokens[token_count]) - 1] = '\0';
                token_count++;
                token = strtok(NULL, ",");
            }

            if (op == 0x0 || op == 0xB || op == 0xC) {
                // NOP, RET, HALT - no operands
            } else if (op == 0x1) {
                // MOV R1, IMM
                r1 = parse_register(tokens[0]);
                int is_label;
                imm = parse_immediate(tokens[1], &is_label);
                if (is_label) {
                    imm = find_label(tokens[1]);
                    if (imm == -1) {
                        fprintf(stderr, "Error on line %d: Undefined label '%s'\n",
                                line_number, tokens[1]);
                    }
                }
            } else if (op == 0x2 || op == 0x3) {
                // ADD/SUB R1, IMM
                r1 = parse_register(tokens[0]);
                int is_label;
                imm = parse_immediate(tokens[1], &is_label);
                if (is_label) {
                    imm = find_label(tokens[1]);
                    if (imm == -1) {
                        fprintf(stderr, "Error on line %d: Undefined label '%s'\n",
                                line_number, tokens[1]);
                    }
                }
            } else if (op == 0x4 || op == 0x5 || op == 0x6 || op == 0x7) {
                // AND/OR/MUL/DIV R1, R2
                r1 = parse_register(tokens[0]);
                r2 = parse_register(tokens[1]);
            } else if (op == 0x8 || op == 0x9 || op == 0xA) {
                // JMP/JZ/CALL IMM (or label)
                int is_label;
                imm = parse_immediate(tokens[0], &is_label);
                if (is_label) {
                    imm = find_label(tokens[0]);
                    if (imm == -1) {
                        fprintf(stderr, "Error on line %d: Undefined label '%s'\n",
                                line_number, tokens[0]);
                    }
                }
            }
        }

        instructions[instruction_count].code = encode_instruction(op, r1, r2, imm);
        instruction_count++;
    }
}

// Output machine code as C header
void output_machine_code(const char *output_file) {
    FILE *fp = fopen(output_file, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open output file '%s'\n", output_file);
        return;
    }

    fprintf(fp, "// Machine code generated by CMPE220 Assembler\n");
    fprintf(fp, "// Total instructions: %d\n\n", instruction_count);
    //fprintf(fp, "typedef unsigned short word_t;\n\n");
    fprintf(fp, "word_t program[] = {\n");

    for (int i = 0; i < instruction_count; i++) {
        fprintf(fp, "    0x%04X", instructions[i].code);
        if (i < instruction_count - 1) fprintf(fp, ",");
        fprintf(fp, "  // [%d] %s", i, instructions[i].original);
    }

    fprintf(fp, "};\n\n");
    fprintf(fp, "int program_size = %d;\n", instruction_count);

    fclose(fp);
}

// Output raw binary file
void output_binary(const char *output_file) {
    FILE *fp = fopen(output_file, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open output file '%s'\n", output_file);
        return;
    }

    for (int i = 0; i < instruction_count; i++) {
        fwrite(&instructions[i].code, sizeof(word_t), 1, fp);
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("CMPE220 Assembler\n");
        printf("Usage: %s <input.asm> [output.h]\n", argv[0]);
        printf("  Assembles .asm file into machine code\n");
        printf("  Default output: program.h (C header file)\n");
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = (argc > 2) ? argv[2] : "program.h";

    FILE *fp = fopen(input_file, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
        return 1;
    }

    printf("CMPE220 Assembler - Assembling '%s'...\n", input_file);

    // First pass: collect labels
    first_pass(fp);
    printf("  Found %d labels\n", label_count);

    // Second pass: generate code
    second_pass(fp);
    printf("  Generated %d instructions\n", instruction_count);

    fclose(fp);

    // Output machine code as C header
    output_machine_code(output_file);
    printf("  Machine code written to '%s'\n", output_file);

    // Also output binary alongside the header
    char binary_file[256];
    strncpy(binary_file, output_file, sizeof(binary_file) - 1);
    binary_file[sizeof(binary_file) - 1] = '\0';
    char *dot = strrchr(binary_file, '.');
    if (dot) {
        strcpy(dot, ".bin");
    } else {
        strcat(binary_file, ".bin");
    }
    output_binary(binary_file);
    printf("  Binary written to '%s'\n", binary_file);

    printf("Assembly complete!\n");
    return 0;
}
