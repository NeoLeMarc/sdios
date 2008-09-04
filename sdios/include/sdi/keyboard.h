//
// File: include/sdi/keyboard.h
//
// Description: Basic structure for keyboard interface usage
//

#ifndef __SDI_KEYBOARD_H
#define __SDI_KEYBOARD_H

// keyboard buffer
typedef struct {
    struct {
        unsigned char numchars : 6;
        unsigned char reserved : 1;
        unsigned char more     : 1;
    } status;
    char chars[7];
} keyboardBuffer;

#endif /* SDI_KEYBOARD_H */
