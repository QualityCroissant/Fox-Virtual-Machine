/* Fox Virtual Machine: Runtime Globals
 * Copyright (C) 2025 Finn Chipp
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "global.h"

enum fvmr_exit_code_value fvmr_exit_code;

void *alloc_buff;
FILE *disk;

struct fvm_file files[NO_FILES];

uint64_t fvm_registers[NO_REGISTERS];

const char *REGISTER_NAMES[NO_REGISTERS] = {
	"MCH (Memory Channel)           ",
	"MAR (Memory Address Register)  ",
	"MDR (Memory Data Register)     ",
	"ACC (Accumulator)              ",
	"DAT (Data)                     ",
	"CEA (Current Execution Address)",
	"CSP (Callstack Pointer)        ",
    "GP0 (General Purpose 0)        ",
    "GP1 (General Purpose 1)        ",
    "GP2 (General Purpose 2)        ",
    "GP3 (General Purpose 3)        ",
    "GP4 (General Purpose 4)        ",
    "GP5 (General Purpose 5)        ",
    "GP6 (General Purpose 6)        ",
    "GP7 (General Purpose 7)        ",
};

void traceback(void) { // Traceback (error report)
	fprintf(stderr,
			"fvmr -> Traceback:\n"
			"\t---Registers---\n"
			"\tNumber\tName                           \tCurrent Value\n");

	for(uint64_t i = 0; i < NO_REGISTERS; i++) { // Display the content of all registers
		fprintf(stderr,
				"\t%zu\t%s\t%zu\n",
				i,
				REGISTER_NAMES[i],
				fvm_registers[i]);
	}

	fprintf(stderr,
			"\t---Callstack---\n"
			"\tAddress\tValue\n");

	for(uint64_t i = 0; i < files[CST].length; i++) { // Display the content of the Callstack
		fprintf(stderr,
				"\t%zu\t%zu%s\n",
				files[CST].length - i - 1,
				files[CST].self[files[CST].length - i - 1],
				files[CST].length - i - 1 == fvm_registers[CSP] ? "\t<- CSP" : "");
	}

	fprintf(stderr,
			"\t---Main Memory---\n"
			"\tAddress\tValue\n");

	for(uint64_t i = 0; i < files[MEM].length; i++) { // Display the content of Main Memory
		fprintf(stderr,
				"\t%zu\t%zu%s%s\n",
				i,
				files[MEM].self[i],
				i == fvm_registers[CEA] ? "\t<- CEA" : "",
				fvm_registers[MCH] == MEM && i == fvm_registers[MAR] ? "\t<- MAR" : "");
	}
}
