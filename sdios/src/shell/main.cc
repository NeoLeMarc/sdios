//
// File:  src/shell/main.cc
//
// Description: Shell 
//

//#include <l4/thread.h>
//#include <l4/kip.h>
#include <l4io.h>
//#include <l4/sigma0.h>

//#include <sdi/types.h>
//#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
//#include <if/ifsyscall.h>
//#include <if/iftask.h>
#include <if/ifbielfloader.h>                                                                             

//#include "../lib/io/ia32-port.h"
//#include "keyboard.h"

#include <if/ifconsole.h>
#include <if/ifkeyboard.h>

#define MAX_COMMAND_LENGTH 200

CORBA_Environment env (idl4_default_environment);
L4_ThreadId_t locator_id, keyboard_id, console_id = L4_nilthread; 
void shell_loop();

int main(){
    printf("[SHELL] starting...");
    // Get locator id & Locate Keyboard-Server and Console-Server
    L4_ThreadId_t ram_dsm_id, nilthread = L4_nilthread;
    ram_dsm_id = L4_Pager();       
    locator_id  = IF_BIELFLOADER_getLocator((CORBA_Object) ram_dsm_id, &env);
    // Sleeping until Keyboard-Server is hopefully started...
    L4_Time_t t = L4_TimePeriod (2000000);
    L4_Sleep(t);
    while(L4_IsNilThread(keyboard_id))
        IF_LOCATOR_Locate((CORBA_Object) locator_id, IF_KEYBOARD_ID, &keyboard_id, &env);
    printf("[SHELL] thinks that keyboad-server is at %lx \n", keyboard_id);
    while(L4_IsNilThread(console_id))
        IF_LOCATOR_Locate((CORBA_Object) locator_id, IF_CONSOLE_ID, &console_id, &env);
    printf("[SHELL] thinks that console-server is at %lx \n", console_id);

    printf("[SHELL] clearing screen \n");
    IF_CONSOLE_clear((CORBA_Object) console_id, &env);

    shell_loop();
}


void shell_loop(){
    L4_Time_t t = L4_TimePeriod (100000);
    
    //inbuffer
    keyboardBuffer kbbuf;
    char mybuf[8];
   
    //outbuffer
    buffer_t* outbuf;
    outbuf->_maximum = 8;
    CORBA_char test[8];
    strncpy(test, "j", 1);
    outbuf->_buffer = test;
    outbuf->_length = 1;
    IF_CONSOLE_write((CORBA_Object) console_id, outbuf, &env);

    strncpy(test, "doof", 4);
    outbuf->_length = 4;
    IF_CONSOLE_write((CORBA_Object) console_id, outbuf, &env);

    printf("[SHELL] Entering Processing Loop \n");
    while(1){
        //Read from Keyboard
        IF_KEYBOARD_read((CORBA_Object) keyboard_id, &kbbuf, &env);
        int numchars = kbbuf.numchars;
        CORBA_char outChars[numchars];
        strncpy(outChars, kbbuf.chars, numchars);
        printf("[SHELL] Read keyboard buffer. Got %d chars (\"%s\" - as int \"%i\"), more: %d\n", numchars, outChars, outChars[0], kbbuf.more);

        
        // Delete last Char if backspace
        if ((int) outChars[0] == 8){
            IF_CONSOLE_delete((CORBA_Object) console_id, 1, &env);
            numchars--;
            //CORBA_char b[8];
            //strncpy(b, outChars, numchars);
            //strncpy(outChars, b, numchars);
        }
        
        //Print chars on Screen and put them into the command-buffer
        if(numchars > 0){
            //outChars[numchars] = '_';
            //(*outbuf)._length = (CORBA_unsigned_long) numchars;
            outbuf->_buffer = outChars;
            printf("[SHELL] Printing buffer='%s' on screen. numchars=%i \n", outbuf->_buffer, numchars);
            IF_CONSOLE_write((CORBA_Object) console_id, outbuf, &env);
        }

        //Store chars in buffer until we get a newline
        //if newline search for command
        if (!kbbuf.more)
            L4_Sleep(t);
    }
}
