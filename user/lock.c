#include <inc/lib.h>
#include <inc/jthread.h>

typedef struct {
  int data;
  jthread_mutex_t lock;
} resource;

resource r;

int stupid_sem = 0;

void waitncycles(int n)
{
  int i = 0;
  while (i != n)
  {
    i++;
    sys_yield();
  }
}

void *
acquire_and_hold(void *arg)
{
  cprintf("acquire_and_hold: getting resource\n");
  jthread_mutex_lock(&r.lock);
  stupid_sem = 1;
  cprintf("acquire_and_hold: r.lock.locked: %s\n", r.lock.locked ? "true" : "false");
  //waitncycles(10);

  for (int i = 0; i < 100; ++i)
  {
    /* code */
    //for (int l = 0; l < 1000000; l++);
    r.data += 1;
  }
  cprintf("resource value is %d\n", r.data);

  while (stupid_sem != 2)
    asm volatile("pause");
  cprintf("acquire_and_hold: releasing resource\n");
  jthread_mutex_unlock(&r.lock);
  return NULL;
}

void
umain(int argc, char *argv[])
{
  jthread_t tid;
  cprintf("umain: creating thread\n");
  stupid_sem = 0;
  jthread_create(&tid, 0, acquire_and_hold, NULL);
  //waitncycles(3);
  //int counter = 0;
  //for (int l = 0; l < 1000000000; l++)
  //  counter += l;
  //cprintf("%d\n", counter);
  // somehow that thread is still not working
  while (stupid_sem == 0)
    asm volatile("pause");
  cprintf("umain: trying to acquire resource\n");
  stupid_sem = 2;
  jthread_mutex_lock(&r.lock);
  r.data += 10;
  cprintf("umain: acquired resource!\n");
  cprintf("umain: resource value is %d\n", r.data);
  jthread_mutex_unlock(&r.lock);
  //void *retval;
  jthread_join(tid, NULL);
  return;
}