/*
 * Copyright (c) 2009, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

/* sasatomt 
   Test the sas atomic functions.
*/

#include <string.h>
#include <unistd.h> 
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "sasatom.h"

int
test_inc_dec_long ()
{
  long value = 0;
  long value2 = __LONG_MAX__;
  int rc = 0;
  
  sas_atomic_inc (&value);
  
  if (value != 1)
  {
    printf("atomic_inc(%p) value=%ld failed expected 1\n",  &value, value);
    rc++;
  }
  
  if (value2 != __LONG_MAX__)
  {
    printf("atomic_inc(%p) clobbered value2=%ld\n",  &value, value2);
    rc++;
  }
  
  sas_atomic_inc (&value);
  
  if (value != 2)
  {
    printf("atomic_inc(%p) value=%ld failed expected 2\n",  &value, value);
    rc++;
  }
  
  sas_atomic_inc (&value);
  
  if (value != 3)
  {
    printf("atomic_inc(%p) value=%ld failed expected 3\n",  &value, value);
    rc++;
  }
  
  sas_atomic_dec (&value);
  
  if (value != 2)
  {
    printf("atomic_dec(%p) value=%ld failed expected 2\n",  &value, value);
    rc++;
  }
  
  if (value2 != __LONG_MAX__)
  {
    printf("atomic_dec(%p) clobbered value2=%ld\n",  &value, value2);
    rc++;
  }
  
  sas_atomic_dec (&value);
  
  if (value != 1)
  {
    printf("atomic_dec(%p) value=%ld failed expected 1\n",  &value, value);
    rc++;
  }
  
  sas_atomic_inc (&value);
  
  if (value != 2)
  {
    printf("atomic_inc(%p) value=%ld failed expected 2\n",  &value, value);
    rc++;
  }
  
  return rc;
}

int
test_post_inc_dec_long ()
{
  long value = 0;
  long value2 = __LONG_MAX__;
  long result;
  int rc = 0;
  
  result = sas_atomic_inc_long (&value);
  
  if (result != 0L)
  {
    printf("sas_atomic_inc_long(%p) result=%ld failed expected 0\n",  &value, result);
    rc++;
  }
  if (value != 1L)
  {
    printf("sas_atomic_inc_long(%p) value=%ld failed expected 1\n",  &value, value);
    rc++;
  }
  if (value2 != __LONG_MAX__)
  {
    printf("sas_atomic_inc_long(%p) clobbered value2=%ld\n",  &value, value2);
    rc++;
  }
  
  result = sas_atomic_inc_long (&value);
  
  if (result != 1L)
  {
    printf("sas_atomic_inc_long(%p) result=%ld failed expected 1\n",  &value, result);
    rc++;
  }
  if (value != 2L)
  {
    printf("sas_atomic_inc_long(%p) value=%ld failed expected 2\n",  &value, value);
    rc++;
  }
  
  result = sas_atomic_inc_long (&value);
  
  if (result != 2L)
  {
    printf("sas_atomic_inc_long(%p) result=%ld failed expected 2\n",  &value, result);
    rc++;
  }
  if (value != 3L)
  {
    printf("sas_atomic_inc_long(%p) value=%ld failed expected 3\n",  &value, value);
    rc++;
  }
  
  result = sas_atomic_dec_long (&value);
  
  if (result != 3L)
  {
    printf("sas_atomic_dec_long(%p) result=%ld failed expected 3\n",  &value, result);
    rc++;
  }
  
  if (value != 2L)
  {
    printf("sas_atomic_dec_long(%p) value=%ld failed expected 2\n",  &value, value);
    rc++;
  }
  
  if (value2 != __LONG_MAX__)
  {
    printf("sas_atomic_dec_long(%p) clobbered value2=%ld\n",  &value, value2);
    rc++;
  }
  
  result = sas_atomic_dec_long (&value);
  
  if (result != 2L)
  {
    printf("sas_atomic_dec_long(%p) result=%ld failed expected 2\n",  &value, result);
    rc++;
  }
  if (value != 1L)
  {
    printf("sas_atomic_dec_long(%p) value=%ld failed expected 1\n",  &value, value);
    rc++;
  }
  
  result = sas_atomic_dec_long (&value);;
  
  if (result != 1L)
  {
    printf("sas_atomic_dec_long(%p) result=%ld failed expected 1\n",  &value, result);
    rc++;
  }
  if (value != 0L)
  {
    printf("sas_atomic_dec_long(%p) value=%ld failed expected 0\n",  &value, value);
    rc++;
  }
  
  result = sas_atomic_dec_long (&value);;
  
  if (result != 0L)
  {
    printf("sas_atomic_dec_long(%p) result=%ld failed expected 0\n",  &value, result);
    rc++;
  }
  if (value != -1L)
  {
    printf("sas_atomic_dec_long(%p) value=%ld failed expected -1\n",  &value, value);
    rc++;
  }
  
  return rc;
}

