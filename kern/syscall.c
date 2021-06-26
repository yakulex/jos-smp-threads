/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/jthread.h>

#include <kern/console.h>
#include <kern/env.h>
#include <kern/kclock.h>
#include <kern/pmap.h>
#include <kern/sched.h>
#include <kern/syscall.h>
#include <kern/trap.h>
#include <kern/traceopt.h>
#include <kern/spinlock.h>

#define DEBUGTHREAD 0

/* Print a string to the system console.
 * The string is exactly 'len' characters long.
 * Destroys the environment on memory errors. */
static int
sys_cputs(const char *s, size_t len) {
    // LAB 8: Your code here
    //for (size_t i = 0; i < len; ++i) {
      //  cputchar(s[i]);
    //}
    //cprintf("g\n");
    user_mem_assert(curenv, s, len, PROT_R); 
    for (size_t i = 0; i < len; ++i) {
        cputchar(s[i]);
    }
    //cprintf("%.*s", (int)len, s);
    /* Check that the user has permission to read memory [s, s+len).
    * Destroy the environment if not. */
    

    return 0;
}

/* Read a character from the system console without blocking.
 * Returns the character, or 0 if there is no input waiting. */
static int
sys_cgetc(void) {
    // LAB 8: Your code here
    return cons_getc();
}

/* Returns the current environment's envid. */
static envid_t
sys_getenvid(void) {
    // LAB 8: Your code here

    return curenv->env_id;
}

/* Destroy a given environment (possibly the currently running environment).
 *
 *  Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 */
static int
sys_env_destroy(envid_t envid) {
    // LAB 8: Your code here.
    int res = 0;
    struct Env* env;
    res = envid2env(envid, &env, 1);
    if (res < 0) {
        return -E_BAD_ENV;
    }

#if 1 /* TIP: Use this snippet to log required for passing grade tests info */
    if (trace_envs) {
        cprintf(env == curenv ?
                        "[%08x] exiting gracefully\n" :
                        "[%08x] destroying %08x\n",
                curenv->env_id, env->env_id);
    }
#endif

    env_destroy(env);

    return 0;
}

/* Deschedule current environment and pick a different one to run. */
static void
sys_yield(void) {
    // LAB 9: Your code here
    sched_yield();
}

/* Allocate a new environment.
 * Returns envid of new environment, or < 0 on error.  Errors are:
 *  -E_NO_FREE_ENV if no free environment is available.
 *  -E_NO_MEM on memory exhaustion. */
static envid_t
sys_exofork(void) {
    /* Create the new environment with env_alloc(), from kern/env.c.
     * It should be left as env_alloc created it, except that
     * status is set to ENV_NOT_RUNNABLE, and the register set is copied
     * from the current environment -- but tweaked so sys_exofork
     * will appear to return 0. */

    // LAB 9: Your code here

    struct Env *e = NULL;
    int res;

    if ((res = env_alloc(&e, curenv->env_id, ENV_TYPE_USER)) < 0) { // maybe_ ENV_TYPE_USER
      return res;
    }

    e->env_status = ENV_NOT_RUNNABLE;
    e->env_tf = curenv->env_tf;
    e->env_pgfault_upcall = curenv->env_pgfault_upcall;
    e->priority = curenv->priority;
    e->ticks = curenv->ticks;

    e->env_tf.tf_regs.reg_rax = 0; //child process return 0

    return e->env_id; 
}

/* Set envid's env_status to status, which must be ENV_RUNNABLE
 * or ENV_NOT_RUNNABLE.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if status is not a valid status for an environment. */
static int
sys_env_set_status(envid_t envid, int status) {
    /* Hint: Use the 'envid2env' function from kern/env.c to translate an
     * envid to a struct Env.
     * You should set envid2env's third argument to 1, which will
     * check whether the current environment has permission to set
     * envid's status. */

    // LAB 9: Your code here

    struct Env *e;
    if (envid2env(envid, &e, 1) < 0) {
        return -E_BAD_ENV;
    }
    if (!(status == ENV_RUNNABLE || status == ENV_NOT_RUNNABLE)) {
        return -E_INVAL;
    }
    e->env_status = status;
    return 0;
}

/* Set the page fault upcall for 'envid' by modifying the corresponding struct
 * Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
 * kernel will push a fault record onto the exception stack, then branch to
 * 'func'.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid. */
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func) {
    // LAB 9: Your code here:
    struct Env *e;
    if (envid2env(envid, &e, 1) < 0) {
        return -E_BAD_ENV;
    }
    e->env_pgfault_upcall = func;

    return 0;
}

