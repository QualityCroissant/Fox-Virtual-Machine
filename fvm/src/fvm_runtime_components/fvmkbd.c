#include "fvmkbd.h"

struct fvmkbd_data fvmkbd;

struct fvmkbd_keypress fvmkbd_dequeue_keypress(void) {
    if(!fvmkbd.keypress_queue_length)
        return (struct fvmkbd_keypress){0};

    struct fvmkbd_keypress removed = fvmkbd.keypress_queue[0];

    fvmkbd.keypress_queue_length--;

    for(size_t i = 0; i < fvmkbd.keypress_queue_length; i++) {
        fvmkbd.keypress_queue[i] = fvmkbd.keypress_queue[i + 1];
    }

    return removed;
}

void fvmkbd_enqueue_keypress(struct fvmkbd_keypress keypress) {
    void *allocBuff;

    if(++fvmkbd.keypress_queue_length > fvmkbd.keypress_queue_size) {
        fvmkbd.keypress_queue_size += ALLOC_SIZE;

        if((allocBuff = (void *)realloc(fvmkbd.keypress_queue, fvmkbd.keypress_queue_size * sizeof(struct fvmkbd_keypress))) == NULL) {
            fprintf(stderr, "fvmr -> Keyboard API -> Couldn't reallocate memory for keyboard input buffer.\n");
            fvmkbd.errors = 1;
            return;
        }

        fvmkbd.keypress_queue = (struct fvmkbd_keypress *)allocBuff;
    }

    fvmkbd.keypress_queue[fvmkbd.keypress_queue_length - 1] = keypress;
}

void fvmkbd_keypress_cb(GLFWwindow *window, int key, int scancode, int action, int modifiers) {
    (void)window;

    fvmkbd_enqueue_keypress (
        (struct fvmkbd_keypress) {
            key,
            scancode,
            action,
            modifiers
        }
    );
}

_Bool fvmkbd_init(GLFWwindow *window) {
    glfwSetKeyCallback(window, fvmkbd_keypress_cb);

    fvmkbd = (struct fvmkbd_data) {
        .window = window,
        .keypress_queue_size = ALLOC_SIZE,
        .keypress_queue_length = 0,
        .keypress_queue = calloc(ALLOC_SIZE, sizeof(struct fvmkbd_keypress)),
        .errors = 0
    };

    return !fvmkbd.keypress_queue;
}

void fvmkbd_get_keypress_queue_length(void) {
    fvm_registers[MDR] = fvmkbd.keypress_queue_length;
}

void fvmkbd_get_next_keypress_as_key(void) {
    struct fvmkbd_keypress keypress = fvmkbd_dequeue_keypress();

    fvm_registers[MDR] = (uint64_t)((uint8_t)keypress.action) << (32 + 8) |
                         (uint64_t)((uint8_t)keypress.modifiers) << 32 |
                         (uint64_t)((uint32_t)keypress.key);
}

void fvmkbd_get_scancode_for_key(void) {
    fvm_registers[MDR] = glfwGetKeyScancode(fvm_registers[MDR]);
}

void fvmkbd_get_next_keypress_as_scancode(void) {
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