int
test_add_long ()
{
  long value = 0;
  long value2 = __LONG_MAX__;
  long prev;
  int rc = 0;
  
  prev = sas_fetch_and_add (&value, 1);
  
  if (prev != 0)
  {
    printf("fetch_and_add(%p,1)= %ld failed\n", &value, prev);
    rc++;
  }
  if (value != 1)
  {
    printf("fetch_and_add(%p,1)= %ld failed\n",  &value, prev);
    rc++;
  }
  
  if (value2 != __LONG_MAX__)
  {
    printf("fetch_and_add(%p) clobbered value2=%ld\n",  &value, value2);
    rc++;
  }
  
  prev = sas_fetch_and_add (&value, 2);
  
  if (prev != 1)
  {
    printf("fetch_and_add(%p,2)= %ld failed\n", &value, prev);
    rc++;
  }
  if (value != 3)
  {
    printf("fetch_and_add(%p,2)= %ld failed\n",  &value, prev);
    rc++;
  }
  
  prev = sas_fetch_and_add (&value, 3);
  
  if (prev != 3)
  {
    printf("fetch_and_add(%p,3)= %ld failed\n", &value, prev);
    rc++;
  }
  if (value != 6)
  {
    printf("fetch_and_add(%p,3)= %ld failed\n",  &value, prev);
    rc++;
  }
  
  prev = sas_fetch_and_add (&value, -3);
  
  if (prev != 6)
  {
    printf("fetch_and_add(%p,-3)= %ld failed\n", &value, prev);
    rc++;
  }
  if (value != 3)
  {
    printf("fetch_and_add(%p,-3)= %ld failed\n",  &value, prev);
    rc++;
  }
  
  return rc;
}

int
test_add_ptr ()
{
  char buf[] = "0123456789";
  char *ptr = buf;
  char *ptr2;
  int rc = 0;
  
  if (*ptr != '0')
  {
    printf("test_add_ptr compiler codegen failure\n");
    rc++;
    return rc;
  }

  ptr2 = (char*)sas_fetch_and_add_ptr ((void**)&ptr, 1);
  
  if (*ptr2 != '0')
  {
    printf("fetch_and_add_ptr(%p,1)= %p failed\n", (void**)&ptr, ptr2);
    rc++;
  }
  if (*ptr != '1')
  {
    printf("fetch_and_add_ptr(%p,1)= %p failed\n", (void**)&ptr, ptr2);
    rc++;
  }

  ptr2 = (char*)sas_fetch_and_add_ptr ((void**)&ptr, 2);
  
  if (*ptr2 != '1')
  {
    printf("fetch_and_add_ptr(%p,2)= %p failed\n", (void**)&ptr, ptr2);
    rc++;
  }
  if (*ptr != '3')
  {
    printf("fetch_and_add_ptr(%p,2)= %p failed\n", (void**)&ptr, ptr2);
    rc++;
  }

  ptr2 = (char*)sas_fetch_and_add_ptr ((void**)&ptr, 3);
  
  if (*ptr2 != '3')
  {
    printf("fetch_and_add_ptr(%p,3)= %p failed\n", (void**)&ptr, ptr2);
    rc++;
  }
  if (*ptr != '6')
  {
    printf("fetch_and_add_ptr(%p,3)= %p failed\n", (void**)&ptr, ptr2);
    rc++;
  }

  ptr2 = (char*)sas_fetch_and_add_ptr ((void**)&ptr, -3);
  
  if (*ptr2 != '6')
  {
    printf("fetch_and_add_ptr(%p,-3)= %p failed\n", (void**)&ptr, ptr2);
    rc++;
  }
  if (*ptr != '3')
  {
    printf("fetch_and_add_ptr(%p,-3)= %p failed\n", (void**)&ptr, ptr2);
    rc++;
  }
  

  return rc;
}

