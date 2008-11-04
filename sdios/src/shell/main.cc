//
// File:  src/shell/main.cc
//
// Description: Shell 
//
#include <l4io.h>
#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/ifbielfloader.h>                                                                             

#include <if/ifconsole.h>
#include <if/ifkeyboard.h>

#define MAX_COMMAND_LENGTH 200
#define OUT_BUF_SIZE 8
#define CURSOR 219
#define KEY_BACKSPACE 8

CORBA_Environment env (idl4_default_environment);
L4_ThreadId_t locator_id, keyboard_id, console_id = L4_nilthread; 
void shell_loop();
void print(char*);
void printCursor();

//Inbuffer
keyboardBuffer kbbuf;
CORBA_char inChars[OUT_BUF_SIZE];

//Outbuffer
buffer_t* outbuf;
CORBA_char outChars[OUT_BUF_SIZE];


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
    
    outbuf->_maximum = OUT_BUF_SIZE;
    outbuf->_length = OUT_BUF_SIZE;
    outbuf->_buffer = outChars;
    
    print("Kurzer Test ohne fancy stuff.\n");
    print("Dies ist ein sehr langer Text der auf magische Weise (mit Pointer-Zauber) irgendwie auf dem Bildschirm erscheint! <- Mit irgendwas muss man es ja test :-)\n");
    printCursor();

    printf("[SHELL] Entering Processing Loop \n");
    while(1){
        //Read from Keyboard
        IF_KEYBOARD_read((CORBA_Object) keyboard_id, &kbbuf, &env);
        strncpy(inChars, kbbuf.chars, OUT_BUF_SIZE - 1);
        int numchars = kbbuf.numchars;
        printf("[SHELL] Read keyboard buffer. Got %d chars (\"%s\" - as int \"%i\"), more: %d\n", numchars, inChars, inChars[0], kbbuf.more);
        
        // Delete last Char if backspace
        CORBA_char* inPointer = inChars;
        for (int i = 0; i < OUT_BUF_SIZE; i++){
            if (inChars[i] != KEY_BACKSPACE) break;
            numchars--;
            IF_CONSOLE_delete((CORBA_Object) console_id, 2, &env);
            inPointer++;
        }
        
        //Print chars on Screen and put them into the command-buffer
        if(numchars > 0 && inPointer == inChars){
            printf("[SHELL] Printing buffer='%s' on screen. numchars=%i \n", *inPointer, numchars);
            IF_CONSOLE_delete((CORBA_Object) console_id, 1, &env); //delete cursor
            print (inPointer);
        }
        
        printCursor();

        //Store chars in buffer until we get a newline
        //if newline search for command


        if (!kbbuf.more)
            L4_Sleep(t);
    }
}

//Print Chars on the Screen
inline void print(char* c){ //ohne inline: user touches kernel area!?
    int numChars = strlen(c);
    //printf("numChars=%i\n", numChars);
    char* out = outChars;
    int i, k;
    // i mal Buffer mit k Zeichen füllen und senden
    for (i = 0; i < numChars; i += OUT_BUF_SIZE){
        out = outChars;
        for (k = 0; k < OUT_BUF_SIZE; k++)
            if ( !( * ( out++ ) = *( c++ ) ) )
                break;
        //printf ("test");
        IF_CONSOLE_write((CORBA_Object) console_id, outbuf, k, &env);
    }
    return;
}

//Handle special Chars like tab and backspace
void handleSpecialChars(char* c){
   return;
}

inline void printCursor(){
    char cur[2] = { CURSOR, '\0' };
    print(cur);
    return;
}



    
