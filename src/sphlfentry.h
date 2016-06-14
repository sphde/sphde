/*
 * Copyright (c) 2011-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SPH_LOCK_FREE_ENTRY_H
#define __SPH_LOCK_FREE_ENTRY_H

/** \file sphlfentry.h
*  \brief Shared Persistent Heap, logger/queue etc event entry status,
*   update, and access functions.
*
*  \author Steven Munroe - initial API and implementation
*
*	For shared memory multi-thread/multi-core applications. Once the
*	Logger or Queue functions have atomically allocated an entry,
*	the "entry" APIs supports adding additional data to the entry and
*	retrieving that date later.
*
*	Supported functions include:
*	storing category specific event data,
*	atomic completion of an entry,
*	Getting entry status (complete and timestamped)
*	Getting entry header elements (timestamp, PID, TID, Category, and Sub-category),
*	Retrieving category specific event data entries,
*	and direct pointer access the header and data of the entry.
*
*	This Entry access API supports read out of the 16 byte
*	entry header including: Entry status and length.  Entry
*	Category and SubCategory codes. Process and Thread Ids.  High
*	resolution timestamp.
*
*	Any additional storage allocated to the entry
*	is available for application specific data.  This API also provides
*	several mechanisms to store application data including; direct
*	array or structure overlay, and a streams like mechanism.  Finally
*	the API provides a completion functions (SPHLFEntryComplete)
*	which provides and memory barriers required by the platform and
*	marks the entry complete.
*/


#include <string.h>
#include "sastype.h"
#include "sasatom.h"
#include "sphtimer.h"

/** \brief unsigned int type, consistent with the size of a pointer and used for pointer calculations **/
typedef unsigned long longPtr_t;

/** \brief Fields defining the entry header details word.
*/
typedef struct {
	/** Boolean: Indicates that this entry is valid (complete). **/
	unsigned int 	valid : 1;
	/** Boolean: Indicates that this entry is time stamped. **/
	unsigned int 	timestamped : 1;
	/** Boolean that indicates this entry is allocated. **/
	unsigned int 	allocated : 1;
	/** reserved **/
	unsigned int 	__reserved : 1;
	/** Application defined Event category code **/
	unsigned int 	category : 12;
	/** Application defined Event sub-category code **/
	unsigned int 	subcat : 8;
	/** Length of this entry **/
	unsigned int 	len : 8;
	} sphLFEntryLayout_t;

/** \brief Aggregate type for handling sphLogEntryLayout_t. **/
typedef unsigned int sphLFEntryID_t;

/** \brief Union of Entry details with 32-bit word for atomic update.
*/
typedef union {
	/** Logger Entry header as a Unit **/
	sphLFEntryID_t		idUnit;
	/** Logger Entry header in detail **/
	sphLFEntryLayout_t	detail;
	} sphLFEntry_t;

/** \brief Common type for PID/TID values stored in event entries.  **/
typedef unsigned short sphpid16_t;

/** \brief Instance of a Lock Free Entry Header.
*
*   Contains fields defining the standard timestamped entry header.
*/
typedef struct {
		/** Entry header detailes **/
        sphLFEntry_t   entryID;
        /** Process ID **/
        sphpid16_t      PID;
        /** Thread ID **/
        sphpid16_t      TID;
        /** High resolution 64-bit Time Stamp **/
        sphtimer_t      timeStamp;
        } SPHLFEntryHeader_t;

/** \brief Instance of a Lock Free event data Entry Handle.
*
*   Contains fields required to: locate the entry, record the total
*   space allocated to the entry, and manage the next location within
*   the entry and remaining storage.
*
*	Entry Handles should be allocated in private (local stack) storage
*	to allow concurrent access to independent entries from multiple threads.
*/
typedef struct {
    /** address of the start of the Queue entry **/
     SPHLFEntryHeader_t	*entry;
	/** address of the next avail byte(s) of storage **/
	char			*next;
	/** Total allocated size of the event entry **/
	unsigned short int	total_size;
	/** Remaining bytes of free storage in the entry **/
	unsigned short int	remaining;
	} SPHLFEntryHandle_t;