int
test_spin_lock ()
{
  int rc = 0;
  int tmp = 0;
  volatile sas_spin_lock_t	lock0;
  volatile sas_spin_lock_t	lock1;
  
  lock0 = 12345678;
  lock1 = 87654321;
  sas_spin_lock_init(&lock0);
  if (lock0 != 0)
  {
     printf("sas_spin_lock_init(%p) failed lock=%x\n",
     		&lock0, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_spin_lock_init(%p) clobbered lock1=%x\n",
     		&lock0, lock1);
     rc++;
  }
  
  sas_spin_lock(&lock0);
  if (lock0 != 1)
  {
     printf("sas_spin_lock(%p) failed lock=%x\n",
     		&lock0, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_spin_lock(%p) clobbered lock1=%x\n",
     		&lock0, lock1);
     rc++;
  }
  
  tmp = sas_spin_trylock(&lock0);
  if (tmp == 0)
  {
     printf("0sas_spin_trylock (%p)=%d failed lock=%x\n",
     		&lock0, tmp, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_spin_trylock(%p) clobbered lock1=%x\n",
     		&lock0, lock1);
     rc++;
  }
  
  sas_spin_unlock(&lock0);
  if (lock0 != 0)
  {
     printf("sas_spin_unlock(%p) failed lock=%x\n",
     		&lock0, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_spin_unlock(%p) clobbered lock1=%x\n",
     		&lock0, lock1);
     rc++;
  }
  
  tmp = sas_spin_trylock(&lock0);
  if (tmp != 0)
  {
     printf("2 sas_spin_trylock(%p)=%d failed lock=%x\n",
     		&lock0, tmp, lock0);
     rc++;
  }
  if (lock0 != 1)
  {
     printf("3 sas_spin_trylock(%p)=%d failed lock=%x\n",
     		&lock0, tmp, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_spin_trylock(%p) clobbered lock1=%x\n",
     		&lock0, lock1);
     rc++;
  }
  
  sas_spin_unlock(&lock0);
  if (lock0 != 0)
  {
     printf("sas_spin_unlock(%p) failed lock=%x\n",
     		&lock0, lock0);
     rc++;
  }
  
  sas_spin_lock_with_yield(&lock0);
  if (lock0 != 1)
  {
     printf("sas_spin_lock_with_yield(%p) failed lock=%x\n",
     		&lock0, lock0);
     rc++;
  }
  
  return rc;
}

