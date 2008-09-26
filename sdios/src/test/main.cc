//
// File:  src/test/main.cc
//
// Description: Testclient
//

#include <l4/thread.h>
#include <l4io.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include <if/iflocator.h>
#include <if/iflogging.h>
#include <if/iftask.h>
#include <if/ifbielfloader.h>                                                                             
#include <if/ifblock.h>                                                                             
#include <if/ifkeyboard.h>                                                                             
#include <sdi/keyboard.h>


L4_ThreadId_t locatorid; 
L4_ThreadId_t taskserver_id;
L4_ThreadId_t ramdisk_id;
L4_ThreadId_t keyboard_id;

int main () {
    printf("[TESTCLIENT] is alive\n");
    printf("[TESTCLIENT] Beginning system component tests\n");
    printf("*********************************************\n");

    // Give system time to initialize
    L4_Time_t t = L4_TimePeriod (5000000);
    L4_Sleep(t);

    // Setup CORBA environment
    CORBA_Environment env (idl4_default_environment);
    L4_ThreadId_t ram_dsm_id;

    // Get locator id & Locate Ramdisk 
    ram_dsm_id = L4_Pager();       
    locatorid  = IF_BIELFLOADER_getLocator((CORBA_Object) ram_dsm_id, &env);
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_TASK_ID, &taskserver_id, &env);
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_BLOCK_ID, &ramdisk_id, &env);
    IF_LOCATOR_Locate((CORBA_Object) locatorid, IF_KEYBOARD_ID, &keyboard_id, &env);
    
    printf("[TESTCLIENT] Located ramdisk at 0x%08lx\n", ramdisk_id.raw);

    // Talk to ramdisk
    //
    // Get blocksize
    int blocksize = IF_BLOCK_getBlockSize((CORBA_Object) ramdisk_id, &env);
    printf("[TESTCLIENT] Ramdisk blocksize is %i\n", blocksize); 

    // Read a block
    char block[4096];
    IF_BLOCK_readBlock((CORBA_Object) ramdisk_id, 1, (buffer_t *)block, &env);
    printf("** Starting block debug **\n");
    //printf("%s", block);
    printf("\n** End of block debug **\n");
    
    printf("*********************************************\n");
    printf("[TESTCLIENT] End of system component tests\n");

    // Read some chars from keyboard
    keyboardBuffer kbbuf;
    char mybuf[8];
    t = L4_TimePeriod (2000000);
    while (42) {
        IF_KEYBOARD_read((CORBA_Object) keyboard_id, &kbbuf, &env);
        strncpy(mybuf, kbbuf.chars, 7);
        printf("[TESTCLIENT] Read keyboard buffer. Got %d chars (\"%s\"), more: %d\n", kbbuf.numchars, mybuf, kbbuf.more);
        if (!kbbuf.more) 
            L4_Sleep(t);

    }
    /* Spin forever */
    while (42);
    
    return 0;
}
