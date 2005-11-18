/* VMDouble.c - java.lang.VMDouble native functions
   Copyright (C) 1998, 1999, 2001, 2003, 2004i, 2005
   Free Software Foundation, Inc.

This file is part of GNU Classpath.

GNU Classpath is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.
 
GNU Classpath is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Classpath; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.

Linking this library statically or dynamically with other modules is
making a combined work based on this library.  Thus, the terms and
conditions of the GNU General Public License cover the whole
combination.

As a special exception, the copyright holders of this library give you
permission to link this library with independent modules to produce an
executable, regardless of the license terms of these independent
modules, and to copy and distribute the resulting executable under
terms of your choice, provided that you also meet, for each linked
independent module, the terms and conditions of the license of that
module.  An independent module is a module which is not derived from
or based on this library.  If you modify this library, you may extend
this exception to your version of the library, but you are not
obligated to do so.  If you do not wish to do so, delete this
exception statement from your version. */


#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mprec.h"
#include "fdlibm.h"
#include "jcl.h"

#include "java_lang_VMDouble.h"

static jclass clsDouble;
static jmethodID isNaNID;
static jdouble NEGATIVE_INFINITY;
static jdouble POSITIVE_INFINITY;
static jdouble NaN;

/*
 * Class:     java_lang_VMDouble
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_lang_VMDouble_initIDs (JNIEnv * env, jclass cls __attribute__ ((__unused__)))
{
  jfieldID negInfID;
  jfieldID posInfID;
  jfieldID nanID;

  clsDouble = (*env)->FindClass (env, "java/lang/Double");
  if (clsDouble == NULL)
    {
      DBG ("unable to get class java.lang.Double\n") return;
    }
  isNaNID = (*env)->GetStaticMethodID (env, clsDouble, "isNaN", "(D)Z");
  if (isNaNID == NULL)
    {
      DBG ("unable to determine method id of isNaN\n") return;
    }
  negInfID = (*env)->GetStaticFieldID (env, clsDouble, "NEGATIVE_INFINITY", "D");
  if (negInfID == NULL)
    {
      DBG ("unable to determine field id of NEGATIVE_INFINITY\n") return;
    }
  posInfID = (*env)->GetStaticFieldID (env, clsDouble, "POSITIVE_INFINITY", "D");
  if (posInfID == NULL)
    {
      DBG ("unable to determine field id of POSITIVE_INFINITY\n") return;
    }
  nanID = (*env)->GetStaticFieldID (env, clsDouble, "NaN", "D");
  if (posInfID == NULL)
    {
      DBG ("unable to determine field id of NaN\n") return;
    }
  POSITIVE_INFINITY = (*env)->GetStaticDoubleField (env, clsDouble, posInfID);
  NEGATIVE_INFINITY = (*env)->GetStaticDoubleField (env, clsDouble, negInfID);
  NaN = (*env)->GetStaticDoubleField (env, clsDouble, nanID);

#ifdef DEBUG
  fprintf (stderr, "java.lang.Double.initIDs() POSITIVE_INFINITY = %g\n",
	   POSITIVE_INFINITY);
  fprintf (stderr, "java.lang.Double.initIDs() NEGATIVE_INFINITY = %g\n",
	   NEGATIVE_INFINITY);
  fprintf (stderr, "java.lang.Double.initIDs() NaN = %g\n", NaN);
#endif
}

/*
 * Class:     java_lang_VMDouble
 * Method:    doubleToLongBits
 * Signature: (D)J
 */
JNIEXPORT jlong JNICALL
Java_java_lang_VMDouble_doubleToLongBits
  (JNIEnv * env __attribute__ ((__unused__)),
   jclass cls __attribute__ ((__unused__)), jdouble doubleValue)
{
  jvalue val;
  jlong e, f;
  val.d = doubleValue;

  e = val.j & 0x7ff0000000000000LL;
  f = val.j & 0x000fffffffffffffLL;

  if (e == 0x7ff0000000000000LL && f != 0L)
    val.j = 0x7ff8000000000000LL;

  return val.j;
}

/*
 * Class:     java_lang_VMDouble
 * Method:    doubleToRawLongBits
 * Signature: (D)J
 */
JNIEXPORT jlong JNICALL
Java_java_lang_VMDouble_doubleToRawLongBits
  (JNIEnv * env __attribute__ ((__unused__)),
   jclass cls __attribute__ ((__unused__)), jdouble doubleValue)
{
  jvalue val;
  val.d = doubleValue;
  return val.j;
}

/*
 * Class:     java_lang_VMDouble
 * Method:    longBitsToDouble
 * Signature: (J)D
 */
JNIEXPORT jdouble JNICALL
Java_java_lang_VMDouble_longBitsToDouble
  (JNIEnv * env __attribute__ ((__unused__)),
   jclass cls __attribute__ ((__unused__)), jlong longValue)
{
  jvalue val;
  val.j = longValue;
  return val.d;
}

