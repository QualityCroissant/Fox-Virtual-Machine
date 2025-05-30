/* Fox Virtual Machine: Keyboard API
 * Copyright (C) 2025 Finn Chipp
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fvmkbd.h"

struct fvmkbd_data fvmkbd; // Define fvmkbd for API runtime data

struct fvmkbd_keypress fvmkbd_dequeue_keypress(void) { // Remove the element at the start of the keypress queue and return it
    if(!fvmkbd.keypress_queue_length) // If the queue is empty return keypress with all-zero values
        return (struct fvmkbd_keypress){0};

    struct fvmkbd_keypress removed = fvmkbd.keypress_queue[0]; // Get the item to be removed

    // Remove the element:

    fvmkbd.keypress_queue_length--;

    for(size_t i = 0; i < fvmkbd.keypress_queue_length; i++) { // Move every element in the queue down by 1, except the first one, which gets overwritten
        fvmkbd.keypress_queue[i] = fvmkbd.keypress_queue[i + 1];
    }

    return removed; // Return the item previously at the start of the queue
}

void fvmkbd_enqueue_keypress(struct fvmkbd_keypress keypress) { // Add a keypress to the end of the fvmkbd.keypress_queue
    void *allocBuff;

    if(++fvmkbd.keypress_queue_length > fvmkbd.keypress_queue_size) { // Increment the length of the queue, and if needed, allocate it more space:
        fvmkbd.keypress_queue_size += ALLOC_SIZE;

        if((allocBuff = (void *)realloc(fvmkbd.keypress_queue, fvmkbd.keypress_queue_size * sizeof(struct fvmkbd_keypress))) == NULL) { // Attempt to reallocate
            fprintf(stderr, "fvmr -> Keyboard API -> Couldn't reallocate memory for keyboard input buffer.\n");
            fvmkbd.errors = 1;
            return;
        }

        fvmkbd.keypress_queue = (struct fvmkbd_keypress *)allocBuff; // If successful, give the memory pointer back to the queue
    }

    fvmkbd.keypress_queue[fvmkbd.keypress_queue_length - 1] = keypress; // Finally, add the new keypress to the end of the queue
}

void fvmkbd_keypress_cb(GLFWwindow *window, int key, int scancode, int action, int modifiers) { // Callback for whenever a new keypress even it detected
    (void)window;

    fvmkbd_enqueue_keypress ( // Enqueue the keypress to fvmkbd.keypress_queue
        (struct fvmkbd_keypress) {
            key,
            scancode,
            action,
            modifiers
        }
    );
}

_Bool fvmkbd_init(GLFWwindow *window) { // Initialise fvmkbd (must be called only after fvmgl is initialised)
    glfwSetKeyCallback(window, fvmkbd_keypress_cb); // Set callback for keypresses (errors will be handled by fvmgl)

    fvmkbd = (struct fvmkbd_data) { // Initialise fvmkbd runtime data
        .window = window,
        .keypress_queue_size = ALLOC_SIZE,
        .keypress_queue_length = 0,
        .keypress_queue = calloc(ALLOC_SIZE, sizeof(struct fvmkbd_keypress)),
        .errors = 0
    };

    return !fvmkbd.keypress_queue; // Return 1 for error if calloc() for keypress queue failed
}

void fvmkbd_get_keypress_queue_length(void) {
    fvm_registers[MDR] = fvmkbd.keypress_queue_length;
}

void fvmkbd_get_next_keypress_as_key(void) {
    // Dequeue the keypress and set MDR to it in the requested format:

    struct fvmkbd_keypress keypress = fvmkbd_dequeue_keypress();

    fvm_registers[MDR] = (uint64_t)((uint8_t)keypress.action) << (32 + 8) |
                         (uint64_t)((uint8_t)keypress.modifiers) << 32 |
                         (uint64_t)((uint32_t)keypress.key);
}

void fvmkbd_get_scancode_for_key(void) {
    fvm_registers[MDR] = glfwGetKeyScancode(fvm_registers[MDR]);
}

void fvmkbd_get_next_keypress_as_scancode(void) {
    // Dequeue the keypress and set MDR to it in the requested format:

    struct fvmkbd_keypress keypress = fvmkbd_dequeue_keypress();

    fvm_registers[MDR] = (uint64_t)((uint8_t)keypress.action) << (32 + 8) |
                         (uint64_t)((uint8_t)keypress.modifiers) << 32 |
                         (uint64_t)((uint32_t)keypress.scancode);
}

_Bool fvmkbd_tick(void) {
    return fvmkbd.errors;
}

void fvmkbd_end(void) {
    free(fvmkbd.keypress_queue);
}
