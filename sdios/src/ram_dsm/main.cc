//
// File:  src/ram_dsm/main.cc
//
// Description: RAM-DataspaceManager
//

#include <l4/thread.h>
#include <l4io.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/iflogging.h>

L4_ThreadId_t locatorid; 

int main () {
    printf ("RAM-DatespaceManager is alive\n");

    /* Try to get all memory from sigma0 */
    L4_Msg_t requestMemoryMsg;

    /* Build message */
    L4_Word_t registers[2];
    registers[0] = 0;
    registers[1] = 0;

    /* Request whole Memory */
    registers[0] = L4_FpageLog2(-1, 32).raw;

    /* Put into requestMemoryMsg */
    L4_MsgPut(&requestMemoryMsg, -6 << 4, 2, registers, 0, NULL);

    /* Load into registers */
    L4_Load(&requestMemoryMsg);

    /* Accept mapping */
    L4_Accept(L4_MapGrantItems(L4_CompleteAddressSpace));

    /* Send & Receive IPC */
    L4_ThreadId_t sigma0 = L4_GlobalId(0x000c0001 >> 14, 1);
    L4_Call(sigma0);

    /* Print response from sigma0 */
    printf("Error Code: %i\n", (int)L4_ErrorCode());

    printf("If you are here, then we propapbly got a mapping from sigma0!\n");
    
    /* Spin forever */
    while (42);
    
    return 0;
}
