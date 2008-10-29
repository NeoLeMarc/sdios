//
// File: lib/sdi/heap.cc
//
// Description: No more that simple heap implemenetation 
//

#include <stdlib.h>

#include <sdi/types.h>
#include <sdi/sdi.h>
#include <l4io.h>

/* Heap Managment */

// import symbols from crt0.S

extern char __elf_end;


// Declarations
typedef long Align;         // for alignment to long boundary
union header {              // block header:
    struct {
        union header * ptr; // next block if on free list
        unsigned long size;  // size of this block
    } s;
    Align x;            // force alignment of blocks
};
typedef union header Header;

void * alloc (unsigned long nbytes);
void free (void *ap);

// Definitions
static Header *freep = NULL;    // start of free list

/// malloc: general-purpose storage allocator
void * alloc (unsigned long nbytes)
{
    Header *p, *prevp;
    unsigned long nunits;

    nunits = (nbytes + sizeof(Header)-1)/sizeof(Header) + 1;
    if ((prevp = freep) == NULL) {  // no free list yet
        // Highly proprietary... everything from __elf_end to 0x3fffffff
        // gets managed by the heap manager
        freep = (Header *)&__elf_end + 1;
        freep->s.ptr = prevp = freep;
        freep->s.size = ((char *)0x3fffffff - (char *)freep)/sizeof(Header);
    }
    for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
        if (p->s.size >= nunits) {  // big enough
            if (p->s.size == nunits)    // exactly
                prevp->s.ptr = p->s.ptr;
            else {              // allocate tail end
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            freep = prevp;
            return (void *)(p+1);
        }
        if (p == freep) // wrapped around free list
            return NULL;    // none left
    }
}

#define NALLOC 1024 // minimum #units to request

// free: put block ap in free list
void free (void * ap)
{
    Header *bp, *p;

    bp = (Header *)ap - 1;  // point to block header
    for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
            break; // freed block at start or end of arena
    
    if (bp + bp->s.size == p->s.ptr) { // join to upper neighbor
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else
        bp->s.ptr = p->s.ptr;
    if (p + p->s.size == bp) {  // join to lower neighbor
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else
        p->s.ptr = bp;
    freep = p;
}
