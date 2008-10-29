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
#include "filesystem.h"

L4_ThreadId_t locatorid; 

int main(){
    // Setup CORBA environment
    CORBA_Environment env (idl4_default_environment);
    L4_ThreadId_t ram_dsm_id, syscall_id, nilthread = L4_nilthread;

    // Get locator id & Locate Filesystem 
    ram_dsm_id = L4_Pager();       
    locatorid  = IF_BIELFLOADER_getLocator((CORBA_Object) ram_dsm_id, &env);

    // Get own ThreadID
    L4_ThreadId_t myId = L4_Myself();

    // Announce Simple-FS Server 
    IF_LOCATOR_Announce((CORBA_Object) locatorid, IF_FILESYSTEM_ID, &myId, &env);

    // Become Filesystem Server
    filesystem_server();
}
