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

#include <if/ifbielfloader.h>

/* local threadids */
L4_ThreadId_t sigma0id;
L4_ThreadId_t locatorid;
L4_ThreadId_t syscallid;


L4_Word_t pagesize;
L4_Word_t utcbsize;
L4_Fpage_t kiparea;
L4_Fpage_t utcbarea;

/* helperstuff */

extern char __elf_start;
extern char __elf_end;

extern char __heap_start;
extern char __heap_end;


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



    /****************************************************************
     
        Start Locator & Syscall Server 

    *****************************************************************/   

    /* startup our locator */
    printf ("Starting locator ...\n");
    /* Generate some threadid */
    locatorid = L4_GlobalId ( L4_ThreadNo (L4_Myself ()) + 1, 1);
    start_thread (locatorid, 
		  (L4_Word_t)&locator_server, 
		  (L4_Word_t)&locator_stack[1023],
		  L4_Pager(),
		  UTCBaddress(1) ); 
    printf ("Started with id %lx\n", locatorid.raw);

    /* startup our syscall server */
    printf("Starting syscall server ... \n");
    syscallid = L4_GlobalId(L4_ThreadNo(L4_Myself()) + 2, 1);
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

    /* Quick check */
    if (!L4_BootInfo_Valid ((void*)L4_BootInfo (L4_KernelInterface ()))) 
	    panic ("Bootinfo not found");

    list_modules ((L4_BootInfo_t*)L4_BootInfo (L4_KernelInterface ()));




    /****************************************************************
     
       Start System Servers

    *****************************************************************/   

    // Start Pager-Thread for the RAM-DSM 
    L4_ThreadId_t ram_dsm_pager_id = L4_GlobalId ( L4_ThreadNo (L4_Myself ()) + 3, 1);
    printf("Starting RAM-DSM Pager with TID: %lx\n", ram_dsm_pager_id.raw);
    start_thread (ram_dsm_pager_id,
            (L4_Word_t)&pager_loop,
            (L4_Word_t)&ram_dsm_pager_stack[1023],
    	    L4_Pager(),
            UTCBaddress(3) );

    // RAM Data Space Manager (Which is our ELF-Loader at the Moment)
    L4_BootRec_t* ram_dsm_module = find_module (3, (L4_BootInfo_t*)L4_BootInfo (L4_KernelInterface ()));
    L4_Word_t ram_dsm_startip = load_elfimage(ram_dsm_module); 

    // some ELF loading and starting 
    L4_ThreadId_t ram_dsm_id = L4_GlobalId ( L4_ThreadNo (L4_Myself ()) + 5, 1);
    start_task (ram_dsm_id, ram_dsm_startip, ram_dsm_pager_id, utcbarea);
    printf ("RAM-DSM started with as %lx@%lx\n", ram_dsm_id.raw, ram_dsm_module);


    // Register module of IO Data Space Manager
    L4_ThreadId_t io_dsm_id  = L4_GlobalId ( L4_ThreadNo (L4_Myself ()) + 4, 1);
    L4_Word_t io_dsm_startip = 0; 

    CORBA_Environment env (idl4_default_environment);
    IF_BIELFLOADER_associateImage( (CORBA_Object)ram_dsm_id, &io_dsm_id, 2, &io_dsm_startip, &env);
    printf("Root-Task is still alive!");

/*
    // IO Data Space Manager
//    L4_BootRec_t* io_dsm_module = find_module (2, (L4_BootInfo_t*)L4_BootInfo (L4_KernelInterface ()));

    // some ELF loading and starting
//   start_task (io_dsm_id, io_dsm_startip, ram_dsm_pager_id, utcbarea);
//    printf ("IO-DSM started with as %lx@%lx\n", io_dsm_id.raw, io_dsm_module);



    // Nameserver 
    L4_BootRec_t* nameserver_module = find_module(4, (L4_BootInfo_t*)L4_BootInfo (L4_KernelInterface ()));
    L4_Word_t nameserver_startip = load_elfimage(nameserver_module); 

    // some ELF loading and starting 
    L4_ThreadId_t nameserver_id = L4_GlobalId ( L4_ThreadNo (L4_Myself ()) + 6, 1);
    start_task (nameserver_id, nameserver_startip, ram_dsm_pager_id, utcbarea);
    printf ("Nameserver started with as %lx@%lx\n", nameserver_id.raw, nameserver_module);



    // Taskserver 
    L4_BootRec_t* taskserver_module = find_module (5, (L4_BootInfo_t*)L4_BootInfo (L4_KernelInterface ()));
    L4_Word_t taskserver_startip = load_elfimage(taskserver_module); 

    // some ELF loading and starting
    L4_ThreadId_t taskserver_id = L4_GlobalId ( L4_ThreadNo (L4_Myself ()) + 7, 1);
    start_task (taskserver_id, taskserver_startip, ram_dsm_pager_id, utcbarea);
    printf ("Taskserver started with as %lx@%lx\n", taskserver_id.raw, taskserver_module);
    // Taskserver needs high Priority because you can set Prios only to values smaller or equal your own
    L4_Set_Priority((L4_ThreadId_t) taskserver_id, (L4_Word_t) 255);
    printf("Taskserver Priority set to 255\n"); 
    */

    /****************************************************************
     
       Start Pager Loop 

    *****************************************************************/   

    /* now it is time to become the pager for all those threads we 
       created recently */
    /* There is nothing left to page :( 
     * Unfortunately its not possible to kill the rootthread so we have to idle.
     * WARNING: busy waiting with threads that have different priorities is evil! (starvation) */
    L4_Set_Priority(L4_Myself(), (L4_Word_t) 100);
    L4_Time_t t = L4_TimePeriod (1000000);
    printf("roottask Priority set to 100\n"); 
    while (42) {
        L4_Sleep (t);
        L4_Yield ();
    }

    
   // printf("try to kill the \"rootthread\"...\n");
   // if (kill (L4_Myself ())) {
   //     printf("\"rootthread\" successfully killed\n");}
   // else {
   //     printf("\"rootthread\" could not be killed. Error Code: %d \n", L4_ErrorCode ());
   // }

    panic ("Unexpected return from PagerLoop()");
}