/*
 * Class:     java_lang_VMDouble
 * Method:    toString
 * Signature: (DZ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_java_lang_VMDouble_toString
  (JNIEnv * env, jclass cls __attribute__ ((__unused__)), jdouble value, jboolean isFloat)
{
  char buffer[50], result[50];
  int decpt, sign;
  char *s, *d;
  int i;

#ifdef DEBUG
  fprintf (stderr, "java.lang.VMDouble.toString (%g)\n", value);
#endif

  if ((*env)->CallStaticBooleanMethod (env, clsDouble, isNaNID, value))
    return (*env)->NewStringUTF (env, "NaN");

  if (value == POSITIVE_INFINITY)
    return (*env)->NewStringUTF (env, "Infinity");

  if (value == NEGATIVE_INFINITY)
    return (*env)->NewStringUTF (env, "-Infinity");

  _dtoa (value, 0, 20, &decpt, &sign, NULL, buffer, (int) isFloat);

  value = fabs (value);

  s = buffer;
  d = result;

  if (sign)
    *d++ = '-';

  if ((value >= 1e-3 && value < 1e7) || (value == 0))
    {
      if (decpt <= 0)
	*d++ = '0';
      else
	{
	  for (i = 0; i < decpt; i++)
	    if (*s)
	      *d++ = *s++;
	    else
	      *d++ = '0';
	}

      *d++ = '.';

      if (*s == 0)
	{
	  *d++ = '0';
	  decpt++;
	}

      while (decpt++ < 0)
	*d++ = '0';

      while (*s)
	*d++ = *s++;

      *d = 0;

      return (*env)->NewStringUTF (env, result);
    }

  *d++ = *s++;
  decpt--;
  *d++ = '.';

  if (*s == 0)
    *d++ = '0';

  while (*s)
    *d++ = *s++;

  *d++ = 'E';

  if (decpt < 0)
    {
      *d++ = '-';
      decpt = -decpt;
    }

  {
    char exp[4];
    char *e = exp + sizeof exp;

    *--e = 0;
    do
      {
	*--e = '0' + decpt % 10;
	decpt /= 10;
      }
    while (decpt > 0);

    while (*e)
      *d++ = *e++;
  }

  *d = 0;

  return (*env)->NewStringUTF (env, result);
}

/*
 * Class:     java_lang_VMDouble
 * Method:    parseDouble
 * Signature: (Ljava/lang/String;)D
 */
JNIEXPORT jdouble JNICALL
Java_java_lang_VMDouble_parseDouble
  (JNIEnv * env, jclass cls __attribute__ ((__unused__)), jstring str)
{
  jboolean isCopy;
  const char *buf;
  char *endptr;
  jdouble val = 0.0;

  if (str == NULL)
    {
      JCL_ThrowException (env, "java/lang/NullPointerException", "null");
      return val;
    }

  buf = (char *) (*env)->GetStringUTFChars (env, str, &isCopy);
  if (buf == NULL)
    {
      /* OutOfMemoryError already thrown */
    }
  else
    {
      const char *p = buf, *end, *last_non_ws, *temp;
      int ok = 1;

#ifdef DEBUG
      fprintf (stderr, "java.lang.VMDouble.parseDouble (%s)\n", buf);
#endif

      /* Trim the buffer, similar to String.trim().  First the leading
         characters.  */
      while (*p && *p <= ' ')
	++p;

      /* Find the last non-whitespace character.  This method is safe
         even with multi-byte UTF-8 characters.  */
      end = p;
      last_non_ws = NULL;
      while (*end)
	{
	  if (*end > ' ')
	    last_non_ws = end;
	  ++end;
	}

      if (last_non_ws == NULL)
	last_non_ws = p + strlen (p);
      else
	{
	  /* Skip past the last non-whitespace character.  */
	  ++last_non_ws;
	}

      /* Check for infinity and NaN */
      temp = p;
      if (temp[0] == '+' || temp[0] == '-')
	temp++;
      if (strncmp ("Infinity", temp, (size_t) 8) == 0)
	{
	  if (p[0] == '-')
	    return NEGATIVE_INFINITY;
	  return POSITIVE_INFINITY;
	}
      if (strncmp ("NaN", temp, (size_t) 3) == 0)
	return NaN;

      /* Skip a trailing `f' or `d'.  */
      if (last_non_ws > p
	  && (last_non_ws[-1] == 'f'
	      || last_non_ws[-1] == 'F'
	      || last_non_ws[-1] == 'd' || last_non_ws[-1] == 'D'))
	--last_non_ws;

      if (last_non_ws > p)
	{
	  struct _Jv_reent reent;
	  memset (&reent, 0, sizeof reent);

	  val = _strtod_r (&reent, p, &endptr);

#ifdef DEBUG
	  fprintf (stderr, "java.lang.VMDouble.parseDouble val = %g\n", val);
	  fprintf (stderr, "java.lang.VMDouble.parseDouble %i != %i ???\n",
		   endptr, last_non_ws);
#endif
	  if (endptr != last_non_ws)
	    ok = 0;
	}
      else
	ok = 0;

      if (!ok)
	{
	  val = 0.0;
	  JCL_ThrowException (env,
			      "java/lang/NumberFormatException",
			      "unable to parse double");
	}

      (*env)->ReleaseStringUTFChars (env, str, buf);
    }

  return val;
}
