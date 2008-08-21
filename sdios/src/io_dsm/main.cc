//
// File:  src/io_dsm/main.cc
//
// Description: IO-DataspaceManager
//

#include <l4/thread.h>
#include <l4/kip.h>
#include <l4io.h>
#include <l4/sigma0.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/iflogging.h>
#include <if/iftask.h>
#include <if/ifbielfloader.h>                                                                             

#include "../lib/io/ia32-port.h"

L4_ThreadId_t locatorid; 
L4_ThreadId_t taskserver_id;
L4_Word_t     test_stack[1024];

extern int consoleServer();

void testThread(){
    CORBA_Environment env (idl4_default_environment);
    printf("Test Thread up & running!\n");
    L4_ThreadId_t myId = L4_Myself();
    IF_TASK_kill((CORBA_Object) taskserver_id, &myId, &env);
    printf("I'm undead!");

    while(42);
}

int main () {
    printf ("IO Dataspace Manager is alive\n");

    /* Try to find all architecture specific memory and request them */

    // Request KIP
    L4_KernelInterfacePage_t* kip = (L4_KernelInterfacePage_t*)L4_KernelInterface ();

    // Iterate through descriptors
    printf("Searching for architecture specific memory...\n");
    L4_MemoryDesc_t * memdesc; 
    int size = 0;    
    L4_Fpage_t requestFpage;

    for(unsigned int i = 0; i < L4_NumMemoryDescriptors(kip); i++){
        memdesc = (L4_MemoryDesc_t *)L4_MemoryDesc(kip, i);
        if((L4_Type(memdesc) & L4_ArchitectureSpecificMemoryType) == L4_ArchitectureSpecificMemoryType){

            // print success message 
            printf("Found architecture specific type at %lx-%lx\n", memdesc->x.low << 10, memdesc->x.high << 10);

            // Calculate size of Memory
            size = L4_High(memdesc) - L4_Low(memdesc);

            // request memory from sigma0
            requestFpage = L4_Fpage(L4_Low(memdesc), size); 
            printf("[IO-DSM] Requesting fpage: %lx\n", requestFpage.raw);
            if(L4_IsNilFpage(L4_Sigma0_GetPage(L4_nilthread, requestFpage, requestFpage))){
                printf("[IO-DSM] Fpage request failed!\n");
            } else {
                printf("[IO-DSM] Fpage request succeeded!\n");
            }
        }
    }
    // Explicitly request VGA area
    requestFpage = L4_Fpage(0xb8000UL, 0x1000UL);
    printf("[IO-DSM] Requesting fpage: %lx\n", requestFpage.raw);
    if(L4_IsNilFpage(L4_Sigma0_GetPage(L4_nilthread, requestFpage, requestFpage))){
        printf("[IO-DSM] Fpage request failed!\n");
    } else {
        printf("[IO-DSM] Fpage request succeeded!\n");
    }
    printf("Finished searching for architecture spefic memory!\n");

    // Check for PCI-Bus
    /* 
    L4_Word32_t pcireg = inb(0xcfc);
    if(pcireg == 0xff){
        printf("There is something in the PCI space! Trying to fetch device information!\n");

        L4_Word_t pciCommand = (0x8000 << 16 | 0x0 << 11 | 0x0 << 8 | 0x00);
        outb(0xCF8, pciCommand);
        pcireg = inb(0xcfc);

        printf("Got response: %x\n", pcireg);

        // Printing some stuff from BDA
        unsigned short * pixel = (unsigned short * )0xb8000;
        * pixel = ('A' | 129 << 8); 
        
        pixel += 1;
        * pixel = ('A' | 255 << 8); 

       


    } else {
        printf("No PCI Bus found!\n");
    }
    */

    // FIXME: Remove
    CORBA_Environment env (idl4_default_environment);
    L4_ThreadId_t ram_dsm_id;                                    

    // Get locator id & Locate Taskserver
    ram_dsm_id = L4_Pager();       
    locatorid  = IF_BIELFLOADER_getLocator((CORBA_Object) ram_dsm_id, &env);
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_TASK_ID, &taskserver_id, &env);

    // Use Taskserver to create a new thred - for testing purposes only
    L4_ThreadId_t thread_id = IF_TASK_createThread((CORBA_Object) taskserver_id, &env);

    // Start thread 
    L4_Start(thread_id, (L4_Word_t)&test_stack[1024], (L4_Word_t)&testThread);

    // Wait for thread to terminate
    printf("Waiting for new thread to terminate...");
    L4_Word_t status = IF_TASK_waitTid((CORBA_Object) taskserver_id, &thread_id, &env);
    printf("... done!%lx\n", status);

    // Start console Server
    consoleServer();

    /* Spin forever */
    while (42);
    
    return 0;
}
