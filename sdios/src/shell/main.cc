//
// File:  src/shell/main.cc
//
// Description: Shell 
//
#include <l4io.h>
#include <idl4glue.h>
#include <string.h>
#include <if/iflocator.h>
#include <if/ifbielfloader.h>                                                                             

#include <if/ifconsole.h>
#include <if/ifkeyboard.h>
#include <if/iffilesystem.h>

#define MAX_COMMAND_LENGTH 200
#define OUT_BUF_SIZE 8
#define KEY_BACKSPACE 8
#define MAXFILE       256

CORBA_Environment env (idl4_default_environment);
L4_ThreadId_t locator_id, keyboard_id, console_id = L4_nilthread, filesystem_id = L4_nilthread; 
void shell_loop();
void print(char*);
void printPromt();

//Outbuffer
buffer_t* outbuf;
CORBA_char outChars[OUT_BUF_SIZE];

//Inbuffer
keyboardBuffer kbbuf;
CORBA_char inChars[OUT_BUF_SIZE];

// Dateibuffer
buffer_t * filenamebuf;
CORBA_char filenames[MAXFILE + 1][16];

//Commandbuffer
CORBA_char cmdChars[MAX_COMMAND_LENGTH];
CORBA_char* cmdPointer;


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

    while(L4_IsNilThread(filesystem_id))
        IF_LOCATOR_Locate((CORBA_Object) locator_id, IF_FILESYSTEM_ID, &filesystem_id, &env);
    printf("[SHELL] thinks that filesystem-server is at %lx \n", filesystem_id);
 

    printf("[SHELL] clearing screen \n");
    IF_CONSOLE_clear((CORBA_Object) console_id, &env);

    shell_loop();
}


void shell_loop(){
    L4_Time_t t = L4_TimePeriod (100000);
    cmdPointer = cmdChars;
    
    print("sdiOS/06 v0.1\n\n");
    print("System architecture Group\n");
    print("Universitaet Karlsruhe (TH)\n");
    print("Forschungsuniversitaet - gegruendet 1825\n");
    print("\n");
    print("(C) Copyright 2008 by Jan Tepelmann, Marcel Noe, Moritz Lapp\n");
    print("\n");

    print("Type \"?\" to see a list of available commands.\n\n");

    printPromt();
    printf("[SHELL] Entering Processing Loop \n");
    while(1){
        //Read from Keyboard
        IF_KEYBOARD_read((CORBA_Object) keyboard_id, &kbbuf, &env);
        strncpy(inChars, kbbuf.chars, OUT_BUF_SIZE - 1);
        int numchars = kbbuf.numchars;
        printf("[SHELL] Read keyboard buffer. Got %d chars (\"%s\" - as int \"%i\"), more: %d\n", numchars, inChars, inChars[0], kbbuf.more);
        
        // handle special chars
        // if backspace:    delete last char
        // if /n:           
        CORBA_char cleanChars[OUT_BUF_SIZE];
        for (int i = 0; i < OUT_BUF_SIZE; i++) cleanChars[i] = '\0'; //clear cleanChars
        CORBA_char* cleanPointer = cleanChars;
        bool exCmd = 0;
        for (int i = 0; i < numchars; i++){
            switch (inChars[i])
            {
                case KEY_BACKSPACE :    
                    if (cmdPointer > cmdChars){ 
                        cmdPointer--;
                        numchars--;
                        IF_CONSOLE_delete((CORBA_Object) console_id, 1, &env);
                    }
                    break;
                case '\n' :
                    *cleanPointer++ = inChars[i];
                    *cmdPointer++   = inChars[i];
                    *cmdPointer++   = '\0';
                    exCmd = 1;
                    break;
                case '\t' :
                    print("    ");
                    break;
                default :
                    *cleanPointer++ = inChars[i];
                    *cmdPointer++   = inChars[i];
            }
            
        }

        //Print chars on Screen
        if(numchars){
            printf("[SHELL] Printing buffer='%s' on screen. numchars=%i \n", cleanChars, numchars);
            print (cleanChars);
        }

        if (exCmd){

            // Command parsen
            int splitPos = strpos(cmdChars, ' ');
            int argLen = strlen(cmdChars) - splitPos - 2;
            char command[10];
            char argument[10];
            char * outpointer;                  

            // Command kopieren
            strncpy(command, cmdChars, (splitPos < 9 ? splitPos : 9));
            // Argument kopieren
            strncpy(argument, &cmdChars[splitPos + 1], (argLen < 9 ? argLen : 9));
            argument[(argLen < 9 ? argLen : 9)] = '\0';
            //printf("splitPos: %i, cmdLen: %i, argLen: %i, argument: %s", splitPos, strlen(cmdChars)-1, argLen, argument);

            switch(cmdChars[0]){
                case 'c': // Cat
                    print("!! CAT !!\n");
                    break;
                case 's': // Start
                    print("!! START !!\n");
                    break;
                case 'd': // Download
                    print("!! DOWNLOAD !!\n");
                    break;
                case 'r': // RM
                    IF_FILESYSTEM_deleteFile((CORBA_Object) filesystem_id, argument, &env);
                    break;
                case 't': // Touch
                    // Testweise eine daemliche Datei erstellen
                    IF_FILESYSTEM_createFile((CORBA_Object) filesystem_id, argument, 9000, &env);
                    
                    break;
                case 'l': // ls
                    // Anzeigen aller Dateien
                    filenamebuf->_maximum = (MAXFILE + 1) * 16;
                    filenamebuf->_length  = (MAXFILE + 1) * 16;
                    filenamebuf->_buffer  = (CORBA_char *)filenames;

                    IF_FILESYSTEM_listFiles((CORBA_Object) filesystem_id, filenamebuf, &env);

                    outpointer = (char *)filenamebuf->_buffer;                  
                    for(int i = 0; i < MAXFILE; i++){
                        if(outpointer[0]){
                            print(outpointer);
                            print("\n");
                        }
                        outpointer += 16;
                    }

                    break;
                case '?': // list all commands
                    print("Available Shell Commands:\n\nstart\ndownload\nrm $filename\ntouch $filename\ncat $filename\nls\n");
                    break;
                default :
                    print("Shell-Error: Command not found\n");
            }
            cmdPointer = cmdChars;
            printPromt();
        }
   
        if (!kbbuf.more)
            L4_Sleep(t);
    }
}

//Print Chars on the Screen
void print(char* c){ //ohne inline: user touches kernel area!?

    //printf("[SHELL] print(): %s\n", c); // Wenn man das entfernt kommt "User touches Kernel Area?!" 
    
    //Outbuffer
    //buffer_t* outbuf;
    //CORBA_char outChars[OUT_BUF_SIZE];
    
    outbuf->_maximum = OUT_BUF_SIZE;
    outbuf->_length = OUT_BUF_SIZE;
    outbuf->_buffer = outChars;
    
    int numChars = strlen(c);
    //printf("Print: numChars=%i\n", numChars);
    char * out = outChars;
    int i, k;
    // i mal Buffer mit k Zeichen füllen und senden
    for (i = 0; i < numChars; i += OUT_BUF_SIZE){
        out = outChars;
        for (k = 0; k < OUT_BUF_SIZE; k++)
            if ( !( * ( out++ ) = *( c++ ) ) )
                break;

        IF_CONSOLE_write((CORBA_Object) console_id, outbuf, k, &env);
    }
    return;
}

inline void printPromt(){
    print("# ");
    return;
}
