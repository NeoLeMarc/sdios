//
// File:  src/io_dsm/main.cc
//
// Description: IO-DataspaceManager
//

#include <l4/thread.h>
#include <l4/kip.h>
#include <l4io.h>

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

void testThread(){
    CORBA_Environment env (idl4_default_environment);
    printf("Test Thread up & running!\n");
    L4_ThreadId_t myId = L4_Myself();
    IF_TASK_kill((CORBA_Object) taskserver_id, &myId, &env);
    printf("I'm undead!");

    while(42);
}

void RequestPage(int base, int size){
    /* Try to get all memory from sigma0 */
    L4_Msg_t requestMemoryMsg;

    /* Build message */
    L4_Word_t registers[2];
    registers[0] = 0;
    registers[1] = 0; //L4_UncacheableMemory;

    /* Request whole Memory */
    L4_Fpage_t fpage = L4_Fpage(base, size);
    registers[0] = fpage.raw;

    /* Put into requestMemoryMsg */
    L4_MsgPut(&requestMemoryMsg, -6 << 4, 2, registers, 0, NULL);

    /* Load into registers */
    L4_Load(&requestMemoryMsg);

    /* Accept mapping idempotently */
    L4_Accept(L4_MapGrantItems(fpage));

    /* Send & Receive IPC */
    L4_ThreadId_t sigma0 = L4_GlobalId(0x000c0001 >> 14, 1);
    L4_Call(sigma0);

    /* Print response from sigma0 */
    printf("Error Code: %i\n", (int)L4_ErrorCode());

    printf("If you are here, then you probably got a mapping from sigma0!\n");
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

    for(unsigned int i = 0; i < L4_NumMemoryDescriptors(kip); i++){
        memdesc = (L4_MemoryDesc_t *)L4_MemoryDesc(kip, i);
        if((L4_Type(memdesc) & L4_ArchitectureSpecificMemoryType) == L4_ArchitectureSpecificMemoryType){

            // print success message 
            printf("Found architecture specific type at %x-%x\n", L4_Low(memdesc), L4_High(memdesc));

            // Calculate size of Memory
            size = L4_High(memdesc) - L4_Low(memdesc);

            // request memory from sigma0
            RequestPage(L4_High(memdesc), size); 
        }
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

    /* Spin forever */
    while (42);
    
    return 0;
}
