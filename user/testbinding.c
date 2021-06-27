/*
 * this program tests if the thread binding is working
 * normally
 *
 */

#include <inc/lib.h>
#include <inc/jthread.h>

void *fun1(void *arg) {
	const volatile struct Env * fun1env = &envs[ENVX(sys_getenvid())];
	int i = 1, j = *((int *)arg), k = 1, l;
	cprintf("\nnumber cpu before sys in fun1 %d\n\n", fun1env->cpunum);
	jthread_setcpu(sys_getenvid(), 1);
	cprintf("\nnaffin mask %ld\n\n", fun1env->affinity_mask);
	sys_yield();
	cprintf("\nnumber cpu in fun1 %d\n\n", fun1env->cpunum);
	cprintf("\nenvId in fun1 %d %d\n\n", sys_getenvid(), fun1env->env_id);
	cprintf("\naffin mask %ld\n\n", fun1env->affinity_mask);
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 1: k = %d\n", k++);
	}
	return NULL;
}
 
void *fun2(void *arg) {
	int i = 1, j = *((int *)arg), k = 1, l;
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 2: k = %d\n", k++);
	}
	while(1){}
	return NULL;
}

void *fun3(void *arg) {
	int i = 1, j = *((int *)arg), k = 1, l;
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 3: k = %d\n", k++);
	}
	return (void*)2;
}

void umain(int argc, char *argv[]) {
	jthread_t t1, t2, t3;
	int repeat = 5;
	int* return_value;
	cprintf("creating thread 1\n");
	jthread_create(&t1, NULL, fun1, &repeat);
	cprintf("creating thread 2\n");
	jthread_create(&t2, NULL, fun2, &repeat);
	cprintf("creating thread 3\n");
	jthread_create(&t3, NULL, fun3, &repeat);
	cprintf("calling join on t1\n");
	jthread_join(t1, NULL);
	jthread_cancel(t2);
	cprintf("calling join on t2\n");
	jthread_join(t2, NULL);
	cprintf("calling join on t3\n");
	jthread_join(t3, (void*)&return_value);
	cprintf("calling join on t4\n");
	cprintf("return_value: %ld\n", (uintptr_t)return_value);
}
