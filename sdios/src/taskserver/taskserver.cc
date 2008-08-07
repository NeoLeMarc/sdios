/*****************************************************************
 * Source file : taskserver.idl
 * Platform    : V4 IA32
 * Mapping     : CORBA C
 * 
 * Generated by IDL4 1.1.0 (roadrunner) on 30/07/2008 18:14
 * Report bugs to haeberlen@ira.uka.de
 *****************************************************************/

#include <l4io.h>
#include <idl4glue.h>
#include <if/ifsyscall.h>
#include <if/ifbielfloader.h>
#include "taskserver-server.h"

// RAM-DSM-ID
extern L4_ThreadId_t ram_dsm_id;
extern L4_ThreadId_t locatorid;
extern L4_ThreadId_t syscallid;

// Thread ID Management
L4_Word_t next_thread_no = L4_ThreadNo(L4_Myself()) + 1; 

inline L4_ThreadId_t get_free_threadid() {
    return L4_GlobalId(next_thread_no++, 1);
}

/* Interface taskserver */

IDL4_INLINE L4_ThreadId_t taskserver_startTask_implementation(CORBA_Object _caller, const CORBA_char *path, const CORBA_char *args, const CORBA_char *task_env, idl4_server_environment *_env)

{

  // Get a new Thread ID
  L4_ThreadId_t threadId = get_free_threadid();
  L4_ThreadId_t nilthread = L4_nilthread;
  L4_ThreadId_t anythread = L4_anythread;
  L4_ThreadId_t myself = L4_Myself();
  L4_Fpage_t kiparea = L4_FpageLog2(0xB0000000, 14);

  // Setup corba environment
  CORBA_Environment env (idl4_default_environment);

  /* implementation of IF_TASK::startTask */


  L4_KernelInterfacePage_t* kip = (L4_KernelInterfacePage_t*)L4_KernelInterface ();
  L4_Fpage_t utcbarea = L4_FpageLog2 ((L4_Word_t) L4_MyLocalId ().raw, L4_UtcbAreaSizeLog2 (kip) + 1);

  // 1. Ask syscall server to do ThreadControl to setup initial thread
  IF_SYSCALL_ThreadControl((CORBA_Object)syscallid, &threadId, &threadId, &myself, &nilthread, -1UL, &env);

  // 2. Ask syscall server to do SpaceControl to setup address space
  L4_Word_t dummy; // Takes result
  IF_SYSCALL_SpaceControl((CORBA_Object)syscallid, &threadId, 0, &kiparea, &utcbarea, &anythread, &dummy, &env);
  
  // 3. Ask syscall server to do second ThreadControl to activate thread
  IF_SYSCALL_ThreadControl((CORBA_Object)syscallid, &threadId, &threadId, &nilthread, &ram_dsm_id, L4_Address (utcbarea), &env);
  

  // 4. Ask BIELFLOADER to associate image
  L4_Word_t moduleId = *path;
  IF_BIELFLOADER_associateImage((CORBA_Object)ram_dsm_id, &threadId, moduleId, &dummy, &env);
 
  return threadId;
}

IDL4_PUBLISH_TASKSERVER_STARTTASK(taskserver_startTask_implementation);

IDL4_INLINE void taskserver_kill_implementation(CORBA_Object _caller, const L4_ThreadId_t *thread, idl4_server_environment *_env)

{

  // Setup corba environment
  CORBA_Environment env (idl4_default_environment);

  // Ask syscall server to do terminate thread 
  L4_ThreadId_t nilthread = L4_nilthread;
  IF_SYSCALL_ThreadControl((CORBA_Object)syscallid, thread, &nilthread, &nilthread, &nilthread, -1, &env);
  
  
  return;
}

IDL4_PUBLISH_TASKSERVER_KILL(taskserver_kill_implementation);

IDL4_INLINE L4_Word_t taskserver_waitTid_implementation(CORBA_Object _caller, const L4_ThreadId_t *thread, idl4_server_environment *_env)

{
  L4_Word_t __retval = 0;

  /* implementation of IF_TASK::waitTid */
  
  return __retval;
}

IDL4_PUBLISH_TASKSERVER_WAITTID(taskserver_waitTid_implementation);

IDL4_INLINE L4_ThreadId_t taskserver_createThread_implementation(CORBA_Object _caller, idl4_server_environment *_env)

{
  L4_ThreadId_t __retval = { raw: 0 };

  /* implementation of IF_TASK::createThread */
  
  return __retval;
}

IDL4_PUBLISH_TASKSERVER_CREATETHREAD(taskserver_createThread_implementation);

IDL4_INLINE L4_Word_t taskserver_getThreadStatus_implementation(CORBA_Object _caller, const L4_ThreadId_t *thread, idl4_server_environment *_env)

{
  L4_Word_t __retval = 0;

  /* implementation of IF_TASK::getThreadStatus */
  
  return __retval;
}

IDL4_PUBLISH_TASKSERVER_GETTHREADSTATUS(taskserver_getThreadStatus_implementation);

void *taskserver_vtable_6[TASKSERVER_DEFAULT_VTABLE_SIZE] = TASKSERVER_DEFAULT_VTABLE_6;
void *taskserver_vtable_discard[TASKSERVER_DEFAULT_VTABLE_SIZE] = TASKSERVER_DEFAULT_VTABLE_DISCARD;
void **taskserver_itable[8] = { taskserver_vtable_discard, taskserver_vtable_discard, taskserver_vtable_discard, taskserver_vtable_discard, taskserver_vtable_discard, taskserver_vtable_discard, taskserver_vtable_6, taskserver_vtable_discard };

void taskserver_server(void)

{
  L4_ThreadId_t partner;
  L4_MsgTag_t msgtag;
  idl4_msgbuf_t msgbuf;
  long cnt;

  idl4_msgbuf_init(&msgbuf);
  for (cnt = 0;cnt < TASKSERVER_STRBUF_SIZE;cnt++)
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

          idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, taskserver_itable[idl4_get_interface_id(&msgtag) & TASKSERVER_IID_MASK][idl4_get_function_id(&msgtag) & TASKSERVER_FID_MASK]);
        }
    }
}

void taskserver_discard(void)

{
}

