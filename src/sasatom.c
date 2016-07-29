/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include <stdlib.h>
#include <sched.h>
#include "sasatom.h"


#ifdef __GNUC__
/* static inline defined in sasatom.h */      
#else
void *SASAtomicPtrAdd(void **pointer, long delta)
{
# ifdef __BCPLUSPLUS__
	void *temp;
#  ifdef __MSDOS__
	asm {
		.486p
		les BX,pointer
		mov AX,delta
		lock
		xadd word ptr ES:[BX],AX
		mov temp,AX
      mov temp+2,ES
	};
#  else
	asm {
		.486p
		mov	EBX,pointer
		mov EAX,delta
		lock
		xadd dword ptr [EBX],EAX
		mov temp,EAX
	};
#  endif
	return temp;
# else
#  ifdef __HIGHC__
#   ifdef __SASPPC__
//	temp = *pointer;
//	*pointer = (void *)(((char *)*pointer) + delta);
	_ASM("loop1:	lwarx	%r5,0,%r3");
	_ASM("			add		%r0,%r4,%r5");
	_ASM("			stwcx.	%r0,0,%r3");
	_ASM("			bne		loop1");
	_ASM("			ori		%r3,%r5,0");
#   else
//	temp = *pointer;
//	*pointer = (void *)(((char *)*pointer) + delta);
	_ASM("	movl	8(%ebp),%edx");
	_ASM("	movl	12(%ebp),%eax");
	_ASM("	lock");
	_ASM("	xaddl	%eax,(%edx)");
//	_ASM("	movl	%eax,-8(%ebp)");
#   endif
#  else
	void *temp;
	temp = *pointer;
	*pointer = (void *)(((char *)*pointer) + delta)
	return temp;
#  endif
# endif
};
#endif

long SASAtomicAdd(long *longptr, long delta)
{
#ifdef __BCPLUSPLUS__
	long temp;
#ifdef __MSDOS__
	asm {
		.486p
		les BX,longptr
		mov EAX,delta
		lock
		xadd dword ptr ES:[BX],EAX
		mov temp,EAX
	};
#else
	asm {
		.486p
		mov	EBX,longptr
		mov EAX,delta
		lock
		xadd dword ptr [EBX],EAX
		mov temp,EAX
	};
#endif
	return temp;
#else
#ifdef __HIGHC__
#ifdef __SASPPC__
//	temp = *longptr;
//	*longptr = *longptr + delta;
	_ASM("loop2:	lwarx	%r5,0,%r3");
	_ASM("			add		%r0,%r4,%r5");
	_ASM("			stwcx.	%r0,0,%r3");
	_ASM("			bne		loop2");
	_ASM("			ori		%r3,%r5,0");
#else
//	temp = *longptr;
//	*longptr = *longptr + delta;
	_ASM("	movl	8(%ebp),%edx");
	_ASM("	movl	12(%ebp),%eax");
	_ASM("	lock");
	_ASM("	xaddl	%eax,(%edx)");
#endif
#else
	long	temp;
#ifdef __GNUC__
   temp = delta;
#ifdef __powerpc__
#ifdef __powerpc64__	
	__asm__ (
		"	ori		12,%0,0;"
		"0:	ldarx	%0,0,%1;"
		"	add		11,%0,12;"
		"	stdcx.	11,0,%1;"
		"	bne		0b;"
		: "+b" (temp)
		: "p" (longptr)
		: "r11", "r12", "memory"
	);
#else	
	__asm__ (
		"	ori		12,%0,0;"
		"0:	lwarx	%0,0,%1;"
		"	add		11,%0,12;"
		"	stwcx.	11,0,%1;"
		"	bne		0b;"
		: "+b" (temp)
		: "p" (longptr)
		: "r11", "r12", "memory"
	);
#endif	
#else
#ifdef __x86_64__
   __asm__ (
   "	lock;"
   "	xadd	%0,(%1);"
      : "+r" (temp)
      : "p" (longptr)
      : "memory"
      );
#else
   __asm__ (
   "	lock;"
   "	xaddl	%0,(%1);"
      : "+r" (temp)
      : "p" (longptr)
      : "memory"
      );
#endif
#endif
#else
	temp = *longptr;
	*longptr = *longptr + delta;
#endif
	return temp;
#endif
#endif
#ifndef __SASPPC__
#endif
};

