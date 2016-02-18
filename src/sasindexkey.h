/*
 * Copyright (c) 2005-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SAS_INDEXKEY_H
#define __SAS_INDEXKEY_H

#include <string.h>

/*!
 * \file  sasindexkey.h
 * \brief API for defining binary Index keys for B-Trees.
 *
 * Intialize the SASIndexKey_t structure with a sequence of values to
 * be used as keys for SASIndex_t BTrees.
 *
 * \todo Enhance this API to build larger (multi-part) keys via a
 * streams interface similar to interface in sphlfentry.h.
 * We have enough to here to implement SPHContext_t but a generalized
 * API would make SASIndex_t more useful.
 *
 */

#ifdef __WORDSIZE_64
/*! \brief word size data unit used for binary keys. */
typedef unsigned long machine_uint_t;
/*! \brief data unit used to store copy and compare lengths. */
typedef unsigned int machine_uhalf_t;
/*! \brief mask use to invert the signbit. */
const unsigned long machine_sign_mask = (0x8000000000000000);
/*! \brief mask use to invert the exponent. */
const unsigned long double_exp_mask = (0x7ff0000000000000);
/*! \brief mask use to invert the sign and exponent. */
const unsigned long double_mask = (0xffffffffffffffff);
#else
/*! \brief word size data unit used for binary keys. */
typedef unsigned long machine_uint_t;
/*! \brief data unit used to store copy and compare lengths. */
typedef unsigned short machine_uhalf_t;
/*! \brief mask use to invert the signbit. */
const unsigned long machine_sign_mask = (0x80000000);
/*! \brief mask use to invert the exponent. */
const unsigned long double_exp_mask = (0x7ff00000);
/*! \brief mask use to invert the sign and exponent. */
const unsigned long double_mask = (0xffffffff);
#endif

/*!
 * \brief Index Key Handle structure for binary index B-trees.
 *
 */
typedef struct SASIndexKey_t
{
  /*! compare size in bytes of the key. */
  machine_uhalf_t copy_size;
  /*! copy size in bytes of the key struct. */
  machine_uhalf_t compare_size;
  /*! data area containing the binary key. */
  machine_uint_t data[15];
} SASIndexKey_t;

/*!
 * \brief binary key compare logic for Index B-Tree keys.
 *
 * Optimized for comparing unsigned long integer word values that are
 * the size of address pointers (void*)
 * Also optimized for early (in the first or only word of the key)
 * misscompare.
 *
 * @param op_a Handle of the left SASIndexKey_t.
 * @param op_b Handle of the right SASIndexKey_t.
 * @return an integer value -1 for op_a < op_b, 0 for op_a == op_b,
 * and +1 for op_a > op_b.
 */
static inline int
SASIndexKeyCompare (SASIndexKey_t * op_a, SASIndexKey_t * op_b)
{
  int rc = 0;
  if (op_a->data[0] != op_b->data[0])
    {
      if (op_a->data[0] < op_b->data[0])
	rc = -1;
      else
	rc = 1;

    }
  else
    {
      size_t len = op_a->compare_size;

      if (op_a->compare_size != op_b->compare_size)
	{
	  if (op_a->compare_size > op_b->compare_size)
	    len = op_b->compare_size;

	  rc = memcmp (&op_a->data[0], &op_b->data[0], len);

	  if ((rc == 0) && (op_a->compare_size != op_b->compare_size))
	    {
	      if (op_a->compare_size < op_b->compare_size)
		rc = -1;
	      else
		rc = 1;
	    }
	}
      else if (len > sizeof (machine_uint_t))
	{
	  unsigned int i = 1;
	  for (i = 1; i < (len / sizeof (machine_uint_t)); i++)
	    {
	      if (op_a->data[i] != op_b->data[i])
		{
		  if (op_a->data[i] > op_b->data[i])
		    rc = 1;
		  else
		    rc = -1;
		  break;
		}
	    }
	}
    }
  return rc;
}

