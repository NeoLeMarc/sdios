//
// File:  src/root/main.cc
//
// Description: a VERY simple root task for sdi
//

#include <l4/thread.h>
#include <l4/space.h>
#include <l4/message.h>
#include <l4/ipc.h>
#include <l4/sigma0.h>
#include <l4/bootinfo.h>
#include <l4/types.h>
#include <l4/schedule.h>

#include <stdlib.h>

#include <sdi/sdi.h>

#include <l4io.h>
#include <elf.h>
#include "root.h"

#include <idl4glue.h>

#include <if/iflocator.h>
#include <if/ifbielfloader.h>
#include <if/iftask.h>

/* local threadids */
L4_ThreadId_t sigma0id;
L4_ThreadId_t locatorid;
L4_ThreadId_t syscallid;

// not so local, but needed to start other tasks
L4_ThreadId_t ram_dsm_id;
L4_ThreadId_t taskserver_id;

L4_Word_t pagesize;
L4_Word_t utcbsize;
L4_Fpage_t kiparea;
L4_Fpage_t utcbarea;

/* helperstuff */

extern char __elf_start;
extern char __elf_end;

extern char __heap_start;
extern char __heap_end;


L4_Word_t next_thread_no;
L4_Word_t syscall_stack[1024];
L4_Word_t locator_stack[1024];
L4_Word_t ram_dsm_pager_stack[1024];


L4_ThreadId_t start_thread (L4_ThreadId_t threadid, L4_Word_t ip, L4_Word_t sp, L4_ThreadId_t pagerid, void* utcblocation) {
    printf ("New thread with ip:%lx / sp:%lx\n", ip, sp);
    /* do the ThreadControl call */
    if (!L4_ThreadControl (threadid, L4_Myself (),  L4_Myself (), pagerid,
		      (void*)utcblocation ))
	panic ("ThreadControl failed");
    /* set thread on our code */
    L4_Start (threadid, sp, ip);

    return threadid;
}

L4_ThreadId_t start_task (L4_ThreadId_t threadid, L4_Word_t ip, L4_ThreadId_t pagerid, L4_Fpage_t nutcbarea) {
    printf ("New task with ip:%lx\n", ip);
    /* First ThreadControl to setup initial thread */
    if (!L4_ThreadControl (threadid, threadid, L4_Myself (), L4_nilthread, (void*)-1UL))
	panic ("ThreadControl failed\n");

    L4_Word_t dummy;

    if (!L4_SpaceControl (threadid, 0, L4_FpageLog2 (0xB0000000,14), 
			   utcbarea, L4_anythread, &dummy))
	panic ("SpaceControl failed\n");

    /* Second ThreadControl, activate thread */
    if (!L4_ThreadControl (threadid, threadid, L4_nilthread, pagerid, 
			   (void*)L4_Address (nutcbarea)))
	panic ("ThreadControl failed\n");

    /* send startup IPC */
    /* only the pager of the thread can send the startup IPC - we have to use IPC Propagation */
    L4_Msg_t msg;
    L4_Clear (&msg);
    L4_MsgTag_t mtag;
    L4_Set_Propagation (&mtag);
    L4_Set_VirtualSender (pagerid);
    L4_Append (&msg, ip);
    L4_Append (&msg, (0));
    L4_Set_MsgTag (&msg, mtag);
    L4_Load (&msg);
    L4_Send (threadid);

    return threadid;
}

L4_Bool_t request_page (L4_Word_t addr) {
    return !(L4_IsNilFpage (L4_Sigma0_GetPage (sigma0id,
					       L4_Fpage (addr, pagesize))));
}

void list_modules (const L4_BootInfo_t* bootinfo) {
    L4_BootRec_t* bootrec = L4_BootInfo_FirstEntry (bootinfo);
    for (unsigned int i=0; i < L4_BootInfo_Entries (bootinfo); i++) {
	printf ("Module: start %lx size %lx type: %d\n", 
		L4_Module_Start (bootrec),
		L4_Module_Size (bootrec),
		(int)L4_Type (bootrec));
	bootrec = L4_Next (bootrec);
    }
}

L4_BootRec_t* find_module (unsigned int index, const L4_BootInfo_t* bootinfo) {
    if (L4_BootInfo_Entries (bootinfo) < index) 
	panic ("Some modules are missing");
    L4_BootRec_t* bootrec = L4_BootInfo_FirstEntry (bootinfo);
    for (unsigned int i = 0; i < index; i++)
	bootrec = L4_Next (bootrec);
    return bootrec;
}

L4_Word_t load_elfimage (L4_BootRec_t* mod) {
    /* Check type of module */
    if (L4_Type (mod) != L4_BootInfo_Module)
	panic ("Wrong module type");
    /* Bring in the memory from sigma0 */
    for (L4_Word_t addr = L4_Module_Start (mod); 
	 addr < L4_Module_Start (mod) + L4_Module_Size (mod); 
	 addr += pagesize) 
	if (!request_page (addr)) {
	    panic ("could not get module pages from sigma0");
	}
    
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)L4_Module_Start (mod);
    Elf32_Phdr* phdr;
    if ((hdr->e_ident[EI_MAG0] !=  ELFMAG0) || 
	(hdr->e_ident[EI_MAG1] !=  ELFMAG1) || 
	(hdr->e_ident[EI_MAG2] !=  ELFMAG2) ||
	(hdr->e_ident[EI_MAG3] !=  ELFMAG3)) {
	return NULL;
    }
    if (hdr->e_type != ET_EXEC) { return NULL; }
    if (hdr->e_machine != EM_386) { return NULL; }
    if (hdr->e_version != EV_CURRENT) { return NULL; }
    if (hdr->e_flags != 0) { return NULL; }
    phdr = (Elf32_Phdr *) (hdr->e_phoff + (unsigned int) hdr);
    if (hdr->e_phnum <= 0) {
	return NULL;
    }
    for (int i = 0; i < hdr->e_phnum; i++) {
	if (phdr[i].p_type == PT_LOAD) {
	    L4_Word_t fstart, mstart;
	    /* found a loadable section */

	    /* figure out the start and end of image in the "file" */
	    fstart = (L4_Word_t)hdr + phdr[i].p_offset;

	    /* figure out the start and end of the final image in memory */
	    mstart =  phdr[i].p_vaddr;

	    printf ("Module psection: fstart: %lx -> mstart: %lx\n", fstart, mstart);

	    /* copy the image of the file */
	    memcpy((void*) mstart, (void*) fstart, phdr[i].p_filesz);

	    /* zero fill the rest(if any) of the destination region */
	    memset((void*)(mstart+phdr[i].p_filesz),0,phdr[i].p_memsz - phdr[i].p_filesz);
	}
    }
    return (hdr->e_entry);
}

void activate_module (L4_ThreadId_t threadId, L4_Word_t moduleId) {
    L4_Word_t module_startip = 0; 

    // First ThreadControl to setup initial thread
    if (!L4_ThreadControl (threadId, threadId, L4_Myself (), L4_nilthread, (void*)-1UL))
	panic ("ThreadControl failed\n");

    L4_Word_t dummy;

    if (!L4_SpaceControl (threadId, 0, L4_FpageLog2 (0xB0000000,14), 
			   utcbarea, L4_anythread, &dummy))
	panic ("SpaceControl failed\n");

    // Second ThreadControl, activate thread //
    if (!L4_ThreadControl (threadId, threadId, L4_nilthread, ram_dsm_id, 
			   (void*)L4_Address (utcbarea)))
	panic ("ThreadControl failed\n");

    CORBA_Environment env (idl4_default_environment);
    IF_BIELFLOADER_associateImage( (CORBA_Object)ram_dsm_id, &threadId, moduleId, &module_startip, &env);
}

inline L4_ThreadId_t get_free_threadid() {
    return L4_GlobalId(next_thread_no++, 1);
}

/* Kills the specified Thread. 
 * Returns: true = success, false = unsuccessful */
bool kill (L4_ThreadId_t tid) {
   return L4_ThreadControl (tid, L4_nilthread, L4_nilthread, L4_nilthread, (void*)-1UL);
}


#define UTCBaddress(x) ((void*)(((L4_Word_t)L4_MyLocalId().raw + utcbsize * (x)) & ~(utcbsize - 1)))

int main(void) {

    /****************************************************************
     
       Initialize Root Server 

    *****************************************************************/   

    L4_KernelInterfacePage_t* kip = (L4_KernelInterfacePage_t*)L4_KernelInterface ();

    sigma0id  = L4_Pager ();
    next_thread_no = L4_ThreadNo(L4_Myself()) + 1;
    locatorid = L4_nilthread;
    syscallid = L4_nilthread;



    /****************************************************************
     
        Print Early System Infos

    *****************************************************************/   

    printf ("Early system infos:\n");
    printf ("Threads: Myself:%lx Sigma0:%lx\n", L4_Myself ().raw, L4_Pager ().raw);
    pagesize = 1 << lsBit (L4_PageSizeMask (kip));
    printf ("Pagesize: %d\n", (int)pagesize);
    kiparea = L4_FpageLog2 ((L4_Word_t)kip, L4_KipAreaSizeLog2 (kip));
    printf ("KernelInterfacePage: %lx size: %d\n", L4_Address (kiparea), (int)L4_Size (kiparea));
    printf ("Bootinfo: %lx\n", L4_BootInfo (kip));
    printf ("ELFimage: from %p to %p\n", &__elf_start, &__elf_end);
    printf ("Heap: start: %p end: %p\n", &__heap_start, &__heap_end);
    utcbsize = L4_UtcbSize (kip);

    utcbarea = L4_FpageLog2 ((L4_Word_t) L4_MyLocalId ().raw,
			      L4_UtcbAreaSizeLog2 (kip) + 1);

    printf("UTCB size: %ld; UTCB area base address: 0x%08lx, size %ld byte\n", utcbsize, L4_Address(utcbarea), L4_Size(utcbarea));


    /****************************************************************
     
        Start Locator & Syscall Server 

    *****************************************************************/   

    /* startup our locator */
    printf ("[RT] Starting locator ... ");
    /* Generate some threadid */
    locatorid = get_free_threadid();
    start_thread (locatorid, 
		  (L4_Word_t)&locator_server, 
		  (L4_Word_t)&locator_stack[1023],
		  L4_Pager(),
		  UTCBaddress(1) ); 
    printf ("Started with id %lx\n", locatorid.raw);

    /* startup our syscall server */
    printf("[RT] Starting syscall server ... ");
    syscallid = get_free_threadid();
    start_thread (syscallid,
            (L4_Word_t)&syscall_server,
            (L4_Word_t)&syscall_stack[1023],
	    L4_Pager(),
            UTCBaddress(2) );
    printf("Started with id %lx\n", syscallid.raw);

    /* We just bring the in the memory of the bootinfo page */
    if (!request_page (L4_BootInfo (L4_KernelInterface ()))) {
    	// no bootinfo, no chance, no future. Break up
	    panic ("Was not able to get bootinfo");
    }

    // Bring in some other pages needed later by other threads in our AS. 
    // Do not remove - or do you want OpenSSL disaster, part 2?
    volatile L4_Word_t dummy;
    dummy = *((L4_Word_t *)0x00304018UL);
    dummy = *((L4_Word_t *)0x00305018UL);
    dummy = *((L4_Word_t *)0x00301d40UL);

    /* Quick check */
    if (!L4_BootInfo_Valid ((void*)L4_BootInfo (L4_KernelInterface ()))) 
	    panic ("Bootinfo not found");

    list_modules ((L4_BootInfo_t*)L4_BootInfo (L4_KernelInterface ()));


    // register syscall server
    CORBA_Environment env (idl4_default_environment);
    IF_LOCATOR_Announce((CORBA_Object) locatorid, IF_SYSCALL_ID, &syscallid, &env);


    /****************************************************************
     
       Start System Servers

    *****************************************************************/   

    // Start Pager-Thread for the RAM-DSM 
    L4_ThreadId_t ram_dsm_pager_id = get_free_threadid();
    printf("[RT] Starting RAM-DSM Pager with TID: %lx\n", ram_dsm_pager_id.raw);
    start_thread (ram_dsm_pager_id,
            (L4_Word_t)&pager_loop,
            (L4_Word_t)&ram_dsm_pager_stack[1023],
    	    L4_Pager(),
            UTCBaddress(3) );

    // RAM Data Space Manager (Which is our ELF-Loader at the Moment)
    L4_BootRec_t* ram_dsm_module = find_module (3, (L4_BootInfo_t*)L4_BootInfo (L4_KernelInterface ()));
    L4_Word_t ram_dsm_startip = load_elfimage(ram_dsm_module); 

    // some ELF loading and starting 
    ram_dsm_id = get_free_threadid();
    start_task (ram_dsm_id, ram_dsm_startip, ram_dsm_pager_id, utcbarea);
    printf ("[RT] RAM-DSM started with as %lx@%lx\n", ram_dsm_id.raw, ram_dsm_module);

    IF_BIELFLOADER_registerLocator((CORBA_Object) ram_dsm_id, &locatorid, &env);
    printf ("[RT] Locator registered with RAM-DSM\n");

    // Taskserver 
    taskserver_id = get_free_threadid();
    activate_module(taskserver_id, 5);
    printf ("[RT] Registered Taskserver\n");

    // Use Taskserver to start IO-DSM
    CORBA_char taskserverPath[256], args[3], penv[3];
  
    *(L4_Word_t *)taskserverPath = 2; // Module 2 == IO-DSM
    L4_ThreadId_t io_dsm_id = IF_TASK_startTask((CORBA_Object) taskserver_id, taskserverPath, args, penv, &env);
    printf("[RT] Taskserver started IO-DSM as %08lx\n", io_dsm_id.raw);

    // Use Taskserver to start Nameserver
    CORBA_char nameserverPath[256]; // We would really like to reuse the above string,
                                    // but IDL4 does something really stupid at this point.

    *(L4_Word_t *)nameserverPath = 4; // Module 4 == Nameserver 
    L4_ThreadId_t nameserver_id = IF_TASK_startTask((CORBA_Object) taskserver_id, nameserverPath, args, penv, &env);
    printf("[RT] Taskserver started Nameserver as %08lx\n", nameserver_id.raw);

    // Use Taskserver to start Consoleserver
    CORBA_char consoleserverPath[256]; // We would really like to reuse the above string,
                                    // but IDL4 does something really stupid at this point.

    *(L4_Word_t *)consoleserverPath = 6; // Module 6 == Consoleserver 
    L4_ThreadId_t consoleserver_id = IF_TASK_startTask((CORBA_Object) taskserver_id, consoleserverPath, args, penv, &env);
    printf("[RT] Taskserver started Consoleserver as %08lx\n", consoleserver_id.raw);

    // Use Taskserver to start Filesystem 
    CORBA_char filesystemPath[256]; // We would really like to reuse the above string,
                                    // but IDL4 does something really stupid at this point.
    *(L4_Word_t *)filesystemPath = 7; // Module 7 == simplefs
    L4_ThreadId_t filesystem_id = IF_TASK_startTask((CORBA_Object) taskserver_id, filesystemPath, args, penv, &env);
    printf("[RT] Taskserver started Filesystem as %08lx\n", filesystem_id.raw);

    // Use Taskserver to start app-pager
    CORBA_char apppagerPath[256]; // We would really like to reuse the above string,
                                    // but IDL4 does something really stupid at this point.
    *(L4_Word_t *)apppagerPath = 8; // Module 8 == application pager
    L4_ThreadId_t apppager_id = IF_TASK_startTask((CORBA_Object) taskserver_id, apppagerPath, args, penv, &env);
    printf("[RT] Taskserver started Apppager as %08lx\n", apppager_id.raw);

    // Use Taskserver to start shell 
    CORBA_char shellPath[256]; // We would really like to reuse the above string,
                                    // but IDL4 does something really stupid at this point.
    *(L4_Word_t *)shellPath = 9; // Module 9 == shell
    L4_ThreadId_t shell_id = IF_TASK_startTask((CORBA_Object) taskserver_id, shellPath, args, penv, &env);
    printf("[RT] Taskserver started Shell as %08lx\n", shell_id.raw);

    // Ask Taskserver to kill IO-DSM
//    IF_TASK_kill((CORBA_Object) taskserver_id, &io_dsm_id, &env);
//    printf("[RT] Told Taskserver to kill IO-DSM\n");


    /****************************************************************
     
       Start Pager Loop 

    *****************************************************************/   

    /* now it is time to become the pager for all those threads we 
       created recently */
    /* There is nothing left to page :( 
     * Unfortunately its not possible to kill the rootthread so we have to idle.
     * WARNING: busy waiting with threads that have different priorities is evil! (starvation) */
    L4_Time_t t = L4_TimePeriod (1000000000);
    while (42) {
        L4_Sleep (t);
    }

    
   // printf("try to kill the \"rootthread\"...\n");
   // if (kill (L4_Myself ())) {
   //     printf("\"rootthread\" successfully killed\n");}
   // else {
   //     printf("\"rootthread\" could not be killed. Error Code: %d \n", L4_ErrorCode ());
   // }

    panic ("Unexpected return from PagerLoop()");
}
// vim:sw=2:ts=2:expandtab: 
