#ifndef FVMKBD_H

#define FVMKBD_H

#include <GLFW/glfw3.h>
#include "global.h"

extern struct fvmkbd_data {
    GLFWwindow *window;
    size_t keypress_queue_size,
           keypress_queue_length;
    struct fvmkbd_keypress {
        int key,
            scancode,
            action,
            modifiers;
    } *keypress_queue;
    _Bool errors;
} fvmkbd;

extern void fvmkbd_keypress_cb(GLFWwindow *, int, int, int, int);
extern _Bool fvmkbd_init(GLFWwindow *);
extern void fvmkbd_get_keypress_queue_length(void);
extern void fvmkbd_get_next_keypress_as_key(void);
extern void fvmkbd_get_scancode_for_key(void);
extern void fvmkbd_get_next_keypress_as_scancode(void);
extern _Bool fvmkbd_tick(void);
extern void fvmkbd_end(void);

#endif
