/****************************************************************************
 *                                                                          *
 *                         GNAT COMPILER COMPONENTS                         *
 *                                                                          *
 *                            T A R G T Y P S                               *
 *                                                                          *
 *                                  Body                                    *
 *                                                                          *
 *                             $Revision: 1.1 $
 *                                                                          *
 *          Copyright (C) 1992-2001 Free Software Foundation, Inc.          *
 *                                                                          *
 * GNAT is free software;  you can  redistribute it  and/or modify it under *
 * terms of the  GNU General Public License as published  by the Free Soft- *
 * ware  Foundation;  either version 2,  or (at your option) any later ver- *
 * sion.  GNAT is distributed in the hope that it will be useful, but WITH- *
 * OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License *
 * for  more details.  You should have  received  a copy of the GNU General *
 * Public License  distributed with GNAT;  see file COPYING.  If not, write *
 * to  the Free Software Foundation,  59 Temple Place - Suite 330,  Boston, *
 * MA 02111-1307, USA.                                                      *
 *                                                                          *
 * As a  special  exception,  if you  link  this file  with other  files to *
 * produce an executable,  this file does not by itself cause the resulting *
 * executable to be covered by the GNU General Public License. This except- *
 * ion does not  however invalidate  any other reasons  why the  executable *
 * file might be covered by the  GNU Public License.                        *
 *                                                                          *
 * GNAT was originally developed  by the GNAT team at  New York University. *
 * It is now maintained by Ada Core Technologies Inc (http://www.gnat.com). *
 *                                                                          *
 ****************************************************************************/

/* Functions for retrieving target types. See Ada package Get_Targ */

#include "config.h"
#include "system.h"
#include "tree.h"
#include "real.h"
#include "rtl.h"
#include "ada.h"
#include "types.h"
#include "atree.h"
#include "elists.h"
#include "namet.h"
#include "nlists.h"
#include "snames.h"
#include "stringt.h"
#include "uintp.h"
#include "urealp.h"
#include "fe.h"
#include "sinfo.h"
#include "einfo.h"
#include "ada-tree.h"
#include "gigi.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

/* Standard data type sizes.  Most of these are not used.  */

#ifndef CHAR_TYPE_SIZE
#define CHAR_TYPE_SIZE BITS_PER_UNIT
#endif

#ifndef SHORT_TYPE_SIZE
#define SHORT_TYPE_SIZE (BITS_PER_UNIT * MIN ((UNITS_PER_WORD + 1) / 2, 2))
#endif

#ifndef INT_TYPE_SIZE
#define INT_TYPE_SIZE BITS_PER_WORD
#endif

#ifdef OPEN_VMS /* A target macro defined in vms.h */
#define LONG_TYPE_SIZE 64
#else
#ifndef LONG_TYPE_SIZE
#define LONG_TYPE_SIZE BITS_PER_WORD
#endif
#endif

#ifndef LONG_LONG_TYPE_SIZE
#define LONG_LONG_TYPE_SIZE (BITS_PER_WORD * 2)
#endif

#ifndef FLOAT_TYPE_SIZE
#define FLOAT_TYPE_SIZE BITS_PER_WORD
#endif

#ifndef DOUBLE_TYPE_SIZE
#define DOUBLE_TYPE_SIZE (BITS_PER_WORD * 2)
#endif

#ifndef LONG_DOUBLE_TYPE_SIZE
#define LONG_DOUBLE_TYPE_SIZE (BITS_PER_WORD * 2)
#endif

#ifndef WIDEST_HARDWARE_FP_SIZE
#define WIDEST_HARDWARE_FP_SIZE LONG_DOUBLE_TYPE_SIZE
#endif

/* The following provide a functional interface for the front end Ada code
   to determine the sizes that are used for various C types. */

Pos
get_target_bits_per_unit ()
{
  return BITS_PER_UNIT;
}

Pos
get_target_bits_per_word ()
{
  return BITS_PER_WORD;
}

Pos
get_target_char_size ()
{
  return CHAR_TYPE_SIZE;
}

Pos
get_target_wchar_t_size ()
{
  /* We never want wide chacters less than "short" in Ada.  */
  return MAX (SHORT_TYPE_SIZE, WCHAR_TYPE_SIZE);
}

Pos
get_target_short_size ()
{
  return SHORT_TYPE_SIZE;
}

Pos
get_target_int_size ()
{
  return INT_TYPE_SIZE;
}

Pos
get_target_long_size ()
{
  return LONG_TYPE_SIZE;
}

Pos
get_target_long_long_size ()
{
  return LONG_LONG_TYPE_SIZE;
}

Pos
get_target_float_size ()
{
  return FLOAT_TYPE_SIZE;
}

Pos
get_target_double_size ()
{
  return DOUBLE_TYPE_SIZE;
}

Pos
get_target_long_double_size ()
{
  return WIDEST_HARDWARE_FP_SIZE;
}

Pos
get_target_pointer_size ()
{
  return POINTER_SIZE;
}

Pos
get_target_maximum_alignment ()
{
  return BIGGEST_ALIGNMENT / BITS_PER_UNIT;
}

Boolean
get_target_no_dollar_in_label ()
{
#ifdef NO_DOLLAR_IN_LABEL
  return 1;
#else
  return 0;
#endif
}

#ifndef FLOAT_WORDS_BIG_ENDIAN
#define FLOAT_WORDS_BIG_ENDIAN WORDS_BIG_ENDIAN
#endif

Nat
get_float_words_be ()
{
  return FLOAT_WORDS_BIG_ENDIAN;
}

Nat
get_words_be ()
{
  return WORDS_BIG_ENDIAN;
}

Nat
get_bytes_be ()
{
  return BYTES_BIG_ENDIAN;
}

Nat
get_bits_be ()
{
  return BITS_BIG_ENDIAN;
}

Nat
get_strict_alignment ()
{
  return STRICT_ALIGNMENT;
}