/* Allocate a region of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 * The page's contents are set to 0.
 * If a page is already mapped at 'va', that page is unmapped as a
 * side effect.
 * 
 * This call should work with or without ALLOC_ZERO/ALLOC_ONE flags
 * (set them if they are not already set)
 * 
 * It allocates memory lazily so you need to use map_region
 * with PROT_LAZY and ALLOC_ONE/ALLOC_ZERO set.
 * 
 * Don't forget to set PROT_USER_
 * 
 * PROT_ALL is useful for validation.
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if va >= MAX_USER_ADDRESS, or va is not page-aligned.
 *  -E_INVAL if perm is inappropriate (see above).
 *  -E_NO_MEM if there's no memory to allocate the new page,
 *      or to allocate any necessary page tables. */
static int
sys_alloc_region(envid_t envid, uintptr_t addr, size_t size, int perm) {
    // LAB 9: Your code here:

    struct Env *e;

    if (envid2env(envid, &e, 1) < 0) {
        return -E_BAD_ENV;
    }
    if ( addr >= MAX_USER_ADDRESS || PAGE_OFFSET(addr)) {
        return -E_INVAL;
    }
    if (perm & ~PROT_ALL) {
        return -E_INVAL;
    }

    if (map_region(&e->address_space, addr, NULL, 0, size, perm | ALLOC_ZERO | PROT_LAZY | PROT_USER_)) {
       panic("Cannot map physical region at %p of size %lud", (void *)0, (uintptr_t)size);
       return -E_NO_MEM;
    }
 
    return 0;
}

/* Map the region of memory at 'srcva' in srcenvid's address space
 * at 'dstva' in dstenvid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_alloc_region, except
 * that it also does not supprt ALLOC_ONE/ALLOC_ONE flags.
 * 
 * You only need to check alignment of addresses, perm flags and
 * that addresses are a part of user space. Everything else is
 * already checked inside map_region().
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
 *      or the caller doesn't have permission to change one of them.
 *  -E_INVAL if srcva >= MAX_USER_ADDRESS or srcva is not page-aligned,
 *      or dstva >= MAX_USER_ADDRESS or dstva is not page-aligned.
 *  -E_INVAL is srcva is not mapped in srcenvid's address space.
 *  -E_INVAL if perm is inappropriate (see sys_page_alloc).
 *  -E_INVAL if (perm & PROT_W), but srcva is read-only in srcenvid's
 *      address space.
 *  -E_NO_MEM if there's no memory to allocate any necessary page tables. */

static int
sys_map_region(envid_t srcenvid, uintptr_t srcva,
               envid_t dstenvid, uintptr_t dstva, size_t size, int perm) {
    // LAB 9: Your code here
    struct Env *srcenv, *dstenv;
    //cprintf("map\n");

    if (envid2env(srcenvid, &srcenv, 1) < 0 || envid2env(dstenvid, &dstenv, 1) < 0) {
        cprintf("env\n");
        return -E_BAD_ENV;
    }
    if (srcva >= MAX_USER_ADDRESS || PAGE_OFFSET(srcva) || 
      dstva >= MAX_USER_ADDRESS || PAGE_OFFSET(dstva)) {
        cprintf("MAX_USER_ADDRESS\n");
        return -E_INVAL;
    }
    // if (perm & ~(PROT_W | )) {
    //     cprintf("flag1\n");
    //     return -E_INVAL;
    // }
    // don't know how -- -E_INVAL is srcva is not mapped in srcenvid's address space.
    // how to prove srcva is read-only in srcenvid's address space ??
    // if ((perm & PROT_W)) {
    //     cprintf("flag2\n");
    //     return -E_INVAL;
    // }
    if (map_region(&dstenv->address_space, dstva, &srcenv->address_space, srcva, size, perm | PROT_USER_)) { // maybe_ without PROT_LAZY
        panic("Cannot map physical region at %p of size %lud", (void *)srcva, (uintptr_t)size);
        cprintf("mem\n");
        return -E_NO_MEM;
    }
    //cprintf("map_end\n");

    return 0;
}