#ifdef __GNUC__
int SASCompareSwap(volatile long int *wordptr, long int match, long int replace)
#else
int SASCompareSwap(long *wordptr, long match, long replace)
#endif
{
#ifdef __BCPLUSPLUS__
	int temp;
#ifdef __MSDOS__
	asm {
		.486p
		les	BX,wordptr
		mov EAX,match
		mov ECX,replace
		lock
		cmpxchg dword ptr ES:[BX],ECX
		mov	AX,1
		jz good
		mov AX,0
	};
	good:
	asm	mov temp,AX
#else
	asm {
		.486p
		mov	EBX,wordptr
		mov EAX,match
		mov ECX,replace
		lock
		cmpxchg dword ptr [EBX],ECX

		mov	EAX,1

		jz good

		mov EAX,0
	};
	good:
	asm	mov temp,EAX
#endif
	return temp;
#else
#ifdef __HIGHC__
#ifdef __SASPPC__
/*
   if (*wordptr == match)
   {
	   *wordptr = replace;
	   temp = 1;
   } else {
	  temp = 0;
   };
   */
	_ASM("			addi	%r7,0,0");
	_ASM("			lwarx	%r6,0,%r3");
	_ASM("			cmpw	%r4,%r6");
	_ASM("			bne		done3");
	_ASM("			stwcx.	%r5,0,%r3");
	_ASM("			bne		done3");
	_ASM("			addi	%r7,0,1");
	_ASM("done3:		ori		%r3,%r7,0");
#else
/*
 int temp;
   if (*wordptr == match)
   {
	   *wordptr = replace;
	   temp = 1;
   } else {
	  temp = 0;
   };
 return temp;
 */ 

	_ASM("	movl	8(%ebp),%edx");
	_ASM("	movl	12(%ebp),%eax");
	_ASM("	movl	16(%ebp),%ecx");
	_ASM("	nop");
	_ASM("	lock");
	_ASM("	cmpxchgl %ecx,(%edx)");
	_ASM("	nop");
	_ASM("	movl	$1,%eax");
	_ASM("	jz	good");
	_ASM("	subl	%eax,%eax");
	_ASM("good:");

#endif
#else
int temp;
#ifdef __GNUC__
   temp = compare_and_swap(wordptr, match, replace);
#else
   if (*wordptr == match)
   {
	   *wordptr = replace;
	   temp = 1;
   } else {
	  temp = 0;
   };
#endif
	return temp;
#endif
#endif
#ifndef __SASPPC__
#endif
};


long SASAtomicSwap (long *wordptr, long replace)
{
#ifdef __BCPLUSPLUS__
	long temp;
#ifdef __MSDOS__
/*
	temp = *wordptr;
	*wordptr = replace;
   */
	asm {
		.486p
		les BX,wordptr
		mov EAX,replace
		lock
		xchg dword ptr ES:[EBX],EAX
		mov temp,EAX
	};
#else
	asm {
		.486p
		mov	EBX,wordptr
		mov EAX,replace
		lock
		xchg dword ptr [EBX],EAX
		mov temp,EAX
	};
#endif
	return temp;
#else
#ifdef __HIGHC__
#ifdef __SASPPC__
//	temp = *wordptr;
//	*wordptr = replace;
	_ASM("loop4:	lwarx	%r5,0,%r3");
	_ASM("			stwcx.	%r4,0,%r3");
	_ASM("			bne		loop4");
	_ASM("			ori		%r3,%r5,0");
#else
//	temp = *wordptr;
//	*wordptr = replace;
	_ASM("	movl	8(%ebp),%edx");
	_ASM("	movl	12(%ebp),%eax");
	_ASM("	lock");
	_ASM("	xchgl	%eax,(%edx)");
//	_ASM("	mov	[EBP-8],EAX");
#endif
#else
	long temp;
#ifdef __GNUC__
   temp = replace;
#ifdef __powerpc__
#ifdef __powerpc64__	
	__asm__ (
		"	ori		12,%0,0;"
		"0:	ldarx	%0,0,%1;"
		"	stdcx.	12,0,%1;"
		"	bne		0b;"
		: "+b" (temp)
		: "p" (wordptr)
		: "r12", "memory"
	);
#else	
	__asm__ (
		"	ori		12,%0,0;"
		"0:	lwarx	%0,0,%1;"
		"	stwcx.	12,0,%1;"
		"	bne		0b;"
		: "+b" (temp)
		: "p" (wordptr)
		: "r12", "memory"
	);
#endif	
#else
#ifdef __x86_64__
   __asm__ (
   "	lock;"
   "	xadd	%0,(%1);"
      : "+r" (temp)
      : "p" (wordptr)
      : "memory"
      );
#else
   __asm__ (
   "	lock;"
   "	xchgl	%0,(%1);"
      : "+r" (temp)
      : "p" (wordptr)
      : "memory"
      );
#endif
#endif
#else
	temp = *wordptr;
	*wordptr = replace;
#endif
	return temp;
#endif
#endif
#ifndef __SASPPC__
#endif
};