/** \brief Marks the entry specified by the entry handle as complete.
*	Also executes any memory barriers required by the platform to ensure
*	that all previous stores to this entry by this thread are visible
*	to other threads.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return a 0 value indicates success.
*/
static inline int
SPHLFEntryStrongComplete (SPHLFEntryHandle_t *handlespace)
{
    int rc = 0;
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;
    sphLFEntry_t	entrytemp;

    sas_read_barrier();
    entrytemp.idUnit = entryPtr->entryID.idUnit;
    entrytemp.detail.valid = 1;
    entryPtr->entryID.idUnit = entrytemp.idUnit;

    return rc;
}

/** \brief Marks the entry specified by the entry handle as complete.
*	No memory barriers are performance.
*	On out-of-order machines there are no guarantee that all
*	previous stores by this thread are visible to other threads.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return a 0 value indicates success.
*/
static inline int
SPHLFEntryWeakComplete (SPHLFEntryHandle_t *handlespace)
{
    int rc = 0;
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;
    sphLFEntry_t	entrytemp;

    entrytemp.idUnit = entryPtr->entryID.idUnit;
    entrytemp.detail.valid = 1;
    entryPtr->entryID.idUnit = entrytemp.idUnit;

    return rc;
}

/** \brief Marks the entry specified by the entry handle as complete.
*	Also executes write memory barries required by the platform to ensure
*	that all previous stores by this thread to this entry are complete.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return a 0 value indicates success.
*/
static inline int
SPHLFEntryComplete (SPHLFEntryHandle_t *handlespace)
{
    int rc = 0;
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;
    sphLFEntry_t	entrytemp;

    sas_write_barrier();
    entrytemp.idUnit = entryPtr->entryID.idUnit;
    entrytemp.detail.valid = 1;
    entryPtr->entryID.idUnit = entrytemp.idUnit;

    return rc;
}

/** \brief Return the status of the entry specified by the entry handle.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return true if the entry was complete (SPHLFLoggerEntryComplete
*	has been called fo this entry). Otherwise False.
*/
static inline int
SPHLFEntryIsComplete (SPHLFEntryHandle_t *handlespace)
{
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;

    return (entryPtr->entryID.detail.valid == 1);
}

/** \brief Return the status of the entry specified by the entry handle.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return true if the entry was time stamped. Otherwise False.
*/
static inline int
SPHLFEntryIsTimestamped (SPHLFEntryHandle_t *handlespace)
{
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;

    return ((entryPtr->entryID.detail.valid == 1)
	  &&(entryPtr->entryID.detail.timestamped == 1));
}

/** \brief Return the time stamp value for the entry specified by the entry handle.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return the time stamp value from the entry, if the entry was valid
*	and time stamped. Otherwise return 0.
*/
static inline sphtimer_t
SPHLFEntryTimeStamp (SPHLFEntryHandle_t *handlespace)
{
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;
    sphtimer_t result = 0;

    if ((entryPtr->entryID.detail.valid == 1)
	  &&(entryPtr->entryID.detail.timestamped == 1))
	result = entryPtr->timeStamp;

    return result;
}

/** \brief Return the process ID for the entry specified by the entry handle.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return the PID from the entry, if the entry was valid
*	and time stamped. Otherwise return 0.
*/
static inline sphpid16_t
SPHLFEntryPID (SPHLFEntryHandle_t *handlespace)
{
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;
    sphpid16_t result = 0;

    if ((entryPtr->entryID.detail.valid == 1)
	  &&(entryPtr->entryID.detail.timestamped == 1))
	result = entryPtr->PID;

    return result;
}

/** \brief Return the thread ID for the entry specified by the entry handle.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return the TID from the entry, if the entry was valid
*	and time stamped. Otherwise return 0.
*/
static inline sphpid16_t
SPHLFEntryTID (SPHLFEntryHandle_t *handlespace)
{
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;
    sphpid16_t result = 0;

    if ((entryPtr->entryID.detail.valid == 1)
	  &&(entryPtr->entryID.detail.timestamped == 1))
	result = entryPtr->TID;

    return result;
}

