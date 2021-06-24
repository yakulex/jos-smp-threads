/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_SCHED_H
#define JOS_KERN_SCHED_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#define MIN_ENV_TICKS 10;
_Noreturn void sched_yield(void);
void sched_decrease(void);

#endif /* !JOS_KERN_SCHED_H */
