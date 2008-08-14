// for main.cc to be able to call taskserver.cc
void taskserver_server(void);

typedef struct {
    L4_Fpage_t utcbarea;
    struct thread * firstThread;
} as_t;

typedef struct thread {
    L4_ThreadId_t globalid;
    L4_ThreadId_t localid;
    as_t * as;
    struct thread *   nextThread;
    struct listener * firstListener;
} thread_t;

// Needed for waitTid
typedef struct listener {
    L4_ThreadId_t listeningThread;
    struct listener * nextListener;
} listener_t;