int
test_lock_ptr ()
{
  int rc = 0;
  char buf[] = "012345678";
  int tmp = 0;
  char buf2[] = "987654321";
  volatile sas_lock_ptr_t lock0;
  volatile long	lock1;
  
  lock0 = NULL;
  lock1 = 87654321;
  sas_lock_ptr_init(&lock0);
  if (lock0 != 0)
  {
     printf("sas_lock_ptr_init(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_lock_ptr_init(%p) clobbered lock1=%ld\n",
     		&lock0, lock1);
     rc++;
  }
  
  sas_lock_ptr(&lock0);
  if ((long)lock0 != 1)
  {
     printf("sas_lock_ptr(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_lock_ptr(%p) clobbered lock1=%ld\n",
     		&lock0, lock1);
     rc++;
  }
  
  tmp = sas_trylock_ptr(&lock0);
  if (tmp == 0)
  {
     printf("sas_trylock_ptr (%p)=%d failed lock=%p\n",
     		&lock0, tmp, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_trylock_ptr(%p) clobbered lock1=%ld\n",
     		&lock0, lock1);
     rc++;
  }
  
  sas_unlock_ptr(&lock0);
  if (lock0 != 0)
  {
     printf("sas_unlock_ptr(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_unlock_ptr(%p) clobbered lock1=%ld\n",
     		&lock0, lock1);
     rc++;
  }
  
  tmp = sas_trylock_ptr(&lock0);
  if (tmp != 0)
  {
     printf("2 sas_trylock_ptr(%p)=%d failed lock=%p\n",
     		&lock0, tmp, lock0);
     rc++;
  }
  if ((long)lock0 != 1)
  {
     printf("3 sas_trylock_ptr(%p)=%d failed lock=%p\n",
     		&lock0, tmp, lock0);
     rc++;
  }
  if (lock1 != 87654321)
  {
     printf("sas_trylock_ptr(%p) clobbered lock1=%ld\n",
     		&lock0, lock1);
     rc++;
  }
  
  sas_set_locked_ptr (&lock0, buf);
  printf("sas_set_locked_ptr(%p,%p) -> %p\n",
     		&lock0, buf, lock0);
  
  sas_unlock_ptr(&lock0);
  if (lock0 != buf)
  {
     printf("0sas_unlock_ptr(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  if (*(char*)lock0 != '0')
  {
     printf("1sas_unlock_ptr(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  
  sas_set_unlocked_ptr (&lock0, buf2);
  printf("sas_set_unlocked_ptr(%p,%p) -> %p\n",
     		&lock0, buf2, lock0);
  
  sas_unlock_ptr(&lock0);
  if (lock0 != buf2)
  {
     printf("2sas_unlock_ptr(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  if (*(char*)lock0 != '9')
  {
     printf("3sas_unlock_ptr(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  
  sas_lock_ptr_with_yield(&lock0);
  if (((long)lock0 & 1L) != 1L)
  {
     printf("sas_lock_ptr_with_yield(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  if (*(char*)lock0 != '8')
  {
     printf("sas_lock_ptr_with_yield(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  
  sas_unlock_ptr(&lock0);
  if (lock0 != buf2)
  {
     printf("4sas_unlock_ptr(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  if (*(char*)lock0 != '9')
  {
     printf("5sas_unlock_ptr(%p) failed lock=%p\n",
     		&lock0, lock0);
     rc++;
  }
  
  return rc;
}

volatile int rc_thread;
volatile sas_spin_lock_t	t_lock;

#define N	4

static void *
tf (void *arg)
{
  int tmp = 0;
  char number[160];
  sprintf(number, "tf(%ld): begin", (long)arg);
  puts (number);
  
  tmp = sas_spin_trylock(&t_lock);
  if (tmp == 0)
  {
    sprintf(number, "tf(%ld): sas_spin_trylock", (long)arg);
    puts (number);
    
    sas_spin_unlock(&t_lock);

    sprintf(number, "tf(%ld): end", (long)arg);
    puts (number);
  } else {
    sas_spin_lock_with_yield(&t_lock);

    sprintf(number, "tf(%ld): sas_spin_lock_with_yield", (long)arg);
    puts (number);
  
    sas_spin_unlock(&t_lock);

    sprintf(number, "tf(%ld): end", (long)arg);
    puts (number);
  }
  return NULL;
}

int
test_spin_lock_threads ()
{
  int n;
  pthread_t th[N];
  
  rc_thread = 0;
  
  sas_spin_lock_init(&t_lock);
  if (t_lock != 0)
  {
     printf("sas_spin_lock_init(%p) failed lock=%x\n",
     		&t_lock, t_lock);
     rc_thread++;
  }
  
  sas_spin_lock(&t_lock);
  if (t_lock != 1)
  {
     printf("sas_spin_lock(%p) failed lock=%x\n",
     		&t_lock, t_lock);
     rc_thread++;
  }

  for (n = 0; n < N; ++n)
    if (pthread_create (&th[n], NULL, tf, (void *) (long int) n) != 0)
      {
	puts ("create failed");
	exit (1);
      }

  puts("after create");
  
  sleep(1);
  
  sas_spin_unlock(&t_lock);

  puts("after sas_spin_unlock");

  for (n = 0; n < N; ++n)
    if (pthread_join (th[n], NULL) != 0)
      {
	puts ("join failed");
	exit (1);
      }


  puts("after join");
    
  if (t_lock != 0)
  {
     printf("sas_spin_unlock(%p) failed lock=%x\n",
     		&t_lock, t_lock);
     rc_thread++;
  }
    
  return rc_thread;
}

volatile long	atomic_cnt;

#define N	4

static void *
tf_for (void *arg)
{
  long cnt;
  char number[160];
  sprintf(number, "tf_for(%ld): begin", (long)arg);
  puts (number);
  
  sas_spin_lock_with_yield(&t_lock);
  sas_spin_unlock(&t_lock);
  
  for (cnt=sas_atomic_inc_long(&atomic_cnt); cnt<20; cnt=sas_atomic_inc_long(&atomic_cnt))
  {
    sprintf(number, "tf_for(%ld): sas_atomic_inc_long(%p)=%ld",
    		(long)arg, &atomic_cnt, cnt);
    puts (number);
  
    sleep(1);
  }

  sprintf(number, "tf_for(%ld): end", (long)arg);
  puts (number);
  
  return NULL;
}

int
test_sas_atomic_inc_long_threads ()
{
  int n;
  pthread_t th[N];
  
  rc_thread = 0;
  
  sas_spin_lock_init(&t_lock);
  if (t_lock != 0)
  {
     printf("sas_spin_lock_init(%p) failed lock=%x\n",
     		&t_lock, t_lock);
     rc_thread++;
  }
  
  sas_spin_lock(&t_lock);
  if (t_lock != 1)
  {
     printf("sas_spin_lock(%p) failed lock=%x\n",
     		&t_lock, t_lock);
     rc_thread++;
  }

  for (n = 0; n < N; ++n)
    if (pthread_create (&th[n], NULL, tf_for, (void *) (long int) n) != 0)
      {
	puts ("create failed");
	exit (1);
      }

  puts("after create");
  
  sleep(1);
  
  sas_spin_unlock(&t_lock);

  puts("after sas_spin_unlock");

  for (n = 0; n < N; ++n)
    if (pthread_join (th[n], NULL) != 0)
      {
	puts ("join failed");
	exit (1);
      }


  puts("after join");
    
  if (t_lock != 0)
  {
     printf("sas_spin_unlock(%p) failed lock=%x\n",
     		&t_lock, t_lock);
     rc_thread++;
  }
    
  return rc_thread;
}

int
main (int argc, char **argv)
{
  int failures = 0;
  
  failures = test_spin_lock();
  
  failures += test_add_ptr();
  
  failures += test_add_long ();
  
  failures += test_inc_dec_long ();
  
  failures += test_post_inc_dec_long ();
  
  failures += test_lock_ptr ();
  
  failures += test_spin_lock_threads ();
  
  failures += test_sas_atomic_inc_long_threads ();
  
  if (failures)
  {
     printf("sasatomt total %d failures\n", failures);
     return 1;
  } else 
    return 0;
}
