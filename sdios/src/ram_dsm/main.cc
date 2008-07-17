//
// File:  src/ram_dsm/main.cc
//
// Description: RAM-DataspaceManager
//

#include <l4/thread.h>
#include <l4/sigma0.h>
#include <l4io.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/iflogging.h>
#include "ram_dsm.h"

L4_ThreadId_t locatorid; 

// Bitmap to store available state
// calculate bitmap size via RAM size / 4KB (page size) / 32 (bits per word) 
#define AVAILABLE_BITMAP_SIZE 256
L4_Word_t available[ASSIGNED_BITMAP_SIZE];

int main () {
    printf ("RAM Dataspace Manager is alive\n");

    /* Try to get Whole Memory from sigma0 */
    for (L4_Word_t i = 0UL; i < 0x02000000UL; i += 0x1000UL) {
        if (!L4_IsNilFpage(L4_Sigma0_GetPage(L4_nilthread, L4_FpageLog2(0UL, 12), L4_FpageLog2(0UL, 12)))) {
            // page size is 2^12, 2^5 bits per word
            available[i >> 17] |= (1UL << ((i >> 12) & 0x1fUL));
        }
    }

    // Start BI-ELF-Loader
    printf("Start Boot-Image-ELF-Loader...\n");
    bielfloader_server(&available);
    printf("Oooooooops, your ELF-Loader just died!\n");


    /* Start Pager */
//    printf("Starting pager...\n");
//    pager_server();
//    printf("Oooops, your pager just died!\n");

    /* Spin forever */
    while (42);
    
    return 0;
}
