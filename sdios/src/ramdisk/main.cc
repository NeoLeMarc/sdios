//
// File:  src/ramdisk/main.cc
//
// Description: Ramdisk Server 
//

#include <l4/thread.h>
#include <l4/kip.h>
#include <l4io.h>
#include <l4/sigma0.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/ifbielfloader.h>
#
#include "ramdisk.h"


#define FILESYSTEM_MODULE_ID 8

L4_ThreadId_t locatorid; 
L4_ThreadId_t ram_dsm_id; 
L4_ThreadId_t syscallid;

int main(){
    printf("[RAMDISK] started\n");

    CORBA_Environment env (idl4_default_environment);

    // Set RAM-DSM-ID
    ram_dsm_id = L4_Pager(); 
    locatorid = IF_BIELFLOADER_getLocator((CORBA_Object) ram_dsm_id, &env); 
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_SYSCALL_ID, &syscallid, &env);

    // Register Ramdisk with Locator
    L4_ThreadId_t taskserverId = L4_Myself();
    IF_LOCATOR_Announce((CORBA_Object) locatorid, IF_BLOCK_ID, &taskserverId, &env);     

    // get filesystem bootloader module 
    L4_BootRec_t *  filesystemModule;
    L4_BootInfo_t * bootinfo = (L4_BootInfo_t *)L4_BootInfo(L4_KernelInterface());

    printf("[RAMDISK] registered with locator\n");

        
    if (L4_BootInfo_Entries (bootinfo) < FILESYSTEM_MODULE_ID)
        panic ("[RAMDISK] Some modules are missing\n");

    filesystemModule  = L4_BootInfo_FirstEntry (bootinfo);

    for (unsigned int i = 0; i < FILESYSTEM_MODULE_ID; i++){
        filesystemModule = L4_Next (filesystemModule);
    }

    printf("[RAMDISK] found filesystem module at 0x%08lx\n", filesystemModule);

    // start ramdisk server
   // ramdisk_server(filesystemModule);
   ramdisk_server(filesystemModule);
}