/* Unmap the region of memory at 'va' in the address space of 'envid'.
 * If no page is mapped, the function silently succeeds.
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if va >= MAX_USER_ADDRESS, or va is not page-aligned. */
static int
sys_unmap_region(envid_t envid, uintptr_t va, size_t size) {
    /* Hint: This function is a wrapper around unmap_region(). */
    //cprintf("unmap\n");
    // LAB 9: Your code here
    struct Env *env;

    if (envid2env(envid, &env, 1) < 0)  {
        return -E_BAD_ENV;
    }

    if (va >= MAX_USER_ADDRESS || PAGE_OFFSET(va)) {
        return -E_INVAL;
    }

    unmap_region(&env->address_space, va, size);

    return 0;
}

/* Try to send 'value' to the target env 'envid'.
 * If srcva < MAX_USER_ADDRESS, then also send region currently mapped at 'srcva',
 * so receiver also gets mapping.
 *
 * The send fails with a return value of -E_IPC_NOT_RECV if the
 * target is not blocked, waiting for an IPC.
 *
 * The send also can fail for the other reasons listed below.
 *
 * Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends;
 *    env_ipc_maxsz is set to min of size and it's current vlaue;
 *    env_ipc_from is set to the sending envid;
 *    env_ipc_value is set to the 'value' parameter;
 *    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
 * The target environment is marked runnable again, returning 0
 * from the paused sys_ipc_recv system call.  (Hint: does the
 * sys_ipc_recv function ever actually return?)
 *
 * If the sender wants to send a page but the receiver isn't asking for one,
 * then no page mapping is transferred, but no error occurs.
 * Send region size is the minimum of sized specified in sys_ipc_try_send() and sys_ipc_recv()
 * 
 * The ipc only happens when no errors occur.
 *
 * Returns 0 on success, < 0 on error.
 * Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist.
 *      (No need to check permissions.)
 *  -E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
 *      or another environment managed to send first.
 *  -E_INVAL if srcva < MAX_USER_ADDRESS but srcva is not page-aligned.
 *  -E_INVAL if srcva < MAX_USER_ADDRESS and perm is inappropriate
 *      (see sys_page_alloc).
 *  -E_INVAL if srcva < MAX_USER_ADDRESS but srcva is not mapped in the caller's
 *      address space.
 *  -E_INVAL if (perm & PTE_W), but srcva is read-only in the
 *      current environment's address space.
 *  -E_NO_MEM if there's not enough memory to map srcva in envid's
 *      address space. */
static int
sys_ipc_try_send(envid_t envid, uint32_t value, uintptr_t srcva, size_t size, int perm) {
    // LAB 9: Your code here
    struct Env *env;

    if (envid2env(envid, &env, 0) < 0)  {
        return -E_BAD_ENV;
    }
    if (!env->env_ipc_recving) {
        return -E_IPC_NOT_RECV;
    }

    if (srcva < MAX_USER_ADDRESS && env->env_ipc_dstva < MAX_USER_ADDRESS) {
        if (PAGE_OFFSET(srcva) || PAGE_OFFSET(env->env_ipc_dstva)) {
            return -E_INVAL;
        }
    }
    if (srcva < MAX_USER_ADDRESS) { 
        if (map_region(&env->address_space, env->env_ipc_dstva, &curenv->address_space, srcva, MIN(size, env->env_ipc_maxsz), perm | PROT_USER_)) { 
            panic("Cannot map physical region at %p of size %lud", (void *)srcva, (uintptr_t)size);
            return -E_NO_MEM;
        }
        env->env_ipc_perm = perm;
    } else env->env_ipc_perm = 0;

    env->env_ipc_recving = 0;
    env->env_ipc_from = curenv->env_id;
    env->env_ipc_value = value;
    env->env_status = ENV_RUNNABLE;
    return 0;
}

/* Block until a value is ready.  Record that you want to receive
 * using the env_ipc_recving, env_ipc_maxsz and env_ipc_dstva fields of struct Env,
 * mark yourself not runnable, and then give up the CPU.
 *
 * If 'dstva' is < MAX_USER_ADDRESS, then you are willing to receive a page of data.
 * 'dstva' is the virtual address at which the sent page should be mapped.
 *
 * This function only returns on error, but the system call will eventually
 * return 0 on success.
 * Return < 0 on error.  Errors are:
 *  -E_INVAL if dstva < MAX_USER_ADDRESS but dstva is not page-aligned;
 *  -E_INVAL if dstva is valid and maxsize is 0,
 *  -E_INVAL if maxsize is not page aligned.
 */
