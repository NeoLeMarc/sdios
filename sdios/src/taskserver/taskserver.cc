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
#include "taskserver.h"

// RAM-DSM-ID
extern L4_ThreadId_t ram_dsm_id;
extern L4_ThreadId_t locatorid;
extern L4_ThreadId_t syscallid;

// Thread ID Management
L4_Word_t next_thread_no = L4_ThreadNo(L4_Myself()) + 1; 

// Super-duper high-sophisticated thread management
#define MAX_NUM_SPACES   64
#define MAX_NUM_THREADS   256
#define MAX_NUM_LISTENERS 256

as_t       spaces[MAX_NUM_SPACES];
thread_t   threads[MAX_NUM_THREADS];
listener_t listeners[MAX_NUM_LISTENERS];

// "Optimization"
L4_Word_t next_thread_idx = 0, next_as_idx = 0, next_listener_idx = 0;

as_t * get_free_ascb() {
  for (L4_Word_t i = next_as_idx; i != next_as_idx - 1; i = (i + 1) % MAX_NUM_SPACES) {
    if (spaces[i].firstThread == NULL) {

      // Initialize ASCB 
      memset((void *)&spaces[i], 0, sizeof(as_t));

      // Save current ASCB idx
      next_as_idx = i + 1;

      return &spaces[i];
    }
  }

  panic("[TS] No more ASCBs");
  return NULL;
}

thread_t * get_free_tcb() {
  for (L4_Word_t i = next_thread_idx; i != next_thread_idx - 1; i = (i + 1) % MAX_NUM_THREADS) {
    if (threads[i].as == NULL) {
     
      // Initialize TCB
      memset((void *)&threads[i], 0, sizeof(thread_t));

      // Save current TCB idx
      next_thread_idx = i + 1;

      return &threads[i];
    }
  }

  panic("[TS] No more TCBs");
  return NULL;
}

listener_t * get_free_listener(){
  for (L4_Word_t i = next_listener_idx; i != next_listener_idx - 1; i = (i + 1) % MAX_NUM_LISTENERS) {
    if (L4_IsNilThread(listeners[i].listeningThread)) {
     
      // Initialize Listener 
      memset((void *)&listeners[i], 0, sizeof(listener_t));

      // Save current Listener idx
      next_listener_idx = i + 1;

      return &listeners[i];
    }
  }

  panic("[TS] No more Listeners");
  return NULL;

}

L4_ThreadId_t get_free_localid(as_t * space) {
  L4_KernelInterfacePage_t* kip = (L4_KernelInterfacePage_t*)L4_KernelInterface ();
  L4_Word_t utcbsize = L4_UtcbSize(kip);
  L4_Word_t utcbareasize = L4_Size(space->utcbarea);
  L4_Word_t utcbareastart = L4_Address(space->utcbarea);
  L4_Word_t utcbareaend = utcbareasize + utcbareastart;
  L4_ThreadId_t retval = L4_nilthread;
  for (L4_Word_t localid = utcbareastart; localid < utcbareaend; localid += utcbsize) {
    thread_t * cur = space->firstThread;
    while (cur != NULL) {
      if (cur->localid.raw == localid)
        break;
      cur = cur->nextThread;
    }
    if (cur == NULL) {
      retval.raw = localid;
      return retval;
    }
  }
  return retval;
}

thread_t * find_tcb(L4_ThreadId_t threadid) {
  for (int i = 0; i < MAX_NUM_THREADS; i++) {
    if (threads[i].globalid == threadid) {
      return &threads[i];
    }
  }
  return NULL;
}

void delete_tcb(thread_t * thread) {
  as_t * space = thread->as;
  if (space->firstThread == thread) {
    space->firstThread = thread->nextThread;
    // if thread->nextThread == NULL, space gets therefore marked free
  } else {
    thread_t * cur = space->firstThread;
    while (cur->nextThread != thread && cur->nextThread != NULL) {
      cur = cur ->nextThread;
    }
    cur->nextThread = thread->nextThread;
  }

  thread->nextThread = NULL;
  thread->as         = NULL;
}

inline L4_ThreadId_t get_free_threadid() {
    return L4_GlobalId(next_thread_no++, 1);
}

/* Interface taskserver */

IDL4_INLINE L4_ThreadId_t taskserver_startTask_implementation(CORBA_Object _caller, const CORBA_char *path, const CORBA_char *args, const CORBA_char *task_env, idl4_server_environment *_env)