/** \brief Return the address for the entry header specified by the entry handle.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return the address from the entry header, if the entry was valid.
*	Otherwise return NULL.
*/
static inline SPHLFEntryHeader_t*
SPHLFEntryHeader (SPHLFEntryHandle_t *handlespace)
{
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;

    if (entryPtr->entryID.detail.valid != 1)
	entryPtr = NULL;

    return entryPtr;
}

/** \brief Return the entry category for the entry specified by the entry handle.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return the category from the entry, if the entry was valid.
*	Otherwise return 0.
*/
static inline int
SPHLFEntryCategory (SPHLFEntryHandle_t *handlespace)
{
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;
    int result = -1;
#if 0
    printf ("SPHLFEntryCategory(%p) entry=%p id=%x\n",
		handlespace, entryPtr, entryPtr->entryID.idUnit);
#endif
    if (entryPtr->entryID.detail.valid == 1)
	result = entryPtr->entryID.detail.category;

    return result;
}

/** \brief Return the entry sub-category for the entry specified by the entry handle.
*
*   @param handlespace Entry Handle for an allocated entry.
*	@return the sub-category from the entry, if the entry was valid.
*	Otherwise return 0.
*/
static inline int
SPHLFEntrySubcat (SPHLFEntryHandle_t *handlespace)
{
    SPHLFEntryHeader_t	*entryPtr = handlespace->entry;
    int result = -1;
#if 0
    printf ("SPHLFEntrySubcat(%p) entry=%p id=%x\n",
		handlespace, entryPtr, entryPtr->entryID.idUnit);
#endif
    if (entryPtr->entryID.detail.valid == 1)
	result = entryPtr->entryID.detail.subcat;

    return result;
}

/** \brief Return the first free byte address for the entry specified by
*   the entry handle.
*
*   \warning This function should be used carefully. It is not safe to use
*   if other application functions may need to update the same entry.
*   It is also not safe to use for software that needs to cross platform
*   because it does not handle platform specific size and alignment
*   requirements.
*
*   \note The SPHLOGENTRYALLOCSTRUCT/SPHLFlogEntryAllocStruct API is recommended
*   for code that needs to operate cross platform.
*
*   @param handle Entry Handle for an allocated entry.
*	@return address the entries free space.
*/
static inline void*
SPHLFEntryGetFreePtr (SPHLFEntryHandle_t *handle)
{
	char		*ptr	= handle->next;
	
	return (void*)ptr;
}

/** \brief Macro for using sizeof/__alignof__ parms with
*   SPHLFEntryGetStructPtr function.  **/
#define SPHENTRYGETSTRUCTPTR(__handle, __struct) SPHLFEntryGetStructPtr(__handle, sizeof(__struct), __alignof__(__struct))

/** \brief Return the correctly aligned pointer for a struct or array
*   starting at the next free location within the entry.
*
*   The entries next pointer is adjusted for alignment and returned.
*   The entries next pointer and remaining fields are updated for the
*   next field following the struct/array.
*
*   \note The SPHENTRYGETSTRUCTPTR can be used to insure the correct usage
*   sizeof and __alignof__ to provide values for the __size and __align
*   parameters.
*
*   @param handle Entry Handle for an allocated entry.
*   @param __size of the struct/array to allocate.
*   @param __align alignment requirement of the struct/array space
*   to allocated.
*	@return address of the allocated free space. NULL indicates failure.
*	For example if remaining entry space
*	is insufficient to hold the struct at the required alignment.
*/
static inline  void*
SPHLFEntryGetStructPtr (SPHLFEntryHandle_t *handle,
		unsigned long __size, unsigned long __align)
{
	char	*ptr	= (char*)handle->next;
	unsigned long	len	= handle->remaining;
	unsigned long	adjust	= __align - 1;
	unsigned long	mask	= ~adjust;

	ptr = (char*)
		((((unsigned long)ptr)
		+ adjust)
		& mask);

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= __size)
	{
		handle->next = ptr + __size;
		handle->remaining = len - __size;
	} else {
		ptr = NULL;
	}
	return ptr;
}

/** \brief Macro for using sizeof/__alignof__ parms with
*   SPHLFEntryAllocStruct function.  **/
#define SPHENTRYALLOCSTRUCT(__handle, __struct) SPHLFEntryAllocStruct(__handle, sizeof(__struct), __alignof__(__struct))

