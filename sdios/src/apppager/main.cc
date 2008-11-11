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
#include <if/iflogging.h>
#include <if/ifbielfloader.h>                                                                             

#include "apppager.h"

L4_ThreadId_t locatorid; 
L4_ThreadId_t fsysid; 

int main () {
    printf ("[AP] Apppager is alive\n");
   
    CORBA_Environment env (idl4_default_environment);

    locatorid = IF_BIELFLOADER_getLocator((CORBA_Object) L4_Pager(), &env); 
    printf("[AP] Got locator id 0x%08lx\n", locatorid.raw);

    // Get Filesystem id
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_FILESYSTEM_ID, &fsysid, &env);
    printf("[AP] Got filesystem id 0x%08lx\n", fsysid.raw);

    // Register Taskserver with Locator
    L4_ThreadId_t apppagerId = L4_Myself();
    IF_LOCATOR_Announce((CORBA_Object) locatorid, IF_APPPAGER_ID, &apppagerId, &env);     

    // Start Server Loop
    apppager_server();
    
    return 0;
}
