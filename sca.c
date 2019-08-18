#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT_LEN 256
#define ERR_LEN 256

typedef enum operands {
    OPERANDS_RA_RB,
    OPERANDS_RB,
    OPERANDS_RB_K,
    OPERANDS_K,
    OPERANDS_NONE
} operands;

typedef struct instruction {
    const char *mnemonic;
    unsigned char opcode;
    operands nops;
} instruction;

static const instruction instructions[] = {
    /* Arithmetic and Logic Instructions */
    {"ADD", 0x80, OPERANDS_RA_RB},
    {"SHR", 0x90, OPERANDS_RA_RB},
    {"SHL", 0xa0, OPERANDS_RA_RB},
    {"NOT", 0xb0, OPERANDS_RA_RB},
    {"AND", 0xc0, OPERANDS_RA_RB},
    {"OR", 0xd0, OPERANDS_RA_RB},
    {"XOR", 0xe0, OPERANDS_RA_RB},
    {"CMP", 0xf0, OPERANDS_RA_RB},

    /* Load and Store Instructions */
    {"LD", 0x00, OPERANDS_RA_RB},
    {"ST", 0x10, OPERANDS_RA_RB},

    /* Data Instruction */
    {"DATA", 0x20, OPERANDS_RB_K},

    /* Branch Instructions */
    {"JMPR", 0x30, OPERANDS_RB},
    {"JMP", 0x40, OPERANDS_K},
    {"JC", 0x58, OPERANDS_K},
    {"JA", 0x54, OPERANDS_K},
    {"JE", 0x52, OPERANDS_K},
    {"JZ", 0x51, OPERANDS_K},
    {"JCA", 0x5c, OPERANDS_K},
    {"JCE", 0x5a, OPERANDS_K},
    {"JCZ", 0x59, OPERANDS_K},
    {"JAE", 0x56, OPERANDS_K},
    {"JAZ", 0x55, OPERANDS_K},
    {"JEZ", 0x53, OPERANDS_K},
    {"JCAE", 0x5e, OPERANDS_K},
    {"JCAZ", 0x5d, OPERANDS_K},
    {"JCEZ", 0x5b, OPERANDS_K},
    {"JAEZ", 0x57, OPERANDS_K},
    {"JCAEZ", 0x5f, OPERANDS_K},

    /* Clear Flags Instruction */
    {"CLF", 0x60, OPERANDS_NONE},

    /* IO Instructions */
    {"IND", 0x70, OPERANDS_RB},
    {"INA", 0x74, OPERANDS_RB},
    {"OUTD", 0x78, OPERANDS_RB},
    {"OUTA", 0x7c, OPERANDS_RB}
};

static inline int inject_ra(char *pch, int line_no, unsigned char *pos, char *errstr) {
    /* OR opcode with value for RA */
    if (strcmp(pch, "r1") == 0) {
        *pos |= 0x04;
    } else if (strcmp(pch, "r2") == 0) {
        *pos |= 0x08;
    } else if (strcmp(pch, "r3") == 0) {
        *pos |= 0x0c;
    } else if (strcmp(pch, "r0") != 0) {
        snprintf(errstr, ERR_LEN, "%d: %s", line_no,
            "Invalid register");
        return -1;
    }

    return 0;
}

static inline int inject_rb(char *pch, int line_no, unsigned char *pos, char *errstr) {
    /* OR opcode with value for RB */
    if (strcmp(pch, "r1") == 0) {
        *pos |= 0x01;
    } else if (strcmp(pch, "r2") == 0) {
        *pos |= 0x02;
    } else if (strcmp(pch, "r3") == 0) {
        *pos |= 0x03;
    } else if (strcmp(pch, "r0") != 0) {
        snprintf(errstr, ERR_LEN, "%d: %s", line_no,
            "Invalid register");
        return -1;
    }

    return 0;
}

/* If a constant cannot be parsed i.e. it's in an invalid format or is a string
 * rather than a number, zero will be written. */

static int parse_constant(char *pch, int line_no, unsigned char *pos, char *errstr) {
    char *pch_end;
    long int k;

    pch_end = pch + strlen(pch) - 1;
    if (*pch_end == 'b') {
        /* Base 2 conversion */
        *pch_end = '\0';
        k = strtol(pch, NULL, 2);
    } else if (*pch_end == 'h') {
        /* Base 16 conversion */
        *pch_end = '\0';
        k = strtol(pch, NULL, 16);
    } else {
        /* Determine base automatically */
        k = strtol(pch, NULL, 0);
    }

    if (k < -128 || k > 255) {
        snprintf(errstr, ERR_LEN, "%d: %s", line_no,
            "Invalid operand size (constant must fit in one byte)");
        return -1;
    }

    if (k < 0) {
        /* Store negative values as char (most probably two's compliment) */
        *pos = (char)k;
    } else {
        *pos = (unsigned char)k;
    }

    return 0;
}

