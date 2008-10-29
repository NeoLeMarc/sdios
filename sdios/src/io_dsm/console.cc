//
// File:  src/io_dsm/main.cc
//
// Description: SDIOS Console Driver 
//

#include <l4/thread.h>
#include <l4/kip.h>
#include <l4io.h>
#include <l4/ipc.h>


#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>

#include "../lib/io/ia32-port.h"

unsigned short * vgaStart = (unsigned short *)0xb8000UL;
int consoleCursorHPos = 0, consoleCursorVPos = 24;

void write(unsigned short length, char input[]){
    for (int pos = 0; pos < length; pos++) {
        if (consoleCursorHPos == 80 || input[pos] == '\n') {
            if (consoleCursorVPos == 24) {
                // Shift whole screen one line up
                memcpy((void *)vgaStart, (void *)(vgaStart + 80), 3840);
                // Clear last line
                if ((length - pos) < 80) 
                    for (int i = 0; i < 80; i++)
                        *(vgaStart + 1920 + i) = 0xa00;

            } else {
                consoleCursorVPos++;
            }
            consoleCursorHPos = 0;
        }
        if (input[pos] != '\n') {
            *((unsigned char *)vgaStart + 160*consoleCursorVPos + 2*consoleCursorHPos) = input[pos];
            consoleCursorHPos++;
        }    
    }    
}

void clear() {
    for (int pos = 0; pos < 2000; pos++)
        *((unsigned short *)vgaStart + pos) = 0xa00;
    consoleCursorHPos = 0;
    consoleCursorVPos = 0;
}

int consoleServer () {
    L4_Time_t t = L4_TimePeriod (500000);
    L4_Sleep(t);
    printf ("[CONSOLE SERVER] Started\n");

#if 0
    write(1, "\n");

    L4_Sleep(t);
    for (int i = 0; i < 32; i++) {
        write(16, "0123456789A\nBCDEF");
    }
    L4_Sleep(t);
    clear();
    for (int i = 0; i < 105; i++) {
        write(15, "0123456789ABCDEF");
    }
#endif

    printf ("[CONSOLE SERVER] Spinning!\n");
 
    /* Spin forever */
    while (42);
    
    return 0;
}
