/* Fox Virtual Machine: Runtime
 * Copyright (C) 2024-2025 Finn Chipp
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fvm_runtime_components/global.h"
#include "fvm_runtime_components/instructions.h"
#include "fvm_runtime_components/fvmgl.h"
#include "fvm_runtime_components/fvmkbd.h"

int main(void) { // Entry point:
	FILE *f;

    if((files[CST] = (struct fvm_file){.self = calloc(ALLOC_SIZE, sizeof(uint64_t)), .size = ALLOC_SIZE, .length = 0}).self == NULL) { // Try to initialise Callstack
        perror("fvmr -> Could not allocate memory for Callstack");

        return FVMR_EXIT_FAILURE_INITIAL_ALLOCATION;
    }

	if((f = fopen(FVM_ROM, "rb")) == NULL) { // Try to open ROM file
		perror("fvmr -> Could not access ROM");

        free(files[CST].self);

		return FVMR_EXIT_FAILURE_INITIAL_FILE_ACCESS;
	}	

    // Get size of ROM:

	files[MEM].size = 0;

	while(fgetc(f) != EOF)
        files[MEM].size++;

	rewind(f); // Go back to beginning of file once number of bytes has been counted

    files[MEM].size = (files[MEM].size >> 3) + (_Bool)(files[MEM].size % 8); // Divide it by 8 (and add one in the case of unclean divide), since fgetc() counts bytes, not qwords

	files[MEM].length = files[MEM].size;

    if(!files[MEM].length) { // Check if ROM is empty and don't continue if it is
        fprintf(stderr, "fvmr -> Found ROM to be empty!\n");

        free(files[CST].self);

        fclose(f);

        return FVMR_EXIT_FAILURE_EXECUTION;
    }

	if((files[MEM].self = calloc(files[MEM].size, sizeof(uint64_t))) == NULL) { // Attempt to allocate space for Main Memory to contain ROM 
		perror("fvmr -> Could not allocate memory for Main Memory");

        free(files[CST].self);

		fclose(f);

		return FVMR_EXIT_FAILURE_INITIAL_ALLOCATION;
	}

	fread(files[MEM].self, files[MEM].size, sizeof(uint64_t), f); // Load ROM into Main Memory

	fclose(f); // Close ROM

    if((disk = fopen(FVM_DISK, "rb+")) == NULL) { // Try to open Secondary Storage for runtime
        perror("fvmr -> Could not access Disk");

        free(files[CST].self);
        free(files[MEM].self);

        return FVMR_EXIT_FAILURE_INITIAL_FILE_ACCESS;
    }

    // Initialise Graphics API et al:

    if(fvmgl_init()) { // Initialise fvmgl
        fprintf(stderr, "fvmr -> Graphics API -> Failed to initialise.\n");

        free(files[CST].self);
        free(files[MEM].self);

        fclose(disk);

        fvmgl_end();

        return FVMR_EXIT_FAILURE_GRAPHICS_LIB;
    }

    if(fvmkbd_init(fvmgl_screen_object.window)) { // Initialise fvmkbd
        fprintf(stderr, "fvmr -> Keyboard API -> Failed to initialise.\n");

        free(files[CST].self);
        free(files[MEM].self);

        fclose(disk);

        fvmgl_end();

        return FVMR_EXIT_FAILURE_KEYBOARD_LIB;
    }

    // Begin execution:

	for(fvm_registers[CEA] = 0; files[MEM].self[fvm_registers[CEA]] != 27; fvm_registers[CEA]++) { // Traverse instructions until instruction 27 (fi - finish) is encountered
		if(files[MEM].self[fvm_registers[CEA]] >= NO_INSTRUCTIONS) { // If a number is encountered that should be an instruction but isn't in the instructions list
			fprintf(stderr, "fvmr -> Encountered unknown instruction '%zu'\n", files[MEM].self[fvm_registers[CEA]]);

			fvmr_exit_code = FVMR_EXIT_FAILURE_EXECUTION;

			break;
		}

        if(instructions[files[MEM].self[fvm_registers[CEA]]]()) { // Otherwise, try to execute the current instruction. If it returns a failed status, exit safely
			fvmr_exit_code = FVMR_EXIT_FAILURE_EXECUTION;

			break;
		}

		if(fvmgl_tick()) {
		    fprintf(stderr, "fvmr -> Graphics library encountered an error.\n");

            fvmr_exit_code = FVMR_EXIT_FAILURE_GRAPHICS_LIB;

            break;
		}

		if(fvmkbd_tick()) {
		    fprintf(stderr, "fvmr -> Keyboard library encountered an error.\n");

		    fvmr_exit_code = FVMR_EXIT_FAILURE_KEYBOARD_LIB;

		    break;
		}
	}

    if(fvmr_exit_code != FVMR_EXIT_SUCCESS) // Produce a traceback if there were errors
        traceback();

    // Cleanup:

    free(files[CST].self);
    free(files[MEM].self);

    fclose(disk);

    fvmkbd_end();
    fvmgl_end();

	return fvmr_exit_code; // Done!
}
