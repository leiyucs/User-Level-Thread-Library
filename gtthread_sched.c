/**********************************************************************
gtthread_sched.c.  

This file contains the implementation of the scheduling subset of the
gtthreads library.  A simple round-robin queue should be used.
 **********************************************************************/
/*
  Include as needed
*/

#include "gtthread.h"
#include "steque.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
  /* 
   Students should define global variables and helper functions as
   they see fit.
 */

steque_t* readyqueue = NULL; 
steque_t* terminatequeue = NULL;


sigset_t sigmaskset;
struct itimerval timer;
struct sigaction timerhandler;

static int thread_count = 0;
static ucontext_t scheduler_context;
static ucontext_t main_context;

static void scheduler(int signum) {
  gtthread *head, *currentthread;
  
  //all threads complete

  if (steque_isempty(readyqueue)) {
    exit(0);
  }

  currentthread = (gtthread* ) steque_front(readyqueue);

  if (steque_size(readyqueue) == 1) {
      if (currentthread->state == Terminated) {
          exit(0);
      }
  }
  else {
       if (currentthread->state == Terminated) {
           steque_pop(readyqueue);
       }
       else {
          steque_cycle(readyqueue);
       }
  }
  head = (gtthread* ) steque_front(readyqueue);
  if (head != currentthread) {
    if (currentthread->state == Terminated) {
      setcontext(&head->context);
    }
    else
      swapcontext(&currentthread->context, &head->context);
  }
}

static void manager() {
  raise(SIGPROF);
}
//wrapperfunction runs the thread's function and collects the result, and do the clean up
static void wrapperfunction(void* (*start_routine) (void*), void *arg) {
  void *retval;
  retval = (*start_routine)(arg);
  sigprocmask(SIG_BLOCK, &sigmaskset, NULL);
  gtthread * head = (gtthread* ) steque_front(readyqueue);
  head->retval = retval;
  head->state = Terminated;
  steque_enqueue(terminatequeue, (steque_item) head);
  sigprocmask(SIG_UNBLOCK, &sigmaskset, NULL);

}



/*
  The gtthread_init() function does not have a corresponding pthread equivalent.
  It must be called from the main thread before any other GTThreads
  functions are called. It allows the caller to specify the scheduling
  period (quantum in micro second), and may also perform any other
  necessary initialization.  If period is zero, then thread switching should
  occur only on calls to gtthread_yield().

  Recall that the initial thread of the program (i.e. the one running
  main() ) is a thread like any other. It should have a
  gtthread_t that clients can retrieve by calling gtthread_self()
  from the initial thread, and they should be able to specify it as an
  argument to other GTThreads functions. The only difference in the
  initial thread is how it behaves when it executes a return
  instruction. You can find details on this difference in the man page
  for pthread_create.
 */
void gtthread_init(long period) {

  if (readyqueue == NULL && terminatequeue == NULL) {
    readyqueue =  (steque_t *) malloc (sizeof(steque_t));
    terminatequeue = (steque_t *) malloc (sizeof(steque_t));
    if (readyqueue == NULL || terminatequeue == NULL) {
      perror("Init Failure");
      exit(EXIT_FAILURE);
    }

    if ( getcontext(&scheduler_context) == -1 ) {
      perror("getcontext");
      exit(EXIT_FAILURE);
    }
    //establish manager context
    scheduler_context.uc_stack.ss_sp = malloc(SIGSTKSZ);
    scheduler_context.uc_stack.ss_size = SIGSTKSZ;
    scheduler_context.uc_stack.ss_flags = 0;
    scheduler_context.uc_link = NULL;
    makecontext(&scheduler_context,(void (*) (void))&manager,0);

    sigemptyset(&sigmaskset);
    sigaddset(&sigmaskset, SIGPROF);

    //setup signal handler
  
    timerhandler.sa_handler = &scheduler;
    sigemptyset(&timerhandler.sa_mask);
    sigaddset(&timerhandler.sa_mask, SIGPROF);
    sigaction(SIGPROF, &timerhandler, NULL);

    //set up time quantum
    timer.it_interval.tv_sec = period/1000000L;
    timer.it_interval.tv_usec = period%1000000L;
    timer.it_value = timer.it_interval;

    //initialize the main thread
    gtthread *mainthread = (gtthread*) malloc(sizeof(gtthread)); 
    mainthread->tid = thread_count++;
    mainthread->state =  Ready;
    if ( getcontext(&main_context) == -1) {
      perror("getcontext");
      exit(EXIT_FAILURE);
    }
    //main_context.uc_stack.ss_sp = malloc(SIGSTKSZ);
    //main_context.uc_stack.ss_size = SIGSTKSZ;
    //main_context.uc_stack.ss_flags = 0;
    mainthread->context = main_context;
    steque_enqueue(readyqueue, (steque_item) mainthread);
    setitimer(ITIMER_PROF, &timer, NULL);

  }
}


