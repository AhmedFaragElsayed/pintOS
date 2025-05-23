#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>
#include "lib/debug.h"

/* A counting semaphore. */
struct semaphore
  {
    unsigned value;             /* Current value. */
    struct list waiters;        /* List of waiting threads. */
  };


void sema_init (struct semaphore *, unsigned value);
void sema_down (struct semaphore *);
bool sema_try_down (struct semaphore *);
void sema_up (struct semaphore *);
void sema_self_test (void);

bool sema_waiter_priority_greater(const struct list_elem *a,
                               const struct list_elem *b, void *aux UNUSED);

/* Lock. */
struct lock
{
  struct thread* holder;
  struct semaphore semaphore;
  struct list_elem elem;
  int max_priority;
};

void lock_init (struct lock *);
void lock_acquire (struct lock *);
bool lock_try_acquire (struct lock *);
void lock_release (struct lock *);
bool lock_held_by_current_thread (const struct lock *);
void priority_restore(struct thread *t);

/* Compare semaphore waiters by highest priority thread waiting */
bool sema_waiter_priority_greater(const struct list_elem *a,
                                 const struct list_elem *b,
                                 void *aux);

/* Condition variable. */
struct condition
  {
    struct list waiters;        /* List of waiting threads. */
  };

void cond_init (struct condition *);
void cond_wait (struct condition *, struct lock *);
void cond_signal (struct condition *, struct lock *);
void cond_broadcast (struct condition *, struct lock *);
/* Add this prototype to synch.h with your other lock-related function declarations */
bool lock_held_by_current_thread(const struct lock *lock);

/* Optimization barrier.

   The compiler will not reorder operations across an
   optimization barrier.  See "Optimization Barriers" in the
   reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */
