/* Fox Virtual Machine: Graphics Library
 * Copyright (C) 2025 Finn Chipp
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// This file handles all primary GLFW and GL operations. As such, fvmkbd cannot be used until fvmgl has been initialised.

#include "fvmgl.h"

const char *FVMGL_GL_ERROR_DESCRIPTIONS[] = {
    [GL_INVALID_ENUM] = "Invalid enum.",
    [GL_INVALID_VALUE] = "Invalid value.",
    [GL_INVALID_OPERATION] = "Invalid operation.",
    [GL_STACK_OVERFLOW] = "Stack overflow.",
    [GL_STACK_UNDERFLOW] = "Stack underflow.",
    [GL_OUT_OF_MEMORY] = "Out of memory."
};


struct fvmgl_screen fvmgl_screen_object = {
    .window = NULL,
    .monitor = NULL,
    .window_width = FVMGL_DEFAULT_WIDTH,
    .window_height = FVMGL_DEFAULT_HEIGHT,
    .working_width = FVMGL_DEFAULT_WIDTH,
    .working_height = FVMGL_DEFAULT_HEIGHT,
    .working_depth = FVMGL_DEFAULT_DEPTH,
    .title = FVMGL_DEFAULT_TITLE,
    .perspective = 0,
    .errors = 0
};

void fvmgl_error(_Bool glError, int error, const char *description) { // Error reporting function used by both the glfw error callback and the manual gl error checks
    fprintf(stderr,
            "fvmr -> Graphics API -> %s Error '%d': %s\n",
            glError ? "GL" : "GLFW",
            error,
            description);

    fvmgl_screen_object.errors = 1;
}

void fvmgl_error_cb(int error, const char *description) { // Error callback function for glfw
    fvmgl_error(0, error, description);
}

_Bool fvmgl_catch_errors(void) { // Error checker to see if glfw or gl encountered errors
    _Bool result = 0;
    GLenum error;

    if(fvmgl_screen_object.errors) // Check if errors are already raised (probably by glfw callback)
        result = 1;

    if((error = glGetError()) != GL_NO_ERROR) { // Check if gl has raised errors
        fvmgl_error(1, error, FVMGL_GL_ERROR_DESCRIPTIONS[error]);
        result = 1;
    }

    return result;
}

void fvmgl_framebuffer_resized_cb(GLFWwindow *window, int width, int height) { // Callback function for when the window is resized. Makes a viewport a box that fits within the dimensions of the window, accounting for difference in aspect ratio
    long double windowAspectRatio,
                workingAspectRatio,
                newViewportWidth,
                newViewportHeight,
                newViewportStartX,
                newViewportStartY;

    (void)window; // We don't need the window parameter because there's only one window and we already know it (fvmgl_screen_object.window).

    // Update the known window width and height in the global screen object:

    fvmgl_screen_object.window_width = width,
    fvmgl_screen_object.window_height = height;

    // Resize the viewport in accordance with the new window dimensions (referenced coördinates by application will still be the same):

    windowAspectRatio = (long double)fvmgl_screen_object.window_width / (long double)fvmgl_screen_object.window_height,
    workingAspectRatio = (long double)fvmgl_screen_object.working_width / (long double)fvmgl_screen_object.working_height;

    if(windowAspectRatio > workingAspectRatio) // If the window is wider than the viewport but not taller
        newViewportWidth = (long double)fvmgl_screen_object.window_height * workingAspectRatio,
        newViewportHeight = fvmgl_screen_object.window_height,

        newViewportStartX = (long double)(fvmgl_screen_object.window_width - newViewportWidth) / 2,
        newViewportStartY = 0;
    else // If the window is taller than the viewport but not wider
        newViewportWidth = fvmgl_screen_object.window_width,
        newViewportHeight = (long double)fvmgl_screen_object.window_width / workingAspectRatio,

        newViewportStartX = 0,
        newViewportStartY = (long double)(fvmgl_screen_object.window_height - newViewportHeight) / 2;

    glViewport(newViewportStartX, newViewportStartY, newViewportWidth, newViewportHeight); // Resize the viewport

    if(fvmgl_catch_errors())
        return;

    // Remake the viewport:

    // Set the projection matrix:
    
    glMatrixMode(GL_PROJECTION);

    if(fvmgl_catch_errors())
        return;

    glLoadIdentity();

    if(fvmgl_catch_errors())
        return;

    (fvmgl_screen_object.perspective ? glFrustum : glOrtho) ( // The new viewport should be referenced under the same coördinates
        0,
        fvmgl_screen_object.working_width,
        0,
        fvmgl_screen_object.working_height,
        1,
        fvmgl_screen_object.working_depth
    );

    if(fvmgl_catch_errors())
        return;

    // Restore model view:

    glMatrixMode(GL_MODELVIEW);

    if(fvmgl_catch_errors())
        return;
    
    glLoadIdentity();

    return;
}

_Bool fvmgl_init(void) { // Setup function
    int width, height;

    // Set error callback and initialise glfw:

    glfwSetErrorCallback(fvmgl_error_cb);

    glfwInit();

    if(fvmgl_catch_errors())
        return 1;

    // Set gl flags:

    glEnable(GL_DEBUG_OUTPUT);

    if(fvmgl_catch_errors())
        return 1;
    
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    if(fvmgl_catch_errors())
        return 1;

    glEnable(GL_DEPTH_TEST);

    if(fvmgl_catch_errors())
        return 1;

    // Initialise screen object and set up glfw:

    fvmgl_screen_object.monitor = glfwGetPrimaryMonitor(); // Set monitor to the primary monitor (for switching to fullscreen)

    if(fvmgl_catch_errors())
        return 1;

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    if(fvmgl_catch_errors())
        return 1;

    fvmgl_screen_object.window = glfwCreateWindow ( // Create the window with the default parameters
        fvmgl_screen_object.window_width,
        fvmgl_screen_object.window_height,
        fvmgl_screen_object.title,
        NULL,
        NULL
    );

    if(fvmgl_catch_errors())
        return 1;

    glfwMakeContextCurrent(fvmgl_screen_object.window); // Set the gl context to the window

    if(fvmgl_catch_errors())
        return 1;

    // Set up the resize callback:

    glfwGetFramebufferSize(fvmgl_screen_object.window, &width, &height); // Get the dimensions of the framebuffer

    if(fvmgl_catch_errors())
        return 1;

    fvmgl_framebuffer_resized_cb(NULL, width, height); // Call the resized function once to set up the viewport correctly

    if(fvmgl_catch_errors())
        return 1;
    
    glfwSetFramebufferSizeCallback(fvmgl_screen_object.window, fvmgl_framebuffer_resized_cb); // Add the callback function to the event listener

    return fvmgl_catch_errors();
}

_Bool fvmgl_update(uint64_t *data) { // Handle calls from the program running on the VM
    int width, height;
    char *title;

    switch(data[0]) { // Depending on the instruction...
        case FVMGL_SET_WINDOW_DIMENSIONS:
            glfwSetWindowSize(fvmgl_screen_object.window, data[1], data[2]);
            
            return fvmgl_catch_errors();
        case FVMGL_SET_WORKING_DIMENSIONS:
            fvmgl_screen_object.working_width = data[1],
            fvmgl_screen_object.working_height = data[2],
            fvmgl_screen_object.working_depth = data[3];

            glfwGetFramebufferSize(fvmgl_screen_object.window, &width, &height);

            if(fvmgl_catch_errors())
                return 1;

            fvmgl_framebuffer_resized_cb(NULL, width, height); // Call the resize function to apply the new dimensions
            
            return fvmgl_catch_errors();
        case FVMGL_SET_WINDOW_TITLE:
            // Put the characters into a character array, from the uint64_t array:

            if((title = calloc(data[1], sizeof(char))) == NULL) {
                fprintf(stderr, "fvmr -> Graphics API -> Couldn't allocate memory for new window title.\n");

                return 1;
            }

            for(uint64_t i = 0; i < data[1]; i++)
                title[i] = data[2 + i];

            // Set the window title to the characters collected, and free the memory used to store the character array:

            glfwSetWindowTitle(fvmgl_screen_object.window, title);

            free(title);

            return fvmgl_catch_errors();
        case FVMGL_SET_WINDOW_VISIBILITY:
            (data[1] ? glfwShowWindow : glfwHideWindow)
                (fvmgl_screen_object.window);

            return fvmgl_catch_errors();
        case FVMGL_SET_WINDOW_FULLSCREEN:
            glfwSetWindowMonitor ( // Use the primary monitor as the fullscreen window
                fvmgl_screen_object.window,
                data[1] ? fvmgl_screen_object.monitor : NULL,
                0,
                0,
                fvmgl_screen_object.window_width,
                fvmgl_screen_object.window_height,
                GLFW_DONT_CARE
            );

            return fvmgl_catch_errors();
        case FVMGL_SET_WINDOW_VSYNC:
            glfwSwapInterval(data[1]);

            return fvmgl_catch_errors();
        case FVMGL_DRAW_TRIANGLE:

            glColor4ub(data[1], data[2], data[3], data[4]);

            if(fvmgl_catch_errors())
                return 1;

            glBegin(GL_TRIANGLES);

            glVertex3i((int64_t)data[5], (int64_t)data[6], (int64_t)data[7]);
            glVertex3i((int64_t)data[8], (int64_t)data[9], (int64_t)data[10]);
            glVertex3i((int64_t)data[11], (int64_t)data[12], (int64_t)data[13]);

            glEnd();
        
            return fvmgl_catch_errors();
        case FVMGL_SWAP_BUFFERS:
            glfwSwapBuffers(fvmgl_screen_object.window);

            return fvmgl_catch_errors();
        case FVMGL_SET_PROJECTION:
            fvmgl_screen_object.perspective = data[1];

            glfwGetFramebufferSize(fvmgl_screen_object.window, &width, &height);

            if(fvmgl_catch_errors())
                return 1;

            fvmgl_framebuffer_resized_cb(NULL, width, height); // Call the resize function, which will change the projection of the viewport depending on fvmgl_screen_object.perspective when it remakes it
            
            return fvmgl_catch_errors();
        case FVMGL_CLEAR_BUFFERS:
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            return fvmgl_catch_errors();
        case FVMGL_GET_WINDOW_SHOULD_CLOSE:
            data[1] = glfwWindowShouldClose(fvmgl_screen_object.window);

            return fvmgl_catch_errors();
        case FVMGL_GET_WINDOW_DIMENSIONS:
            data[1] = fvmgl_screen_object.window_width,
            data[2] = fvmgl_screen_object.window_height;

            return 0;
        default:
            fprintf(stderr,
                    "fvmr -> Graphics API -> Got invalid instruction '%zu'!\n",
                    data[0]);

            return 1;
    }
}

_Bool fvmgl_tick(void) { // Update the graphics API each execution cycle
    glfwPollEvents();

    return fvmgl_screen_object.errors;
}

void fvmgl_end(void) { // Cleanup
    glfwDestroyWindow(fvmgl_screen_object.window);
    glfwTerminate();
}
