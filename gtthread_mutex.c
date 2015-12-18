/**********************************************************************
gtthread_mutex.c.  

This file contains the implementation of the mutex subset of the
gtthreads library.  The locks can be implemented with a simple queue.
 **********************************************************************/

/*
  Include as needed
*/


#include "gtthread.h"
#include "steque.h"
#include <stdlib.h>
extern steque_t* readyqueue; 

extern steque_t* terminatequeue;

static steque_t* mutexqueue = NULL;


gtthread_mutex_node* Allocatenewmutexnode() {
  static long currentmutexid = 0;
  gtthread_mutex_node *newmutexnode = (gtthread_mutex_node *) malloc (sizeof(gtthread_mutex_node));
  newmutexnode->mutex = (gtthread_mutex_t *) malloc (sizeof(gtthread_mutex_t));
  newmutexnode->mutex->mutexid = currentmutexid++;
  newmutexnode->holder = NULL;
  return newmutexnode;
}

/*
  The gtthread_mutex_init() function is analogous to
  pthread_mutex_init with the default parameters enforced.
  There is no need to create a static initializer analogous to
  PTHREAD_MUTEX_INITIALIZER.
 */
int gtthread_mutex_init(gtthread_mutex_t* mutex){
  if(mutex != NULL) {
    if (mutexqueue == NULL) {
          mutexqueue = (steque_t *) malloc (sizeof(steque_t));
    }
    gtthread_mutex_node* newnode = Allocatenewmutexnode();
    mutex->mutexid = newnode->mutex->mutexid;
    steque_enqueue(mutexqueue, (steque_item) newnode);
    return 0;
  }
  return -1;
}

/*
  The gtthread_mutex_lock() is analogous to pthread_mutex_lock.
  Returns zero on success.
 */
int gtthread_mutex_lock(gtthread_mutex_t* mutex){

    if (mutex == NULL || mutexqueue == NULL) return -1;
    int queuesize = steque_size(mutexqueue);
    while(queuesize >0) {
      gtthread_mutex_node *head=steque_front(mutexqueue);
      if (head->mutex->mutexid == mutex->mutexid)
      {
        gtthread *currentthread = (gtthread *)steque_front(readyqueue);
        if ( currentthread != NULL) {
          while(head->holder);
          head->holder = currentthread;
          return 0;
        }

      }
      steque_cycle(mutexqueue);
      queuesize--;
    }
    return -1;
}
/*
  The gtthread_mutex_unlock() is analogous to pthread_mutex_unlock.
  Returns zero on success.
 */
int gtthread_mutex_unlock(gtthread_mutex_t *mutex){

    if (mutex == NULL || mutexqueue == NULL) return -1;
    int queuesize = steque_size(mutexqueue);
    while(queuesize >0) {
      gtthread_mutex_node *head=steque_front(mutexqueue);
      if (head->mutex->mutexid == mutex->mutexid)
      {
        gtthread *currentthread = (gtthread *)steque_front(readyqueue);
        if ( currentthread == head->holder) {
          head->holder = NULL;
          return 0;
        }

      }
      steque_cycle(mutexqueue);
      queuesize--;
    }
    return -1;
}

/*
  The gtthread_mutex_destroy() function is analogous to
  pthread_mutex_destroy and frees any resourcs associated with the mutex.
*/
int gtthread_mutex_destroy(gtthread_mutex_t *mutex){
    if (mutex == NULL || mutexqueue == NULL) return -1;
    int queuesize = steque_size(mutexqueue);
    while(queuesize >0) {
      gtthread_mutex_node *head=steque_front(mutexqueue);
      if (head->mutex->mutexid == mutex->mutexid)
      {
        if (head->holder!=NULL) return -1;
        free(head->mutex);
        steque_pop(mutexqueue);
        free(head);
        return 0;
      }
      steque_cycle(mutexqueue);
      queuesize--;
    }
    return -1;
}
