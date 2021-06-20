#ifndef JOS_INC_SPINLOCK_H
#define JOS_INC_SPINLOCK_H

#include <inc/types.h>
#include <kern/traceopt.h>
#include <kern/cpu.h>

/* Mutual exclusion lock */
struct spinlock {
    unsigned locked; /* Is the lock held? */

#if trace_spinlock
    /* For debugging: */
    char *name;        /* Name of lock */
    uintptr_t pcs[10]; /* The call stack (an array of program counters)
                        * that locked the lock */
#endif
};

void __spin_initlock(struct spinlock *lk, char *name);
void spin_lock(struct spinlock *lk);
void spin_unlock(struct spinlock *lk);

#define spin_initlock(lock) __spin_initlock(lock, #lock)

extern struct spinlock kernel_lock;

static inline void
lock_kernel(void) {
    spin_lock(&kernel_lock);
}

static inline void
smart_lock_kernel(void) {
    if (!thiscpu->in_kernel)
    {
        spin_lock(&kernel_lock);
    }
    thiscpu->in_kernel = true;

}

static inline void
unlock_kernel(void) {
    spin_unlock(&kernel_lock);

    /* Normally we wouldn't need to do this, but QEMU only runs
     * one CPU at a time and has a long time-slice.  Without the
     * pause, this CPU is likely to reacquire the lock before
     * another CPU has even been given a chance to acquire it. */
    asm volatile("pause");
}

static inline void
smart_unlock_kernel(void) {
    assert (thiscpu->in_kernel);
    spin_unlock(&kernel_lock);
    thiscpu->in_kernel = false;
    /* Normally we wouldn't need to do this, but QEMU only runs
     * one CPU at a time and has a long time-slice.  Without the
     * pause, this CPU is likely to reacquire the lock before
     * another CPU has even been given a chance to acquire it. */
    asm volatile("pause");
}

#endif