/** \brief Allocate space for struct starting at the next free location
*   within the entry.
*
*   Allocate space in the log entry for a multi-field structure or an array.
*   The returned pointer can then be used to directly store data into struct
*   fields or array entries.
*   The SPHENTRYALLOCSTRUCT can be used to insure the correct usage
*   sizeof and __alignof__ to provide values for the __size and __align
*   parameters.
*
*   \note This function should be used instead of SPHLFEntryGetFreePtr
*   if additional data may be added later to the same entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param __size of the struct/array to allocate.
*   @param __align alignment requirement of the struct/array space
*   to allocated.
*	@return address of the allocated free space. NULL indicates failure.
*	For example if remaining entry space
*	is insufficient to hold the struct at the required alignment.
*/
static inline  void*
SPHLFEntryAllocStruct (SPHLFEntryHandle_t *handle,
		unsigned long __size, unsigned long __align)
{
	char	*ptr	= (char*)handle->next;
	unsigned long	len	= handle->remaining;
	unsigned long	adjust	= __align - 1;
	unsigned long	mask	= ~adjust;

	ptr = (char*)
		((((unsigned long)ptr)
		+ adjust)
		& mask);

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= __size)
	{
		handle->next = ptr + __size;
		handle->remaining = len - __size;
	} else {
		ptr = NULL;
	}
	return ptr;
}

/** \brief Insert a C string at the next free location within the entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param value pointer to a C string value.
*	@return 0 if successful, -1 if the insert fails.
*	For example if the string is too
*	large for the remain entry free space.
*/
static inline int
SPHLFEntryAddString (SPHLFEntryHandle_t *handle,
			char *value)
{
	char		*ptr	= handle->next;
	unsigned short int	len	= handle->remaining;
	int		vlen	= (strlen(value) + 1);
	int		rc	= 0;

	if (len >= vlen)
	{
		strcpy (ptr, value);
		ptr += vlen;
		len -= vlen;
		handle->next = ptr;
		handle->remaining = len;
	} else {
		rc = -1;
	}
	return rc;
}

/** \brief Insert a character at the next free location within the entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param value a char value.
*	@return 0 if successful, -1 if the insert fails.
*	For example if remaining entry space
*	is insufficient to hold a char.
*/
static inline int
SPHLFEntryAddChar (SPHLFEntryHandle_t *handle,
			char value)
{
	char	*ptr	= handle->next;
	unsigned short int	len	= handle->remaining;
	int	rc	= 0;

	if (len >= sizeof(char))
	{
		*ptr++ = value;
		len -= sizeof(char);
		handle->next = (char*)ptr;
		handle->remaining = len;
	} else {
		rc = -1;
	}
	return rc;
}

/** \brief Insert a short int at the next free location within the entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param value a short int value.
*	@return 0 if successful, -1 if the insert fails.
*	For example if remaining entry space
*	is insufficient to hold a short int plus any required alignment.
*/
static inline  int
SPHLFEntryAddShort (SPHLFEntryHandle_t *handle,
			short int value)
{
	short int	*ptr	= (short int*)handle->next;
	unsigned short int	len	= handle->remaining;
	int		rc	= 0;
	unsigned long	adjust	= __alignof__(short int) -1;
	unsigned long	mask	= ~adjust;

	ptr = (short int*)
		((((unsigned long)ptr)
		+ adjust)
		& mask);

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(short int))
	{
		*ptr++ = value;
		len -= sizeof(short);
		handle->next = (char*)ptr;
		handle->remaining = len;
	} else {
		rc = -1;
	}
	return rc;
}

