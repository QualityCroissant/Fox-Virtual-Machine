#ifndef FVMA_PARSER_H

#define FVMA_PARSER_H

#include "shared.h"

extern struct fvma_parser { // These are intended as variables, so will be written in camelCase:
    bool characterWasLegal, // If the character currently being checked in the currently processing label-definition was a valid character for a label, or not
         labelWasFound, // If the label being called upon exists in the Label Table
         escape; // When processing the characters of a string literal, was an escape-sequence initiated?
    uint64_t nextValue; // the next value to be written to the output buffer
    size_t labelTableSize, // Number of struct labels allocated to the Label Table
           labelTableLength; // Amount of labels stored in the Label Table
} parser;

extern int parser_run(void);

#endif