int check_output_length(int line_no, unsigned char *out, unsigned char *pos, char *errstr) {
    if (pos - out >= OUT_LEN) {
        snprintf(errstr, ERR_LEN, "%d: %s %u %s", line_no,
            "Resulting file too large - output file size limit:", OUT_LEN,
            "bytes");
        return -1;
    }

    return 0;
}

/* Parse a line from the source file and increment *pos accordingly. If the
 * line contains a pseudo instruction such as an org directive or a label, pos
 * is not incremented. If the line contains an instruction that gets translated
 * to one opcode then *pos is incremented by 1, two opcodes and *pos is
 * incremented by 2. The function returns 0 on success, -1 on error with
 * errstr set appropriately. */

static int parse_line(char *line, int line_no, unsigned char *out, unsigned char **pos, char *errstr) {
    char *pch, *pch_up;
    unsigned char padding;
    const instruction *inst;
    size_t n;

    /* Remove comments from the line if present */
    pch = strstr(line, ";");
    if (pch) {
        *pch = '\0';
    }

    /* Trim any delimiter characters from the left and extract mnemonic */
    pch = strtok(line, " \t\n");
    if (!pch) {
        /* Line contains only spaces, tabs and/or newlines. This is a valid
         * line. */
        return 0;
    }

    /* Ensure mnemonic is upper case for comparison with instructions table */
    for (pch_up = pch; *pch_up != '\0'; ++pch_up) {
        *pch_up = toupper(*pch_up);
    }

    /* Parse PAD compiler directive */
    if (strcmp(pch, "PAD") == 0) {
        /* Extract K */
        pch = strtok(NULL, " \t\n");
        if (!pch) {
            snprintf(errstr, ERR_LEN, "%d: %s", line_no,
                "Invalid combination of operands for directive");
            return -1;
        }

        if (parse_constant(pch, line_no, &padding, errstr) == -1) {
            return -1;
        }

        /* Check that padding is in out file bounds to avoid overflow. This
         * also protects the memset below. */
        if (check_output_length(line_no, out, *pos + padding, errstr) == -1) {
            return -1;
        }
        memset(*pos, 0, padding);
        *pos += padding;

        return 0;
    }

    /* At this point we are ready to parse an instruction that will result
     * in machine code output so we need to check that we have not exceeded
     * OUT_LEN as the machine only has OUT_LEN bytes of RAM */
    if (check_output_length(line_no, out, *pos, errstr) == -1) {
        return -1;
    }

    /* Lookup mnemonic in instructions table */
    inst = NULL;
    for (n = 0; n < sizeof instructions / sizeof instructions[0]; ++n) {
        if (strcmp(instructions[n].mnemonic, pch) == 0) {
            inst = &instructions[n];
            break;
        }
    }

    /* Test if a match was found */
    if (inst == NULL) {
        snprintf(errstr, ERR_LEN, "%d: %s", line_no,
            "Invalid instruction mnemonic");
        return -1;
    }

    // TODO: check pos - out <= OUT_LEN

    /* Set **pos to the instruction opcode */
    **pos = inst->opcode;

    switch (inst->nops) {
        case OPERANDS_RA_RB:
            /* Extract RA */
            pch = strtok(NULL, " ,\t\n");
            if (!pch) {
                snprintf(errstr, ERR_LEN, "%d: %s", line_no,
                    "Invalid combination of operands");
                return -1;
            }

            /* Inject value for RA into the opcode */
            if (inject_ra(pch, line_no, *pos, errstr) == -1) {
                return -1;
            }

            /* Extract RB */
            pch = strtok(NULL, " ,\t\n");
            if (!pch) {
                snprintf(errstr, ERR_LEN, "%d: %s", line_no,
                    "Invalid combination of operands");
                return -1;
            }

            /* Inject value for RB into the opcode */
            if (inject_rb(pch, line_no, *pos, errstr) == -1) {
                return -1;
            }

            break;
        case OPERANDS_RB:
            /* Extract RB */
            pch = strtok(NULL, " \t\n");
            if (pch == NULL) {
                snprintf(errstr, ERR_LEN, "%d: %s", line_no,
                    "Invalid combination of operands");
                return -1;
            }

            if (inject_rb(pch, line_no, *pos, errstr) == -1) {
                return -1;
            }

            break;
        case OPERANDS_RB_K:
            /* Extract RB */
            pch = strtok(NULL, " ,\t\n");
            if (!pch) {
                snprintf(errstr, ERR_LEN, "%d: %s", line_no,
                    "Invalid combination of operands");
                return -1;
            }

            /* Inject value for RB into the opcode */
            if (inject_rb(pch, line_no, *pos, errstr) == -1) {
                return -1;
            }

            /* Increment *pos ready to store the following constant, K. Check
             * that this increment does not cause output overflow */
            (*pos)++;
            if (check_output_length(line_no, out, *pos, errstr) == -1) {
                return -1;
            }

            /* Extract K */
            pch = strtok(NULL, " ,\t\n");
            if (!pch) {
                snprintf(errstr, ERR_LEN, "%d: %s", line_no,
                    "Invalid combination of operands");
                return -1;
            }

            if (parse_constant(pch, line_no, *pos, errstr) == -1) {
                return -1;
            }

            break;
        case OPERANDS_K:
            /* Increment *pos ready to store the following constant, K. Check
             * that this increment does not cause output overflow */
            (*pos)++;
            if (check_output_length(line_no, out, *pos, errstr) == -1) {
                return -1;
            }

            /* Extract K */
            pch = strtok(NULL, " \t\n");
            if (!pch) {
                snprintf(errstr, ERR_LEN, "%d: %s", line_no,
                    "Invalid combination of operands");
                return -1;
            }

            if (parse_constant(pch, line_no, *pos, errstr) == -1) {
                return -1;
            }

            break;
        case OPERANDS_NONE:
            break;
    }
    (*pos)++;

    return 0;
}

