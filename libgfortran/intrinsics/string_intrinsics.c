/* String intrinsics helper functions.
   Copyright 2002 Free Software Foundation, Inc.
   Contributed by Paul Brook <paul@nowt.org>

This file is part of the GNU Fortran 95 runtime library (libgfor).

GNU G95 is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

GNU G95 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with libgfor; see the file COPYING.LIB.  If not,
write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


/* Unlike what the name of this file suggests, we don't actually
   implement the Fortran intrinsics here.  At least, not with the
   names they have in the standard.  The functions here provide all
   the support we need for the standard string intrinsics, and the
   compiler translates the actual intrinsics calls to calls to
   functions in this file.  */

#include <stdlib.h>
#include <string.h>

#include "libgfortran.h"


/* String functions.  */

#define copy_string prefix(copy_string)
void copy_string (G95_INTEGER_4, char *, G95_INTEGER_4, const char *);

#define concat_string prefix(concat_string)
void concat_string (G95_INTEGER_4, char *,
		    G95_INTEGER_4, const char *,
		    G95_INTEGER_4, const char *);

#define string_len_trim prefix(string_len_trim)
G95_INTEGER_4 string_len_trim (G95_INTEGER_4, const char *);

#define adjustl prefix(adjustl)
void adjustl (char *, G95_INTEGER_4, const char *);

#define adjustr prefix(adjustr)
void adjustr (char *, G95_INTEGER_4, const char *);

#define string_index prefix(string_index)
G95_INTEGER_4 string_index (G95_INTEGER_4, const char *, G95_INTEGER_4,
			    const char *, G95_LOGICAL_4);

#define string_scan prefix(string_scan)
G95_INTEGER_4 string_scan (G95_INTEGER_4, const char *, G95_INTEGER_4,
                           const char *, G95_LOGICAL_4);

#define string_verify prefix(string_verify)
G95_INTEGER_4 string_verify (G95_INTEGER_4, const char *, G95_INTEGER_4,
                             const char *, G95_LOGICAL_4);


/* The two areas may overlap so we use memmove.  */

void
copy_string (G95_INTEGER_4 destlen, char * dest,
	     G95_INTEGER_4 srclen, const char * src)
{
  if (srclen >= destlen)
    {
      /* This will truncate if too long.  */
      memmove (dest, src, destlen);
      /*memcpy (dest, src, destlen);*/
    }
  else
    {
      memmove (dest, src, srclen);
      /*memcpy (dest, src, srclen);*/
      /* Pad with spaces.  */
      memset (&dest[srclen], ' ', destlen - srclen);
    }
}


/* Strings of unequal length are extended with pad characters.  */

G95_INTEGER_4
compare_string (G95_INTEGER_4 len1, const char * s1,
		G95_INTEGER_4 len2, const char * s2)
{
  int res;
  const char *s;
  int len;

  res = strncmp (s1, s2, (len1 < len2) ? len1 : len2);
  if (res != 0)
    return res;

  if (len1 == len2)
    return 0;

  if (len1 < len2)
    {
      len = len2 - len1;
      s = &s2[len1];
      res = -1;
    }
  else
    {
      len = len1 - len2;
      s = &s1[len2];
      res = 1;
    }

  while (len--)
    {
      if (*s != ' ')
        {
          if (*s > ' ')
            return res;
          else
            return -res;
        }
      s++;
    }

  return 0;
}


/* The destination and source should not overlap.  */

void
concat_string (G95_INTEGER_4 destlen, char * dest,
	       G95_INTEGER_4 len1, const char * s1,
	       G95_INTEGER_4 len2, const char * s2)
{
  if (len1 >= destlen)
    {
      memcpy (dest, s1, destlen);
      return;
    }
  memcpy (dest, s1, len1);
  dest += len1;
  destlen -= len1;

  if (len2 >= destlen)
    {
      memcpy (dest, s2, destlen);
      return;
    }

  memcpy (dest, s2, len2);
  memset (&dest[len2], ' ', destlen - len2);
}


/* The length of a string not including trailing blanks.  */

G95_INTEGER_4
string_len_trim (G95_INTEGER_4 len, const char * s)
{
  int i;

  for (i = len - 1; i >= 0; i--)
    {
      if (s[i] != ' ')
        break;
    }
  return i + 1;
}


/* Find a substring within a string.  */

G95_INTEGER_4
string_index (G95_INTEGER_4 slen, const char * str, G95_INTEGER_4 sslen,
	      const char * sstr, G95_LOGICAL_4 back)
{
  int start;
  int last;
  int i;
  int delta;

  if (sslen == 0)
    return 1;

  if (back)
    {
      last = slen + 1 - sslen;
      start = 0;
      delta = 1;
    }
  else
    {
      last = -1;
      start = slen - sslen;
      delta = -1;
    }
  i = 0;
  for (; start != last; start+= delta)
    {
      for (i = 0; i < sslen; i++)
        {
          if (str[start + i] != sstr[i])
            break;
        }
      if (i == sslen)
        return (start + 1);
    }
  return 0;
}


/* Remove leading blanks from a string, padding at end.  The src and dest
   should not overlap.  */

void
adjustl (char *dest, G95_INTEGER_4 len, const char *src)
{
  int i;

  i = 0;
  while (i<len && src[i] == ' ')
    i++;

  if (i < len)
    memcpy (dest, &src[i], len - i);
  if (i > 0)
    memset (&dest[len - i], ' ', i);
}


/* Remove trailing blanks from a string.  */

void
adjustr (char *dest, G95_INTEGER_4 len, const char *src)
{
  int i;

  i = len;
  while (i > 0 && src[i - 1] == ' ')
    i++;

  if (i < len)
    memcpy (&dest[len - i], &src, i);
  if (i < len)
    memset (dest, ' ', len - i);
}


/* Scan a string for any one of the characters in a set of characters.  */

G95_INTEGER_4
string_scan (G95_INTEGER_4 slen, const char * str, G95_INTEGER_4 setlen,
             const char * set, G95_LOGICAL_4 back)
{
  int start;
  int last;
  int i;
  int delta;

  if (slen == 0 || setlen == 0)
    return 0;

  if (back)
    {
      last =  0;
      start = slen - 1;
      delta = -1;
    }
  else
    {
      last = slen - 1;
      start = 0;
      delta = 1;
    }

  i = 0;
  for (; start != last; start += delta)
    {
      for (i = 0; i < setlen; i++)
        {
          if (str[start] == set[i])
            return (start + 1);
        }
    }

  return 0;
}


/* Verify that a set of characters contains all the characters in a
   string by indentifying the position of the first character in a
   characters that dose not appear in a given set of characters.  */

G95_INTEGER_4
string_verify (G95_INTEGER_4 slen, const char * str, G95_INTEGER_4 setlen,
               const char * set, G95_LOGICAL_4 back)
{
  int start;
  int last;
  int i;
  int delta;

  if (slen == 0)
    return 0;

  if (back)
    {
      last =  0;
      start = slen - 1;
      delta = -1;
    }
  else
    {
      last = slen - 1;
      start = 0;
      delta = 1;
    }
  i = 0;
  for (; start != last; start += delta)
    {
      for (i = 0; i < setlen; i++)
        {
          if (str[start] == set[i])
            break;
        }
      if (i == setlen)
        return (start + 1);
    }

  return 0;
}
