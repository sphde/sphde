/*
 * Copyright (c) 2007-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 * sjm 2010-09-05       Added externs for proc/thread ID 
 *                      Added fast inline PID/TID get functions *
 *     IBM Corporation, Adhemerval Zanella - documentation
 */

#ifndef __SPHTHREAD_H
#define __SPHTHREAD_H

#include <unistd.h>

/*!
 * \file  sphthread.h
 * \brief Thread utility functions
 *
 * The process pid can be obtained using ::sphdeGetPID, thread id by
 * ::sphdeGetTID, and command line string by ::sphdeGetCmdLine. Fast versions
 * that access static variables and avoid syscall where possible are
 * provided: ::sphFastGetPID for process id and ::sphFastGetTID for thread id.
 *
 * For instance, to provide a fast event log header:
 *
 * \code
 *
 * struct eventHeader
 * {
 *   unsigned int eventID;
 *   unsigned short pid;
 *   unsigned short tid;
 *   unsigned sphtimer_t timestamp;
 * } evh;
 *
 * evh.eventID = 0;
 * evh.pid = sphFastGetPID ();
 * evh.tid = sphFastGetTID ();
 * evh.timestamp = sphgettimer ();
 *
 * \endcode
 */

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/*!
 * \brief Return the process identification.
 *
 * The function issues a syscall when needed and it also update its internal
 * command line value (so a subsequent call to ::sphdeGetCmdLine will just
 * return the already available command line value).
 *
 * @return The process id or 0 if an error occurs.
 */
extern __C__ pid_t
sphdeGetPID (void);

/*!
 * \brief Return the thread identification.
 *
 * The function issues a syscall when needed. The thread identification value
 * is a per-thread variable so a subsequent call to ::sphFastGetTID will just
 * need to return a variable value (no syscall needed).
 *
 * @return The thread identification or -1 if an error occurs.
 */
extern __C__ pid_t
sphdeGetTID (void);

/*!
 * \brief Return the command line string.
 *
 * The command line is obtained from '/proc/pid/cmdline' and maintained in
 * an internal static variable.
 *
 * @return A null-terminated string with the process command line.
 */
extern __C__ char *
sphdeGetCmdLine (void);

/*!
 * Process identification variable. Should not be accessed directly, instead
 * the function ::sphFastGetPID should be used.
 */
extern pid_t procID;

/*!
 * \brief Return the process identification.
 *
 * Fast version of ::sphdeGetPID.
 *
 * @return The process id or 0 if an error occurs.
 */
static inline int
sphFastGetPID (void)
{
  int pid = procID;
  if (__builtin_expect ((!pid), 0))
    {
      pid = sphdeGetPID ();
    }
  return pid;
}

/*!
 * Per-thread thread identification variable. Should not be accessed directly,
 * instead the function ::sphFastGetTID should be used.
 */
extern __thread pid_t threadID __attribute__ ((tls_model ("initial-exec")));

/*!
 * \brief Return the thread identification.
 *
 * Fast version of ::sphdeGetTID.
 *
 * @return The thread identification or -1 if an error occurs.
 */
static inline int
sphFastGetTID (void)
{
  int tid = threadID;
  if (__builtin_expect ((!tid), 0))
    {
      tid = sphdeGetTID ();
    }
  return tid;
}

#endif
