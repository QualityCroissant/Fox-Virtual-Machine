/* Fox Virtual Machine: Assembler's Lexer Component
 * Copyright (C) 2025 Finn Chipp
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "lexer.h"

struct fvma_lexer lexer = {
    .comment = false,
    .whitespace = false,
    .rawText = false,
    .label = false,
    .maxAddress = 0,
    .textBuffSize = ALLOC_SIZE,
    .textBuffLength = 0,
    .rawTextLength = 0,
    .operands = 0
};

int lexer_run(void) {
	for(size_t i = 0; i < shared.sourceLength; i++) { // Go through the source code char-by-char
		if(lexer.source[i] == '\n') // Increment the line-count if it's a newline
			shared.line++;

		if(!lexer.rawText) { // If it's not within a literal
			if(lexer.source[i] == ';') // If it's a comment
				lexer.comment = true;
			else if(lexer.source[i] == '\n') // If it's the end of a comment
				lexer.comment = false;
		}

		// We're in whitespace if: not in a literal, and this character and the next one are one of ';', '\n', ' ', or '\t'

		lexer.whitespace = !lexer.rawText && i + 1 < shared.sourceLength && (lexer.source[i] == ';' || lexer.source[i] == '\n' || lexer.source[i] == ' ' || lexer.source[i] == '\t') && (lexer.source[i + 1] == ';' || lexer.source[i + 1] == '\n' || lexer.source[i + 1] == ' ' || lexer.source[i + 1] == '\t');

		if(lexer.source[i] == '[') { // If it's the start of a literal
			lexer.rawText = true;
			continue; // Skip to next character
		} else if(lexer.source[i] == ']' && i > 1 && lexer.source[i - 1] != '\\') { // If it's the end of a literal
			lexer.rawText = false;
		}

		if(lexer.comment || lexer.whitespace) // If we're in a comment or whitespace
			continue; // Skip to the next character

		if(!lexer.rawText) { // If we're not collecting characters for a literal
			switch(lexer.source[i]) { // Turn any whitespace into a newline (every bit of whitespace gets turned into a single character of whitespace beforehand)
				case '\n':
				case ' ':
				case '\t':
					lexer.source[i] = '\n';
					break;
				case ':': // Enable label-mode if the current token's a label
				case '=':
					lexer.label = true;
			}

			if(lexer.source[i] == '\n' && lexer.textBuffLength) { // If we've reached the end of a token that's not blank
				lexer.textBuff[lexer.textBuffLength] = '\0'; // Null-terminate the token's text!

				// Allocate more memory for sourceInstructions if required for the upcoming accomodation of the new token

				if(++shared.sourceInstructionsLength > shared.sourceInstructionsSize) {
					shared.sourceInstructionsSize += ALLOC_SIZE;

					if((shared.allocBuff = (void *)realloc(shared.sourceInstructions, shared.sourceInstructionsSize * sizeof(struct token))) == NULL) { // If doing so fails, fail
						perror("fvma -> Could not allocate more memory to intermidiary representation");

						free(lexer.textBuff);

						for(size_t j = 0; j < shared.sourceInstructionsLength; j++)
							free(shared.sourceInstructions[j].text);
						
						free(shared.sourceInstructions);
						free(lexer.source);
						free(shared.labelTable);
						free(shared.output);
						fclose(shared.f);

						return 3;
					}

					shared.sourceInstructions = (struct token *)shared.allocBuff;
				}

				shared.sourceInstructions[shared.sourceInstructionsLength - 1] = (struct token){ // Initialise the attributes of the token with what we know at this point
					.text = strdup(lexer.textBuff),
					.text_length = lexer.textBuffLength,
					.text_size = lexer.textBuffLength + 1,
					.address = lexer.maxAddress,
					.line = shared.line
				};

				if(shared.sourceInstructions[shared.sourceInstructionsLength - 1].text == NULL) { // If memory couldn't be allocated for holding the text of the token in the sourceInstructions
					perror("fvma -> Couldn't allocate memory for keyword in token list"); // Report and fail:

					free(lexer.textBuff);

					for(size_t j = 0; j < shared.sourceInstructionsLength - 1; j++)
						free(shared.sourceInstructions[j].text);

					free(shared.sourceInstructions);
					free(lexer.source);
					free(shared.labelTable);
					free(shared.output);

					fclose(shared.f);

					return 3;
				}

				// Then! work out what on earth it is:

				if(lexer.operands) // If the number of operands remaining to collect for the last instruction-token is non-zero
					lexer.operands--; // Decrement the number of tokens remaining that fulfill this role

				if(lexer.textBuffLength > 2 && lexer.textBuff[lexer.textBuffLength - 2] == ']') { // If it's a literal of some kind (yes, that ']' *was* left in on-purpose!)
					switch(lexer.textBuff[lexer.textBuffLength - 1]) { // Assign its type based on the specifier at the end of the token
						case 's':
							shared.sourceInstructions[shared.sourceInstructionsLength - 1].type = STRING;
							break;
						case 'b':
							shared.sourceInstructions[shared.sourceInstructionsLength - 1].type = BINARY;
							break;
						case 'x':
							shared.sourceInstructions[shared.sourceInstructionsLength - 1].type = HEXADECIMAL;
							break;
						case 'o':
							shared.sourceInstructions[shared.sourceInstructionsLength - 1].type = OCTAL;
							break;
						case 'd':
							shared.sourceInstructions[shared.sourceInstructionsLength - 1].type = DECIMAL;
							break;
						default: // Or if there's a letter there that isn't a specifier, report it!
							fprintf(stderr, "fvma -> Line %zu: Unrecognised raw-data type specifier '%c'\n", shared.line, lexer.textBuff[lexer.textBuffLength - 1]);
							errors = true;
					}
				} else if(lexer.textBuff[lexer.textBuffLength - 1] == ':' || lexer.textBuff[lexer.textBuffLength - 1] == '=') { // Or is it a label definition of some kind?
					shared.sourceInstructions[shared.sourceInstructionsLength - 1].type = LABEL_DEFINITION;
				} else if(!lexer.operands) { // Or is it something else, that is possibly an instruction?
					shared.sourceInstructions[shared.sourceInstructionsLength - 1].type = INSTRUCTION; // Assume that the token's an instruction

					lexer.operands = MAX_NO_OPERANDS + 1;

					for(size_t j = 0; j < NO_INSTRUCTIONS; j++) // Loop through the list of instructions to see if the token matches one
						if(!strcmp(INSTRUCTIONS[j].text, lexer.textBuff)) {
							lexer.operands = INSTRUCTIONS[j].no_operands;
							break;
						}

					if(lexer.operands == MAX_NO_OPERANDS + 1) { // If it doesn't, it's a label instead
						shared.sourceInstructions[shared.sourceInstructionsLength - 1].type = LABEL;
						lexer.operands = 0; // Which means that it doesn't have any operands, either!
					}
				} else { // Otherwise, it's certainly a label
					shared.sourceInstructions[shared.sourceInstructionsLength - 1].type = LABEL;
				}

				// Now, set the parameters for the next token:

				lexer.textBuffLength = 0;

				if(lexer.label) { // Labels shouldn't change the address of the next token
					lexer.label = false;
				} else {
                   	if(shared.sourceInstructions[shared.sourceInstructionsLength - 1].type == STRING) // Strings take up 1 address per character, so the address after a string should be advanced by the amount of characters in the string
                    	lexer.maxAddress += lexer.rawTextLength;
                   	else // Otherwise, it's a single number, so the following token only has to be 1 address along
            				    	lexer.maxAddress++;

					lexer.rawTextLength = 0; // Reset this for the next literal to come around!
				}
			}
		} else { // Otherwise, continue to count the amount of characters in the current literal
			lexer.rawTextLength++;
		}

		if(lexer.source[i] != '\n') { // If the current character was not the end of a token, add it to the token-text buffer (textBuff) to be written to sourceInstructions
			if(++lexer.textBuffLength + 1 > lexer.textBuffSize) { // If textBuff isn't big enough to hold the number of characters in this token, allocate it more memory
				lexer.textBuffSize += ALLOC_SIZE;

				if((shared.allocBuff = (void *)realloc(lexer.textBuff, lexer.textBuffSize * sizeof(char))) == NULL) { // If it fails, fail
					perror("fvma -> Could not allocate more memory to token buffer");

					free(lexer.textBuff);


					for(size_t j = 0; j < shared.sourceInstructionsLength; j++)
						free(shared.sourceInstructions[j].text);
					
					free(shared.sourceInstructions);
					free(lexer.source);
					free(shared.labelTable);
					free(shared.output);

					fclose(shared.f);

					return 3;
				}

				lexer.textBuff = (char *)shared.allocBuff;
			}

			lexer.textBuff[lexer.textBuffLength - 1] = lexer.source[i]; // Add the character of the token to textBuff
		}
	}

    free(lexer.textBuff);
    free(lexer.source);

	return 0;
}
