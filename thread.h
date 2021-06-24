#pragma once
#include <pthread.h>

typedef enum
{
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_SLEEP,
    THREAD_EXIT
} thread_state;

typedef void *(*thread_routine)(void *);

typedef int thread_id;
typedef long kthread_id;
#define THREAD_NULL -1

typedef struct
{
    int flags;
    int edi;
    int esi;
    int ebp;
    int stack_frame; // stack frame, for convenient
    int ebx;
    int edx;
    int ecx;
    int eax;
    int esp;
    int origin; // the address to continue execute
} thread_context;

typedef struct
{
    unsigned int wakeuptime; // the time to wakeup thread
    int snippet;             // time snippet
    int priority;
    pthread_mutex_t lock; // the mutex for scheduler
} sched_data;

typedef struct
{
    thread_id id;
    /* The thread context located in stack.  */
    thread_context *context;
    /* The schedule data of the thread. */
    sched_data _sched_data;
    /* The result of the thread function.  */
    void *result;
    /* Start position of the code to be executed and the argument passed
     to the function.  */
    thread_routine start_routine;
    void *arg;
    /* If nonzero, pointer to the area allocated for the stack and guard. */
    void *stackblock;
    /* Size of the stackblock area including the guard.  */
    int stackblock_size;

    /* The current state of thread. */
    thread_state state;
    /* The kernel thread's id executing the thread. */
    kthread_id kid;
} thread;

typedef struct
{
    int stacksize;
    int priority;
} thread_attr;

/* Obtain the identifier of the current thread.  */
thread_id get_thread_self();
/* Create a new thread, starting with execution of START-ROUTINE
   getting passed ARG.  Creation attributed come from ATTR.  The new
   handle is stored in *id.  */
int thread_create(thread_id *id, thread_attr *attr, thread_routine start_routine, void *arg);
/* Block current thread execution for at least msec */
void thread_sleep(int msec);
/* Make calling thread wait for termination of the target thread. */
void thread_join(thread_id id);
/* Yield the processor to another thread or process. */
void thread_yield();

#define THREAD_CURTIME (                      \
    {                                         \
        struct timeval tv;                    \
        if (gettimeofday(&tv, NULL) < 0)      \
        {                                     \
            perror("get thread time fail!");  \
            exit(-1);                         \
        }                                     \
        tv.tv_sec * 1000 + tv.tv_usec / 1000; \
    })
