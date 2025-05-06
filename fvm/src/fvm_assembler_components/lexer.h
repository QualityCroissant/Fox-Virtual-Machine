#ifndef FVMA_LEXER_H

#define FVMA_LEXER_H

#include "shared.h"

extern struct fvma_lexer { // These are intended as variables, so will be written in camelCase:
    bool comment, // If the current char is within a comment - used for ignoring characters
         whitespace, // If the current char is within whitespace - used for ignoring characters
         rawText, // If the current char is within a literal - used for ignoring what would otherwise be tokenised
         label; // If the current char is within a label - used for determining the start address of the next token
    size_t maxAddress, // Address used to provide parser with the address of each token in the output
           textBuffSize, // Number of chars allocated to textBuff
           textBuffLength, // No. characters stored in textBuff (discluding \0)
           rawTextLength, // No. raw chars read from recently inputted literal
           operands; // Number of operands possessed by last instruction token, so that it can be known not to check for instruction tokens if given tokens are in the places of an instruction's operands
    char *source, // Raw source code from input file;
         *textBuff; // Buffer for the text of the current token to be put into the sourceInstructions array;
} lexer;

extern int lexer_run(void);

#endif
