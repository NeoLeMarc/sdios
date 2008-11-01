//
// File:  src/consoleserver/main.cc
//
// Description: Console Server 
//

#include <l4/thread.h>
#include <l4/kip.h>
#include <l4io.h>
#include <l4/sigma0.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/ifsyscall.h>
#include <if/iftask.h>
#include <if/ifbielfloader.h>                                                                             

#include "../lib/io/ia32-port.h"
#include "keyboard.h"

#include <if/ifconsole.h>

L4_ThreadId_t locatorid; 
L4_ThreadId_t taskserver_id;
L4_Word_t   keyboard_stack[1024];
L4_Word_t   console_stack[1024];
void console_server();

int main(){
    // Setup CORBA environment
    CORBA_Environment env (idl4_default_environment);
    L4_ThreadId_t ram_dsm_id, syscall_id, nilthread = L4_nilthread;

    // Get locator id & Locate Taskserver
    ram_dsm_id = L4_Pager();       
    locatorid  = IF_BIELFLOADER_getLocator((CORBA_Object) ram_dsm_id, &env);
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_TASK_ID, &taskserver_id, &env);
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_SYSCALL_ID, &syscall_id, &env);
    
    
    // --- Start the console server as new thread ---
    // Use Taskserver to create a new thread
    L4_ThreadId_t console_id = IF_TASK_createThread((CORBA_Object) taskserver_id, &env);

    // Start thread 
    L4_Start(console_id, (L4_Word_t)&console_stack[1024], (L4_Word_t)&console_server);

    // Announce console server
    IF_LOCATOR_Announce((CORBA_Object) locatorid, IF_CONSOLE_ID, &console_id, &env);


    // --- Start keyboard server ---
    // Use Taskserver to create a new thread
    L4_ThreadId_t keyboard_id = IF_TASK_createThread((CORBA_Object) taskserver_id, &env);

    // Start thread 
    L4_Start(keyboard_id, (L4_Word_t)&keyboard_stack[1024], (L4_Word_t)&keyboard_server);

    // Associate interrupt 1 with the keyboard server
    L4_ThreadId_t kbdInterrupt = L4_GlobalId(1, 1);
    IF_SYSCALL_ThreadControl((CORBA_Object) syscall_id, &kbdInterrupt, &kbdInterrupt, &nilthread, &keyboard_id, -1, &env);

    // Announce keyboard server for test client
    IF_LOCATOR_Announce((CORBA_Object) locatorid, IF_KEYBOARD_ID, &keyboard_id, &env);

    /*
    // --- Testing ---
    L4_Time_t t = L4_TimePeriod (1000000);
    buffer_t* test;
   
    printf("[DEBUG] trying to clear the screen\n");
    IF_CONSOLE_clear((CORBA_Object) console_id, &env);
    int i = 0;
    for(i = 0; i < 1; i++){ //wenn hier statt der "1" eine "2" steht -> user touches kernel area - keine Ahnung wieso?
       printf("test %s\n", test->_buffer);
    }
    */

    /*
    CORBA_char ctest[60];
    strncpy(ctest, "Testing Console... printing all available symbols ...\n", 54);
    test->_buffer = ctest;
    test->_length = 54;
    IF_CONSOLE_write((CORBA_Object) console_id, test, &env);

    for(i = 49; i < 128; i++){
       //char* symbol;
       //snprintf(symbol, 1, "%c", i);
       strncpy(ctest, (const char*) i, 1);
       //printf("i=%i, i as char=%c, symbol=%c \n", i, i, *symbol);
       test->_buffer = ctest;
       test->_length = 1;
       IF_CONSOLE_write((CORBA_Object) console_id, test, &env);
    }
    */
    
    
    while (42);
}
