/*
 * this program tests if the thread are being created and run
 * normally
 *
 */
#include <inc/lib.h>
#include <inc/jthread.h>

int a = 0;
int b = 0;

void *fun1(void *arg) {
	const volatile struct Env * fun1env = &envs[ENVX(sys_getenvid())];
	int i = 1, j = *((int *)arg), k = 1, l;
	a+=1;
	b+=1;
	cprintf("func1 b = %d\n", b);
	jthread_setcpu(sys_getenvid(), 2);
	cprintf("\nnaffin mask %ld\n\n", fun1env->affinity_mask);
	cprintf("\nnumber cpu before sys in fun1 %d\n\n", fun1env->cpunum);
	sys_yield();
	cprintf("\nnumber cpu in fun1 %d\n\n", fun1env->cpunum);
	cprintf("\nenvId in fun1 %d %d\n\n", sys_getenvid(), fun1env->env_id);
	cprintf("\nnaffin mask %ld\n\n", fun1env->affinity_mask);
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 1: k = %d\n", k++);
	}
	return NULL;
}
 
void *fun2(void *arg) {
	int i = 1, j = *((int *)arg), k = 1, l;
	a+=1;
	b+=2;
	cprintf("func2 b = %d\n", b);
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 2: k = %d\n", k++);
	}
	while(1){}
	return NULL;
}

void *fun3(void *arg) {
	int i = 1, j = *((int *)arg), k = 1, l;
	a+=1;
	b+=3;
	cprintf("func3 b = %d\n", b);
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 3: k = %d\n", k++);
	}
	return (void*)2;
}

void umain(int argc, char *argv[]) {
	jthread_t t1, t2, t3;//, t4;
	int repeat = 5;
	int* return_value;
	cprintf("creating thread 1\n");
	jthread_create(&t1, NULL, fun1, &repeat);
	cprintf("creating thread 1.1\n");
	//jthread_create(&t4, NULL, fun1, &repeat);
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
	//jthread_join(t4, NULL);
	cprintf("return_value: %ld\n", (uintptr_t)return_value);
	cprintf("a not thread_local %d\n", a);
	cprintf("b thread_local %d\n", b);
}
