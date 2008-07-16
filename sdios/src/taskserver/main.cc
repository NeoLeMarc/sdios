//
// File:  src/taskserver/main.cc
//
// Description: Taskserver
//

#include <l4/thread.h>
#include <l4io.h>
#include <l4/schedule.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/iflogging.h>

L4_ThreadId_t locatorid; 

int main () {
    printf ("Taskserver is alive\n");
    
    /* Spin forever */
    L4_Time_t t = L4_TimePeriod (1000000);
    while (42) {
        L4_Sleep(t);
	L4_Yield();
    }
    
    return 0;
}
