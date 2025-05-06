/* Fox Virtual Machine: Assembler
 * Copyright (C) 2025 Finn Chipp
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fvm_assembler_components/shared.h"
#include "fvm_assembler_components/lexer.h"
#include "fvm_assembler_components/parser.h"

// Start of assembler execution:

int main(int argc, char **argv) { // Main function

	// Declarations:

    int failureStatus;

	// Initialisations:

    shared.argc = argc;
    shared.argv = argv;

    // Run assembler components:

    if((failureStatus = init_run()) || // Initialise assembler components
       (failureStatus = lexer_run()) || // Run the lexer
       (failureStatus = parser_run()) || // Run the parser
       (failureStatus = end_run())) { // Cleanup, finalisations, and write output to file
        return failureStatus;
    }

	return 0; // Done!
}
