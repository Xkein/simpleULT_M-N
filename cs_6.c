#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include "thread.h"

void spin()
{
  for (size_t i = 0; i < 10000; i++)
    for (size_t j = 0; j < 10000; j++)
      ;
}

int volatile thread1_count = 0;
void *thread1(void *arg)
{
  int cur = thread1_count++;
  printf("thread1 %d created! arg = %s\n", cur, (char *)arg);
  int count = cur;
  while (count--)
  {
    spin();
    thread_sleep(cur * 500);
    printf("thread1 %d\n", cur);
  }

  return NULL;
}

int volatile thread2_count = 1;
// test yield
void *thread2(void *arg)
{
  int cur = thread1_count++;
  while (1)
  {
    spin();
    printf("thread2 %d\n", cur);
    thread_yield();
  }
}

void *thread3(void *arg)
{
  printf("thread3 created! arg = %s\n", (char *)arg);
  while (1)
  {
    spin();
    spin();
    printf("thread3\n");
  }
}

int main()
{
  printf("my pid:%d\n", getpid());
  printf("my thread id:%ld\n", pthread_self());
  thread_id tid1, tid2, tid3, tid4, tid5;
  thread_create(&tid1, NULL, thread1, "loop 1 times");
  thread_create(&tid2, NULL, thread1, "loop 2 times");
  thread_create(&tid3, NULL, thread1, "loop 3 times");
  thread_create(&tid5, NULL, thread3, "no end thread.");

  pthread_t id;
  //pthread_create(&id, NULL, thread2, NULL);

  int i = 15;
  while (i--)
  {
    spin();
    printf("main thread\n");
    thread_yield();
  }
  thread_join(tid1);
  thread_join(tid2);
  thread_join(tid3);

  thread_sleep(1000);
  thread_create(&tid4, NULL, thread1, "loop 4 times");
  thread_join(tid4);

  return 0;
}