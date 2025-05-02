#ifndef FVMA_LEXER_H

#define FVMA_LEXER_H

#include "shared.h"

extern struct fvma_lexer {
    bool comment, // (Lexer) If the current char is within a comment - used for ignoring characters
         whitespace, // (Lexer) If the current char is within whitespace - used for ignoring characters
         rawText, // (Lexer) If the current char is within a literal - used for ignoring what would otherwise be tokenised
         label; // (Lexer) If the current char is within a label - used for determining the start address of the next token
    size_t maxAddress, // (Lexer) Address used to provide parser with the address of each token in the output
           textBuffSize, // (Lexer) Number of chars allocated to textBuff
           textBuffLength, // (Lexer) No. characters stored in textBuff (discluding \0)
           rawTextLength, // (Lexer) No. raw chars read from recently inputted literal
           operands; // (Lexer) Number of operands possessed by last instruction token, so that it can be known not to check for instruction tokens if given tokens are in the places of an instruction's operands
    char *source, // (Lexer) Raw source code from input file;
         *textBuff; // buffer for the text of the current token to be put into the sourceInstructions array;
} lexer;

extern int lexer_run(void);

#endif
