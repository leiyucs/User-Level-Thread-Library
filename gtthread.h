#ifndef GTTHREAD_H
#define GTTHREAD_H

#include "steque.h"
#include <ucontext.h>

/* Define gtthread_t and gtthread_mutex_t types here */

typedef enum threadstate_t {
	Ready,
	Blocked,
	Terminated

 } threadstate_t;

typedef unsigned long int gtthread_t;

typedef struct gtthread {

	gtthread_t tid;
	ucontext_t context;
	threadstate_t state;
	steque_t* waitthreadlist;
	void* retval;

} gtthread;

typedef struct gtthread_mutex_t {
	int mutexid;
} gtthread_mutex_t;

typedef struct gtthread_mutex_node 
{
	gtthread_mutex_t *mutex;
	gtthread *holder;
} gtthread_mutex_node;


void gtthread_init(long period);
int  gtthread_create(gtthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);
int  gtthread_join(gtthread_t thread, void **status);
void gtthread_exit(void *retval);
void gtthread_yield(void);
int  gtthread_equal(gtthread_t t1, gtthread_t t2);
int  gtthread_cancel(gtthread_t thread);
gtthread_t gtthread_self(void);


int  gtthread_mutex_init(gtthread_mutex_t *mutex);
int  gtthread_mutex_lock(gtthread_mutex_t *mutex);
int  gtthread_mutex_unlock(gtthread_mutex_t *mutex);
int  gtthread_mutex_destroy(gtthread_mutex_t *mutex);
#endif
