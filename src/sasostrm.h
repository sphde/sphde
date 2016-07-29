/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Maynard Johnson - initial API and implementation
 */


/*
* This class is primarily intended for use by SasUserLockTest.C to
* provide a thread-safe cout object.
*
*/
#ifndef __SASOSTREAM_H
#define __SASOSTREAM_H

#include <stdio.h>
#include "sasulock.h"

// The hc++ compiler has a problem with the asm code in the cthreads.h file
#ifndef _NO_INLINE_ASM
#define _NO_INLINE_ASM
#endif

extern "C"
{
//#include <cthreads.h>	// spin_lock_t
}

#define endl "\n"

//This header file declares the following class:

class SasOstream
{
public:
  SasOstream (void);
  SasOstream & operator<<(void * a);
  SasOstream & operator<<(char * s);
  SasOstream & operator<<(int a);

private:
  spin_lock_t cout_lock;
};

extern SasOstream SasCout;

#endif

