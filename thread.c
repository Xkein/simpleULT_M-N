
#include "thread.h"
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <threads.h>
#include <stdbool.h>

#define RET_INIT 1234
void switch_to(thread *from, thread *to);
void thread_yield();

int thread_count = 100;
thread *threads[100] = {};

typedef struct
{
    kthread_id kid;
    thread *thread;
} thread_relation;

thread_relation thread_relations[100] = {};

thread_attr default_attr = {.priority = 10, .stacksize = 4096};

/* Allocate new id or replace old thread's id. */
static thread_id allocate_id()
{
    thread_id id = 0;
    while (id < thread_count && threads[id])
    {
        thread *t = threads[id];
        if (t->state == THREAD_EXIT)
        {
            free(t);
            threads[id] = NULL;
            break;
        }
        id++;
    }
    return threads[id] ? THREAD_NULL : id;
}

/* Make thread information from kernel thread. */
static thread *_thread_from_kernel(kthread_id kid)
{
    thread *new_thread = malloc(sizeof(thread));

    new_thread->id = allocate_id();
    new_thread->context = NULL;
    new_thread->start_routine = NULL;
    new_thread->arg = NULL;
    new_thread->stackblock = NULL;
    new_thread->stackblock_size = 0;
    new_thread->state = THREAD_RUNNING;
    new_thread->kid = kid;
    new_thread->_sched_data.snippet = 15;
    new_thread->_sched_data.priority = default_attr.priority;
    pthread_mutex_lock(&new_thread->_sched_data.lock);

    threads[new_thread->id] = new_thread;

    return new_thread;
}

void _set_thread_self(kthread_id kid, thread *thread)
{
    for (int idx = 0; idx < thread_count; idx++)
    {
        thread_relation *rel = &thread_relations[idx];
        if (rel->kid == kid)
        {
            rel->thread->kid = THREAD_NULL;
            rel->thread = thread;
            thread->kid = kid;
            break;
        }
    }
}

thread_id get_thread_self()
{
    kthread_id kid = thrd_current();
    int idx;
    for (idx = 0; idx < thread_count; idx++)
    {
        thread_relation *rel = &thread_relations[idx];
        if (rel->thread == NULL)
        {
            break;
        }
        if (rel->kid == kid)
        {
            return rel->thread->id;
        }
    }

    thread *new_thread = _thread_from_kernel(kid);
    thread_relations[idx].kid = kid;
    thread_relations[idx].thread = new_thread;
    return new_thread->id;
}

void thread_start(thread *t)
{
    t->state = THREAD_RUNNING;
    t->result = t->start_routine(t->arg);
    t->state = THREAD_EXIT;
    printf("thread(%d) exit.\n", t->id);
    while (1)
    {
        thread_yield();
        perror("exited thread yield fail!");
        thrd_exit(0);
    }
}

/* Allocate stack for thread. */
static int allocate_stack(const thread_attr *attr, thread **pdp)
{
    thread *new_thread = malloc(sizeof(thread));
    if (new_thread == NULL)
    {
        return errno;
    }
    *pdp = new_thread;

    new_thread->id = allocate_id();

    if (attr == NULL)
    {
        attr = &default_attr;
    }

    void *stack = malloc(attr->stacksize);
    if (stack == NULL)
    {
        return errno;
    }

    new_thread->stackblock = stack;
    new_thread->stackblock_size = attr->stacksize;

    return 0;
}

int thread_create(thread_id *id, thread_attr *attr, thread_routine start_routine, void *arg)
{
    thread *new_thread = NULL;
    int err = allocate_stack(attr, &new_thread);
    if (err)
    {
        perror("thread alloc fail!");
        return err;
    }

    new_thread->start_routine = start_routine;
    new_thread->arg = arg;
    new_thread->result = NULL;

    if (attr == NULL)
    {
        attr = &default_attr;
    }

    new_thread->_sched_data.priority = attr->priority;
    new_thread->_sched_data.snippet = 15;
    new_thread->_sched_data.wakeuptime = 0;
    // new_thread->_sched_data.lock; // not running

    void **stack_bottom = new_thread->stackblock + new_thread->stackblock_size;
    thread_context *context = (thread_context *)(stack_bottom - sizeof(thread_context) / sizeof(long) - 2);

    context->flags = 8;
    context->edi = 7;
    context->esi = 6;
    context->ebp = 5;
    context->stack_frame = 4;
    context->ebx = 3;
    context->edx = 2;
    context->ecx = 1;
    context->eax = 0;
    context->esp = 4;

    context->origin = (int)thread_start;
    stack_bottom[-2] = (void *)RET_INIT;
    stack_bottom[-1] = new_thread;

    new_thread->context = context;
    new_thread->state = THREAD_READY;
    new_thread->kid = THREAD_NULL;

    threads[new_thread->id] = new_thread;

    *id = new_thread->id;

    return 0;
}

void thread_sleep(int msec)
{
    thread_id id = get_thread_self();
    thread *cur = threads[id];
    cur->_sched_data.wakeuptime = THREAD_CURTIME + msec;
    cur->state = THREAD_SLEEP;
    thread_yield();
    cur->state = THREAD_RUNNING;
}

void thread_join(thread_id id)
{
    while (threads[id]->state != THREAD_EXIT)
    {
        thread_yield();
    }
}

void _thread_switch(thread *from, thread *to)
{
    _set_thread_self(from->kid, to);
    switch_to(from, to);
}
