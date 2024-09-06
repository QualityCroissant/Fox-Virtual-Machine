/* Fox Virtual Machine: Runtime
 * Copyright (C) 2024 Finn Chipp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define FVM_ROM "hardware/rom" // The ROM file
#define FVM_DISK "hardware/disk" // The Disk file
#define NO_FILES 4 // Number of files/memory channels
#define NO_REGISTERS 7 // Number of registers
#define NO_INSTRUCTIONS 27 // Number of instructions

#define ALLOC_SIZE 50 // Size to reallocate/allocate memory

void *alloc_buff; // Buffer for memory allocation
FILE *disk; // File pointer to disk file at boot

enum fvm_file_no { // Files' designated numbers
	MEM = 0,
	INP = 1,
	OUT = 2,
	CST = 3
};

struct fvm_file {
	uint64_t *self,
			 size,
			 length;
} files[NO_FILES]; // files/memory channels (only MEM and CST are actually stored like this)

enum fvm_register { // Registers' designated numbers
	MCH = 0,
	MAR = 1,
	MDR = 2,
	ACC = 3,
	DAT = 4,
	CEA = 5,
	CSP = 6
};

uint64_t fvm_registers[NO_REGISTERS]; // All the registers

const char *REGISTER_NAMES[NO_REGISTERS] = { // Register names for traceback
	"MCH (Memory Channel)           ",
	"MAR (Memory Address Register)  ",
	"MDR (Memory Data Register)     ",
	"ACC (Accumulator)              ",
	"DAT (Data)                     ",
	"CEA (Current Execution Address)",
	"CSP (Callstack Pointer)        "
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

// Instruction functions:
// (Each instruction returns 0 if it executes successfully, and 1 if it fails)

_Bool place(void) { // pl <value> <register>
//    printf("place %zu in %zu\n", files[MEM].self[fvm_registers[CEA] + 1], files[MEM].self[fvm_registers[CEA] + 2]);

	if(files[MEM].self[fvm_registers[CEA] + 2] >= NO_REGISTERS) { // If the register specified is not a known register
		fprintf(stderr,
				"fvmr -> Attempted to place value into unknown register '%zu'\n",
				files[MEM].self[fvm_registers[CEA] + 2]);

		return 1;
	}

    // Otherwise:

	fvm_registers[files[MEM].self[fvm_registers[CEA] + 2]] = files[MEM].self[fvm_registers[CEA] + 1]; // Place the value into the register

	fvm_registers[CEA] += 2; // Move the instruction pointer along by two

	return 0;
}

_Bool move(void) { // mv <register> <register>
//    printf("move %zu to %zu\n", files[MEM].self[fvm_registers[CEA] + 1], files[MEM].self[fvm_registers[CEA] + 2]);

	if(files[MEM].self[fvm_registers[CEA] + 2] >= NO_REGISTERS) { // If the destination register is unknown
		fprintf(stderr,
				"fvmr -> Attempted to move register's value into unknown register '%zu'\n",
				files[MEM].self[fvm_registers[CEA] + 2]);
		return 1;
	}

	if(files[MEM].self[fvm_registers[CEA] + 1] >= NO_REGISTERS) { // If the source register is unknown
		fprintf(stderr,
	    		"fvmr -> Attempted to move value in unknown register '%zu' into another register\n",
				files[MEM].self[fvm_registers[CEA] + 1]);
	
		return 1;
	}

    // Otherwise:
	
	fvm_registers[files[MEM].self[fvm_registers[CEA] + 2]] = fvm_registers[files[MEM].self[fvm_registers[CEA] + 1]]; // Move the number in the source register to the destination register

	fvm_registers[CEA] += 2; // Move the instruction pointer along by two

	return 0;
}

_Bool store(void) { // st <mdr> at <mar> in <mch>
//    printf("store %zu at %zu in %zu\n", fvm_registers[MDR], fvm_registers[MAR], fvm_registers[MCH]);

    switch(fvm_registers[MCH]) { // Depending on the Memory Channel, write in a different way
        case MEM: // For Main Memory:
            if(fvm_registers[MAR] + 1 > files[MEM].length) { // If the address is bigger than what's used
                files[MEM].length = fvm_registers[MAR] + 1;

                if(files[MEM].length > files[MEM].size) { // If it's bigger than what's allocated
                    files[MEM].size = files[MEM].length;

                    if((alloc_buff = (void *)realloc(files[MEM].self, files[MEM].size * sizeof(uint64_t))) == NULL) { // Attempt to reallocate Main Memory more space to accomodate the write
                        perror("fvmr -> Failure accessing memory at specified address");

                        return 1;
                    }

                    files[MEM].self = (uint64_t *)alloc_buff;
                }
            }

            files[MEM].self[fvm_registers[MAR]] = fvm_registers[MDR]; // Store MDR at address MAR in Main Memory

            return 0;
        case INP: // For Input:
            switch(fvm_registers[MAR]) { // Write to input in a different place depending on MAR
                case 0: // For Standard I/O
                    fprintf(stdin, "%c", (uint8_t)fvm_registers[MDR]); // Write the lowest byte to stdin

                    return 0;
                case 1: // For disk:
                    fwrite(&fvm_registers[MDR], sizeof(uint8_t), 1, disk); // Write the lowest byte to disk

                    return 0;
                case 3: // For screen buffer:
                default: // For peripheral fd:
                    fprintf(stderr, "fvmr -> Warning, writing to address on MCH 1 that is currently unimplemented\n");

                    return 0;
            }
        case OUT: // For Output:
            switch(fvm_registers[MAR]) { // Write to output in a different place depending on MAR
                case 0: // For Standard I/O
                    fprintf(stdout, "%c", (uint8_t)fvm_registers[MDR]); // Write the lowest byte to stdout

                    return 0;
                case 1: // For disk:
                    fwrite(&fvm_registers[MDR], sizeof(uint8_t), 1, disk); // Write the lowest byte to disk

                    return 0;
                case 3: // For screen buffer:
                default: // For peripheral fd:
                    fprintf(stderr, "fvmr -> Warning, writing to address on MCH 1 that is currently unimplemented\n");

                    return 0;
            }
        case CST: // For Callstack
            if(fvm_registers[MAR] + 1 > files[CST].size) { // If MAR is an address not currently in the allocated memory's range
                files[CST].size = fvm_registers[MAR] + 1;

                if((alloc_buff = (void *)realloc(files[CST].self, files[CST].size * sizeof(uint64_t))) == NULL) { // Attempt to reallocate the callstack to accomodate it
                    perror("fvmr -> Failure to reallocate memory for Callstack to perform write to custom address thereupon");

                    return 1;
                }

                files[CST].self = (uint64_t *)alloc_buff;
            }

            files[CST].self[fvm_registers[MAR]] = fvm_registers[MDR]; // Write MDR to address MAR in CST

            return 0;
        default: // For an any other given Memory Channel:
            fprintf(stderr, "fvmr -> Attempted write to unknown MCH '%zu'\n", fvm_registers[MCH]);

            return 1;
    }
}

_Bool load(void) { // ld to <mdr> from <mar> in <mch>
//    printf("load %zu in %zu\n", fvm_registers[MAR], fvm_registers[MCH]);

    switch(fvm_registers[MCH]) { // Load in a different way depending on MCH
        case MEM: // For Main Memory:
            if(fvm_registers[MAR] + 1 > files[MEM].length) { // If the address to load from is outside the bounds currently allocated
                files[MEM].length = fvm_registers[MAR] + 1; // Resize the memory known

                if(files[MEM].length > files[MEM].size) { // If a reallocation needs to be done in accordance with the new size
                    files[MEM].size = files[MEM].length;

                    if((alloc_buff = (void *)realloc(files[MEM].self, files[MEM].size * sizeof(uint64_t))) == NULL) { // Attempt to reallocate Main Memory
                        perror("fvmr -> Failure accessing memory at specified address");

                        return 1;
                    }

                    files[MEM].self = (uint64_t *)alloc_buff;
                }
            }

            fvm_registers[MDR] = files[MEM].self[fvm_registers[MAR]]; // Place the value from Main Memory at MAR into MDR

            return 0;
        case INP: // For Input:
            switch(fvm_registers[MAR]) { // Depending on where to input from (indicated in MAR)
                case 0: // For Standard I/O:
                    fvm_registers[MDR] = fgetc(stdin); // Place a byte from stdin into MDR

                    return 0;
                case 1: // For Secondary Storage:
                    fread(&fvm_registers[MDR], sizeof(uint8_t), 1, disk); // Read a byte from the disk into MDR

                    return 0;
                case 3: // For Screen Buffer:
                default: // For Peripheral fd:
                    fprintf(stderr, "fvmr -> Warning, reading from address on MCH 1 that is currently unimplemented\n");

                    return 0;
            }
        case OUT: // For Output:
            switch(fvm_registers[MAR]) { // Depending on MAR load from a different output source:
                case 0: // For Standard I/O:
                    fvm_registers[MDR] = fgetc(stdout); // Retrieve one byte from stdout into MDR

                    return 0;
                case 1: // For Secondary Storage:
                    fread(&fvm_registers[MDR], sizeof(uint8_t), 1, disk); // Read one byte from the disk into MDR

                    return 0;
                case 3: // For Screen Buffer:
                default: // For Peripheral fd:
                    fprintf(stderr, "fvmr -> Warning, reading from address on MCH 1 that is currently unimplemented\n");

                    return 0;
            }
        case CST: // For Callstack:
            if(fvm_registers[MAR] + 1 > files[CST].size) { // If the address to read from is outside of the allocated size for the Callstack
                files[CST].size = fvm_registers[MAR] + 1;

                if((alloc_buff = (void *)realloc(files[CST].self, files[CST].size * sizeof(uint64_t))) == NULL) { // Try to reallocate the Callstack's memory to retrieve the address
                    perror("fvmr -> Failure to reallocate memory for Callstack to perform read from custom address thereupon");

                    return 1;
                }

                files[CST].self = (uint64_t *)alloc_buff;
            }

            fvm_registers[MDR] = files[CST].self[fvm_registers[MAR]]; // Place the value at MAR on the Callstack into MDR

            return 0;
        default: // For an unrecognised MCH:
            fprintf(stderr, "fvmr -> Attempted read from unknown MCH '%zu'\n", fvm_registers[MCH]);

            return 1;
    }
}

_Bool jump(void) { // jm <address>
    fvm_registers[CEA] = files[MEM].self[fvm_registers[CEA] + 1] - 1; // Set CEA = number in front of this one, take one to combat the increment at the end of each cycle

    return 0;
}

_Bool jump_if_set(void) { // js <address>
    if(fvm_registers[ACC]) // If ACC is non-zero:
        fvm_registers[CEA] = files[MEM].self[fvm_registers[CEA] + 1] - 1; // Set CEA = number in front of this one, take one to combat the increment at the end of each cycle
    else
        fvm_registers[CEA]++; // Otherwise, make sure to not treat the supplied address as an instruction, by skipping over it

    return 0;
}

_Bool jump_if_clear(void) { // jc <address>
    if(!fvm_registers[ACC]) // If ACC is zero:
        fvm_registers[CEA] = files[MEM].self[fvm_registers[CEA] + 1] - 1; // Set CEA = number in front of this one, take one to combat the increment at the end of each cycle
    else
        fvm_registers[CEA]++; // Otherwise, make sure not to treat the supplied address as an instruction, by skipping over it

    return 0;
}

_Bool accumulator_add(void) { // a+
//    printf("acc += %zu\n", fvm_registers[DAT]);

    fvm_registers[ACC] += fvm_registers[DAT]; // ACC += DAT

    return 0;
}

_Bool accumulator_sub(void) { // a-
    fvm_registers[ACC] -= fvm_registers[DAT]; // ACC -= DAT

    return 0;
}

_Bool accumulator_not(void) { // a!
    fvm_registers[ACC] = ~fvm_registers[ACC]; // Invert bits of ACC

    return 0;
}


_Bool accumulator_increment(void) { // ai
    fvm_registers[ACC]++; // ACC += 1

    return 0;
}


_Bool accumulator_decrement(void) { // ad
    fvm_registers[ACC]--; // ACC -= 1

    return 0;
}


_Bool accumulator_mul(void) { // a*
    fvm_registers[ACC] *= fvm_registers[DAT]; // ACC *= DAT

    return 0;
}


_Bool accumulator_div(void) { // a/
    fvm_registers[ACC] /= fvm_registers[DAT]; // ACC /= DAT

    return 0;
}


_Bool accumulator_and(void) { // a&
    fvm_registers[ACC] &= fvm_registers[DAT]; // ACC = Logical AND bits of ACC with DAT

    return 0;
}

_Bool accumulator_or(void) { // a|
    fvm_registers[ACC] |= fvm_registers[DAT]; // ACC = Logical OR bits of ACC with DAT

    return 0;
}


_Bool accumulator_xor(void) { // a^
    fvm_registers[ACC] ^= fvm_registers[DAT]; // ACC = Logical XOR bits of ACC with DAT

    return 0;
}


_Bool accumulator_lsh(void) { // al
    fvm_registers[ACC] <<= fvm_registers[DAT]; // Left shift bits of ACC by DAT amount

    return 0;
}


_Bool accumulator_rsh(void) { // ar
    fvm_registers[ACC] >>= fvm_registers[DAT]; // Right shift bits of ACC by DAT amount

    return 0;
}


_Bool accumulator_gt(void) { // gt
    fvm_registers[ACC] = fvm_registers[ACC] > fvm_registers[DAT]; // ACC = 1 if ACC > DAT otherwise ACC = 0

    return 0;
}


_Bool accumulator_lt(void) { // lt
    fvm_registers[ACC] = fvm_registers[ACC] < fvm_registers[DAT]; // ACC = 1 if ACC < DAT otherwise ACC = 0

    return 0;
}

_Bool accumulator_ge(void) { // ge
    fvm_registers[ACC] = fvm_registers[ACC] >= fvm_registers[DAT]; // ACC = 1 if ACC >= DAT otherwise ACC = 0

    return 0;
}


_Bool accumulator_le(void) { // le
    fvm_registers[ACC] = fvm_registers[ACC] <= fvm_registers[DAT]; // ACC = 1 if ACC <= DAT otherwise ACC = 0

    return 0;
}

_Bool accumulator_eq(void) { // eq
    fvm_registers[ACC] = fvm_registers[ACC] == fvm_registers[DAT]; // ACC = 1 if ACC == DAT otherwise ACC = 0

    return 0;
}


_Bool accumulator_ne(void) { // ne
    fvm_registers[ACC] = fvm_registers[ACC] != fvm_registers[DAT]; // ACC = 1 if ACC != DAT otherwise ACC = 0

    return 0;
}

_Bool call_address(void) { // cl
    if(++files[CST].length > files[CST].size) { // If the callstack needs reallocating to include the address of this call
        files[CST].size += ALLOC_SIZE;

        if((alloc_buff = (void *)realloc(files[CST].self, files[CST].size * sizeof(uint64_t))) == NULL) { // Try to allocate it more space
            perror("fvmr -> Failure reallocating memory for Callstack");

            return 1;
        }

        files[CST].self = (uint64_t *)alloc_buff;
    }

    fvm_registers[CSP] = files[CST].length - 1; // Set CSP to new value

    files[CST].self[fvm_registers[CSP]] = fvm_registers[CEA]; // Push CEA onto the Callstack
    fvm_registers[CEA] = files[MEM].self[fvm_registers[CEA] + 1] - 1; // Set CEA = the address being called upon, take one to combat the increment of CEA each cycle

    return 0;
}

_Bool return_address(void) { // rt
    if(!(fvm_registers[CSP] + 1)) { // If there is nothing to pop from the Callstack
        fprintf(stderr, "fvmr -> Callstack underflow");

        return 1;
    }

    // Otherwise:

    files[CST].length = fvm_registers[CSP]; // Reassign the length of the callstack to the value of CSP before decrementing CSP
    fvm_registers[CEA] = files[CST].self[fvm_registers[CSP]--]; // CEA = pop(CST)

    return 0;
}

_Bool (*instructions[NO_INSTRUCTIONS])(void) = { // Array of function-pointers for each instruction
	[0] = &place,
	[1] = &move,
	[2] = &store,
	[3] = &load,
	[4] = &jump,
	[5] = &jump_if_set,
	[6] = &jump_if_clear,
	[7] = &accumulator_add,
	[8] = &accumulator_sub,
	[9] = &accumulator_not,
	[10] = &accumulator_increment,
	[11] = &accumulator_decrement,
	[12] = &accumulator_mul,
	[13] = &accumulator_div,
	[14] = &accumulator_and,
	[15] = &accumulator_or,
	[16] = &accumulator_xor,
	[17] = &accumulator_lsh,
	[18] = &accumulator_rsh,
	[19] = &accumulator_gt,
	[20] = &accumulator_lt,
	[21] = &accumulator_ge,
	[22] = &accumulator_le,
	[23] = &accumulator_eq,
	[24] = &accumulator_ne,
	[25] = &call_address,
	[26] = &return_address
};

int main(void) { // Entry point:
	FILE *f;

	if((files[CST] = (struct fvm_file){.self = calloc(ALLOC_SIZE, sizeof(uint64_t)), .size = ALLOC_SIZE, .length = 0}).self == NULL) { // Try to initialise Callstack
		perror("fvmr -> Could not allocate memory for Callstack");

		return 3;
    }

	if((f = fopen(FVM_ROM, "rb")) == NULL) { // Try to open ROM file
		perror("fvmr -> Could not access ROM");

        free(files[CST].self);

		return 2;
	}	

    // Get size of ROM:

	files[MEM].size = 0;

	while(fgetc(f) != EOF) files[MEM].size++; // TODO: Replace with solution using fseek()

	rewind(f); // Go back to beginning of file once number of bytes has been counted

    files[MEM].size = files[MEM].size / 4 + 1; // Divide it by 4 (plus 1 incase of truncation), since fgetc() counts bytes, not qwords

	files[MEM].length = files[MEM].size;

	if((files[MEM].self = calloc(files[MEM].size, sizeof(uint64_t))) == NULL) { // Attempt to allocate space for Main Memory to contain ROM 
		perror("fvmr -> Could not allocate memory for Main Memory");

        free(files[CST].self);

		fclose(f);

		return 3;
	}

	fread(files[MEM].self, files[MEM].size, sizeof(uint64_t), f); // Load ROM into Main Memory

	fclose(f); // Close ROM

    if((disk = fopen(FVM_DISK, "rb+")) == NULL) { // Try to open Secondary Storage for runtime
        perror("fvmr -> Could not access Disk");

        free(files[CST].self);
        free(files[MEM].self);

        return 2;
    }

    // Begin execution:

	for(fvm_registers[CEA] = 0; files[MEM].self[fvm_registers[CEA]] != 27; fvm_registers[CEA]++) { // Traverse instructions until instruction 27 (fi - finish) is encountered
		if(files[MEM].self[fvm_registers[CEA]] >= NO_INSTRUCTIONS) { // If a number is encountered that should be an instruction but isn't in the instructions list
			fprintf(stderr, "fvmr -> Encountered unknown instruction '%zu'\n", files[MEM].self[fvm_registers[CEA]]);

			traceback();

            free(files[CST].self);
            free(files[MEM].self);

			return 4;
		}

        if(instructions[files[MEM].self[fvm_registers[CEA]]]()) { // Otherwise, try to execute the current instruction. If it returns a failed status, exit safely
			traceback();

            free(files[CST].self);
            free(files[MEM].self);

			return 4;
		}
	}

    // Cleanup:

    free(files[CST].self);
    free(files[MEM].self);

    fclose(disk);

	return 0; // Done!
}
