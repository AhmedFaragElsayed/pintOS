/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */

void sema_init(struct semaphore *sema, unsigned value)
{
  ASSERT(sema != NULL);

  sema->value = value;
  list_init(&sema->waiters);
}
bool
lock_held_by_current_thread(const struct lock *lock)
{
  ASSERT(lock != NULL);

  return lock->holder == thread_current();
}
/* Yields the CPU if a higher priority thread is ready to run */


/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void sema_down(struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT(sema != NULL);
  ASSERT(!intr_context());

  old_level = intr_disable();
  while (sema->value == 0)
  {
    list_push_back(&sema->waiters, &thread_current()->elem);
    thread_block();
  }
  sema->value--;
  intr_set_level(old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool sema_try_down(struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT(sema != NULL);

  old_level = intr_disable();
  if (sema->value > 0)
  {
    sema->value--;
    success = true;
  }
  else
    success = false;
  intr_set_level(old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
   void
sema_up(struct semaphore *sema)
{
  enum intr_level old_level;
  bool need_yield = false; /* Local variable instead of global */

  ASSERT(sema != NULL);

  old_level = intr_disable();

  /* Check if there are waiters */
  if (!list_empty(&sema->waiters))
  {
    /* Sort waiters by priority (highest first) */
    list_sort(&sema->waiters, thread_priority_greater, NULL);

    /* Unblock the highest priority waiter */
    struct thread *highest_waiter = list_entry(list_pop_front(&sema->waiters),
                                             struct thread, elem);
    thread_unblock(highest_waiter);

    /* If the awakened thread has higher priority than current,
       mark that we should yield */
    if (highest_waiter->priority > thread_current()->priority)
      need_yield = true;
  }

  sema->value++;
  intr_set_level(old_level);

  /* If not in interrupt context and we should yield, do so */
  if (!intr_context() && need_yield)
  {
    thread_yield();
  }
}
static void sema_test_helper(void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void sema_self_test(void)
{
  struct semaphore sema[2];
  int i;

  printf("Testing semaphores...");
  sema_init(&sema[0], 0);
  sema_init(&sema[1], 0);
  thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
  {
    sema_up(&sema[0]);
    sema_down(&sema[1]);
  }
  printf("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper(void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
  {
    sema_down(&sema[0]);
    sema_up(&sema[1]);
  }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void lock_init(struct lock *lock)
{
  ASSERT(lock != NULL);

  lock->holder = NULL;
  sema_init(&lock->semaphore, 1);

}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
   void
   lock_acquire(struct lock *lock)
   {
     ASSERT(lock != NULL);
     ASSERT(!intr_context());
     ASSERT(!lock_held_by_current_thread(lock));

     struct thread *current = thread_current();
     enum intr_level old_level = intr_disable();

     /* If lock is already held by another thread */
     if (lock->holder != NULL)
     {
       /* Set up donation relationship */
       current->waiting_on_lock = lock;

       /* Start the priority donation process */
       struct thread *holder = lock->holder;
       int depth = 0;

       /* Handle nested donations (limit depth to prevent infinite loops) */
       while (holder != NULL && depth < 8)
       {
         /* Only donate if our priority is higher */
         if (holder->priority < current->priority)
         {
           /* Donate our priority to the holder */
           holder->priority = current->priority;

           /* If holder is waiting on another lock, continue donation chain */
           if (holder->waiting_on_lock != NULL)
           {
             holder = holder->waiting_on_lock->holder;
             depth++;
           }
           else
             break;  /* End of donation chain */
         }
         else
           break;  /* No need to donate if holder has higher priority */
       }
     }

     intr_set_level(old_level);

     /* Try to acquire the lock (will block if not available) */
     sema_down(&lock->semaphore);

     /* After acquiring the lock, update relationships */
     old_level = intr_disable();

     current->waiting_on_lock = NULL;  /* Not waiting anymore */
     lock->holder = current;           /* Set ourselves as the holder */

     /* Add the lock to our list of held locks */
     list_push_back(&current->locks_held, &lock->elem);

     intr_set_level(old_level);
   }

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool lock_try_acquire(struct lock *lock)
{
  bool success;

  ASSERT(lock != NULL);
  ASSERT(!lock_held_by_current_thread(lock));

  success = sema_try_down(&lock->semaphore);
  if (success)
    lock->holder = thread_current();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
   void
   lock_release(struct lock *lock)
   {
     ASSERT(lock != NULL);
     ASSERT(lock_held_by_current_thread(lock));

     enum intr_level old_level = intr_disable();

     /* Remove the lock from our held locks list */
     list_remove(&lock->elem);

     /* Reset lock holder before releasing */
     lock->holder = NULL;

     /* Recalculate our priority based on remaining locks/donations */
     struct thread *current = thread_current();
     current->priority = current->original_priority;

     /* Check if we still have other locks with potential donations */
     if (!list_empty(&current->locks_held))
     {
       struct list_elem *e;

       /* Find the highest priority waiter among all our held locks */
       for (e = list_begin(&current->locks_held);
            e != list_end(&current->locks_held);
            e = list_next(e))
       {
         struct lock *held_lock = list_entry(e, struct lock, elem);
         struct semaphore *sema = &held_lock->semaphore;

         /* Check if this lock has waiters */
         if (!list_empty(&sema->waiters))
         {
           /* Find highest priority waiter */
           /* Find highest priority waiter */
struct list_elem *max_elem = list_max(&sema->waiters, thread_priority_greater, NULL);
struct thread *waiter = list_entry(max_elem, struct thread, elem);

           /* Keep the highest donation */
           if (waiter->priority > current->priority)
             current->priority = waiter->priority;
         }
       }
     }

     intr_set_level(old_level);

     /* Release the lock */
     sema_up(&lock->semaphore);

     /* We may need to yield if our priority decreased */
     thread_yield_to_higher_priority();
   }
void priority_restore(struct thread *t)
{
  /* Start with original priority */
  t->priority = t->original_priority;

  /* Check if there are any locks with waiters of higher priority */
  if (!list_empty(&t->locks_held))
  {
    struct list_elem *e;
    for (e = list_begin(&t->locks_held); e != list_end(&t->locks_held); e = list_next(e))
    {
      struct lock *lock = list_entry(e, struct lock, elem);
      /* Find the highest priority among waiters */
      struct semaphore *sema = &lock->semaphore;
      if (!list_empty(&sema->waiters))
      {
        struct thread *waiter = list_entry(list_front(&sema->waiters), struct thread, elem);
        if (waiter->priority > t->priority)
          t->priority = waiter->priority;
      }
    }
  }
}

/* One semaphore in a list. */
struct semaphore_elem
{
  struct list_elem elem;      /* List element. */
  struct semaphore semaphore; /* This semaphore. */
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void cond_init(struct condition *cond)
{
  ASSERT(cond != NULL);

  list_init(&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void cond_wait(struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  sema_init(&waiter.semaphore, 0);
  list_push_back(&cond->waiters, &waiter.elem);
  lock_release(lock);
  sema_down(&waiter.semaphore);
  lock_acquire(lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
/* Returns true if semaphore a has higher priority waiters than semaphore b */
/* Returns true if semaphore a has higher priority waiters than semaphore b */
bool sema_waiter_priority_greater(const struct list_elem *a,
  const struct list_elem *b, void *aux UNUSED)
{
struct semaphore_elem *sa = list_entry(a, struct semaphore_elem, elem);
struct semaphore_elem *sb = list_entry(b, struct semaphore_elem, elem);

/* Find the highest priority thread in each semaphore */
struct thread *ta = NULL, *tb = NULL;

/* Sort the waiters list to ensure we get the highest priority thread */
if (!list_empty(&sa->semaphore.waiters))
{
struct list_elem *highest_a = list_max(&sa->semaphore.waiters,
         thread_priority_greater, NULL);
ta = list_entry(highest_a, struct thread, elem);
}

if (!list_empty(&sb->semaphore.waiters))
{
struct list_elem *highest_b = list_max(&sb->semaphore.waiters,
         thread_priority_greater, NULL);
tb = list_entry(highest_b, struct thread, elem);
}

/* Compare priorities */
if (ta == NULL)
return false;
if (tb == NULL)
return true;
return ta->priority > tb->priority;
}
void cond_signal(struct condition *cond, struct lock *lock)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  if (!list_empty(&cond->waiters))
  {
    /* Find the highest priority waiter */
    struct list_elem *max_elem = list_max(&cond->waiters,
                                        sema_waiter_priority_greater, NULL);
    list_remove(max_elem);
    sema_up(&list_entry(max_elem, struct semaphore_elem, elem)->semaphore);
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_broadcast(struct condition *cond, struct lock *lock)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);

  while (!list_empty(&cond->waiters))
    cond_signal(cond, lock);
}
