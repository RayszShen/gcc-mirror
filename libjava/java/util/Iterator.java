/* Iterator.java -- Interface for iterating over collections
   Copyright (C) 1998, 2001 Free Software Foundation, Inc.

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


package java.util;

/**
 * An object which iterates over a collection. An Iterator is used to return
 * the items once only, in sequence, by successive calls to the next method.
 * It is also possible to remove elements from the underlying collection by
 * using the optional remove method. Iterator is intended as a replacement
 * for the Enumeration interface of previous versions of Java, which did not
 * have the remove method and had less conveniently named methods.
 *
 * @author Original author unknown
 * @author Eric Blake <ebb9@email.byu.edu>
 * @see Collection
 * @see ListIterator
 * @see Enumeration
 * @since 1.2
 * @status updated to 1.4
 */
public interface Iterator
{
  /**
   * Tests whether there are elements remaining in the collection. In other
   * words, calling <code>next()</code> will not throw an exception.
   *
   * @return true if there is at least one more element in the collection
   */
  boolean hasNext();

  /**
   * Obtain the next element in the collection.
   *
   * @return the next element in the collection
   * @throws NoSuchElementException if there are no more elements
   */
  Object next();

  /**
   * Remove from the underlying collection the last element returned by next
   * (optional operation). This method can be called only once after each
   * call to <code>next()</code>. It does not affect what will be returned
   * by subsequent calls to next.
   *
   * @throws IllegalStateException if next has not yet been called or remove
   *         has already been called since the last call to next.
   * @throws UnsupportedOperationException if this Iterator does not support
   *         the remove operation.
   */
  void remove();
}
