#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/env.h>
#include <kern/monitor.h>
#include <kern/spinlock.h>
#include <kern/sched.h>


_Noreturn void sched_halt(void);
void sched_update_ticks(void);

/* Choose a user environment to run and run it */
_Noreturn void
sched_yield(void) {
    /* Implement simple round-robin scheduling.
     *
     * Search through 'envs' for an ENV_RUNNABLE environment in
     * circular fashion starting just after the env was
     * last running.  Switch to the first such environment found.
     *
     * If no envs are runnable, but the environment previously
     * running is still ENV_RUNNING, it's okay to
     * choose that environment.
     *
     * If there are no runnable environments,
     * simply drop through to the code
     * below to halt the cpu */

    // LAB 3: Your code here:
    int go = curenv ? ENVX(curenv_getid()) : 0;
    int max_ticks = 0;
    struct Env * next_env = curenv;
    for (int i = 0; i < NENV; ++i) {
        int cur = ENVX(i + go);
        if (envs[cur].env_status == ENV_RUNNABLE && envs[cur].ticks > max_ticks && (envs[cur].affinity_mask & 1ULL << cpunum())) {
            cprintf("b %d %lu %llu %llu\n", envs[cur].env_id, envs[cur].affinity_mask, 1ULL << cpunum(), envs[cur].affinity_mask & 1ULL << cpunum());
            max_ticks = envs[cur].ticks;
            next_env = envs + cur;
        }
    }

    if (curenv && curenv->env_status == ENV_RUNNING && curenv->ticks > max_ticks && (curenv->affinity_mask & 1ULL << cpunum())) {
        cprintf("b %d %lu %llu %llu\n", curenv->env_id, curenv->affinity_mask, 1ULL << cpunum(), curenv->affinity_mask & 1ULL << cpunum());
        
        max_ticks = curenv->ticks;
        next_env = curenv;
    }
    if (max_ticks > 0){
        cprintf("A%lu\n", next_env->affinity_mask);
        env_run(next_env);
    }

    sched_update_ticks();

    go = curenv ? ENVX(curenv_getid()) : 0;
    max_ticks = 0;
    next_env = curenv;
    for (int i = 0; i < NENV; ++i) {
        int cur = ENVX(i + go);
        if (envs[cur].env_status == ENV_RUNNABLE && envs[cur].ticks > max_ticks && (envs[cur].affinity_mask & 1ULL << cpunum())) {
            cprintf("b %d %lu %llu %llu\n", envs[cur].env_id, envs[cur].affinity_mask, 1ULL << cpunum(), envs[cur].affinity_mask & 1ULL << cpunum());
            max_ticks = envs[cur].ticks;
            next_env = envs + cur;
        }
    }

    if (curenv && curenv->env_status == ENV_RUNNING && curenv->ticks > max_ticks && (curenv->affinity_mask & 1ULL << cpunum())) {
        cprintf("b %d %lu %llu %llu\n", curenv->env_id, curenv->affinity_mask, 1ULL << cpunum(), curenv->affinity_mask & 1ULL << cpunum());
        
        max_ticks = curenv->ticks;
        next_env = curenv;
    }
    if (max_ticks > 0){
        cprintf("A%lu\n", next_env->affinity_mask);
        env_run(next_env);
    }

    /* No runnable environments,
     * so just halt the cpu */
    cprintf("num cpu halt %d\n", cpunum());
    sched_halt(); 
}

void 
sched_decrease(void) {
    curenv->ticks--;
}

void
sched_update_ticks(void){
    int i;
    for (i = 0; i < NENV; i++){
        if (envs[i].env_status == ENV_RUNNABLE){
            envs[i].ticks = envs[i].priority + MIN_ENV_TICKS;
        }
    }
    if (curenv && curenv->env_status == ENV_RUNNING){
        curenv->ticks = curenv->priority + MIN_ENV_TICKS;
    }
}


/* Halt this CPU when there is nothing to do. Wait until the
 * timer interrupt wakes it up. This function never returns */
_Noreturn void
sched_halt(void) {

    /* For debugging and testing purposes, if there are no runnable
     * environments in the system, then drop into the kernel monitor */
    int i;
    for (i = 0; i < NENV; i++)
        if (envs[i].env_status == ENV_RUNNABLE ||
            envs[i].env_status == ENV_RUNNING) break;
    if (i == NENV) {
        cprintf("No runnable environments in the system!\n");
        for (;;) monitor(NULL);
    }

    /* Mark that no environment is running on CPU */
    curenv = NULL;
    smart_unlock_kernel();
    /* Reset stack pointer, enable interrupts and then halt */
    asm volatile(
            "movq $0, %%rbp\n"
            "movq %0, %%rsp\n"
            "pushq $0\n"
            "pushq $0\n"
            "sti\n"
            "hlt\n" ::"a"(thiscpu->cpu_ts.ts_rsp0));

    /* Unreachable */
    for (;;)
        ;
}
