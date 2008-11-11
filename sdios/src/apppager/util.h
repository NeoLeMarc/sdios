#ifndef __apppager_util_h__
#define __apppager_util_h__
#include <l4/types.h>
#include <stdlib.h>
#include <new.h>

class Thread;

class Page {
    static Page * globalFirstFreePage;
    static L4_Word_t nextAllocablePageStart;
    L4_Fpage_t desc;
    Page() { }
    Page(L4_Fpage_t descriptor) : desc(descriptor), nextPage(NULL) {}
public:
    Page * nextPage;

    L4_Fpage_t getDesc() { return desc; }
    void free();
    static Page * alloc();
};

class Space {
    static Space * globalFirstFreeSpace;
    static L4_Word_t nextAllocableFileSlot;
    Space * globalNextFreeSpace;
    Page * firstPage;

    L4_Fpage_t fileSlot;
    Space() : globalNextFreeSpace(NULL), firstPage(NULL), firstThread(NULL) {};

public:
    Thread * firstThread;

    L4_Fpage_t getFileSlot() { return fileSlot; }
    void addPage(Page * page);

    static Space * alloc(Thread * firstThread);
    void free();
};

class Thread {
    static Thread * globalFirstThread;
    Thread * globalNextThread;

public:
    Thread * nextThread;
    L4_ThreadId_t globalid;
    Space * space;

    Thread(L4_ThreadId_t globalid);
    Thread(L4_ThreadId_t globalid, Space * inSpace);
    ~Thread();
    static Thread * find(L4_ThreadId_t globalid);
};
#endif
