//
// File:  src/taskserver/main.cc
//
// Description: Taskserver
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
    printf ("Taskserver is alive\n");
    
    /* Spin forever */
    while (42);
    
    return 0;
}
