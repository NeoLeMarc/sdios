//
// File:  src/shell/main.cc
//
// Description: Shell 
//
#include <l4io.h>
#include <idl4glue.h>
#include <string.h>
#include <l4/bootinfo.h>
#include <l4/sigma0.h>
#include <if/iflocator.h>
#include <if/ifbielfloader.h>                                                                             

#include <if/ifconsole.h>
#include <if/ifkeyboard.h>
#include <if/iffilesystem.h>
#include <if/iftask.h>

#define MAX_COMMAND_LENGTH 200
#define OUT_BUF_SIZE 8
#define KEY_BACKSPACE 8
#define MAXFILE       256

CORBA_Environment env (idl4_default_environment);
L4_ThreadId_t locator_id = L4_nilthread, keyboard_id = L4_nilthread, console_id = L4_nilthread;
L4_ThreadId_t filesystem_id = L4_nilthread, sigma0 = L4_nilthread, roottask = L4_nilthread; 
L4_ThreadId_t taskserver_id = L4_nilthread;

// Bootinfo
L4_BootInfo_t * bootInfo = 0;

void shell_loop();
void print(char*);
void printPromt();
void synRootPf(L4_Word_t physAddr, L4_Word_t target);


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

// Modulposition
char * modulePos[256];
L4_Word_t   moduleSize[256];
char * nextPos = (char *) 0x80000000;

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

    while(L4_IsNilThread(taskserver_id))
        IF_LOCATOR_Locate((CORBA_Object) locator_id, IF_TASK_ID, &taskserver_id, &env);
    printf("[SHELL] thinks that taskserver is at %lx \n", taskserver_id);

    // Get Bootinfo
    printf("[SHELL] Trying to map bootinfo from RootTask\n");
                                                      
    // Calculate ID of Root-Task by ThreadNo User Base                  
    L4_KernelInterfacePage_t * kip = (L4_KernelInterfacePage_t *)L4_KernelInterface();
    sigma0 = L4_GlobalId(kip->ThreadInfo.X.UserBase, 1);
    roottask = L4_GlobalId(kip->ThreadInfo.X.UserBase + 5, 1);                                            
    printf("[SHELL] Roottask should be: %lx\n", roottask.raw);                                          
                                                                                                          
    // Synthesize pagefault, want bootinfo at 1,5 GB                                                        
    synRootPf(L4_BootInfo(kip), 0x60000000);                                                              
                                                                                                          
    // Set pointer to bootinfo                                                                            
    bootInfo = (L4_BootInfo_t *)(0x60000000);                                                             
                                                                                                          
    // If necessary, bring in further pages to get complete bootinfo                                      
    L4_Word_t base = 1024;                                                                                
    while (base < bootInfo->size)                                                                         
        synRootPf(L4_BootInfo(kip) + base, 0x60000000 + base);                                            
                                                                                                          
    printf("[SHELL] Hopefully got mapping from RootTask\n"); 


    printf("[SHELL] clearing screen \n");
    IF_CONSOLE_clear((CORBA_Object) console_id, &env);

    shell_loop();
}


