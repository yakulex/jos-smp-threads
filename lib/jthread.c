// JOS Threading
// Hugh O'Cinneide
// November 2014

#include <inc/lib.h>
#include <inc/jthread.h>
#include <inc/x86.h>

#define DEBUG 1

void *
jthread_main(void *(*start_routine)(void *), void *arg)
{
  if (DEBUG)
    cprintf("In jthread_main\n");
  void *ret = start_routine(arg);
  jthread_exit(ret);
  // Should not reach here
  return NULL;
}

int
jthread_create(jthread_t *thread,
               const jthread_attr_t *attr,
               void *(*start_routine)(void *),
               void *arg)
{
  if (start_routine == NULL || thread == NULL)
    return -1;

  jthread_t tid;
  if ((tid = sys_kthread_create((void *)jthread_main, (void *)start_routine, arg)) < 0)
    return -1;
  
  *thread = tid;

  return 0;
}

int
jthread_join(jthread_t th, void **thread_return)
{
  int ret = 0;
  while ((ret = sys_kthread_join(th, thread_return)) < 0)
  {
    sys_yield();
  }
  return 0;
}

void
jthread_exit(void *retval)
{
  sys_kthread_exit(retval);
}

int
jthread_mutex_lock(jthread_mutex_t *mutex)
{
  while (xchg((uint32_t *)&mutex->locked, 1) != 0)
    asm volatile ("pause");
  mutex->owner = thisenv->env_id;
  return 0;
}

int
jthread_mutex_trylock(jthread_mutex_t *mutex)
{
  if (xchg((uint32_t *)&mutex->locked, 1) != 0)
    return -1;
  return 0;
}

int
jthread_mutex_unlock(jthread_mutex_t *mutex)
{
  if (mutex->owner != thisenv->env_id)
    return -1;
  xchg((uint32_t *)&mutex->locked, 0);
  return 0;
}
