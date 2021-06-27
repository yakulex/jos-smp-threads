#include <inc/lib.h>
#include <inc/jthread.h>
#include <inc/x86.h>
#include <inc/errno.h>
#include <inc/error.h>

#define DEBUG 0

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
    return errno = E_INVAL, -E_INVAL; //smth like this, errno = 3, func returns -3

  jthread_t tid;
  if ((tid = sys_kthread_create((void *)jthread_main, (void *)start_routine, arg)) < 0)
    return errno = -tid, tid;
  *thread = tid;
  

  return 0;
}

int
jthread_join(jthread_t th, void **thread_return)
{
  int ret = 0;
  while ((ret = sys_kthread_join(th, thread_return)) < 0) 
  {
    errno = -ret;
    if ((errno == E_NOT_IN_PROP_PROC || errno == E_NOT_ZOMB_OR_CANC))
      sys_yield();
    else return ret;
  }
  return 0;
}

void
jthread_exit(void *retval)
{
  sys_kthread_exit(retval);
}

int 
jthread_cancel(jthread_t thread){
  int ret = sys_kthread_cancel(thread);
  return errno = -ret, ret;
}

int 
jthread_setcpu(jthread_t thread, int cpunum){
  int ret = sys_kthread_setaffinity(thread, 1ULL << cpunum);
  return errno = -ret, ret;
}


int
jthread_mutex_lock(jthread_mutex_t *mutex)
{
  while (xchg((uint32_t *)&mutex->locked, 1) != 0)
    asm volatile ("pause");
  mutex->owner = envs[ENVX(sys_getenvid())].env_id;
  return 0;
}

int
jthread_mutex_trylock(jthread_mutex_t *mutex)
{
  if (xchg((uint32_t *)&mutex->locked, 1) != 0)
    return errno = E_UNSPECIFIED, -E_UNSPECIFIED;
  return 0;
}

int
jthread_mutex_unlock(jthread_mutex_t *mutex)
{
  if (mutex->owner != envs[ENVX(sys_getenvid())].env_id)
    return -1;
  xchg((uint32_t *)&mutex->locked, 0);
  return 0;
}
