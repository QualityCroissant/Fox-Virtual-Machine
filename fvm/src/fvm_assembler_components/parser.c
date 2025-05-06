/* Fox Virtual Machine: Assembler's Parser Component
 * Copyright (C) 2025 Finn Chipp
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "parser.h"

struct fvma_parser parser = {
    .nextValue = 0,
    .labelTableSize = ALLOC_SIZE,
    .labelTableLength = NO_DEFAULT_LABELS
};

int parser_run(void) {
    // Process label definitions and add their text and value to the Label Table:

    for(size_t i = 0; i < shared.sourceInstructionsLength; i++) { // Loop through each token
    	if(shared.sourceInstructions[i].type == LABEL_DEFINITION) { // If it's a label definition:
    		for(size_t j = 0; j < shared.sourceInstructions[i].text_length - 1; j++) { // Find out if it's a legal character for an identifier or not
 				parser.characterWasLegal = false;

    			for(size_t k = 0; k < NO_LEGAL_LABEL_CHARACTER_RANGES; k++) {
    				if(LEGAL_LABEL_CHARACTER_RANGES[k][0] <= shared.sourceInstructions[i].text[j] && shared.sourceInstructions[i].text[j] <= LEGAL_LABEL_CHARACTER_RANGES[k][1]) {
    					parser.characterWasLegal = true;
    					break;
    				}
    			}

    			if(!parser.characterWasLegal) { // If it wasn't, report it
    				fprintf(stderr,
    						"fvma -> Line %zu: In label declaration for '%s', found illegal character '%c'\n",
    						shared.sourceInstructions[i].line,
    						shared.sourceInstructions[i].text,
    						shared.sourceInstructions[i].text[j]);

    				errors = true;
    			}
    		}

    		// Regardless, add it to the Label Table:

    		if(++parser.labelTableLength > parser.labelTableSize) { // If the Label Table needs more memory allocated to it
    			parser.labelTableSize += ALLOC_SIZE;

    			shared.allocBuff = (void *)realloc(shared.labelTable, parser.labelTableSize * sizeof(struct label)); // Attempt to do so
         
    			if(shared.allocBuff == NULL) { // If doing so fails, fail
    				perror("fvma -> Could not allocate more memory to Label Table");

    				for(size_t j = 0; j < shared.sourceInstructionsLength; j++)
    					free(shared.sourceInstructions[j].text);

    				free(shared.sourceInstructions);
    				free(shared.labelTable);
    				free(shared.output);

    				fclose(shared.f);

    				return 3;
    			}

    			shared.labelTable = (struct label *)shared.allocBuff;
    		}

    		shared.labelTable[parser.labelTableLength - 1].text = shared.sourceInstructions[i].text; // Add the label's text to the Table

    		switch(shared.sourceInstructions[i].text[shared.sourceInstructions[i].text_length - 1]) { // Find out what type of label definition it is in order to assign its value:
    			case ':': // If it represents an address
    				shared.labelTable[parser.labelTableLength - 1].meaning = (uint64_t)shared.sourceInstructions[i].address;
    				break;
    			case '=': // If it represents a numeric value
    				if(i + 1 < shared.sourceInstructionsLength) {
    					if(shared.sourceInstructions[i + 1].type == STRING) {
    						fprintf(stderr,
    								"fvma -> Line %zu: You can't assign a label to a string: labels can only represent addresses or single values\n",
    								shared.sourceInstructions[i].line);

    						errors = true;
    					} else {
    						shared.labelTable[parser.labelTableLength - 1].meaning = convert(&shared.sourceInstructions[i + 1]);
    					}
    				} else {
    					fprintf(stderr,
    							"fvma -> Line %zu: Expected token after variable declaration using '=', but got nothing\n",
    							shared.sourceInstructions[i].line);

    					errors = true;
    				}
    		}

    		shared.sourceInstructions[i].text[--shared.sourceInstructions[i].text_length] = '\0'; // Remove the : or = from the end of the name, so that calls to the label don't have to contain it
    	}
    }

    // For all non label-definitions:

    for(size_t i = 0; i < shared.sourceInstructionsLength; i++) { // Go through the instructions one-by-one again
    	switch(shared.sourceInstructions[i].type) {
    		case INSTRUCTION: // If it's an instruction:
    			for(uint64_t j = 0; j < NO_INSTRUCTIONS; j++) // Find which instruction it is
    				if(!strcmp(shared.sourceInstructions[i].text, INSTRUCTIONS[j].text)) {
    					parser.nextValue = j; // Send the numerical value it's a mnemonic for to the output buffer
    					break;
    				}

    			break;

    		case LABEL: // If it's a label:
    			parser.labelWasFound = false; // Assume that it's not in the Label Table
    		
    			for(size_t j = 0; j < parser.labelTableLength; j++) { // Try to find it in the Label Table
    				if(!strcmp(shared.labelTable[j].text, shared.sourceInstructions[i].text)) { // If it's there
    					parser.nextValue = shared.labelTable[j].meaning; // Grab the value it represents from the Label Table, and send that to the output buffer
    					parser.labelWasFound = true;
    					break;
    				}
    			}

    			if(!parser.labelWasFound) { // If it wasn't in the Label Table, then it wasn't defined
    				fprintf(stderr,
    						"fvma -> Line %zu: What is '%s'? Unrecognised label\n",
    						shared.sourceInstructions[i].line,
    						shared.sourceInstructions[i].text);

    				errors = true;

    				continue;
    			}

    			break;

    		case STRING: // If it's a string
    			shared.sourceInstructions[i].text[shared.sourceInstructions[i].text_length -= 2] = '\0'; // Get rid of the "]s" at the end

    			parser.escape = false;

    			for(size_t j = 0; j < shared.sourceInstructions[i].text_length; j++) { // Go through each character of the string
    				if(shared.sourceInstructions[i].text[j] == '\\') { // If it's a backslash ignore it
    					parser.escape = true;
    					continue;
    				}

    				if(parser.escape) { // If the last character was a backslash
    					switch(shared.sourceInstructions[i].text[j]) {
    						case '/': // If the current character's a forwardslash
    							shared.sourceInstructions[i].text[j] = '\\'; // Send a backslash to the output buffer instead
    							break;
    						case 'n': // If the current character's an 'n'
    							shared.sourceInstructions[i].text[j] = '\n'; // Send a newline to the output buffer instead
    							break;
    						case 'b': // Ditto
    							shared.sourceInstructions[i].text[j] = '\b';
    							break;
    						case 'r': // Ditto
    							shared.sourceInstructions[i].text[j] = '\r';
    					}

    					parser.escape = false; // The escape sequence is complete
    				}

    				if(++shared.outputLength > shared.outputSize) { // If the output buffer needs more space allocating to accomodate the next character of the string
    					shared.outputSize += ALLOC_SIZE;

    					shared.allocBuff = (void *)realloc(shared.output, shared.outputSize * sizeof(uint64_t)); // Attempt to allocate it more space

    					if(shared.allocBuff == NULL) { // If doing so fails, fail
    						perror("fvma -> Could not allocate more memory to output buffer");

    						for(size_t k = 0; k < shared.sourceInstructionsLength; k++)
    							free(shared.sourceInstructions[k].text);

    						free(shared.sourceInstructions);
    						free(shared.labelTable);
    						free(shared.output);

    						fclose(shared.f);

    						return 3;
    					}

    					shared.output = (uint64_t *)shared.allocBuff;
    				}

    				shared.output[shared.outputLength - 1] = shared.sourceInstructions[i].text[j]; // Send each character of the string to the output buffer
    			}
    			
    			continue;
    		default: // If it's something else
    			if(shared.sourceInstructions[i].type == LABEL_DEFINITION) // That's not a label definition
    				continue;

    			parser.nextValue = convert(&shared.sourceInstructions[i]); // Then it must be a numerical value, represented by the raw text of a literal!
    	}

    	// If nextValue has been set:

    	if(++shared.outputLength > shared.outputSize) { // Allocate extra space to the output buffer if necessary to accomodate it
    		shared.outputSize += ALLOC_SIZE;

    		shared.allocBuff = (void *)realloc(shared.output, shared.outputSize * sizeof(uint64_t));

    		if(shared.allocBuff == NULL) { // If attempting to do so fails, fail
    			perror("fvma -> Could not allocate more memory to output buffer");

    			for(size_t j = 0; j < shared.sourceInstructionsLength; j++)
    				free(shared.sourceInstructions[j].text);

    			free(shared.sourceInstructions);
    			free(shared.labelTable);
    			free(shared.output);

    			fclose(shared.f);

    			return 3;
    		}

    		shared.output = (uint64_t *)shared.allocBuff;
    	}

    	shared.output[shared.outputLength - 1] = parser.nextValue; // Push nextValue onto the output buffer
    }

    return 0;
}