static int
sys_ipc_recv(uintptr_t dstva, uintptr_t maxsize) {
    // LAB 9: Your code here
    if (maxsize == 0 && dstva < MAX_USER_ADDRESS){
        return -E_INVAL;
    }
    if (PAGE_OFFSET(maxsize)){
        return -E_INVAL;
    }
    if (dstva < MAX_USER_ADDRESS && PAGE_OFFSET(dstva)){
        return -E_INVAL;
    }

    curenv->env_ipc_recving = 1;
    curenv->env_status = ENV_NOT_RUNNABLE;
    if (dstva < MAX_USER_ADDRESS) {
        curenv->env_ipc_dstva = dstva;
        curenv->env_ipc_maxsz = maxsize;
    } 
    curenv->env_tf.tf_regs.reg_rax = 0;
    sched_yield();
    return 0;
}

/*
 * This function sets trapframe and is unsafe
 * so you need:
 *   -Check environment id to be valid and accessible
 *   -Check argument to be valid memory
 *   -Use nosan_memcpy to copy from usespace
 *   -Prevent privilege escalation by overriding segments
 *   -Only allow program to set safe flags in RFLAGS register
 *   -Force IF to be set in RFLAGS
 */
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf) {
    // LAB 11: Your code here
    struct Env *env;
    int res;

    if ((res = envid2env(envid, &env, 1)) < 0)
        return res;

    user_mem_assert(env, tf, sizeof(struct Trapframe), 0);

    nosan_memcpy(&env->env_tf, tf, sizeof(struct Trapframe));

    env->env_tf.tf_cs = GD_UT | 3;
    env->env_tf.tf_ds = GD_UD | 3;
    env->env_tf.tf_es = GD_UD | 3;
    env->env_tf.tf_ss = GD_UD | 3;

    env->env_tf.tf_rflags &= 0xFFF;
    env->env_tf.tf_rflags |= FL_IF;

    return 0;
}

/* Return date and time in UNIX timestamp format: seconds passed
 * from 1970-01-01 00:00:00 UTC. */
static int
sys_gettime(void) {
    // LAB 12: Your code here
    return gettime();
}

/*
 * This function return the difference between maximal
 * number of references of regions [addr, addr + size] and [addr2,addr2+size2]
 * if addr2 is less than MAX_USER_ADDRESS, or just
 * maximal number of references to [addr, addr + size]
 * 
 * Use region_maxref() here.
 */
static int
sys_region_refs(uintptr_t addr, size_t size, uintptr_t addr2, uintptr_t size2) {
    // LAB 10: Your code here
    
    int max1 = region_maxref(current_space, addr, size);
    
    if (addr2 >= MAX_USER_ADDRESS) return max1;

    int max2 = region_maxref(current_space, addr2, size2);

    return max1 - max2;
}

