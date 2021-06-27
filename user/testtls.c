/*
 * this program tests if the thread local storage is working normally
 */
#include <inc/lib.h>
#include <inc/jthread.h>
#include <inc/errno.h>

int a = 0;
__thread struct Trapframe trp1[1000]; // testing huge amount of memory in tls
__thread uint64_t b = 5;
__thread uint64_t d;
__thread uint64_t e = 0;
__thread uint64_t f = 2;
__thread uint64_t g = 2;

void *fun1(void *arg) {
	int i = 1, j = *((int *)arg), k = 1, l;
	a+=1;
	b+=1;
	d+=1;
	e+=1;
	f+=1;
	g+=1;
	cprintf("b thread_local f1 %ld\n", b); // 6
	cprintf("d thread_local f1 %ld\n", d); // 1
	cprintf("e thread_local f1 %ld\n", e); // 1 
	cprintf("f thread_local f1 %ld\n", f); // 3
	cprintf("g thread_local f1 %ld\n", g); // 3
	cprintf("func1 b = %ld\n", b); // 6
	cprintf("a not thread_local in fun1 %d\n", a); // 7
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 1: k = %d\n", k++);
	}
	return NULL;
}
 
void *fun2(void *arg) {
	int i = 1, j = *((int *)arg), k = 1, l;
	a+=2;
	b+=2;
	cprintf("func2 b = %ld\n", b); // 7
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 2: k = %d\n", k++);
	}
	while(1){}
	return NULL;
}

void *fun3(void *arg) {
	int i = 1, j = *((int *)arg), k = 1, l;
	a+=3;
	b+=3;
	cprintf("func3 b = %ld\n", b);
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 3: k = %d\n", k++);
	}
	return (void*)2;
}

void umain(int argc, char *argv[]) {
	jthread_t t1, t2, t3, t4;
	int repeat = 5;
	cprintf("b thread_local %ld\n", b);
	cprintf("d thread_local %ld\n", d);
	cprintf("e thread_local %ld\n", e);
	cprintf("f thread_local %ld\n", f);
	cprintf("g thread_local %ld\n", g);

	int* return_value;
	cprintf("creating thread 1\n");
	jthread_create(&t1, NULL, fun1, &repeat);
	cprintf("creating thread 1.1\n");
	jthread_create(&t4, NULL, fun1, &repeat);
	cprintf("creating thread 2\n");
	jthread_create(&t2, NULL, fun2, &repeat);
	cprintf("creating thread 3\n");
	jthread_create(&t3, NULL, fun3, &repeat);
	cprintf("calling join on t1\n");
	int ret_join_error = jthread_join(t1, NULL);
	cprintf("join  error %d\n", ret_join_error); // 0
	int ret_cancel_error = jthread_cancel(t2);
	cprintf("cancel error %d\n", ret_cancel_error); 
	cprintf("calling join on t2\n");
	jthread_join(t2, NULL);
	cprintf("calling join on t3\n");
	jthread_join(t3, (void*)&return_value);
	cprintf("calling join on t4\n");
	jthread_join(t4, NULL);
	cprintf("return_value: %ld\n", (uintptr_t)return_value);

	ret_join_error = jthread_join(t1, NULL);
	cprintf("%d\n", ret_join_error); // 0

	cprintf("a not thread_local %d\n", a); // 7
	// must be same
	cprintf("b thread_local %ld\n", b);
	cprintf("d thread_local %ld\n", d);
	cprintf("e thread_local %ld\n", e);
	cprintf("f thread_local %ld\n", f);
	cprintf("g thread_local %ld\n", g);

	cprintf("error from create = %d\n", jthread_create(&t1, NULL, NULL, &repeat));
	cprintf("errno = %d\n", errno);
}

