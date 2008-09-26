//
// File: lib/sdi/heap.cc
//
// Description: VERY simple heap implemenetation 
//

#include <stdlib.h>

#include <sdi/types.h>
#include <sdi/sdi.h>
#include <l4io.h>

/* Heap Managment */

// import symbols from crt0.S

extern char __heap_start;
extern char __heap_end;
extern void* __heap_ptr;
L4_Word_t lastsize = 0;

void* alloc (L4_Word_t size) {
    if (__heap_ptr==NULL)
    	bailout ("__heap_ptr not initialized check *-crt0.S , halted");
    void* ret = __heap_ptr;

    __heap_ptr = (void*)((L4_Word_t)__heap_ptr + size);

    if ((L4_Word_t)__heap_ptr > (L4_Word_t)&__heap_end)
	    bailout ("no more heap left, halted");

    printf("[HEAP]: Giving away pointer: %x (%i)\n", ret, size);
    return ret;
}

void free (void* freeptr) {
    //panic ("free () is not implemented. Put your code in here.");
    
    // Hack for macking free work for our filesystem
    printf("[HEAP] Getting back pointer: %x\n", freeptr);
}