static int
sys_kthread_create(void *entry, void *start, void *arg)
{
  jthread_t tid;
  int ret;
  if ((tid = sys_exofork()) < 0)
    return tid;

  if (DEBUGTHREAD)
    cprintf("[%08x] Making new thread with id: %08x\n", curenv->env_id, tid);
  // Find the Env
  struct Env *e;
  if ((ret = envid2env(tid, &e, 0)) < 0)
    return -E_INVAL;

  // Set thread state

  e->env_child_thread = true;
  e->env_process_envid = curenv->env_process_envid;

  struct Env *process;
  if ((ret = envid2env(e->env_process_envid, &process, 0)) < 0)
    return ret;
  if (DEBUGTHREAD)
    cprintf("Our process's envid: %08x\n", process->env_id);
  // Sanity check about the calling process
  if (!(process->env_status == ENV_NOT_RUNNABLE ||
        process->env_status == ENV_RUNNABLE ||
        process->env_status == ENV_RUNNING))
    return -E_INVAL;
  process->env_num_threads++;
  int threadnum = process->env_num_threads;
  if (DEBUGTHREAD)
    cprintf("%08x's Thread number %d\n", curenv->env_id, threadnum);

  // Find the last thread on the linked list
  struct Env *next_thread = curenv;
  while (next_thread->env_next_thread)
    next_thread = next_thread->env_next_thread;
  // Add this env to the thread list
  next_thread->env_next_thread = e;

  // Use the same address space and pgfault handler
  int r;
  if ((r = sys_map_region(curenv->env_id, 0, tid, 0, MAX_USER_ADDRESS, PROT_ALL & ~PROT_LAZY)) < 0)
        panic("map region fault: %i", r);

  if ((r = sys_env_set_pgfault_upcall(tid, curenv->env_pgfault_upcall)) < 0)
       panic("sys_env_set_pgfault_upcall: %i", r);

  // Allocate new stack

    uintptr_t va = (uintptr_t)(USER_STACK_TOP - (threadnum * (USER_STACK_SIZE + PAGE_SIZE) + USER_STACK_SIZE));
    int perm = PROT_RWX | PROT_USER_ | ALLOC_ZERO;
    if (map_region(&e->address_space, va, NULL, 0, USER_STACK_SIZE, perm))
        panic("Cannot map physical region at %p of size %lu", (void *)va, (uintptr_t) USER_STACK_SIZE);

    if (DEBUGTHREAD)
        cprintf("New page mapped at va:0x%08lx\n", va);
    
    if (curenv->env_tls){
        size_t master_tls_size = curenv->env_tls->tls_master_size;
        size_t master_tls_size_aligned = -(-master_tls_size & ~(curenv->env_tls->tls_master_align-1));
        size_t tls_size = master_tls_size_aligned + sizeof(struct EnvTLS);
        size_t envtls_struct_offset = master_tls_size_aligned;

        size_t page_size_of_tls = ROUNDUP(tls_size, PAGE_SIZE);
        uintptr_t tls_va = ROUNDDOWN((uintptr_t)curenv->env_tls->tls_master_mmap, PAGE_SIZE) - page_size_of_tls * threadnum - tls_size;
        int perm = PROT_R | PROT_W | PROT_USER_ | ALLOC_ZERO;
        if (map_region(&e->address_space, ROUNDDOWN(tls_va, PAGE_SIZE), NULL, 0, page_size_of_tls, perm))
            panic("Cannot map physical region at %p of size %lu", (void *)tls_va, (uintptr_t) page_size_of_tls);

        // cprintf("master tls 0x%08lx\n", master_tls_size);
        
        struct AddressSpace *curr_asp = switch_address_space(&e->address_space);
        memcpy((void *)tls_va, curenv->env_tls->tls_master_mmap, master_tls_size);
        struct EnvTLS *env_tls = (struct EnvTLS *) (tls_va + envtls_struct_offset);
        env_tls->self = env_tls;
        env_tls->tls_master_mmap = curenv->env_tls->tls_master_mmap;
        env_tls->tls_master_size = master_tls_size;
        env_tls->tls_master_align = curenv->env_tls->tls_master_align;
        env_tls->tls_mmap = (void *)tls_va;
        env_tls->tls_size = tls_size;

        e->env_tls = env_tls;
        e->env_tf.tf_fsbase = (uint64_t)e->env_tls;
        switch_address_space(curr_asp);

    }

  
  // Set the rip and rsp to the new values
  e->env_tf.tf_rip = (uintptr_t)entry;
  e->env_tf.tf_rsp = (uintptr_t)(va + USER_STACK_SIZE);

  // First arg
  e->env_tf.tf_regs.reg_rdi = (uint64_t)start;
  // Second arg
  e->env_tf.tf_regs.reg_rsi = (uint64_t)arg;

  if ((r = sys_env_set_status(tid, ENV_RUNNABLE)) < 0)
        panic("sys_env_set_status: %i", r);

  
  if (DEBUGTHREAD)
  {
    cprintf("Made a runnable thread\n");
    cprintf("arg: %08llx\n", va + USER_STACK_SIZE - 8);
    cprintf("start: %08llx\n", va + USER_STACK_SIZE - 16);
    cprintf("Thread's rip: %08lx\n", e->env_tf.tf_rip);
    cprintf("Thread's rsp: %08lx\n", e->env_tf.tf_rsp);
  }

  return tid;
}

static int
sys_kthread_join(jthread_t tid, void **retstore)
{

  struct Env *e;
  int ret;
  if ((ret = envid2env(tid, &e, 0)) < 0)
    return ret;
  if (e->env_thread_status != THREAD_ZOMBIE && e->env_thread_status != THREAD_CANCELLED){
    return -1;
  }
    
  // Check that we have permission to reap this thread
  if (e->env_process_envid != curenv->env_process_envid){
    return -1;
  }

  // set retval
  if (retstore) *retstore = e->env_thread_retval;
  

  // Mark the env we reaped as done

  e->env_thread_status = THREAD_DONE;
  e->env_thread_retval = NULL;

  return 0;
}

