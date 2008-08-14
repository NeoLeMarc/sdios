//
// File:  src/taskserver/main.cc
//
// Description: Taskserver
//

#include <l4/thread.h>
#include <l4io.h>
#include <l4/schedule.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/ifbielfloader.h>
#include <if/iflogging.h>

#include "taskserver.h"

L4_ThreadId_t locatorid; 
L4_ThreadId_t ram_dsm_id; 
L4_ThreadId_t syscallid;

int main () {
    printf ("[TS] Taskserver is alive\n");
   
    CORBA_Environment env (idl4_default_environment);

    // Set RAM-DSM-ID
    ram_dsm_id = L4_Pager(); 
    printf("[TS] Got pager id 0x%08lx\n", ram_dsm_id);
    locatorid = IF_BIELFLOADER_getLocator((CORBA_Object) ram_dsm_id, &env); 
    printf("[TS] Got locator id 0x%08lx\n", locatorid);
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_SYSCALL_ID, &syscallid, &env);
    printf("[TS] Located syscall server at 0x%08lx\n", syscallid);

    // Register Taskserver with Locator
    L4_ThreadId_t taskserverId = L4_Myself();
    IF_LOCATOR_Announce((CORBA_Object) locatorid, IF_TASK_ID, &taskserverId, &env);     

    // Start Server Loop
    taskserver_server();

    return 0;
}
