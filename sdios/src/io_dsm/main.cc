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

L4_ThreadId_t locatorid; 

void RequestPage(int base, int size){
    /* Try to get all memory from sigma0 */
    L4_Msg_t requestMemoryMsg;

    /* Build message */
    L4_Word_t registers[2];
    registers[0] = 0;
    registers[1] = 0; //L4_UncacheableMemory;

    /* Request whole Memory */
    registers[0] = L4_Fpage(base, size).raw;

    /* Put into requestMemoryMsg */
    L4_MsgPut(&requestMemoryMsg, -6 << 4, 2, registers, 0, NULL);

    /* Load into registers */
    L4_Load(&requestMemoryMsg);

    /* Accept mapping idempotently */
    L4_Accept(L4_MapGrantItems(L4_Fpage(base, size)));

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
   
    /* Spin forever */
    while (42);
    
    return 0;
}
