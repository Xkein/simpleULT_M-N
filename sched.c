
#include "thread.h"
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern int thread_count;
extern thread *threads[100];

void _thread_switch(thread *from, thread *to);

#define EMPTY_SIGSET        \
  (                         \
      {                     \
        sigset_t mask;      \
        sigemptyset(&mask); \
        mask;               \
      })

#define ALRM_SIGSET                   \
  (                                   \
      {                               \
        sigset_t mask = EMPTY_SIGSET; \
        sigaddset(&mask, SIGALRM);    \
        mask;                         \
      })

void closealarm()
{
  sigset_t mask = ALRM_SIGSET;
  if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
  {
    perror("sigprocmask BLOCK");
  }
}

void openalarm()
{
  sigset_t mask = ALRM_SIGSET;
  if (sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0)
  {
    perror("sigprocmask UNBLOCK");
  }
}

thread *pick()
{
  int i, next, c;
  thread *t;

  for (i = 0; i < thread_count; ++i)
  {
    t = threads[i];
    if (t && t->state != THREAD_EXIT && THREAD_CURTIME > t->_sched_data.wakeuptime)
    {
      t->state = THREAD_RUNNING;
    }
  }

  while (1)
  {
    c = -1;
    next = 0;
    // select thread where has max snippet
    for (i = 0; i < thread_count; ++i)
    {
      if (t = threads[i])
      {
        if (t->kid != THREAD_NULL)
        { // thread is running on kernel thread. skip it.
          continue;
        }
        sched_data *data = &t->_sched_data;
        if (t->state == THREAD_RUNNING && data->snippet > c)
        {
          c = data->snippet;
          next = i;
        }
      }
    }
    if (c == -1)
    { // no idel thread
      return NULL;
    }
    if (c)
      break;

    // 如果所有任务时间片都是 0，重新调整时间片的值
    if (c == 0)
    {
      for (i = 0; i < thread_count; ++i)
      {
        if (t = threads[i])
        {
          sched_data *data = &t->_sched_data;
          data->snippet = data->priority + (data->snippet >> 1);
        }
      }
    }
  }

  return threads[next];
}

#include <pthread.h>
pthread_mutex_t yield_mutex;
void thread_yield()
{
  closealarm();
  thread_id cur_id = get_thread_self();
  thread *cur = threads[cur_id];
  thread *next = pick();
  if (next && cur != next)
  {
    // ensure each thread running atomically
    if (!pthread_mutex_trylock(&next->_sched_data.lock))
    {
      pthread_mutex_unlock(&cur->_sched_data.lock);
      _thread_switch(cur, next);
    }
  }
}

static int tick_count = 1;
void tick(int sig)
{
  bool should_schedule = false;
  int cur = tick_count++;
  for (size_t i = 0; i < thread_count; i++)
  {
    thread *t = threads[i];
    if (t && t->kid != THREAD_NULL)
    {

      if (t->_sched_data.snippet == 0)
      {
        should_schedule = true;
      }
      else
      {
        t->_sched_data.snippet--;
      }
    }
  }

  if (should_schedule)
  {
    thread_yield();
  }
}

__attribute__((constructor)) static void init()
{
  struct itimerval value;
  value.it_value.tv_sec = 0;
  value.it_value.tv_usec = 1000;
  value.it_interval.tv_sec = 0;
  value.it_interval.tv_usec = 1000 * 10; // 10 ms
  if (setitimer(ITIMER_REAL, &value, NULL) < 0)
  {
    perror("setitimer");
  }
  signal(SIGALRM, tick);
}