/*!
 * \brief Copy a binary key from source to destination.
 *
 * Use the copy_size to copy the required header and the
 * significant words of key data.
 *
 * @param dest Handle of the destination SASIndexKey_t.
 * @param src Handle of the source SASIndexKey_t.
 */
static inline void
SASIndexKeyCopy (SASIndexKey_t * dest, SASIndexKey_t * src)
{
  memcpy (dest, src, src->copy_size);
}

/*!
 * \brief Return the binary index key copy_size.
 *
 * Use the copy_size to copy the required header and the
 * significant words of key data.
 *
 * @param key Handle of a SASIndexKey_t.
 * @return current copy_size of the referenced key.
 */
static inline size_t
SASIndexKeySize (SASIndexKey_t * key)
{
  return key->copy_size;
}

/*!
 * \brief Initial a binary key @ destination with a address value
 *
 * @param dest Handle of the destination SASIndexKey_t.
 * @param value Address value which will be the key.
 */
static inline void
SASIndexKeyInitRef (SASIndexKey_t * dest, void *value)
{
  dest->compare_size = sizeof (void *);
  dest->copy_size = 2 * sizeof (void *);
  dest->data[0] = (machine_uint_t) value;
}


/** \brief Union of value types and key machine integer types.
*/
typedef union {
    /*! data area containing the binary key. */
#ifdef __WORDSIZE_64
	machine_uint_t	key_element;
#else
    struct {
#if __BYTE_ORDER == __BIG_ENDIAN
		machine_uint_t	data_0;
		machine_uint_t	data_1;
#else
		machine_uint_t	data_1;
		machine_uint_t	data_0;
#endif
    } key_element;
#endif
    /*! key element type as an unsigned 64-bit integer. */
    unsigned long long	uint64_key;
    /*! key element type as a signed 64-bit integer. */
    long long			int64_key;
    /*! key element type as a double. */
    double				double_key;
	} sasindexkeymap_t;

/*!
 * \brief Initial a binary key @ destination with a unsigned 64-bit
 * integer value.
 *
 * @param dest Handle of the destination SASIndexKey_t.
 * @param value unsigned long long value which will be the key.
 */
static inline void
SASIndexKeyInitUInt64 (SASIndexKey_t * dest, unsigned long long value)
{
  sasindexkeymap_t	key_val;

  key_val.uint64_key = value;
#ifdef __WORDSIZE_64
  dest->compare_size = sizeof (unsigned long long);
  dest->copy_size = sizeof (void *) + sizeof (unsigned long long);
  dest->data[0] = key_val.key_element;
#else
  dest->compare_size = sizeof (unsigned long long);
  dest->copy_size = sizeof (void *) + sizeof (unsigned long long);
  dest->data[0] = key_val.key_element.data_0;
  dest->data[1] = key_val.key_element.data_1;
#endif
}

/*!
 * \brief Return the value of the 1st binary key as a unsigned 64-bit
 * integer value.
 *
 * @param dest Handle of the source SASIndexKey_t.
 * @return value of the 1st key element as a unsigned 64-bit integer.
 */
static inline unsigned long long
SASIndexKeyReturn1stUInt64 (SASIndexKey_t * dest)
{
  sasindexkeymap_t	key_val;

#ifdef __WORDSIZE_64
  key_val.key_element = dest->data[0];
#else
  key_val.key_element.data_0 = dest->data[0];
  key_val.key_element.data_1 = dest->data[1];
#endif
  return key_val.uint64_key;
}

/*!
 * \brief Initial a binary key @ destination with a signed 64-bit
 * integer value.
 *
 * \note Need to flip the sign bit to get the correct ordering with a
 * mix of signed and unsigned key values.
 *
 * @param dest Handle of the destination SASIndexKey_t.
 * @param value signed long long value which will be the key.
 */
