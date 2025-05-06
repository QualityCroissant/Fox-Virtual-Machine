#ifndef FVMR_GLOBAL_H

#define FVMR_GLOBAL_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define FVM_ROM "hardware/rom" // The ROM file
#define FVM_DISK "hardware/disk" // The Disk file
#define NO_FILES 4 // Number of files/memory channels
#define NO_REGISTERS 7 // Number of registers
#define NO_INSTRUCTIONS 27 // Number of instructions

#define ALLOC_SIZE 50 // Size to reallocate/allocate memory

extern enum fvmr_exit_code_value {
    FVMR_EXIT_SUCCESS = 0,
    FVMR_EXIT_FAILURE_INITIAL_ALLOCATION = 1,
    FVMR_EXIT_FAILURE_INITIAL_FILE_ACCESS = 2,
    FVMR_EXIT_FAILURE_EXECUTION = 3,
    FVMR_EXIT_FAILURE_GRAPHICS_LIB = 4
} fvmr_exit_code;

extern void *alloc_buff; // Buffer for memory allocation
extern FILE *disk; // File pointer to disk file at boot

enum fvm_file_no { // Files' designated numbers
	MEM = 0,
	INP = 1,
	OUT = 2,
	CST = 3
};

extern struct fvm_file {
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

extern uint64_t fvm_registers[NO_REGISTERS]; // All the registers

extern const char *REGISTER_NAMES[NO_REGISTERS]; // Register names for traceback

extern void traceback(void); // Traceback (error report)

#endif
