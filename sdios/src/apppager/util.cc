#include "util.h"
#include <l4/space.h>
// implementation for Page class {{{

Page * Page::globalFirstFreePage = NULL;
L4_Word_t Page::nextAllocablePageStart = 0x40000000;

Page * Page::alloc() {
    Page * freePage = globalFirstFreePage;
    if (freePage == NULL) {
        // don't go too far...
        if (nextAllocablePageStart >= 0x80000000)
            return NULL;
        // touch free space
        *reinterpret_cast<L4_Word_t*>(nextAllocablePageStart) = 0;
        freePage = new Page(L4_FpageLog2(nextAllocablePageStart, 12));
        nextAllocablePageStart += 0x1000;
    } else {
        // reuse already available page
        // dequeue page
        globalFirstFreePage = freePage->nextPage;
        freePage->nextPage = NULL;
    }
    return freePage;
}

void Page::free() {
    Page * p = this;
    while (p->nextPage != NULL)
        p = p->nextPage;
    p->nextPage = globalFirstFreePage;
    globalFirstFreePage = this;
}
// }}}

// implementation for Thread class {{{

// initialise list head
Thread * Thread::globalFirstThread = NULL;

Thread::Thread(L4_ThreadId_t global_id) {
    // prepend to global list
    globalNextThread = globalFirstThread;
    globalFirstThread = this;

    // initialize
    this->nextThread = NULL;
    this->globalid = global_id;

    // first thread, allocate space
    this->space = Space::alloc(this);
}

Thread::Thread(L4_ThreadId_t global_id, Space * inSpace) {
    // prepend to global list
    globalNextThread = globalFirstThread;
    globalFirstThread = this;

    // prepend to space list
    nextThread = inSpace->firstThread;
    inSpace->firstThread = this;

    // initialize
    this->globalid = global_id;
    this->space = inSpace;
}

Thread::~Thread() {
    // remove from global thread list
    Thread * p = globalFirstThread;
    if (p == NULL)
        return;
    else if (p == this)
        globalFirstThread = globalNextThread;
    else {
        while (p->globalNextThread != NULL) {
            if (p->globalNextThread == this) {
                p->globalNextThread = globalNextThread;
                break;
            }
            p = p->globalNextThread;
        }
    }

    // remove from space-local thread list
    p = space->firstThread;
    if (p == NULL)
        return;
    else if (p == this)
        space->firstThread = nextThread;
    while (p->nextThread != NULL) {
        if (p->nextThread == this) {
            p->nextThread = nextThread;
            break;
        }
        p = p->nextThread;
    }

    // free space if empty
    if (space->firstThread == NULL) 
        space->free();
}

Thread * Thread::find(L4_ThreadId_t globalid) {
    Thread * p = globalFirstThread;
    while (p != NULL) {
        if (p->globalid.raw == globalid.raw) {
            return p;
        }
        p = p->globalNextThread;
    }
    return NULL;
}
// }}}

// implementation for Space class {{{

// initialise list head
Space * Space::globalFirstFreeSpace = NULL;
L4_Word_t Space::nextAllocableFileSlot = 0x80000000;

void Space::addPage(Page * page) {
    // prepend to list
    page->nextPage = firstPage;
    firstPage = page;
}

Space * Space::alloc(Thread * firstThread) {
    Space * freeSpace = globalFirstFreeSpace;
    if (freeSpace == NULL) {
        // don't go too far...
        if (nextAllocableFileSlot >= 0xa0000000)
            return NULL;
        freeSpace = new Space();
        freeSpace->fileSlot = L4_FpageLog2(nextAllocableFileSlot, 18);
    } else {
        // reuse already existing space template and file slot
        globalFirstFreeSpace = freeSpace->globalNextFreeSpace;
        freeSpace->globalNextFreeSpace = NULL;
    }

    freeSpace->firstThread = firstThread;
    freeSpace->firstPage = NULL;
    return freeSpace;
}

void Space::free() {
    // Page::free() takes care of the whole list
    firstPage->free();

    // prepend to free space list
    globalNextFreeSpace = globalFirstFreeSpace;
    globalFirstFreeSpace = this;
    L4_UnmapFpage(fileSlot);
}

// }}}
