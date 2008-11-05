/*****************************************************************
 * Source file : console.idl
 * Platform    : V4 IA32
 * Mapping     : CORBA C
 * 
 * Generated by IDL4 1.0.2 (roadrunner) on 29/10/2008 13:15
 * Report bugs to haeberlen@ira.uka.de
 *****************************************************************/
#include <l4/thread.h>
#include <l4/kip.h>
#include <l4io.h>
#include <l4/ipc.h>
#include <l4/sigma0.h>

#include <sdi/types.h>
#include <sdi/sdi.h>

#include <idl4glue.h>
#include "console-server.h"

#include "../lib/io/ia32-port.h"

unsigned short * vgaStart = (unsigned short *)0xb8000UL;
int consoleCursorHPos = 0, consoleCursorVPos = 24;

/* Interface console */

IDL4_INLINE void console_write_implementation(CORBA_Object _caller, buffer_t *input, int length, idl4_server_environment *_env)

{
  printf("[DEBUG CONSOLE] arguments: length=%i, chars=%s\n", length, input->_buffer);
  /* implementation of IF_CONSOLE::write */
  for (int pos = 0; pos < length; pos++) {
    //printf("test %i\n", input->_buffer[pos]);
	if (consoleCursorHPos == 80 || input->_buffer[pos] == '\n') {
            if (consoleCursorVPos == 24) {
                // Shift whole screen one line up
                memcpy((void *)vgaStart, (void *)(vgaStart + 80), 3840);
                // Clear last line
                if ((input->_length - pos) < 80) 
                    for (int i = 0; i < 80; i++)
                        *(vgaStart + 1920 + i) = 0xa00;

            } else {
                consoleCursorVPos++;
            }
            consoleCursorHPos = 0;
    }
    if (input->_buffer[pos] != '\n') {
        *((unsigned char *)vgaStart + 160*consoleCursorVPos + 2*consoleCursorHPos) = input->_buffer[pos];
        consoleCursorHPos++;
    }    
  }
  return;
}

IDL4_PUBLISH_CONSOLE_WRITE(console_write_implementation);

IDL4_INLINE void console_delete_implementation(CORBA_Object _caller, const CORBA_long numChars, idl4_server_environment *_env)

{
  printf("[CONSOLE SERVER] now we should delete n chars...\n");
  for (int i = 0; i<numChars; i++){
    if (consoleCursorHPos == 0){
        if (consoleCursorVPos != 0){
            consoleCursorVPos--;
            consoleCursorHPos = 79;
        }
    } else {
        consoleCursorHPos--;
    }
    *((unsigned char *)vgaStart + 160*consoleCursorVPos + 2*consoleCursorHPos) = 0xa00;
  }
  return;
}

IDL4_PUBLISH_CONSOLE_DELETE(console_delete_implementation);

IDL4_INLINE void console_clear_implementation(CORBA_Object _caller, idl4_server_environment *_env)

{
  /* implementation of IF_CONSOLE::clear */
  printf("[DEBUG CONSOLE] clearing screen\n");
  for (int pos = 0; pos < 2000; pos++)
        *((unsigned short *)vgaStart + pos) = 0xa00;
    consoleCursorHPos = 0;
    consoleCursorVPos = 0;

  return;
}

IDL4_PUBLISH_CONSOLE_CLEAR(console_clear_implementation);

void *console_vtable_11[CONSOLE_DEFAULT_VTABLE_SIZE] = CONSOLE_DEFAULT_VTABLE_11;
void *console_vtable_discard[CONSOLE_DEFAULT_VTABLE_SIZE] = CONSOLE_DEFAULT_VTABLE_DISCARD;
void **console_itable[16] = { console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_11, console_vtable_discard, console_vtable_discard, console_vtable_discard, console_vtable_discard };

void console_server(void)

{
  // Explicitly request VGA area
  L4_Fpage_t requestFpage;
  requestFpage = L4_Fpage(0xb8000UL, 0x1000UL);
  printf("[CONSOLE SERVER] Requesting VGA memory: %lx\n", requestFpage.raw);
  if(L4_IsNilFpage(L4_Sigma0_GetPage(L4_nilthread, requestFpage, requestFpage))){
     printf("[CONSOLE SERVER] Fpage request failed!\n");
  } else {
     printf("[CONSOLE SERVER] Fpage request succeeded!\n");
  }

  /*
  L4_Time_t t = L4_TimePeriod (500000);
  L4_Sleep(t);
  printf ("[CONSOLE SERVER] Started\n");

  console_write_implementation(0, 1, "\n");

  L4_Sleep(t);
  for (int i = 0; i < 32; i++) {
     console_write_implementation(0, 16, "0123456789A\nBCDEF");
  }
  L4_Sleep(t);
  console_clear_implementation();
  for (int i = 0; i < 105; i++) {
      console_write_implementation(0, 15, "0123456789ABCDEF");
  }

  printf ("[CONSOLE SERVER] Spinning!\n");
  */

  L4_ThreadId_t partner;
  L4_MsgTag_t msgtag;
  idl4_msgbuf_t msgbuf;
  long cnt;

  idl4_msgbuf_init(&msgbuf);
  for (cnt = 0;cnt < CONSOLE_STRBUF_SIZE;cnt++)
    idl4_msgbuf_add_buffer(&msgbuf, malloc(8000), 8000);

  while (1)
    {
      partner = L4_nilthread;
      msgtag.raw = 0;
      cnt = 0;

      while (1)
        {
          idl4_msgbuf_sync(&msgbuf);
	  
	  //printf("[DEBUG CONSOLE] waiting for incoming message\n");
          idl4_reply_and_wait(&partner, &msgtag, &msgbuf, &cnt);

          if (idl4_is_error(&msgtag))
            break;

          idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, console_itable[idl4_get_interface_id(&msgtag) & CONSOLE_IID_MASK][idl4_get_function_id(&msgtag) & CONSOLE_FID_MASK]);
        }
    }
}

void console_discard(void)

{
}
