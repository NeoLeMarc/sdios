/*****************************************************************
 * Source file : bielfloader.idl
 * Platform    : V4 IA32
 * Mapping     : CORBA C
 * 
 * Generated by IDL4 1.1.0 (roadrunner) on 10/07/2008 18:11
 * Report bugs to haeberlen@ira.uka.de
 *****************************************************************/

#include <l4/bootinfo.h>
#include <l4/types.h>
#include <idl4glue.h>
#include <l4io.h>
#include <elf.h>

#include "bielfloader-server.h"


// Data structure to store associations
#define ASSOC_TABLE_SIZE 64

typedef struct {
    L4_ThreadId_t thread;
    L4_Word_t     module;
} association_t;

association_t associationTable[ASSOC_TABLE_SIZE];
int associationFreePos = 0;

L4_Word_t * available_b;
L4_Word_t lastFreePageBase = 0;

void appendToAssociationTable(const association_t association){
    if(associationFreePos < ASSOC_TABLE_SIZE - 1)
        associationTable[associationFreePos++] = association;
    else { 
        panic("[RAM-DSM-BIEFLOADER] Association table is full!\n");
    }
    
}

L4_Word_t getModuleId(L4_ThreadId_t thread){
    for(int i = 0; i < associationFreePos; i++)
        if(associationTable[i].thread == thread)
            return associationTable[i].module;
    // FIXME: Hier brauchen wir einen richtigen Fehler
    return 0;
}

L4_Fpage_t getFreePage() {
    for (L4_Word_t i = lastFreePageBase; i < 0x02000000UL; i += 0x1000UL) {
        if ( available_b[i >> 17] & (1UL << ((i >> 12) & 0x1fUL))) {
            available_b[i >> 17] &= ~(1UL << ((i >> 12) & 0x1fUL));
            return L4_FpageLog2(i, 12);
        }
    }
    return L4_Nilpage;
}


/* ELF Helper Functions */
void elfLoadHeader(Elf32_Ehdr ** hdr, L4_BootRec_t * mod){
    /* Check type of module */
    if (L4_Type (mod) != L4_BootInfo_Module)
        panic ("[RAM-DSM-BIELFLOADER] Wrong module type");
   

    /* Load Header from ELF File */
    *hdr = (Elf32_Ehdr*)L4_Module_Start (mod);
    
    return;
}

L4_BootRec_t * find_module (unsigned int index, const L4_BootInfo_t* bootinfo) {
    if (L4_BootInfo_Entries (bootinfo) < index) 
        panic ("Some modules are missing");

    L4_BootRec_t* bootrec = L4_BootInfo_FirstEntry (bootinfo);

    for (unsigned int i = 0; i < index; i++)
        bootrec = L4_Next (bootrec);
    return bootrec;
}

/* END OF ELF Helper Functions */


/* Interface bielfloader */

IDL4_INLINE void bielfloader_pagefault_implementation(CORBA_Object _caller, const L4_Word_t address, const L4_Word_t ip, const L4_Word_t privileges, idl4_fpage_t *return_page, idl4_server_environment *_env)

{
  // implementation of IF_PAGEFAULT::pagefault
  printf("[RAM-DSM-BIELFLOADER] Received pagefault from 0x%x at 0x%x\n", (unsigned int)_caller.raw, (unsigned int)address);  
  L4_KDB_Enter("Pagefault reveiced!");

  // determine boot image
  L4_Word_t moduleId = getModuleId((L4_ThreadId_t)_caller);
  if (moduleId == 0) {
      // error handling
  }
  L4_BootRec_t * module = find_module(moduleId, (L4_BootInfo_t *)L4_BootInfo(L4_KernelInterface()));
  
  // load ELF header
  Elf32_Ehdr * hdr = 0;
  elfLoadHeader(&hdr, module);

  L4_Word_t page_start = address & 0xfffff000UL;
  L4_Word_t page_end   = address | 0x00000fffUL;

  // reserve and zero free page
  L4_Fpage_t page = getFreePage();
  printf("Got free fpage at %lx\n", L4_Address(page));

  if (L4_IsNilFpage(page)) {
      panic("[RAM-DSM-BIELFLOADER] Out Of Memory in pagefault handling\n");
  }

  memset((void *)L4_Address(page), 0, L4_Size(page));

  printf("Page zeroed.\n");

  // find and copy relevant ELF sections
  Elf32_Phdr * phdr = (Elf32_Phdr *)(hdr->e_phoff + (unsigned int)hdr);

  // Debug output
  printf("ELF entry point == 0x%lx, HDR start == 0x%lx, PHDR start == 0x%lx\n", hdr->e_entry, hdr, phdr);
  printf("PHDR Dump: %lx-%lx-%lx-%lx\n", *phdr, *(phdr + 1), *(phdr + 2), *(phdr + 3));
  printf("HDR Dump: %lx-%lx-%lx-%lx\n", *hdr, *(hdr + 1), *(hdr + 2), *(hdr + 3));

  for(int i = 0; i < hdr->e_phnum; i++){
      printf("For-loop, i == %i, p_type = %i, PT_LOAD = %i\n", i, phdr[i].p_type, PT_LOAD);  

      if(phdr[i].p_type == PT_LOAD){

          printf("Found loadable program section - beginning copy\n");

          // Do page and program section overlap?
          if((page_end >= phdr[i].p_vaddr) // Page ends after section start
                  && (phdr[i].p_vaddr + phdr[i].p_memsz > page_start)){ // and vice versa

            // compute copy source start
            L4_Word_t s_offset = 0;
            if (phdr[i].p_vaddr < page_start) {
              s_offset  = page_start - phdr[i].p_vaddr; // Aligned offset of page
            }

            // compute copy destination start
            L4_Word_t d_offset = 0;
            if (phdr[i].p_vaddr > page_start) {
              d_offset = phdr[i].p_vaddr - page_start;
            }

            // determine amount to copy
            L4_Word_t s_max = phdr[i].p_filesz - s_offset;
            L4_Word_t d_max = 0x1000UL - d_offset;
            L4_Word_t copylength = (s_max > d_max) ? d_max : s_max;

            // add respective base addresses
            s_offset += (L4_Word_t)hdr + phdr[i].p_offset;
            d_offset += L4_Address(page);

            // Copy from image to page
            memcpy((void *)d_offset, (void *)s_offset, copylength);
            
          }
      }
  }

  // Set return page
  idl4_fpage_set_mode(return_page, IDL4_MODE_MAP);
  idl4_fpage_set_page(return_page, page);
  idl4_fpage_set_base(return_page, page_end);
  idl4_fpage_set_permissions(return_page, IDL4_PERM_FULL);

  printf("About to return.\n");
  return;
}

