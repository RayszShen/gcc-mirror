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

As a special exception, if you link this library with other files to
produce an executable, this library does not by itself cause the
resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why the
executable file might be covered by the GNU General Public License. */

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