void shell_loop(){
    L4_Time_t t = L4_TimePeriod (100000);
    cmdPointer = cmdChars;

    print("sdiOS/06 v0.1\n\n");
    print("System Architecture Group\n");
    print("Universit\x84t Karlsruhe (TH)\n");
    print("Forschungsuniversit\x84t - gegr\x81ndet 1825\n");
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
        //printf("[SHELL] Read keyboard buffer. Got %d chars (\"%s\" - as int \"%i\"), more: %d\n", numchars, inChars, inChars[0], kbbuf.more);
        
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
            //printf("[SHELL] Printing buffer='%s' on screen. numchars=%i \n", cleanChars, numchars);
            print (cleanChars);
        }

        if (exCmd){

            // Command parsen
            int splitPos = strpos(cmdChars, ' ');

            // Es gibt maximal 256 Argumente
            int argc = 1;
            char * argv[256];
            argv[0] = cmdChars;

            for(int i = 0; cmdChars[i] != '\0'; i++){
                if(cmdChars[i] == ' '){
                   argv[argc]  = cmdChars + i + 1;
                   argc++;
                   cmdChars[i] = '\0';
                } else if(cmdChars[i] == '\n'){
                    cmdChars[i] = '\0';
                    break;
                }
            }

            switch(argv[0][0]){
                case 'c': // Cat
                    {
                        // Datei Mappen
                        L4_Fpage_t recvWindow = L4_FpageLog2(0x40000000, 18);
                        env._rcv_window = L4_MapGrantItems(recvWindow);

                        idl4_fpage_t datei;
                        IF_FILESYSTEM_mapFile((CORBA_Object) filesystem_id, argv[1], &datei, &env);  

                        // Datei ausgeben
                        print(reinterpret_cast<char *>(0x40000000));

                        // Datei unmappen
                        L4_Flush(recvWindow);

                    }

                    break;
                case 's': // Start
                    {
                        // Prepend a "/" to path - taskserver expects so.
                        *(--argv[1]) = '/';
                        const CORBA_char dummy = 0;
                        L4_ThreadId_t tid;
                        tid = IF_TASK_startTask((CORBA_Object) taskserver_id, argv[1], &dummy, &dummy, &env);
                        printf("[SHELL] Started %s as 0x%lx\n", argv[1], tid.raw);
                        IF_TASK_waitTid((CORBA_Object) taskserver_id, &tid, &env);
                    }
                    break;
                case 'd': // Download
                    {
                        // Module Nummer bestimmen
                        int moduleNr = 0;
                        for(int i = 0; argv[1][i] != '\0'; i++){
                            moduleNr *= 10;
                            moduleNr += argv[1][i] - '0';
                        }

                        // Datei anlegen
                        IF_FILESYSTEM_createFile((CORBA_Object) filesystem_id, argv[2], 1024 * 256, &env);

                        // Datei Mappen
                        L4_Fpage_t recvWindow = L4_FpageLog2(0x40000000, 18);
                        env._rcv_window = L4_MapGrantItems(recvWindow);

                        idl4_fpage_t datei;
                        IF_FILESYSTEM_mapFile((CORBA_Object) filesystem_id, argv[2], &datei, &env);  

                        // Bootmodul besorgen
                        void * module;
                        L4_Word_t      size = 0;

                        if(modulePos[moduleNr] == 0){
                            printf("[SHELL] Got boot info, trying to locate Module ID\n");
                                
                            if (L4_BootInfo_Entries (bootInfo) < moduleNr)
                                panic ("[SHELL] Some modules are missing\n");

                            printf("[SHELL] trying to locate first boot info entry\n");

                            L4_BootRec_t *  aModule; // Grub module storing the filesystem
                            aModule  = L4_BootInfo_FirstEntry (bootInfo);

                            printf("[SHELL] iterating through boot info\n");

                            for (unsigned int i = 0; i < moduleNr; i++){
                                aModule = L4_Next (aModule);
                            }

                            if (L4_Type(aModule) != L4_BootInfo_Module) {
                                panic("[SHELL] Module entry is not a \"Module\"");
                            }

                            L4_Boot_Module_t * bModule = (L4_Boot_Module_t *)aModule;

                            printf("[SHELL] found module at 0x%08lx\n", (L4_Word_t)bModule);

                            modulePos[moduleNr]  = (char *)nextPos;
                            moduleSize[moduleNr] = bModule->size;
                            nextPos += 1024 * 256;

                            for (L4_Word_t pos = 0; pos <= bModule->size; pos += 1024){
                                printf("[SHELL] requesting phys 0x%lx to 0x%lx\n", L4_FpageLog2(bModule->start + pos, 12), L4_FpageLog2((L4_Word_t)modulePos[moduleNr] + pos, 12));
                               if (L4_IsNilFpage(L4_Sigma0_GetPage(sigma0, L4_FpageLog2(bModule->start + pos, 12), L4_FpageLog2((L4_Word_t)modulePos[moduleNr] + pos, 12)))) 
                                   panic("sigma0 rejected request");
                            }

                            printf("[RAMDISK] should have got module mapped from sigma0\n");
                       }

                        module = modulePos[moduleNr];
                        size   = moduleSize[moduleNr];

                        // Inhalt kopieren
                        memcpy((void *)0x40000000, (void *)module, size);

                        // Datei unmappen
                        L4_Fpage_t dateiFpage = {raw : datei.fpage};
                        L4_Flush(dateiFpage);

                    }
                    break;
                case 'r': // RM
                    IF_FILESYSTEM_deleteFile((CORBA_Object) filesystem_id, argv[1], &env);
                    break;
                case 't': // Touch
                    // Testweise eine daemliche Datei erstellen
                    IF_FILESYSTEM_createFile((CORBA_Object) filesystem_id, argv[1], 9000, &env);
                    
                    break;
                case 'l': // ls
                    {
                        char * outpointer; 

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

// Synthesize pagefault to root task's RAM-DSM-Pager
inline void synRootPf(L4_Word_t physAddr, L4_Word_t target) {
    L4_Accept(L4_MapGrantItems(L4_FpageLog2(target, 12)));
    L4_LoadMR(0, 0x40002 + (-2UL << 20)); // Label -2, read, 2 untyped
    L4_LoadMR(1, physAddr); // fault address                
    L4_LoadMR(2, 0x12345678); // bogus IP                                  
    L4_Call(roottask);                                                                                
}    