IDL4_PUBLISH_BIELFLOADER_PAGEFAULT(bielfloader_pagefault_implementation);

IDL4_INLINE void bielfloader_associateImage_implementation(CORBA_Object _caller, const L4_ThreadId_t *thread, const L4_Word_t bootModuleId, L4_Word_t *initialIp, idl4_server_environment *_env)

{
  /* implementation of IF_BIELFLOADER::associateImage */
  printf("Entering associate image...\n");  

  // Insert association in association table  
  association_t association;
  association.thread = *thread;
  association.module = bootModuleId;

  appendToAssociationTable(association);

  // Load Image Header
  L4_BootRec_t * module = find_module(bootModuleId, (L4_BootInfo_t *)L4_BootInfo(L4_KernelInterface()));

  Elf32_Ehdr * hdr = 0;
  elfLoadHeader(&hdr, module);

  // Set instruction pointer
  * initialIp = hdr->e_entry;

  // Send startup IPC
  L4_Msg_t msg;
  L4_Clear(&msg);
  L4_Append(&msg, (L4_Word_t)hdr->e_entry);
  L4_Append(&msg, 0);
  L4_Load(&msg);
  L4_Send(*thread);


  printf("... leaving associate image!\n");

  return;
}

IDL4_PUBLISH_BIELFLOADER_ASSOCIATEIMAGE(bielfloader_associateImage_implementation);

void *bielfloader_vtable_5[BIELFLOADER_DEFAULT_VTABLE_SIZE] = BIELFLOADER_DEFAULT_VTABLE_5;
void *bielfloader_vtable_discard[BIELFLOADER_DEFAULT_VTABLE_SIZE] = BIELFLOADER_DEFAULT_VTABLE_DISCARD;
void **bielfloader_itable[8] = { bielfloader_vtable_discard, bielfloader_vtable_discard, bielfloader_vtable_discard, bielfloader_vtable_discard, bielfloader_vtable_discard, bielfloader_vtable_5, bielfloader_vtable_discard, bielfloader_vtable_discard };
void *bielfloader_ktable[BIELFLOADER_DEFAULT_KTABLE_SIZE] = BIELFLOADER_DEFAULT_KTABLE;

void bielfloader_server(L4_Word_t * available_p)

{
  available_b = available_p;
  L4_ThreadId_t partner;
  L4_MsgTag_t msgtag;
  idl4_msgbuf_t msgbuf;
  long cnt;

  idl4_msgbuf_init(&msgbuf);
  for (cnt = 0;cnt < BIELFLOADER_STRBUF_SIZE;cnt++)
    idl4_msgbuf_add_buffer(&msgbuf, malloc(8000), 8000);

  while (1)
    {
      partner = L4_nilthread;
      msgtag.raw = 0;
      cnt = 0;

      while (1)
        {
          idl4_msgbuf_sync(&msgbuf);

          idl4_reply_and_wait(&partner, &msgtag, &msgbuf, &cnt);

          if (idl4_is_error(&msgtag))
            break;

          if (IDL4_EXPECT_FALSE(idl4_is_kernel_message(msgtag)))
            idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, bielfloader_ktable[idl4_get_kernel_message_id(msgtag) & BIELFLOADER_KID_MASK]);
            else idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, bielfloader_itable[idl4_get_interface_id(&msgtag) & BIELFLOADER_IID_MASK][idl4_get_function_id(&msgtag) & BIELFLOADER_FID_MASK]);
        }
    }
}

void bielfloader_discard(void)

{
}

