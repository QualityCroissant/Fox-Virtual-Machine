#ifndef FVMA_SHARED_H

#define FVMA_SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define ALLOC_SIZE 50 // No. bytes to allocate and reallocate memory by
#define NO_INSTRUCTIONS 28 // No. instructions
#define MAX_NO_OPERANDS 2 // Maximum operands an instruction can have
#define NO_LEGAL_LABEL_CHARACTER_RANGES 4 // No. ranges that exist for what a legal character in a label can exist within
#define NO_DEFAULT_LABELS 19 // Number of default labels to go in the Label Table
#define NO_DIGIT_CHARS 16 // Nummber of characters that can represent a digit (0-9, A-Z)

#define DEFAULT_OUTPUT_FILENAME "a.fb"

extern const char LEGAL_LABEL_CHARACTER_RANGES[NO_LEGAL_LABEL_CHARACTER_RANGES][2]; // A legal character in an identifier/label is one that lies within one of these ranges


extern const struct instruction {
    char text[3];
    unsigned char no_operands;
} INSTRUCTIONS[NO_INSTRUCTIONS];


extern const struct label {
	char *text;
	uint64_t meaning;
} DEFAULT_LABELS[NO_DEFAULT_LABELS];

struct token {
	enum token_type {
		INSTRUCTION,
		LABEL_DEFINITION,
		LABEL,
		STRING,
		BINARY,
		HEXADECIMAL,
		OCTAL,
		DECIMAL
	} type;

	size_t text_size, // No. bytes that the text is allocated
		   text_length, // No chars in the text (discluding \0)
		   address, // Address at which the token's placement will start within the rom
		   line;

	char *text;
};

extern const unsigned char DIGIT_CHARS[NO_DIGIT_CHARS];

extern bool errors; // Whether or not errors that should prevent output being generated have occurred

extern uint64_t convert(struct token *);

extern struct fvma_shared { // These are intended as variables, so will be written in camelCase:
    void *allocBuff; // Buffer for memory (re)allocation, so that memory may be freed if an operation on it fails
    size_t sourceLength, // Length of source in chars (discluding \0)
           sourceInstructionsSize, // Initial number of instructions to allocate space for, for sourceInstructions
           sourceInstructionsLength, // Number of sourceInstructions
           line, // Line count, for error reports
           outputSize, // Number of uint64_ts allocated to the output buffer
           outputLength, // Number of uint64_ts in the output buffer
           lengthBuff; // Buffer to detect if a third argument passed to the script is greater than 2 chars in length
    FILE *f; // General-purpose file pointer; only one file is ever opened at once
    struct token *sourceInstructions; // List of tokens passed from the lexer to the parser
    struct label *labelTable; // Label Tabel, the table of labels :3
    uint64_t *output; // Bytes to be written to rom
    int argc;
    char **argv,
         *outputFilename; // name of output file;
} shared;

extern int init_run(void);
extern int end_run(void);

#endif
