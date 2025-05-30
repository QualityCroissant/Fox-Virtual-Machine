#ifndef FVMR_FVMGL_H

#define FVMR_FVMGL_H

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define FVMGL_DEFAULT_WIDTH 1280
#define FVMGL_DEFAULT_HEIGHT 720
#define FVMGL_DEFAULT_DEPTH 1280

#define FVMGL_DEFAULT_TITLE "FVMR Screen"

extern const char *FVMGL_GL_ERROR_DESCRIPTIONS[]; // Descriptions of errors to print out in place of their codes if they occur

enum fvmgl_instruction {                // Instructions that can be executed by the program running in the VM
    FVMGL_SET_WINDOW_DIMENSIONS = 0,    // Size of the actual window
    FVMGL_SET_WORKING_DIMENSIONS = 1,   // Coördinates as referenced by the application
    FVMGL_SET_WINDOW_TITLE = 2,         // Title of window
    FVMGL_SET_WINDOW_VISIBILITY = 3,    // Is window hidden?
    FVMGL_SET_WINDOW_FULLSCREEN = 4,    // Is window fullscreen?
    FVMGL_SET_WINDOW_VSYNC = 5,         // Does window use vsync?
    FVMGL_DRAW_TRIANGLE = 6,            // Draw a triangle via its vertexes
    FVMGL_SWAP_BUFFERS = 7,             // Swap the buffers (draw to the screen)
    FVMGL_SET_PROJECTION = 8,           // Set perspective or orthographic projection (0 = orthographic, 1 = perspective)
    FVMGL_CLEAR_BUFFERS = 9,            // Clear the depth and colour buffers
    FVMGL_GET_WINDOW_SHOULD_CLOSE = 10, // Place 1 in the cell after the instruction, if the window should closed
    FVMGL_GET_WINDOW_DIMENSIONS = 11    // Set the two cells after the instruction to the width and the height of the window
};

extern struct fvmgl_screen { // An object to keep track of the screen and its parameters
    GLFWwindow *window;
    GLFWmonitor *monitor;
    uint64_t window_width,
             window_height,
             working_width,
             working_height,
             working_depth;
    char *title;
    _Bool perspective,
          errors;
} fvmgl_screen_object;

extern void fvmgl_error(_Bool glError, int error, const char *description); // Error reporting function used by both the glfw error callback and the manual gl error checks
extern void fvmgl_error_cb(int error, const char *description); // Error callback function for glfw
extern _Bool fvmgl_catch_errors(void); // Error checker to see if glfw or gl encountered errors
extern void fvmgl_framebuffer_resized_cb(GLFWwindow *window, int width, int height); // Callback function for when the window is resized. Makes a viewport a box that fits within the dimensions of the window, accounting for difference in aspect ratio
extern _Bool fvmgl_init(void); // Setup function
extern _Bool fvmgl_update(uint64_t *data); // Handle calls from the program running on the VM
extern _Bool fvmgl_tick(void); // Update the graphics API each execution cycle
extern void fvmgl_end(void); // Cleanup

#endif
