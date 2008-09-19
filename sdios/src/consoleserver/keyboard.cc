/*****************************************************************
 * Source file : keyboard.idl
 * Platform    : V4 IA32
 * Mapping     : CORBA C
 * 
 * Generated by IDL4 1.1.0 (roadrunner) on 04/09/2008 17:11
 * Report bugs to haeberlen@ira.uka.de
 *****************************************************************/

#include <idl4glue.h>
#include <l4io.h>
#include "keyboard-server.h"
#include "keyboard.h"
#include "keymap.cc"


/* Interface keyboard */

IDL4_INLINE void keyboard_read_implementation(CORBA_Object _caller, keyboardBuffer *buffer, idl4_server_environment *_env)

{
  /* implementation of IF_KEYBOARD::read */
  
  return;
}

IDL4_PUBLISH_KEYBOARD_READ(keyboard_read_implementation);

IDL4_INLINE void keyboard_interrupt_implementation(CORBA_Object _caller, idl4_server_environment *_env)

{
  /* implementation of IF_INTERRUPT::interrupt */

  L4_Word_t kbreg=0x60, ctrlreg=0x64;
  L4_Word8_t scancode = 0, status = 0, ledstatus = 0;
  char charstr[MAX_CHAR_LEN] = "";
  extern modifiers_t modifiers;

  asm volatile ("inb %w1, %0" : "=a"(status):"dN"(ctrlreg));

  while (status & 1) { // Output buffer full, can be read
    asm volatile ("inb %w1, %0" : "=a"(scancode):"dN"(kbreg));
    printf("[KBD] Received scancode %x\n", scancode);

    keycodeToChars(scancode, (char *) charstr);
    if (scancode == 0x3a  // CapsLock make
     || scancode == 0x45  // NumLock make
     || scancode == 0x46) // ScrollLock make
    {
        ledstatus = modifiers.capsLock << 2 + modifiers.numLock << 1 + modifiers.scrollLock << 1;
        asm volatile ("outb %1, %w0 \n" :: "dN"(kbreg), "a"(0xed));
        asm volatile ("outb %1, %w0 \n" :: "dN"(kbreg), "a"(ledstatus)); // update LEDs
    }

    printf("[KBD] Resulting char sequence \"%s\"\n", charstr);

    asm volatile ("inb %w1, %0" : "=a"(status):"dN"(ctrlreg));
  }

  // reenable interrupt
  L4_LoadMR(0, 0UL);
  L4_Send(L4_GlobalId(1,1));
  return;
}

IDL4_PUBLISH_KEYBOARD_INTERRUPT(keyboard_interrupt_implementation);

void *keyboard_vtable_8[KEYBOARD_DEFAULT_VTABLE_SIZE] = KEYBOARD_DEFAULT_VTABLE_8;
void *keyboard_vtable_discard[KEYBOARD_DEFAULT_VTABLE_SIZE] = KEYBOARD_DEFAULT_VTABLE_DISCARD;
void **keyboard_itable[16] = { keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_8, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard, keyboard_vtable_discard };
void *keyboard_ktable[KEYBOARD_DEFAULT_KTABLE_SIZE] = KEYBOARD_DEFAULT_KTABLE;

void keyboard_server(void)

{
  L4_ThreadId_t partner;
  L4_MsgTag_t msgtag;
  idl4_msgbuf_t msgbuf;
  long cnt;

  idl4_msgbuf_init(&msgbuf);
  for (cnt = 0;cnt < KEYBOARD_STRBUF_SIZE;cnt++)
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
            idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, keyboard_ktable[idl4_get_kernel_message_id(msgtag) & KEYBOARD_KID_MASK]);
            else idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, keyboard_itable[idl4_get_interface_id(&msgtag) & KEYBOARD_IID_MASK][idl4_get_function_id(&msgtag) & KEYBOARD_FID_MASK]);
        }
    }
}

void keyboard_discard(void)

{
}
