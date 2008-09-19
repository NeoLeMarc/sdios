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

#include "ramdisk.h"

#define FILESYSTEM_MODULE_ID 8

int main(){
    printf("[RAMDISK] started\n");

    // get filesystem bootloader module 
    L4_BootRec_t *  filesystemModule;
    L4_BootInfo_t * bootinfo = (L4_BootInfo_t *)L4_BootInfo(L4_KernelInterface());
        
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
