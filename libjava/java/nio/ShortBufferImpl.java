/* ShortBufferImpl.java -- 
   Copyright (C) 2002, 2003 Free Software Foundation, Inc.

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


package java.nio;

/**
 * This is a Heap memory implementation
 */
final class ShortBufferImpl extends ShortBuffer
{
  private boolean readOnly;

  ShortBufferImpl (int capacity)
  {
    this (new short [capacity], 0, capacity, capacity, 0, -1, false);
  }
  
  ShortBufferImpl (short[] buffer, int offset, int capacity, int limit, int position, int mark, boolean readOnly)
  {
    super (capacity, limit, position, mark);
    this.backing_buffer = buffer;
    this.array_offset = offset;
    this.readOnly = readOnly;
  }
  
  public boolean isReadOnly ()
  {
    return readOnly;
  }
  
  public ShortBuffer slice ()
  {
    return new ShortBufferImpl (backing_buffer, array_offset + position (), remaining (), remaining (), 0, -1, isReadOnly ());
  }
  
  public ShortBuffer duplicate ()
  {
    return new ShortBufferImpl (backing_buffer, array_offset, capacity (), limit (), position (), mark, isReadOnly ());
  }
  
  public ShortBuffer asReadOnlyBuffer ()
  {
    return new ShortBufferImpl (backing_buffer, array_offset, capacity (), limit (), position (), mark, true);
  }
  
  public ShortBuffer compact ()
  {
    int copied = 0;
    
    while (remaining () > 0)
      {
	put (copied, get ());
	copied++;
      }

    position (copied);
    return this;
  }
  
  public boolean isDirect ()
  {
    return false;
  }

  /**
   * Relative get method. Reads the next <code>short</code> from the buffer.
   */
  public short get ()
  {
    short result = backing_buffer [position ()];
    position (position () + 1);
    return result;
  }
  
  /**
   * Relative put method. Writes <code>value</code> to the next position
   * in the buffer.
   * 
   * @exception ReadOnlyBufferException If this buffer is read-only.
   */
  public ShortBuffer put (short value)
  {
    if (readOnly)
      throw new ReadOnlyBufferException ();
	  	    
    backing_buffer [position ()] = value;
    position (position () + 1);
    return this;
  }
  
  /**
   * Absolute get method. Reads the <code>short</code> at position
   * <code>index</code>.
   *
   * @exception IndexOutOfBoundsException If index is negative or not smaller
   * than the buffer's limit.
   */
  public short get (int index)
  {
    return backing_buffer [index];
  }
  
  /**
   * Absolute put method. Writes <code>value</value> to position
   * <code>index</code> in the buffer.
   *
   * @exception IndexOutOfBoundsException If index is negative or not smaller
   * than the buffer's limit.
   * @exception ReadOnlyBufferException If this buffer is read-only.
   */
  public ShortBuffer put (int index, short value)
  {
    if (readOnly)
      throw new ReadOnlyBufferException ();
    	    
    backing_buffer [index] = value;
    return this;
  }
  
  public ByteOrder order ()
  {
    return ByteOrder.nativeOrder ();
  }
}
