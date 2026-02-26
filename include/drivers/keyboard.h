#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

/* Key codes */
#define KEY_ESC    0x01
#define KEY_ENTER  0x1C
#define KEY_BACKSPACE 0x0E
#define KEY_TAB    0x0F
#define KEY_SPACE  0x39

/* Keyboard event */
typedef struct {
    u8 scancode;
    bool pressed;
    char character;
} keyboard_event_t;

/* Initialize keyboard */
void keyboard_init(void);

/* Get key event (non-blocking) */
bool keyboard_get_event(keyboard_event_t* event);

/* Read character (blocking) */
char keyboard_read_char(void);

#endif /* KEYBOARD_H */
