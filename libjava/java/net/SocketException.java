/* SocketException.java -- An exception occurred while performing a socket op
   Copyright (C) 1998, 1999, 2001 Free Software Foundation, Inc.

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
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA.

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

package java.net;

/* Written using on-line Java Platform 1.2 API Specification.
 * Believed complete and correct.
 */

/**
  * This exception indicates that a generic error occurred related to an
  * operation on a socket.  Check the descriptive message (if any) for
  * details on the nature of this error
  *
  * @author Aaron M. Renn (arenn@urbanophile.com)
  * @author Per Bothner 
  * @date January 6, 1999.
  */
public class SocketException extends java.io.IOException
{

/*
 * Constructors
 */

/**
  * Initializes a new instance of <code>SocketException</code> without
  * a descriptive error message.
  */
public
SocketException()
{
  super();
}

/*************************************************************************/

/**
  * Initializes a new instance of <code>SocketException</code> without
  * a descriptive error message.
  *
  * @param message A message describing the error that occurred.
  */
public
SocketException(String message)
{
  super(message);
}

} // class SocketException