void SASAtomicInc (volatile long *longptr)
{
#ifdef __BCPLUSPLUS__
#ifdef __MSDOS__
	asm {
		.486p
		les	BX,longptr
		lock
		inc DWORD PTR ES:[BX]
	};
#else
	asm {
		.486p
		mov	EBX,longptr
		lock
		inc dword ptr [EBX]
	};
#endif
#else
#ifdef __HIGHC__
#ifdef __SASPPC__
//	(*longptr)++;
	_ASM("loop5:	lwarx	%r5,0,%r3");
	_ASM("			addi	%r0,%r5,1");
	_ASM("			stwcx.	%r0,0,%r3");
	_ASM("			bne		loop5");
#else
//	(*longptr)++;
	_ASM("	movl	8(%ebp),%edx");
	_ASM("	lock");
	_ASM("	incl	(%edx)");
#endif
#else
#ifdef __GNUC__
#ifdef __powerpc__
	long temp = 0;
#ifdef __powerpc64__	
	__asm__ (
		"0:	ldarx	%0,0,%1;"
		"	addi	%0,%0,1;"
		"	stdcx.	%0,0,%1;"
		"	bne		0b;"
		: "+b" (temp)
		: "p" (longptr), "m" (*longptr)
		: "cr0", "memory"
	);
#else	
	__asm__ (
		"0:	lwarx	%0,0,%1;"
		"	addi	%0,%0,1;"
		"	stwcx.	%0,0,%1;"
		"	bne		0b;"
		: "+b" (temp)
		: "p" (longptr), "m" (*longptr)
		: "cr0", "memory"
	);
#endif	
#else
   __asm__ (
   "	lock;"
   "	incl	(%0);"
      :
      : "p" (longptr)
      : "memory"
      );
#endif      
#else
	(*longptr)++;
#endif
#endif
#endif
};

void SASAtomicDec (volatile long *longptr)
{
#ifdef __BCPLUSPLUS__
#ifdef __MSDOS__
	asm {
		.486p
		les	BX,longptr
		lock
		dec dword ptr ES:[BX]
	};
#else
	asm {
		.486p
		mov	EBX,longptr
		lock
		dec	dword ptr [EBX]
	};
#endif
#else
#ifdef __HIGHC__
#ifdef __SASPPC__ 
//	(*longptr)--;
	_ASM("loop6:	lwarx	%r5,0,%r3");
	_ASM("			addi	%r0,%r5,-1");
	_ASM("			stwcx.	%r0,0,%r3");
	_ASM("			bne		loop6");
#else 
//	(*longptr)--;
	_ASM("	movl	8(%ebp),%edx");
	_ASM("	lock");
	_ASM("	dec	(%edx)");
#endif
#else
#ifdef __GNUC__
#ifdef __powerpc__
	long temp = 0;
#ifdef __powerpc64__	
	__asm__ (
		"0:	ldarx	%0,0,%1;"
		"	addi	%0,%0,-1;"
		"	stdcx.	%0,0,%1;"
		"	bne		0b;"
		: "+b" (temp)
		: "b" (longptr), "m" (*longptr)
		: "memory"
	);
#else	
	__asm__ (
		"0:	lwarx	%0,0,%1;"
		"	addi	%0,%0,-1;"
		"	stwcx.	%0,0,%1;"
		"	bne		0b;"
		: "+b" (temp)
		: "b" (longptr), "m" (*longptr)
		: "memory"
	);
#endif	
#else
   __asm__ (
   "	lock;"
   "	decl	(%0);"
      :
      : "p" (longptr)
      : "memory"
      );
#endif
#else
	(*longptr)--;
#endif
#endif
#endif
};

void
sas_spin_lock_with_yield (volatile  sas_spin_lock_t *lock)
{
  int rc;
  
  if (sas_spin_trylock(lock) == 0)
    return;
  if (sas_spin_trylock(lock) == 0)
    return;
  if (sas_spin_trylock(lock) == 0)
    return;
  if (sas_spin_trylock(lock) == 0)
    return;
    
  do
    {
      sched_yield();
      rc = sas_spin_trylock(lock);
    } 
  while (rc);

  return;
}

void
sas_lock_ptr_with_yield (volatile  void **lock)
{
  int rc;
  
  if (sas_trylock_ptr(lock) == 0)
    return;
  if (sas_trylock_ptr(lock) == 0)
    return;
  if (sas_trylock_ptr(lock) == 0)
    return;
  if (sas_trylock_ptr(lock) == 0)
    return;
    
  do
    {
      sched_yield();
      rc = sas_trylock_ptr(lock);
    } 
  while (rc);

  return;
}