static inline void
SASIndexKeyInitInt64 (SASIndexKey_t * dest, signed long long value)
{
  sasindexkeymap_t	key_val;

  key_val.int64_key = value;
#ifdef __WORDSIZE_64
  dest->compare_size = sizeof (unsigned long long);
  dest->copy_size = sizeof (void *) + sizeof (unsigned long long);
  dest->data[0] = key_val.key_element ^ machine_sign_mask;
#else
  dest->compare_size = sizeof (unsigned long long);
  dest->copy_size = sizeof (void *) + sizeof (unsigned long long);
  dest->data[0] = key_val.key_element.data_0 ^ machine_sign_mask;
  dest->data[1] = key_val.key_element.data_1;
#endif
}

/*!
 * \brief Return the value of the 1st binary key as a signed 64-bit
 * integer value.
 *
 * @param dest Handle of the source SASIndexKey_t.
 * @return value of the 1st key element as a unsigned 64-bit integer.
 */
static inline long long
SASIndexKeyReturn1stInt64 (SASIndexKey_t * dest)
{
  sasindexkeymap_t	key_val;

#ifdef __WORDSIZE_64
  key_val.key_element = dest->data[0] ^ machine_sign_mask;
#else
  key_val.key_element.data_0 = dest->data[0] ^ machine_sign_mask;
  key_val.key_element.data_1 = dest->data[1];
#endif
  return key_val.int64_key;
}

/*!
 * \brief Initial a binary key @ destination with a signed 64-bit
 * integer value.
 *
 * \note Need to flip the sign bit to get the correct ordering with a
 * mix of signed and unsigned key values.
 *
 * @param dest Handle of the destination SASIndexKey_t.
 * @param value signed long long value which will be the key.
 */
static inline void
SASIndexKeyInitDouble (SASIndexKey_t * dest, double value)
{
  sasindexkeymap_t	key_val;

  key_val.double_key = value;
#ifdef __WORDSIZE_64
  dest->compare_size = sizeof (unsigned long long);
  dest->copy_size = sizeof (void *) + sizeof (unsigned long long);
  if (value < 0.0)
	  dest->data[0] = key_val.key_element ^ double_mask;
  else
	  dest->data[0] = key_val.key_element ^ machine_sign_mask;
#else
  dest->compare_size = sizeof (unsigned long long);
  dest->copy_size = sizeof (void *) + sizeof (unsigned long long);
  if (value < 0.0)
    {
	  dest->data[0] = key_val.key_element.data_0 ^ double_mask;
	  dest->data[1] = key_val.key_element.data_1 ^ double_mask;
    }
  else
    {
	  dest->data[0] = key_val.key_element.data_0 ^ machine_sign_mask;
	  dest->data[1] = key_val.key_element.data_1;
    }

#endif
}

/*!
 * \brief Return the value of the 1st binary key as a signed 64-bit
 * integer value.
 *
 * @param dest Handle of the source SASIndexKey_t.
 * @return value of the 1st key element as a unsigned 64-bit integer.
 */
static inline double
SASIndexKeyReturn1stDouble (SASIndexKey_t * dest)
{
  sasindexkeymap_t	key_val;

#ifdef __WORDSIZE_64
  if (dest->data[0] & machine_sign_mask)
	  key_val.key_element = dest->data[0] ^ machine_sign_mask;
  else
	  key_val.key_element = dest->data[0] ^ double_mask;
#else
  if (dest->data[0] & machine_sign_mask)
  {
	  key_val.key_element.data_0 = dest->data[0] ^ machine_sign_mask;
	  key_val.key_element.data_1 = dest->data[1];
  }
  else
  {
	  key_val.key_element.data_0 = dest->data[0] ^ double_mask;
	  key_val.key_element.data_1 = dest->data[1] ^ double_mask;
  }
#endif
  return key_val.double_key;
}

#endif /* __SAS_INDEXKEY_H */
