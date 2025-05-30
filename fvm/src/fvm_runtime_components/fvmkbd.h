#ifndef FVMKBD_H

#define FVMKBD_H

#include <GLFW/glfw3.h>
#include "global.h"

extern struct fvmkbd_data { // Runtime data used by the Keyboard API
    GLFWwindow *window;
    size_t keypress_queue_size, // Slots allocated in memory for the keypress queue
           keypress_queue_length; // Number of slots actually used in queue
    struct fvmkbd_keypress { // Fields correspond to GLFW inputs of the same likeness
        int key,
            scancode,
            action,
            modifiers;
    } *keypress_queue; // Queue of keyboard events (key presses, releases, and repeats)
    _Bool errors; // If errors have occurred
} fvmkbd;

extern void fvmkbd_keypress_cb(GLFWwindow *, int, int, int, int); // Keypress callback that adds the enqueues the keypresses to fvmkbd.keypress_queue
extern _Bool fvmkbd_init(GLFWwindow *); // fvmkbd initialisation
extern void fvmkbd_get_keypress_queue_length(void); // Set mdr = length of the keypress queue
extern void fvmkbd_get_next_keypress_as_key(void); // Dequeue keypress from keypress queue and place it in mdr in the format [2 bytes 0][1 byte action][1 byte modifiers][4 bytes GLFW key]
extern void fvmkbd_get_scancode_for_key(void); // Translate GLFW key in MDR to its platform-specific scancode and place the value in the MDR
extern void fvmkbd_get_next_keypress_as_scancode(void); // Dequeue keypress from keypress queue and place it in mdr in the format [2 bytes 0][1 byte action][1 byte modifiers][4 bytes platform-specific scancode]
extern _Bool fvmkbd_tick(void); // Updates events for fvmkbd every CPU cycle, and returns 1 if there were errors and the VM should be subsequently shut down
extern void fvmkbd_end(void); // Cleanup

#endif