/*
  The gtthread_create() function mirrors the pthread_create() function,
  only default attributes are always assumed.
 */
int gtthread_create(gtthread_t *thread,
		    void *(*start_routine)(void *),
		    void *arg){
  sigprocmask(SIG_BLOCK, &sigmaskset, NULL);
  if (readyqueue != NULL) {
    gtthread *newthread = (gtthread*) malloc (sizeof(gtthread));
    newthread->tid =  thread_count;
    newthread->state = Ready;
    sigprocmask(SIG_UNBLOCK, &sigmaskset, NULL);
    if (getcontext(&newthread->context)==-1) {
      perror("getcontext");
      exit(EXIT_FAILURE);
    }
    newthread->context.uc_stack.ss_sp = malloc(SIGSTKSZ);
    newthread->context.uc_stack.ss_size = SIGSTKSZ;
    newthread->context.uc_stack.ss_flags = 0;
    newthread->context.uc_link = &scheduler_context;
    if (newthread->context.uc_stack.ss_sp == NULL) {
      return -1;
    }
    makecontext(&newthread->context, (void (*)(void)) wrapperfunction, 2, start_routine, arg);
    steque_enqueue(readyqueue, (steque_item) newthread);
    *thread = thread_count++;
    return 0;
  }
  else {
    return -1;
  }
  sigprocmask(SIG_UNBLOCK, &sigmaskset, NULL);

}

/*
  The gtthread_join() function is analogous to pthread_join.
  All gtthreads are joinable.
 */
int gtthread_join(gtthread_t thread, void **status){
  sigprocmask(SIG_BLOCK, &sigmaskset, NULL);
  int queuesize = steque_size(readyqueue);
  int i;
  gtthread *temp;
  int exsitinqueue=0;
  for (i=0; i<queuesize; i++) {

    temp = (gtthread *)steque_front(readyqueue);
    if (temp->tid == thread) {
      exsitinqueue = 1; 
    }
    steque_cycle(readyqueue);
  }
  sigprocmask(SIG_UNBLOCK, &sigmaskset, NULL);
  
  // check terminated queue
  while(1) {
    int exsitinterminate = 0;
    sigprocmask(SIG_BLOCK, &sigmaskset, NULL);
    if (!steque_isempty(terminatequeue)) {
      queuesize = steque_size(terminatequeue);
      for (i=0; i<queuesize; i++) {
        temp = (gtthread *)steque_front(terminatequeue);
        if (temp->tid == thread) {
          exsitinqueue=1;
          exsitinterminate=1;
          if(status!=NULL) *status = temp->retval;
          //release the resource
          free(temp->context.uc_stack.ss_sp);
          steque_pop(terminatequeue);
          break;
        }     
        steque_cycle(terminatequeue);
      }
    }
    sigprocmask(SIG_UNBLOCK, &sigmaskset, NULL);
    if (exsitinqueue == 0) return -1;
    if (exsitinterminate == 1) break;
    gtthread_yield();
  }
  return 0;

}

/*
  The gtthread_exit() function is analogous to pthread_exit.
 */
void gtthread_exit(void* retval){
  sigprocmask(SIG_BLOCK, &sigmaskset, NULL);
  gtthread * currentthread = (gtthread *) steque_front (readyqueue);
  currentthread->retval = retval;
  currentthread->state = Terminated;
  steque_enqueue(terminatequeue, (steque_item) currentthread);
  sigprocmask(SIG_UNBLOCK, &sigmaskset, NULL);
  raise(SIGPROF);
}


/*
  The gtthread_yield() function is analogous to pthread_yield, causing
  the calling thread to relinquish the cpu and place itself at the
  back of the schedule queue.
 */
void gtthread_yield(void){
  raise(SIGPROF);
}

/*
  The gtthread_yield() function is analogous to pthread_equal,
  returning zero if the threads are the same and non-zero otherwise.
 */
int  gtthread_equal(gtthread_t t1, gtthread_t t2){
    return (t1 == t2 )?1:0;

}

/*
  The gtthread_cancel() function is analogous to pthread_cancel,
  allowing one thread to terminate another asynchronously.
 */
int  gtthread_cancel(gtthread_t thread){
  sigprocmask(SIG_BLOCK, &sigmaskset, NULL);
  int success =-1;
  gtthread *first = (gtthread *)steque_front(readyqueue);

  do {
    gtthread * temp = (gtthread *)steque_front(readyqueue);
    if(temp->tid == thread) {
      steque_enqueue(terminatequeue, (steque_item) temp);
      temp->state = Terminated;
      steque_pop(readyqueue);
      success = 0;
      if (temp == first) break;
    }
    steque_cycle(readyqueue);
  } while((gtthread *)steque_front(readyqueue) != first); 
  sigprocmask(SIG_UNBLOCK, &sigmaskset, NULL);
  return success; 

}

/*
  Returns calling thread.
 */
gtthread_t gtthread_self(void){
  return ((gtthread *) steque_front(readyqueue))->tid;
   
}

