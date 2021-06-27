/*
 * this program tests if the thread are being created and run
 * normally
 *
 */
#include <inc/lib.h>
#include <inc/jthread.h>


void *fun1(void *arg) {
  cprintf("wow i am fun 1, i am now being executed\n");
  return NULL;
}
 
void *fun2(void *arg) {
  cprintf("i am fun 2, now being executed, i am in an endless cycle, please kill me\n");
  while(1){}
  return NULL;
}

void *fun3(void *arg) {
  cprintf("wohoo i am fun 3, i am now being executed\n");
  return NULL;
}

void umain(int argc, char *argv[]) {
  jthread_t t1, t2, t3;
  cprintf("creating thread 1\n");
  jthread_create(&t1, NULL, fun1, NULL);
  cprintf("creating thread 2\n");
  jthread_create(&t2, NULL, fun2, NULL);
  cprintf("creating thread 3\n");
  jthread_create(&t3, NULL, fun3, NULL);
  cprintf("calling join on t1\n");
  jthread_join(t1, NULL);
  cprintf("cancelling thread 2\n");
  jthread_cancel(t2);
  cprintf("calling join on t2\n");
  jthread_join(t2, NULL);
  cprintf("calling join on t3\n");
  jthread_join(t3, NULL);

}
