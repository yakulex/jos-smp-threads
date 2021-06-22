#include <inc/lib.h>
#include <inc/jthread.h>

#define MATRIXSIZE 12
#define NUMTRIALS  10
#define DEBUG 1
#define DEBUGDET 1

int determ(int a[MATRIXSIZE][MATRIXSIZE], int n, bool thread, bool fork);

envid_t Ppid;

int
determ_fork(int a[MATRIXSIZE][MATRIXSIZE])
{
  return determ(a, MATRIXSIZE - 1, false, false);
}

int thread_determ(int a[MATRIXSIZE][MATRIXSIZE])
{
  if (DEBUGDET)
  {
    //cprintf("matrix location passed: 0x%08x\n", a);
    cprintf("matrix passed to thread:\n");
  }
  int i, j;
  if (DEBUGDET)
  {
    for (i = 0; i < MATRIXSIZE-1; i++)
    {
      for (j = 0; j < MATRIXSIZE-1; j++)
      {
        cprintf("%d ", a[i][j]);
      }
      cprintf("\n");
    }
  }
  return determ(a, MATRIXSIZE - 1, false, false);
}

// Modified code found at
// stackoverflow.com/questions/21220504/matrix-determinant-algorithm-c
int determ(int a[MATRIXSIZE][MATRIXSIZE], int n, bool thread, bool ipc)
{
  int det = 0, jt = 0, pt = 0, p, h, k, i, j, temp[MATRIXSIZE][MATRIXSIZE];
  jthread_t tids[MATRIXSIZE];
  envid_t pids[MATRIXSIZE];
  if (n == 1) 
  {
    return a[0][0];
  }
  if (n == 2)
  {
    return a[0][0] * a[1][1] - a[0][1] * a[1][0];
  }
  else
  {
    for (p = 0; p < n; p++)
    {
      h = 0;
      k = 0;
      for (i = 1; i < n; i++)
      {
        for (j = 0; j < n; j++)
        {
          if (j == p)
            continue;
          temp[h][k] = a[i][j];
          k++;
          if (k == n - 1)
          {
            h++;
            k = 0;
          }
        }
      }
      if (thread || ipc)
      {
        int* thread_matrix[MATRIXSIZE][MATRIXSIZE];
        int ti, tj;
        if (DEBUGDET)
        {
          cprintf("Before threading:\ntemp matrix:\n");
          for (ti = 0; ti < MATRIXSIZE - 1; ti++)
          {
            for (tj = 0; tj < MATRIXSIZE - 1; tj++)
              cprintf("%d ", temp[ti][tj]);
            cprintf("\n");
          }
        }
        for (ti = 0; ti < MATRIXSIZE - 1; ti++)
        {
          for (tj = 0; tj < MATRIXSIZE - 1; tj++) {
            cprintf("AAAAAAAAAAAAAAA\n");
            (*thread_matrix)[ti][tj] = temp[ti][tj];
            cprintf("AAAAAAAAAAAAAAA\n"); }
        }
        if (DEBUGDET)
        {
          cprintf("Copied matrix:\n");
          for (ti =0; ti < MATRIXSIZE - 1; ti++)
          {
            for(tj = 0 ; tj < MATRIXSIZE - 1; tj++)
              cprintf("%d ", (*thread_matrix)[ti][tj]);
            cprintf("\n");
          }
          //cprintf("Copied matrix location: 0x%08x\n", thread_matrix);
        }
        if (thread)
        {
          jthread_create(&tids[jt++], NULL, (void *(*)(void *))thread_determ, (void *)thread_matrix);
        }
        else // IPC
        {
          int ret;
          if ((ret = fork()) < 0)
            return ret;
          if (ret == 0)
          {
            int r;
            ipc_recv(&Ppid, NULL, NULL, NULL);
            r = determ(temp, n-1, false, false);
            ipc_send(Ppid, r, NULL, (size_t)PAGE_SIZE, 0);
            sys_kthread_exit(NULL); // XXX Hack in place of an exit syscall
          }
          else
          {
            pids[pt++] = ret;
            ipc_send(ret, 0, NULL, (size_t)PAGE_SIZE, 0); // Let them know who their daddy is
          }
        }
      }
      else
      {
        det = det + a[0][p]*(p % 2 ? -1 : 1) * determ(temp, n-1, false, false);
      }
    }
    if (thread)
    {
      for (jt = 0; jt < MATRIXSIZE; jt++)
      {
        int *ret;
        jthread_join(tids[jt], (void **)&ret);
        if (DEBUGDET)
        {
          cprintf("thread %d returns %d\n", jt, (int) *ret);
          cprintf("%d + %d * %d * %d = ", det, a[0][jt], jt % 2 ? -1 : 1, (int)*ret);
        }
        det += a[0][jt]*(jt % 2 ? -1 : 1) * ((int)*ret);
        if (DEBUGDET)
          cprintf("%d\n", det);
      }
    }
    else if (ipc)
    {
      for (pt = 0; pt < MATRIXSIZE; pt++)
      {
        int ret;
        envid_t env;
        ret = ipc_recv(&env, NULL, NULL, NULL);
        int n;
        for (n = 0; n < MATRIXSIZE; n++)
          if (pids[n] == env)
            break;
        det += a[0][n]*(n % 2 ? -1 : 1) * ret;
      }
    }
    return det;
  }
}

void
umain(int argc, char *argv[])
{
  int i, j;
  int matrix[MATRIXSIZE][MATRIXSIZE];
  for (i = 0; i < MATRIXSIZE; i++)
  {
    for (j = 0; j < MATRIXSIZE; j++)
    {
      matrix[i][j] = (i * MATRIXSIZE) + j;
      cprintf("%d ", matrix[i][j]);
    }
    cprintf("\n");
  }
  int total = 0;
  unsigned tic, toc;
  int det;
  // cprintf("\nSerial run:\n");
  // for (i = 0; i < NUMTRIALS; i++)
  // {
  //   tic = sys_gettime();
  //   det = determ(matrix, MATRIXSIZE, false, false);
  //   // cprintf("det is: %d\n", det);
  //   toc = sys_gettime();
  //   cprintf("~~time that serial run took: %d msec~~\n", toc - tic);
  //   total += toc - tic;
  // }
  // cprintf("\n~~~~~~~ SERIAL AVE: %d ~~~~~~\n\n", total / NUMTRIALS);

  total = 0;
  cprintf("Thread run:\n");
  for (i = 0; i < NUMTRIALS; i++)
  {
    tic = sys_gettime();
    det = determ(matrix, MATRIXSIZE, true, false);
    // cprintf("det is: %d\n", det);
    toc = sys_gettime();
    cprintf("!!time the thread run took: %d msec!!\n", toc - tic);
    total += toc - tic;
  }
  cprintf("\n!!!!!! THREAD AVE: %d !!!!!!\n\n", total / NUMTRIALS);

  total = 0;
  cprintf("\nIPC run:\n");
  for (i = 0; i < NUMTRIALS; i++)
  {
    tic = sys_gettime();
    det = determ(matrix, MATRIXSIZE, false, true);
    // cprintf("det is: %d\n", det);
    toc = sys_gettime();
    cprintf("**time the ipc run took: %d msec**\n", toc - tic);
    total += toc - tic;
  }
  cprintf("\n****** IPC AVE: %d ******\n\n", total / NUMTRIALS);
}