/** \brief Insert a int at the next free location within the entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param value a int value.
*	@return 0 if successful, -1 if the insert fails.
*	For example if remaining entry space
*	is insufficient to hold a int plus any required alignment.
*/
static inline  int
SPHLFEntryAddInt (SPHLFEntryHandle_t *handle,
			int value)
{
	int		*ptr	= (int*)handle->next;
	unsigned short int	len	= handle->remaining;
	int		rc	= 0;

	ptr = (int*)(((((unsigned long)ptr))
		+ (__alignof__(int) -1))
		& (~(__alignof__(int) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(int))
	{
		*ptr++ = value;
		len -= sizeof(int);
		handle->next = (char*)ptr;
		handle->remaining = len;
	} else {
		rc = -1;
	}
	return rc;
}

/** \brief Insert a long int at the next free location within the entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param value a long int value.
*	@return 0 if successful, -1 if the insert fails.
*	For example if remaining entry space
*	is insufficient to hold a long int plus any required alignment.
*/
static inline  int
SPHLFEntryAddLong (SPHLFEntryHandle_t *handle,
			long value)
{
	long		*ptr	= (long*)handle->next;
	unsigned short int	len	= handle->remaining;
	int		rc	= 0;

	ptr = (long*)(((((unsigned long)ptr))
		+ (__alignof__(long) -1))
		& (~(__alignof__(long) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(long))
	{
		*ptr++ = value;
		len -= sizeof(long);
		handle->next = (char*)ptr;
		handle->remaining = len;
	} else {
		rc = -1;
	}
	return rc;
}

/** \brief Insert a void* at the next free location within the entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param value a void* (C pointer) value.
*	@return 0 if successful, -1 if the insert fails.
*	For example if remaining entry space
*	is insufficient to hold a void* plus any required alignment.
*/
static inline  int
SPHLFEntryAddPtr (SPHLFEntryHandle_t *handle,
			void *value)
{
	void		**ptr	= (void**)handle->next;
	unsigned short int	len	= handle->remaining;
	int		rc	= 0;

	ptr = (void**)(((((unsigned long)ptr))
		+ (__alignof__(void*) -1))
		& (~(__alignof__(void*) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(void*))
	{
		*ptr++ = value;
		len -= sizeof(void*);
		handle->next = (char*)ptr;
		handle->remaining = len;
	} else {
		rc = -1;
	}
	return rc;
}

/** \brief Insert a long long int at the next free location within the entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param value a long long int value.
*	@return 0 if successful, -1 if the insert fails.
*	For example if remaining entry space
*	is insufficient to hold a long long int plus any required alignment.
*/
static inline  int
SPHLFEntryAddLongLong (SPHLFEntryHandle_t *handle,
			long long value)
{
	long long	*ptr	= (long long*)handle->next;
	unsigned short int	len	= handle->remaining;
	int		rc	= 0;

	ptr = (long long*)(((((unsigned long)ptr))
		+ (__alignof__(long long) -1))
		& (~(__alignof__(long long) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(long long))
	{
		*ptr++ = value;
		len -= sizeof(long long);
		handle->next = (char*)ptr;
		handle->remaining = len;
	} else {
		rc = -1;
	}
	return rc;
}

/** \brief Insert a float at the next free location within the entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param value a float value.
*	@return 0 if successful, -1 if the insert fails.
*	For example if remaining entry space
*	is insufficient to hold a float plus any required alignment.
*/
static inline  int
SPHLFEntryAddFloat (SPHLFEntryHandle_t *handle,
			float value)
{
	float		*ptr	= (float*)handle->next;
	unsigned short int	len	= handle->remaining;
	int		rc	= 0;

	ptr = (float*)(((((unsigned long)ptr))
		+ (__alignof__(float) -1))
		& (~(__alignof__(float) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(float))
	{
		*ptr++ = value;
		len -= sizeof(float);
		handle->next = (char*)ptr;
		handle->remaining = len;
	} else {
		rc = -1;
	}
	return rc;
}

/** \brief Insert a double at the next free location within the entry.
*
*   @param handle Entry Handle for an allocated entry.
*   @param value a double value.
*	@return 0 if successful, -1 if the insert fails.
*	For example if remaining entry space
*	is insufficient to hold a double plus any required alignment.
*/
static inline  int
SPHLFEntryAddDouble (SPHLFEntryHandle_t *handle,
			double value)
{
	double		*ptr	= (double*)handle->next;
	unsigned short int	len	= handle->remaining;
	int		rc	= 0;

	ptr = (double*)(((((unsigned long)ptr))
		+ (__alignof__(double) -1))
		& (~(__alignof__(double) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(double))
	{
		*ptr++ = value;
		len -= sizeof(double);
		handle->next = (char*)ptr;
		handle->remaining = len;
	} else {
		rc = -1;
	}
	return rc;
}

/** \brief Return the next char from the entry via the current
*   next value pointer.
*	The internal next value pointer is advanced to the next location.
*
*   @param handle Entry Handle for an allocated entry.
*	@return the char value if successful, 0 (NUL) if the get fails.
*	For example if the next is at the end of the Logger entry.
*/
static inline  char
SPHLFEntryGetNextChar (SPHLFEntryHandle_t *handle)
{
	char	*ptr	= handle->next;
	unsigned short int	len	= handle->remaining;
	char	value = 0;

	if (len >= sizeof(char))
	{
		value = *ptr++;
		len -= sizeof(char);
		handle->next = (char*)ptr;
		handle->remaining = len;
	}
	return value;
}

/** \brief Return the pointer to the next C string from the logger
*   entry via the current next value pointer.
*	The internal next value pointer is advanced to the next location after the string NUL char.
*
*   @param handle Entry Handle for an allocated entry.
*	@return the C string pointer value if successful, 0 (NULL) if the get fails.
*	For example if the next is at the end of the Logger entry.
*/
static inline  char*
SPHLFEntryGetNextString (SPHLFEntryHandle_t *handle)
{
	char		*ptr	= handle->next;
	unsigned short int	len	= handle->remaining;
	int		vlen	= (strlen(ptr) + 1);
	char		*value	= 0;

	if (len >= vlen)
	{
		value = ptr;
		ptr += vlen;
		len -= vlen;
		handle->next = ptr;
		handle->remaining = len;
	}
	return value;
}

/** \brief Return the next short int from the entry via the
*   current next value pointer.
*	Leading bytes may be skipped to get the required alignment.
*	The internal next value pointer is advanced to the next location.
*
*   @param handle Entry Handle for an allocated entry.
*	@return the short int value if successful, 0 if the get fails.
*	For example if the next is at the end of the Logger entry.
*/
static inline short int
SPHLFEntryGetNextShort (SPHLFEntryHandle_t *handle)
{
	short int	*ptr	= (short int*)handle->next;
	unsigned short int	len	= handle->remaining;
	unsigned long	adjust	= __alignof__(short int) -1;
	unsigned long	mask	= ~adjust;
	short int	value	= 0;

	ptr = (short int*)
		((((unsigned long)ptr)
		+ adjust)
		& mask);

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(short int))
	{
		value = *ptr++;
		len -= sizeof(short);
		handle->next = (char*)ptr;
		handle->remaining = len;
	}
	return value;
}

/** \brief Return the next int from the entry via the
*   current next value pointer.
*	Leading bytes may be skipped to get the required alignment.
*	The internal next value pointer is advanced to the next location.
*
*   @param handle Entry Handle for an allocated entry.
*	@return the int value if successful, 0 if the get fails.
*	For example if the next is at the end of the Logger entry.
*/
static inline  int
SPHLFEntryGetNextInt (SPHLFEntryHandle_t *handle)
{
	int		*ptr	= (int*)handle->next;
	unsigned short int	len	= handle->remaining;
	int		value	= 0;

	ptr = (int*)(((((unsigned long)ptr))
		+ (__alignof__(int) -1))
		& (~(__alignof__(int) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(int))
	{
		value = *ptr++;
		len -= sizeof(int);
		handle->next = (char*)ptr;
		handle->remaining = len;
	}
	return value;
}

/** \brief Return the next long int from the entry via the
*   current next value pointer.
*	Leading bytes may be skipped to get the required alignment.
*	The internal next value pointer is advanced to the next location.
*
*   @param handle Entry Handle for an allocated entry.
*	@return the long int value if successful, 0 if the get fails.
*	For example if the next is at the end of the Logger entry.
*/
static inline  long
SPHLFEntryGetNextLong (SPHLFEntryHandle_t *handle)
{
	long		*ptr	= (long*)handle->next;
	unsigned short int	len	= handle->remaining;
	long		value	= 0;

	ptr = (long*)(((((unsigned long)ptr))
		+ (__alignof__(long) -1))
		& (~(__alignof__(long) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(long))
	{
		value = *ptr++;
		len -= sizeof(long);
		handle->next = (char*)ptr;
		handle->remaining = len;
	}
	return value;
}

/** \brief Return the next void* from the entry via the current
*   next value pointer.
*	Leading bytes may be skipped to get the required alignment.
*	The internal next value pointer is advanced to the next location.
*
*   @param handle Entry Handle for an allocated entry.
*	@return the void* value if successful, 0 (NULL) if the get fails.
*	For example if the next is at the end of the Logger entry.
*/
static inline  void*
SPHLFEntryGetNextPtr (SPHLFEntryHandle_t *handle)
{
	void		**ptr	= (void**)handle->next;
	unsigned short int	len	= handle->remaining;
	void		*value	= NULL;

	ptr = (void**)(((((unsigned long)ptr))
		+ (__alignof__(void*) -1))
		& (~(__alignof__(void*) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(void*))
	{
		value = *ptr++;
		len -= sizeof(void*);
		handle->next = (char*)ptr;
		handle->remaining = len;
	}
	return value;
}

/** \brief Return the next long long int from the entry via the
*   current next value pointer.
*	Leading bytes may be skipped to get the required alignment.
*	The internal next value pointer is advanced to the next location.
*
*   @param handle Entry Handle for an allocated entry.
*	@return the long long int value if successful,0 if the get fails. For example if the next is at the end
*	of the Logger entry.
*/
static inline  long long
SPHLFEntryGetNextLongLong (SPHLFEntryHandle_t *handle)
{
	long long	*ptr	= (long long*)handle->next;
	unsigned short int	len	= handle->remaining;
	long long		value	= 0LL;

	ptr = (long long*)(((((unsigned long)ptr))
		+ (__alignof__(long long) -1))
		& (~(__alignof__(long long) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(long long))
	{
		value = *ptr++;
		len -= sizeof(long long);
		handle->next = (char*)ptr;
		handle->remaining = len;
	}
	return value;
}

/** \brief Return the next float from the entry via the current
*   next value pointer.
*	Leading bytes may be skipped to get the required alignment.
*	The internal next value pointer is advanced to the next location.
*
*   @param handle Entry Handle for an allocated entry.
*	@return the float value if successful, 0.0 if the get fails.
*	For example if the next is at the end of the Logger entry.
*/
static inline  float
SPHLFEntryGetNextFloat (SPHLFEntryHandle_t *handle)
{
	float		*ptr	= (float*)handle->next;
	unsigned short int	len	= handle->remaining;
	float		value	= 0.0;

	ptr = (float*)(((((unsigned long)ptr))
		+ (__alignof__(float) -1))
		& (~(__alignof__(float) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(float))
	{
		value = *ptr++;
		len -= sizeof(float);
		handle->next = (char*)ptr;
		handle->remaining = len;
	}
	return value;
}

/** \brief Return the next double from the entry via the current
*   next value pointer.
*	Leading bytes may be skipped to get the required alignment.
*	The internal next value pointer is advanced to the next location.
*
*   @param handle Entry Handle for an allocated entry.
*	@return the double value if successful, 0.0 if the get fails.
*	For example if the next is at the end of the Logger entry.
*/
static inline  double
SPHLFEntryGetNextDouble (SPHLFEntryHandle_t *handle)
{
	double		*ptr	= (double*)handle->next;
	unsigned short int	len	= handle->remaining;
	double		value	= 0.0;

	ptr = (double*)(((((unsigned long)ptr))
		+ (__alignof__(double) -1))
		& (~(__alignof__(double) -1)));

	if ((unsigned long)ptr != (unsigned long)handle->next)
	{
		len -= ((unsigned long)ptr
			- (unsigned long)handle->next);
	}

	if (len >= sizeof(double))
	{
		value = *ptr++;
		len -= sizeof(double);
		handle->next = (char*)ptr;
		handle->remaining = len;
	}
	return value;
}

#endif /* __SPH_LOCK_FREE_ENTRY_H */
