/* Fox Virtual Machine: Assembler
 * Copyright (C) 2024-2025 Finn Chipp
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define ALLOC_SIZE 50 // No. bytes to allocate and reallocate memory by
#define NO_INSTRUCTIONS 28 // No. instructions
#define MAX_NO_OPERANDS 2 // Maximum operands an instruction can have
#define NO_LEGAL_LABEL_CHARACTER_RANGES 4 // No. ranges that exist for what a legal character in a label can exist within
#define NO_DEFAULT_LABELS 11 // Number of default labels to go in the Label Table
#define NO_DIGIT_CHARS 16 // Nummber of characters that can represent a digit (0-9, A-Z)

#define DEFAULT_OUTPUT_FILENAME "a.fb"

const char LEGAL_LABEL_CHARACTER_RANGES[NO_LEGAL_LABEL_CHARACTER_RANGES][2] = { // A legal character in an identifier/label is one that lies within one of these ranges
	{'0', '9'},
	{'A', 'Z'},
	{'a', 'z'},
	{'_', '_'}
};

// List of valid instruction keywords, their numerical translation (their index in the array), and their number of operands:

const struct instruction {
	char text[3];
	unsigned char no_operands;
} INSTRUCTIONS[NO_INSTRUCTIONS] = {
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

// List of labels and their values to go in the Label Table by default:

const struct label {
	char *text;
	uint64_t meaning;
} DEFAULT_LABELS[NO_DEFAULT_LABELS] = {
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
	{"csp", 6}
};

// Definition of tokens that the syntax can be broken down into:

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

// List of chars the index of which being the numerical value that they represent as a digit:

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

bool errors = false; // Whether or not errors that should prevent output being generated have occurred

// Functions:

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

// Start of assembler execution:

int main(int argc, char **argv) { // Main function

	// Declarations:

	bool comment = false, // (Lexer) If the current char is within a comment - used for ignoring characters
		 whitespace = false, // (Lexer) If the current char is within whitespace - used for ignoring characters
		 rawText = false, // (Lexer) If the current char is within a literal - used for ignoring what would otherwise be tokenised
		 label = false, // (Lexer) If the current char is within a label - used for determining the start address of the next token
		 characterWasLegal, // (Parser) If the character currently being checked in the currently processing label-definition was a valid character for a label, or not
		 labelWasFound, // (Parser) If the label being called upon exists in the Label Table
		 escape; // (Parser) When processing the characters of a string literal, was an escape-sequence initiated?
	uint64_t *output, // (Parser) Bytes to be written to rom
			 nextValue = 0; // (Parser) the next value to be written to the output buffer
	void *allocBuff; // Buffer for memory (re)allocation, so that memory may be freed if an operation on it fails
	size_t sourceLength = 0, // Length of source in chars (discluding \0)
		   maxAddress = 0, // (Lexer) Address used to provide parser with the address of each token in the output
		   sourceInstructionsSize = ALLOC_SIZE, // Initial number of instructions to allocate space for, for sourceInstructions
		   sourceInstructionsLength = 0, // Number of sourceInstructions
		   textBuffSize = ALLOC_SIZE, // (Lexer) Number of chars allocated to textBuff
		   textBuffLength = 0, // (Lexer) No. characters stored in textBuff (discluding \0)
		   rawTextLength = 0, // (Lexer) No. raw chars read from recently inputted literal
		   line = 1, // Line count, for error reports
		   operands = 0, // (Lexer) Number of operands possessed by last instruction token, so that it can be known not to check for instruction tokens if given tokens are in the places of an instruction's operands
		   labelTableSize = ALLOC_SIZE, // (Parser) Number of struct labels allocated to the Label Table
		   labelTableLength = NO_DEFAULT_LABELS, // (Parser) Amount of labels stored in the Label Table
		   outputSize = ALLOC_SIZE, // Number of uint64_ts allocated to the output buffer
		   outputLength = 0, // Number of uint64_ts in the output buffer
		   lengthBuff; // Buffer to detect if a third argument passed to the script is greater than 2 chars in length
	char *source, *textBuff, *outputFilename; // (Lexer) Raw source code from input file; buffer for the text of the current token to be put into the sourceInstructions array; name of output file
	FILE *f; // General-purpose file pointer; only one file is ever opened at once
	struct token *sourceInstructions; // List of tokens passed from the lexer to the parser
	struct label *labelTable; // Label Tabel, the table of labels :3

	// Initialisations:

	if(argc > 3 || argc < 2) { // Fail if incorrect No. arguments provided
		fprintf(stderr, "fvma -> Incorrect number of arguments passed to fvma\n");
		return 1;
	}

	f = fopen(argv[1], "r"); // Attempt to open source file for reading

	if(f == NULL) { // Fail if it couldn't be opened
		perror("fvma -> Could not open specified file");
		return 2;
	}

	while(fgetc(f) != EOF) sourceLength++; // set sourceLength = No. chars in file. TODO: Replace with solution using fseek()

	rewind(f); // Go back to the start of the file

	allocBuff = (void *)calloc(sourceLength + 1, sizeof(char)); // Attempt to allocate memory for source

	if(allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for file-read");

		fclose(f);
		return 3;
	}

	source = (char *)allocBuff;

	fread(source, sizeof(char), sourceLength, f); // Set source = contents of source file
	source[sourceLength] = '\0'; // Done't forget to null-terminate!

	allocBuff = (void *)calloc(sourceInstructionsSize, sizeof(struct token)); // Attempt to allocate initial memory for sourceInstructions

	if(allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for intermidiary representation");

		free(source);
		fclose(f);
		return 3;
	}

	sourceInstructions = (struct token *)allocBuff;

	allocBuff = (void *)calloc(textBuffSize, sizeof(char)); // Attempt to allocate initial memory for textBuff

	if(allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for token buffer");

		free(sourceInstructions);
		free(source);
		fclose(f);
		return 3;
	}

	textBuff = (char *)allocBuff;

	allocBuff = (void *)calloc(labelTableSize, sizeof(struct label)); // Attempt to allocate initial memory for labelTable

	if(allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for Label Table");

		free(sourceInstructions);
		free(source);
		fclose(f);

		return 3;
	}

	labelTable = (struct label *)allocBuff;

	for(size_t i = 0; i < NO_DEFAULT_LABELS; i++) // Insert all of the default labels into the Label Table
		labelTable[i] = DEFAULT_LABELS[i];

	allocBuff = (void *)calloc(outputSize, sizeof(uint64_t)); // Attempt to allocate memory for the output buffer

	if(allocBuff == NULL) { // If doing so fails, fail
		perror("fvma -> Could not allocate memory for output buffer");

		free(sourceInstructions);
		free(source);
		free(textBuff);

		fclose(f);

		return 3;
	}

	output = (uint64_t *)allocBuff;
	
	// Begin lexing:

	for(size_t i = 0; i < sourceLength; i++) { // Go through the source code char-by-char
		if(source[i] == '\n') // Increment the line-count if it's a newline
			line++;

		if(!rawText) { // If it's not within a literal
			if(source[i] == ';') // If it's a comment
				comment = true;
			else if(source[i] == '\n') // If it's the end of a comment
				comment = false;
		}

		// We're in whitespace if: not in a literal, and this character and the next one are one of ';', '\n', ' ', or '\t'

		whitespace = !rawText && i + 1 < sourceLength && (source[i] == ';' || source[i] == '\n' || source[i] == ' ' || source[i] == '\t') && (source[i + 1] == ';' || source[i + 1] == '\n' || source[i + 1] == ' ' || source[i + 1] == '\t');

		if(source[i] == '[') { // If it's the start of a literal
			rawText = true;
			continue; // Skip to next character
		} else if(source[i] == ']' && i > 1 && source[i - 1] != '\\') { // If it's the end of a literal
			rawText = false;
		}

		if(comment || whitespace) // If we're in a comment or whitespace
			continue; // Skip to the next character

		if(!rawText) { // If we're not collecting characters for a literal
			switch(source[i]) { // Turn any whitespace into a newline (every bit of whitespace gets turned into a single character of whitespace beforehand)
				case '\n':
				case ' ':
				case '\t':
					source[i] = '\n';
					break;
				case ':': // Enable label-mode if the current token's a label
				case '=':
					label = true;
			}

			if(source[i] == '\n' && textBuffLength) { // If we've reached the end of a token that's not blank
				textBuff[textBuffLength] = '\0'; // Null-terminate the token's text!

				// Allocate more memory for sourceInstructions if required for the upcoming accomodation of the new token

				if(++sourceInstructionsLength > sourceInstructionsSize) {
					sourceInstructionsSize += ALLOC_SIZE;

					if((allocBuff = (void *)realloc(sourceInstructions, sourceInstructionsSize * sizeof(struct token))) == NULL) { // If doing so fails, fail
						perror("fvma -> Could not allocate more memory to intermidiary representation");

						free(textBuff);

						for(size_t j = 0; j < sourceInstructionsLength; j++)
							free(sourceInstructions[j].text);
						
						free(sourceInstructions);
						free(source);
						free(labelTable);
						free(output);
						fclose(f);

						return 3;
					}

					sourceInstructions = (struct token *)allocBuff;
				}

				sourceInstructions[sourceInstructionsLength - 1] = (struct token){ // Initialise the attributes of the token with what we know at this point
					.text = strdup(textBuff),
					.text_length = textBuffLength,
					.text_size = textBuffLength + 1,
					.address = maxAddress,
					.line = line
				};

				if(sourceInstructions[sourceInstructionsLength - 1].text == NULL) { // If memory couldn't be allocated for holding the text of the token in the sourceInstructions
					perror("fvma -> Couldn't allocate memory for keyword in token list"); // Report and fail:

					free(textBuff);

					for(size_t j = 0; j < sourceInstructionsLength - 1; j++)
						free(sourceInstructions[j].text);

					free(sourceInstructions);
					free(source);
					free(labelTable);
					free(output);

					fclose(f);

					return 3;
				}

				// Then! work out what on earth it is:

				if(operands) // If the number of operands remaining to collect for the last instruction-token is non-zero
					operands--; // Decrement the number of tokens remaining that fulfill this role

				if(textBuffLength > 2 && textBuff[textBuffLength - 2] == ']') { // If it's a literal of some kind (yes, that ']' *was* left in on-purpose!)
					switch(textBuff[textBuffLength - 1]) { // Assign its type based on the specifier at the end of the token
						case 's':
							sourceInstructions[sourceInstructionsLength - 1].type = STRING;
							break;
						case 'b':
							sourceInstructions[sourceInstructionsLength - 1].type = BINARY;
							break;
						case 'x':
							sourceInstructions[sourceInstructionsLength - 1].type = HEXADECIMAL;
							break;
						case 'o':
							sourceInstructions[sourceInstructionsLength - 1].type = OCTAL;
							break;
						case 'd':
							sourceInstructions[sourceInstructionsLength - 1].type = DECIMAL;
							break;
						default: // Or if there's a letter there that isn't a specifier, report it!
							fprintf(stderr, "fvma -> Line %zu: Unrecognised raw-data type specifier '%c'\n", line, textBuff[textBuffLength - 1]);
							errors = true;
					}
				} else if(textBuff[textBuffLength - 1] == ':' || textBuff[textBuffLength - 1] == '=') { // Or is it a label definition of some kind?
					sourceInstructions[sourceInstructionsLength - 1].type = LABEL_DEFINITION;
				} else if(!operands) { // Or is it something else, that is possibly an instruction?
					sourceInstructions[sourceInstructionsLength - 1].type = INSTRUCTION; // Assume that the token's an instruction

					operands = MAX_NO_OPERANDS + 1;

					for(size_t j = 0; j < NO_INSTRUCTIONS; j++) // Loop through the list of instructions to see if the token matches one
						if(!strcmp(INSTRUCTIONS[j].text, textBuff)) {
							operands = INSTRUCTIONS[j].no_operands;
							break;
						}

					if(operands == MAX_NO_OPERANDS + 1) { // If it doesn't, it's a label instead
						sourceInstructions[sourceInstructionsLength - 1].type = LABEL;
						operands = 0; // Which means that it doesn't have any operands, either!
					}
				} else { // Otherwise, it's certainly a label
					sourceInstructions[sourceInstructionsLength - 1].type = LABEL;
				}

				// Now, set the parameters for the next token:

				textBuffLength = 0;

				if(label) { // Labels shouldn't change the address of the next token
					label = false;
				} else {
			        	if(sourceInstructions[sourceInstructionsLength - 1].type == STRING) // Strings take up 1 address per character, so the address after a string should be advanced by the amount of characters in the string
				        	maxAddress += rawTextLength;
			        	else // Otherwise, it's a single number, so the following token only has to be 1 address along
    				    		maxAddress++;

					rawTextLength = 0; // Reset this for the next literal to come around!
				}
			}
		} else { // Otherwise, continue to count the amount of characters in the current literal
			rawTextLength++;
		}

		if(source[i] != '\n') { // If the current character was not the end of a token, add it to the token-text buffer (textBuff) to be written to sourceInstructions
			if(++textBuffLength + 1 > textBuffSize) { // If textBuff isn't big enough to hold the number of characters in this token, allocate it more memory
				textBuffSize += ALLOC_SIZE;

				if((allocBuff = (void *)realloc(textBuff, textBuffSize * sizeof(char))) == NULL) { // If it fails, fail
					perror("fvma -> Could not allocate more memory to token buffer");

					free(textBuff);


					for(size_t j = 0; j < sourceInstructionsLength; j++)
						free(sourceInstructions[j].text);
					
					free(sourceInstructions);
					free(source);
					free(labelTable);
					free(output);

					fclose(f);

					return 3;
				}

				textBuff = (char *)allocBuff;
			}

			textBuff[textBuffLength - 1] = source[i]; // Add the character of the token to textBuff
		}
	}

	// Begin parsing:

	// Process label definitions and add their text and value to the Label Table:

	for(size_t i = 0; i < sourceInstructionsLength; i++) { // Loop through each token
		if(sourceInstructions[i].type == LABEL_DEFINITION) { // If it's a label definition:
			for(size_t j = 0; j < sourceInstructions[i].text_length - 1; j++) { // Find out if it's a legal character for an identifier or not
 				characterWasLegal = false;

				for(size_t k = 0; k < NO_LEGAL_LABEL_CHARACTER_RANGES; k++) {
					if(LEGAL_LABEL_CHARACTER_RANGES[k][0] <= sourceInstructions[i].text[j] && sourceInstructions[i].text[j] <= LEGAL_LABEL_CHARACTER_RANGES[k][1]) {
						characterWasLegal = true;
						break;
					}
				}

				if(!characterWasLegal) { // If it wasn't, report it
					fprintf(stderr,
							"fvma -> Line %zu: In label declaration for '%s', found illegal character '%c'\n",
							sourceInstructions[i].line,
							sourceInstructions[i].text,
							sourceInstructions[i].text[j]);

					errors = true;
				}
			}

			// Regardless, add it to the Label Table:

			if(++labelTableLength > labelTableSize) { // If the Label Table needs more memory allocated to it
				labelTableSize += ALLOC_SIZE;

				allocBuff = (void *)realloc(labelTable, labelTableSize * sizeof(struct label)); // Attempt to do so
         
				if(allocBuff == NULL) { // If doing so fails, fail
					perror("fvma -> Could not allocate more memory to Label Table");

					free(textBuff);

					for(size_t j = 0; j < sourceInstructionsLength; j++)
						free(sourceInstructions[j].text);

					free(sourceInstructions);
					free(source);
					free(labelTable);
					free(output);

					fclose(f);

					return 3;
				}

				labelTable = (struct label *)allocBuff;
			}

			labelTable[labelTableLength - 1].text = sourceInstructions[i].text; // Add the label's text to the Table

			switch(sourceInstructions[i].text[sourceInstructions[i].text_length - 1]) { // Find out what type of label definition it is in order to assign its value:
				case ':': // If it represents an address
					labelTable[labelTableLength - 1].meaning = (uint64_t)sourceInstructions[i].address;
					break;
				case '=': // If it represents a numeric value
					if(i + 1 < sourceInstructionsLength) {
						if(sourceInstructions[i + 1].type == STRING) {
							fprintf(stderr,
									"fvma -> Line %zu: You can't assign a label to a string: labels can only represent addresses or single values\n",
									sourceInstructions[i].line);

							errors = true;
						} else {
							labelTable[labelTableLength - 1].meaning = convert(&sourceInstructions[i + 1]);
						}
					} else {
						fprintf(stderr,
								"fvma -> Line %zu: Expected token after variable declaration using '=', but got nothing\n",
								sourceInstructions[i].line);

						errors = true;
					}
			}

			sourceInstructions[i].text[--sourceInstructions[i].text_length] = '\0'; // Remove the : or = from the end of the name, so that calls to the label don't have to contain it
		}
	}

	// For all non label-definitions:

	for(size_t i = 0; i < sourceInstructionsLength; i++) { // Go through the instructions one-by-one again
		switch(sourceInstructions[i].type) {
			case INSTRUCTION: // If it's an instruction:
				for(uint64_t j = 0; j < NO_INSTRUCTIONS; j++) // Find which instruction it is
					if(!strcmp(sourceInstructions[i].text, INSTRUCTIONS[j].text)) {
						nextValue = j; // Send the numerical value it's a mnemonic for to the output buffer
						break;
					}

				break;

			case LABEL: // If it's a label:
				labelWasFound = false; // Assume that it's not in the Label Table
			
				for(size_t j = 0; j < labelTableLength; j++) { // Try to find it in the Label Table
					if(!strcmp(labelTable[j].text, sourceInstructions[i].text)) { // If it's there
						nextValue = labelTable[j].meaning; // Grab the value it represents from the Label Table, and send that to the output buffer
						labelWasFound = true;
						break;
					}
				}

				if(!labelWasFound) { // If it wasn't in the Label Table, then it wasn't defined
					fprintf(stderr,
							"fvma -> Line %zu: What is '%s'? Unrecognised label\n",
							sourceInstructions[i].line,
							sourceInstructions[i].text);

					errors = true;

					continue;
				}

				break;

			case STRING: // If it's a string
				sourceInstructions[i].text[sourceInstructions[i].text_length -= 2] = '\0'; // Get rid of the "]s" at the end

				escape = false;

				for(size_t j = 0; j < sourceInstructions[i].text_length; j++) { // Go through each character of the string
					if(sourceInstructions[i].text[j] == '\\') { // If it's a backslash ignore it
						escape = true;
						continue;
					}

					if(escape) { // If the last character was a backslash
						switch(sourceInstructions[i].text[j]) {
							case '/': // If the current character's a forwardslash
								sourceInstructions[i].text[j] = '\\'; // Send a backslash to the output buffer instead
								break;
							case 'n': // If the current character's an 'n'
								sourceInstructions[i].text[j] = '\n'; // Send a newline to the output buffer instead
								break;
							case 'b': // Ditto
								sourceInstructions[i].text[j] = '\b';
								break;
							case 'r': // Ditto
								sourceInstructions[i].text[j] = '\r';
						}

						escape = false; // The escape sequence is complete
					}

					if(++outputLength > outputSize) { // If the output buffer needs more space allocating to accomodate the next character of the string
						outputSize += ALLOC_SIZE;

						allocBuff = (void *)realloc(output, outputSize * sizeof(uint64_t)); // Attempt to allocate it more space

						if(allocBuff == NULL) { // If doing so fails, fail
							perror("fvma -> Could not allocate more memory to output buffer");

							free(textBuff);

							for(size_t k = 0; k < sourceInstructionsLength; k++)
								free(sourceInstructions[k].text);

							free(sourceInstructions);
							free(source);
							free(labelTable);
							free(output);

							fclose(f);

							return 3;
						}

						output = (uint64_t *)allocBuff;
					}

					output[outputLength - 1] = sourceInstructions[i].text[j]; // Send each character of the string to the output buffer
				}
				
				continue;
			default: // If it's something else
				if(sourceInstructions[i].type == LABEL_DEFINITION) // That's not a label definition
					continue;

				nextValue = convert(&sourceInstructions[i]); // Then it must be a numerical value, represented by the raw text of a literal!
		}

		// If nextValue has been set:

		if(++outputLength > outputSize) { // Allocate extra space to the output buffer if necessary to accomodate it
			outputSize += ALLOC_SIZE;

			allocBuff = (void *)realloc(output, outputSize * sizeof(uint64_t));

			if(allocBuff == NULL) { // If attempting to do so fails, fail
				perror("fvma -> Could not allocate more memory to output buffer");

				free(textBuff);

				for(size_t j = 0; j < sourceInstructionsLength; j++)
					free(sourceInstructions[j].text);

				free(sourceInstructions);
				free(source);
				free(labelTable);
				free(output);

				fclose(f);

				return 3;
			}

			output = (uint64_t *)allocBuff;
		}

		output[outputLength - 1] = nextValue; // Push nextValue onto the output buffer
	}

	// Write output to file:

	fclose(f);

	if(argc == 3) { // If the user specified the output filename
		if((lengthBuff = strlen(argv[2])) < 3 || strcmp(argv[2] + lengthBuff - 3, ".fb")) {
			fprintf(stderr, "fvma -> Output filename does not end with '.fb'\n");
			errors = true;
		} else {
			outputFilename = argv[2];
		}
	} else { // Otherwise, use the default filename
		outputFilename = DEFAULT_OUTPUT_FILENAME;
	}

	if(errors) { // If there were errors, report it
		fprintf(stderr, "fvma -> Something smells fishy, so output file was not overwritten with generated binary\n");
	} else { // Otherwise, write the output buffer to the output file
		f = fopen(outputFilename, "wb");
		fwrite(output, sizeof(uint64_t), outputLength, f);
	}

	// Cleanup:

	free(textBuff);

	for(size_t i = 0; i < sourceInstructionsLength; i++)
		free(sourceInstructions[i].text);

	free(sourceInstructions);
	free(source);
	free(labelTable);
	free(output);

	fclose(f);

	return 0; // Done!
}
