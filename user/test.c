/*
 * this program tests if the thread are being created and run
 * normally
 * compile this program with any mythread library (one-one or many-one)
 * and then run the executable by giving it the number of times for
 * which each function prints a line
 * after all functions print lines for that number, then they exit and
 * main program also exits
 *
 */

#include <inc/lib.h>
#include <inc/jthread.h>

void *fun1(void *arg) {
	cprintf("IN FUN 1");
	int i = 1, j = *((int *)arg), k = 1, l;
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
	return NULL;
}

void *fun3(void *arg) {
	int i = 1, j = *((int *)arg), k = 1, l;
	while(i++ <= j) {
		for(l = 0; l < 1000000; l++);
		cprintf("I am in function 3: k = %d\n", k++);
	}
	return NULL;
}

void umain(int argc, char *argv[]) {
	jthread_t t1; //, t2, t3;
	int repeat = 5;
	// if(argc < 2) {
	// 	printf("Usage: %s number_of_repeat\n", argv[0]);
	// 	exit(0);
	// }
	cprintf("creating thread 1\n");
	jthread_create(&t1, NULL, fun1, &repeat);
	// cprintf("creating thread 2\n");
	// jthread_create(&t2, NULL, fun2, &repeat);
	// cprintf("creating thread 3\n");
	// jthread_create(&t3, NULL, fun3, &repeat);
	// cprintf("calling join on t1\n");
	jthread_join(t1, NULL);
	cprintf("calling join on t2\n");
	// jthread_join(t2, NULL);
	// cprintf("calling join on t3\n");
	// jthread_join(t3, NULL);
}