{

  thread_t * thread  = get_free_tcb();
  as_t * space       = get_free_ascb();
  space->firstThread = thread;
  thread->as         = space;

  // Get a new Thread ID
  L4_ThreadId_t threadId  = get_free_threadid();
  thread->globalid        = threadId;
  L4_ThreadId_t nilthread = L4_nilthread;
  L4_ThreadId_t anythread = L4_anythread;
  L4_ThreadId_t myself    = L4_Myself();
  L4_Fpage_t kiparea      = L4_FpageLog2(0xB0000000, 14);

  // Setup corba environment
  CORBA_Environment env (idl4_default_environment);

  /* implementation of IF_TASK::startTask */


  L4_KernelInterfacePage_t* kip = (L4_KernelInterfacePage_t*)L4_KernelInterface ();
  L4_Fpage_t utcbarea = L4_FpageLog2 ((L4_Word_t) L4_MyLocalId ().raw, L4_UtcbAreaSizeLog2 (kip) + 1);
  space->utcbarea = utcbarea;
  thread->localid.raw = L4_Address(utcbarea);

  // 1. Ask syscall server to do ThreadControl to setup initial thread
  IF_SYSCALL_ThreadControl((CORBA_Object)syscallid, &threadId, &threadId, &myself, &nilthread, -1UL, &env);

  // 2. Ask syscall server to do SpaceControl to setup address space
  L4_Word_t dummy; // Takes result
  IF_SYSCALL_SpaceControl((CORBA_Object)syscallid, &threadId, 0, &kiparea, &utcbarea, &anythread, &dummy, &env);
  
  // 3. Ask syscall server to do second ThreadControl to activate thread
  IF_SYSCALL_ThreadControl((CORBA_Object)syscallid, &threadId, &threadId, &nilthread, &ram_dsm_id, thread->localid.raw, &env);
  

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

  // Find corresponding TCB
  thread_t * tcb = find_tcb(*thread);
  if (tcb == NULL) 
    panic("[TS] Attempt to kill an unknown thread");

  // Ask syscall server to do terminate thread 
  L4_ThreadId_t nilthread = L4_nilthread;
  IF_SYSCALL_ThreadControl((CORBA_Object)syscallid, thread, &nilthread, &nilthread, &nilthread, -1UL, &env);

  // Notify all waiting threads
  listener_t * cur = tcb->firstListener;
  while (cur != NULL) {

    IF_TASK_waitTid_reply((CORBA_Object) cur->listeningThread, 0);

    // Process next Listener
    cur = cur->nextListener;
  }
 
  delete_tcb(tcb);
  
  return;
}

IDL4_PUBLISH_TASKSERVER_KILL(taskserver_kill_implementation);

IDL4_INLINE L4_Word_t taskserver_waitTid_implementation(CORBA_Object _caller, const L4_ThreadId_t *thread, idl4_server_environment *_env)

{
  // First we have to check whether thread exists
  thread_t * tcb = find_tcb(*thread);
  if (tcb == NULL){
    // Thread does not exist, so we return immediatly
    return ~0UL;

  } else { 

    // We wont return at this point
    idl4_set_no_response(_env);

    // Create listener object 
    listener_t * listener = get_free_listener();
    listener->listeningThread = (L4_ThreadId_t)_caller; 

    // Register listener in TCB of to-be-waited-on thread
    listener->nextListener = tcb->firstListener;
    tcb->firstListener     = listener;

    // Dummy return
    return 0;
  }
}

IDL4_PUBLISH_TASKSERVER_WAITTID(taskserver_waitTid_implementation);

IDL4_INLINE L4_ThreadId_t taskserver_createThread_implementation(CORBA_Object _caller, idl4_server_environment *_env)

{

  // Find address space
  thread_t * caller = find_tcb((L4_ThreadId_t)_caller);
  as_t * space      = caller->as;
    
  // Setup corba environment
  CORBA_Environment env (idl4_default_environment);

  // Acquire a free global thread ID
  L4_ThreadId_t newthreadid = get_free_threadid();
  thread_t * newthread      = get_free_tcb();
  newthread->nextThread     = caller->nextThread;
  caller->nextThread        = newthread;
  newthread->globalid       = newthreadid;
  newthread->localid        = get_free_localid(space);
  newthread->as             = space;
  L4_ThreadId_t myself      = L4_Myself();

  /* implementation of IF_TASK::createThread */

  IF_SYSCALL_ThreadControl((CORBA_Object)syscallid, &newthreadid, (L4_ThreadId_t*)&_caller, &myself, &ram_dsm_id, newthread->localid.raw, &env);
  
  return newthreadid;
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

// vim:sw=2:ts=2:expandtab: 
