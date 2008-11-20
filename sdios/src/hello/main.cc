//
// File:  src/test/main.cc
//
// Description: Testclient
//

#include <l4/thread.h>
#include <l4io.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/iftask.h>
#include <if/ifapppager.h>                                                                             
#include <if/ifconsole.h>                                                                             
#include <sdi/keyboard.h>


L4_ThreadId_t locatorid; 
L4_ThreadId_t taskserver_id;
L4_ThreadId_t console_id;
void print(char*);

int main () {
    // Setup CORBA environment
    CORBA_Environment env (idl4_default_environment);
    L4_ThreadId_t pager_id, myself = L4_Myself();

    // Get locator id & Locate Ramdisk 
    pager_id = L4_Pager();       
    locatorid  = IF_APPPAGER_getLocator((CORBA_Object) pager_id, &env);
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_TASK_ID, &taskserver_id, &env);
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_CONSOLE_ID, &console_id, &env);
    
    print("Hallo Welt\n");
    
    // Commit suicide as finish
    IF_TASK_kill((CORBA_Object) taskserver_id, &myself, &env);
    return 0;
}

//Print Chars on the Screen
#define OUT_BUF_SIZE 8
//Outbuffer
buffer_t* outbuf;
CORBA_char outChars[OUT_BUF_SIZE];
void print(char* c){ 
    CORBA_Environment env (idl4_default_environment);

    outbuf->_maximum = OUT_BUF_SIZE;
    outbuf->_length = OUT_BUF_SIZE;
    outbuf->_buffer = outChars;
    
    int numChars = strlen(c);
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
