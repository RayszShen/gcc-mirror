/* UTFDataFormatException.java -- Bad format in UTF data
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


package java.io;

/* Written using "Java Class Libraries", 2nd edition, ISBN 0-201-31002-3
 * "The Java Language Specification", ISBN 0-201-63451-1
 * Status:  Complete to 1.1.
 */

/**
  * When reading a UTF string from an input stream, this exception is thrown
  * to indicate that the data read is invalid.
  *
  * @version 0.0
  *
  * @author Aaron M. Renn (arenn@urbanophile.com)
  * @author Tom Tromey <tromey@cygnus.com>
  * @date September 24, 1998
  */
public class UTFDataFormatException extends IOException
{

/*
 * Constructors
 */

/**
  * Create a new UTFDataFormatException without a descriptive error message
  */
public
UTFDataFormatException()
{
  super();
}

/*************************************************************************/

/**
  * Create a new UTFDataFormatException with a descriptive error message String
  *
  * @param message The descriptive error message
  */
public
UTFDataFormatException(String message)
{
  super(message);
}

} // class UTFDataFormatException

