/* Fox Virtual Machine: Assembler's Shared Components
 * Copyright (C) 2025 Finn Chipp
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "lexer.h"
#include "parser.h"

const char LEGAL_LABEL_CHARACTER_RANGES[NO_LEGAL_LABEL_CHARACTER_RANGES][2] = {
	{'0', '9'},
	{'A', 'Z'},
	{'a', 'z'},
	{'_', '_'}
};

const struct instruction INSTRUCTIONS[NO_INSTRUCTIONS] = {
	[0] = {"pl", 2},
	[1] = {"mv", 2},

	[2] = {"st", 0},
	[3] = {"ld", 0},

	[4] = {"jm", 1},
	[5] = {"js", 1},
	[6] = {"jc", 1},

	[7] = {"a+", 0},
	[8] = {"a-", 0},
	[9] = {"a!", 0},
	[10] = {"ai", 0},
	[11] = {"ad", 0},
	[12] = {"a*", 0},
	[13] = {"a/", 0},

	[14] = {"a&", 0},
	[15] = {"a|", 0},
	[16] = {"a^", 0},
	[17] = {"al", 0},
	[18] = {"ar", 0},

	[19] = {"gt", 0},
	[20] = {"lt", 0},
	[21] = {"ge", 0},
	[22] = {"le", 0},
	[23] = {"eq", 0},
	[24] = {"ne", 0},

	[25] = {"cl", 1},
	[26] = {"rt", 0},

	[27] = {"fi", 0}
};

const struct label DEFAULT_LABELS[NO_DEFAULT_LABELS] = {
	{"cst", 3},
	{"mem", 0},
	{"inp", 1},
	{"out", 2},

	{"mch", 0},
	{"mar", 1},
	{"mdr", 2},
	{"acc", 3},
	{"dat", 4},
	{"cea", 5},
	{"csp", 6},
    {"gp0", 7},
    {"gp1", 8},
    {"gp2", 9},
    {"gp3", 10},
    {"gp4", 11},
    {"gp5", 12},
    {"gp6", 13},
    {"gp7", 14},
};

const unsigned char DIGIT_CHARS[NO_DIGIT_CHARS] = {
	[0] = '0',
	[1] = '1',
	[2] = '2',
	[3] = '3',
	[4] = '4',
	[5] = '5',
	[6] = '6',
	[7] = '7',
	[8] = '8',
	[9] = '9',
	[10] = 'a',
	[11] = 'b',
	[12] = 'c',
	[13] = 'd',
	[14] = 'e',
	[15] = 'f'
};

bool errors = false;

struct fvma_shared shared = {
    .sourceLength = 0,
    .sourceInstructionsSize = ALLOC_SIZE,
    .sourceInstructionsLength = 0,
    .line = 1,
    .outputSize = ALLOC_SIZE,
    .outputLength = 0
};

uint64_t convert(struct token *raw) { // Convert the text of a literal-token into the number it represents
	bool foundDigit; // For seeing if the each digit in the literal is valid as a number
	uint64_t digit, // Value of the current digit
			 value = 0, // The overall value that the literal represents (the return value of this function)
			 multiplier = 0, // The multiplicative difference from one digit to the next (equal to the base of the number)
			 digitMultiple = 1; // The multiple of the digit that is represented by its position in the overall number

	switch(raw->text[raw->text_length - 1]) { // Find the multiplier (base of the number, denoted by a specifier on the end of the literal)
		case 'b': // binary
			multiplier = 2;
			break;
		case 'x': // hexadecimal
			multiplier = 16;
			break;
		case 'o': // octal
			multiplier = 8;
			break;
		case 'd': // decimal
			multiplier = 10;
	}

	for(size_t i = raw->text_length - 2; i > 0; i--) { // Loop through the number from the end to the start, digit-by-digit
		if(raw->text[i - 1] == '\'') // If it's a single-quote (')
			continue; // Skip to the next digit
	
		foundDigit = false; // Assume that the digit is not valid

		for(uint64_t j = 0; j < NO_DIGIT_CHARS; j++) { // Try to find it in the list of valid digits
			if(DIGIT_CHARS[j] == raw->text[i - 1] || ((DIGIT_CHARS[j] & 0b01000000) && DIGIT_CHARS[j] == (raw->text[i - 1] | 0b00100000))) { // If it is there
				digit = j; // Set digit = the value it represents
				foundDigit = true; // The digit is valid

				break;
			}

		}

		if(!foundDigit) { // If the digit was not found in the list of valid digits
			fprintf(stderr, // Report it
					"fvma -> Line %zu: Invalid character in literal; chars must be 0-9, A-Z, or a single-quote (') as separator, but got '%c'\n",
					raw->line,
					raw->text[i - 1]);

			errors = true;

			return 0; // Do not calculate any further digits of the number
		}

		value += digit * digitMultiple, // Add to the resulting number the value of the digit, multiplied by whatever it should be at that position in the overall number
		digitMultiple *= multiplier; // Multiply the digit-multiplier-difference by the base of the number to get what the next digit should be multiplied by, for its position in the overall number
	}

	return value; // Return the numerical value of the literal
}

int init_run(void) {
	if(shared.argc > 3 || shared.argc < 2) { // Fail if incorrect No. arguments provided
		fprintf(stderr, "fvma -> Incorrect number of arguments passed to fvma\n");
		return 1;
	}

	shared.f = fopen(shared.argv[1], "r"); // Attempt to open source file for reading

	if(shared.f == NULL) { // Fail if it couldn't be opened
		perror("fvma -> Could not open specified file");
		return 2;
	}

	while(fgetc(shared.f) != EOF) shared.sourceLength++; // set sourceLength = No. chars in file. TODO: Replace with solution using fseek()

	rewind(shared.f); // Go back to the start of the file

	shared.allocBuff = (void *)calloc(shared.sourceLength + 1, sizeof(char)); // Attempt to allocate memory for source

	if(shared.allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for file-read");

		fclose(shared.f);
		return 3;
	}

	lexer.source = (char *)shared.allocBuff;

	fread(lexer.source, sizeof(char), shared.sourceLength, shared.f); // Set source = contents of source file
	lexer.source[shared.sourceLength] = '\0'; // Done't forget to null-terminate!

	shared.allocBuff = (void *)calloc(shared.sourceInstructionsSize, sizeof(struct token)); // Attempt to allocate sharedial memory for sourceInstructions

	if(shared.allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for intermidiary representation");

		free(lexer.source);
		fclose(shared.f);
		return 3;
	}

	shared.sourceInstructions = (struct token *)shared.allocBuff;

	shared.allocBuff = (void *)calloc(lexer.textBuffSize, sizeof(char)); // Attempt to allocate sharedial memory for textBuff

	if(shared.allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for token buffer");

		free(shared.sourceInstructions);
		free(lexer.source);
		fclose(shared.f);
		return 3;
	}

	lexer.textBuff = (char *)shared.allocBuff;

	shared.allocBuff = (void *)calloc(parser.labelTableSize, sizeof(struct label)); // Attempt to allocate sharedial memory for labelTable

	if(shared.allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for Label Table");

		free(shared.sourceInstructions);
		free(lexer.source);
		fclose(shared.f);

		return 3;
	}

	shared.labelTable = (struct label *)shared.allocBuff;

	for(size_t i = 0; i < NO_DEFAULT_LABELS; i++) // Insert all of the default labels into the Label Table
		shared.labelTable[i] = DEFAULT_LABELS[i];

	shared.allocBuff = (void *)calloc(shared.outputSize, sizeof(uint64_t)); // Attempt to allocate memory for the output buffer

	if(shared.allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for output buffer");

		free(shared.sourceInstructions);
		free(lexer.source);
		free(lexer.textBuff);

		fclose(shared.f);

		return 3;
	}

	shared.output = (uint64_t *)shared.allocBuff;

    return 0;
}

int end_run(void) {
    // Write output to file:

    fclose(shared.f);

    if(shared.argc == 3) { // If the user specified the output filename
    	if((shared.lengthBuff = strlen(shared.argv[2])) < 3 || strcmp(shared.argv[2] + shared.lengthBuff - 3, ".fb")) {
    		fprintf(stderr, "fvma -> Output filename does not end with '.fb'\n");
    		errors = true;
    	} else {
    		shared.outputFilename = shared.argv[2];
    	}
    } else { // Otherwise, use the default filename
    	shared.outputFilename = DEFAULT_OUTPUT_FILENAME;
    }

    if(errors) { // If there were errors, report it
    	fprintf(stderr, "fvma -> Something smells fishy, so output file was not overwritten with generated binary\n");
    } else { // Otherwise, write the output buffer to the output file
    	shared.f = fopen(shared.outputFilename, "wb");
    	fwrite(shared.output, sizeof(uint64_t), shared.outputLength, shared.f);
    }

    // Cleanup:

    for(size_t i = 0; i < shared.sourceInstructionsLength; i++)
    	free(shared.sourceInstructions[i].text);

    free(shared.sourceInstructions);
    free(shared.labelTable);
    free(shared.output);

    fclose(shared.f);

    return 0;
}