static ssize_t parse_source_file(FILE *file, unsigned char *out, char *errstr) {
    char *line;
    size_t linecap;
    ssize_t linelen;
    unsigned char *pos;
    int line_no;
    int res;

    line = NULL;
    pos = out;
    line_no = 1;
    while ((linelen = getline(&line, &linecap, file)) > 0) {
        if ((res = parse_line(line, line_no, out, &pos, errstr)) == -1) {
            break;
        }
        ++line_no;
    }
    free(line);

    if (res == -1) {
        return -1;
    }

    return pos - out;
}

char *get_output_file_name(const char *input_file_name) {
    char *file_name, *file_name_trunc;
    file_name = malloc(strlen(input_file_name) + 4 + 1); // extra space for ".bin" if needed
    strcpy(file_name, input_file_name);
    file_name_trunc = strtok(file_name, ".");
    strcat(file_name, ".bin");
    return file_name;
}

int main(int argc, const char **argv) {
    FILE *file;
    unsigned char out[OUT_LEN];
    ssize_t outlen;
    char errstr[ERR_LEN];
    char *file_name;

    if (argc < 2) {
        fprintf(stderr, "Please pass a file to the assembler\n");
        return EXIT_FAILURE;
    } else if (argc > 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "-h") == 0) {
        printf("Usage: %s <file>\n", argv[0]);
        return EXIT_SUCCESS;
    }

    file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Could not open \"%s\": %s\n",
            argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    /* read input source file */
    outlen = parse_source_file(file, out, errstr);
    fclose(file);

    file_name = get_output_file_name(argv[1]);

    if (outlen == -1) {
        /* handle error */
        fprintf(stderr, "Error: %s:%s\n", argv[1], errstr);
        /* Remove output file if exists so it isn't accidently ran after an
         * error occured */
        remove(file_name);
        free(file_name);
        return EXIT_FAILURE;
    } else if (outlen == 0) {
        printf("Warning: no output generated from input file\n");
        return EXIT_SUCCESS;
    }

    file = fopen(file_name, "wb");
    if (!file) {
        fprintf(stderr, "Could not create \"%s\": %s\n", file_name,
            strerror(errno));
        return EXIT_FAILURE;
    }
    free(file_name);

    /* write contents of output to file */
    if (fwrite(out, 1, outlen, file) != (size_t)outlen) {
        fclose(file);
        fprintf(stderr, "Error writing to output file\n");
        return EXIT_FAILURE;
    }

    fclose(file);

    return EXIT_SUCCESS;
}
