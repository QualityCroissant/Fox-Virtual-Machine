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

    if((failureStatus = init_run()) || 
       (failureStatus = lexer_run()) ||
       (failureStatus = parser_run()) ||
       (failureStatus = end_run())) {
        return failureStatus;
    }

	return 0; // Done!
}