static int
sys_kthread_exit(void *retval)
{
  if (DEBUGTHREAD)
    cprintf("In jthread_exit\n");
  if (curenv->env_child_thread)
  {
    curenv->env_thread_status = THREAD_ZOMBIE;
    curenv->env_thread_retval = retval;
    curenv->env_status = ENV_NOT_RUNNABLE;
  }
  else
  {
    env_destroy(curenv);
  }
  return 0;
}

static int 
sys_kthread_cancel(jthread_t tid){
    if (DEBUGTHREAD)
        cprintf("In jthread_cancel\n");
    struct Env *e;
    int ret;
    if ((ret = envid2env(tid, &e, 0)) < 0)
        return ret;
    
    // Check that we have permission to reap this thread
    if (e->env_process_envid != curenv->env_process_envid){
        return -1;
    }

    // Mark the env as CANCELED
    if (e->env_child_thread){

        e->env_thread_status = THREAD_CANCELLED;
        e->env_thread_retval = NULL;
        e->env_status = ENV_NOT_RUNNABLE;

    } else {
        env_destroy(e);
    }

    return 0;
}

static int 
sys_kthread_setaffinity(jthread_t tid, uint64_t mask) {
    if (DEBUGTHREAD)
        cprintf("In kthread_setaffinity\n");
    struct Env *e;
    int ret;
    if ((ret = envid2env(tid, &e, 0)) < 0)
        return ret;
    
    // Check that we have permission to reap this thread
    if (e->env_process_envid != curenv->env_process_envid) {
        return -1;
    }

    if (!(-1ULL >> (64 - ncpu) & mask)) {
        return -1;
    }

     e->affinity_mask = mask;

     return 0;
}

/* Dispatches to the correct kernel function, passing the arguments. */
uintptr_t
syscall(uintptr_t syscallno, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6) {
    /* Call the function corresponding to the 'syscallno' parameter.
     * Return any appropriate return value. */

    // LAB 8: Your code here

    if (syscallno == SYS_cputs) {
        sys_cputs((const char *)a1, (size_t)a2);
        return 0;
    } else if (syscallno == SYS_cgetc) {
        return sys_cgetc();
    } else if (syscallno == SYS_getenvid) {
        return sys_getenvid();
    } else if (syscallno == SYS_env_destroy) {
        return sys_env_destroy((envid_t)a1);
    } else if (syscallno == SYS_exofork) {
        return sys_exofork();
    } else if (syscallno == SYS_env_set_status) {
        return sys_env_set_status((envid_t) a1, (int) a2);
    } else if (syscallno == SYS_alloc_region) {
        return sys_alloc_region((envid_t) a1, a2, (size_t) a3, (int) a4);
    } else if (syscallno == SYS_map_region) {
        return sys_map_region((envid_t) a1, a2, (envid_t) a3, a4, (size_t) a5, (int) a6);
    } else if (syscallno == SYS_region_refs) {
        return sys_region_refs(a1, (size_t)a2, a3, a4);
    } else if (syscallno == SYS_env_set_trapframe) {
        return sys_env_set_trapframe((envid_t)a1, (struct Trapframe *)a2);
    } else if (syscallno == SYS_unmap_region) {
        return sys_unmap_region((envid_t) a1, a2, (size_t) a3);
    } else if (syscallno == SYS_env_set_pgfault_upcall) {
        return sys_env_set_pgfault_upcall((envid_t) a1, (void *) a2);
    } else if (syscallno == SYS_gettime) {
        return sys_gettime();
    } else if (syscallno == SYS_yield) {
        sys_yield();
        return 0;
    } else if (syscallno == SYS_ipc_try_send) {
        return sys_ipc_try_send((envid_t) a1, (uint32_t) a2, a3, (size_t) a4, (int) a5);
    } else if (syscallno == SYS_ipc_recv) {
        return sys_ipc_recv(a1, a2);
    } else if (syscallno == SYS_kthread_create) {
        return sys_kthread_create((void *)a1, (void *)a2, (void *)a3);
    } else if (syscallno == SYS_kthread_join) {
        return sys_kthread_join((jthread_t)a1, (void **)a2);
    } else if (syscallno == SYS_kthread_exit) {
        return sys_kthread_exit((void *)a1);
    } else if (syscallno == SYS_kthread_cancel) {
        return sys_kthread_cancel((jthread_t)a1);
    } else if (syscallno == SYS_kthread_setaffinity) {
        return sys_kthread_setaffinity((jthread_t)a1, (uint64_t)a2);
    }

    return -E_NO_SYS;
}
