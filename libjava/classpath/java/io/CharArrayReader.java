/* CharArrayReader.java -- Read an array of characters as a stream
   Copyright (C) 1998, 2001, 2005  Free Software Foundation, Inc.

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


package java.io;

/**
  * This class permits an array of chars to be read as an input stream.
  *
  * @author Aaron M. Renn (arenn@urbanophile.com)
  * @author Warren Levy (warrenl@cygnus.com)
  */
public class CharArrayReader extends Reader
{
  /**
   * The array that contains the data supplied during read operations
   */
  protected char[] buf;

  /**
   * The array index of the next char to be read from the buffer
   * <code>buf</code>
   */
  protected int pos;

  /**
   * The currently marked position in the stream.  This defaults to 0, so a
   * reset operation on the stream resets it to read from array index 0 in
   * the buffer - even if the stream was initially created with an offset
   * greater than 0
   */
  protected int markedPos;

  /**
   * This indicates the maximum number of chars that can be read from this
   * stream.  It is the array index of the position after the last valid
   * char in the buffer <code>buf</code>
   */
  protected int count;

  /**
   * Create a new CharArrayReader that will read chars from the passed
   * in char array.  This stream will read from the beginning to the end
   * of the array.  It is identical to calling an overloaded constructor
   * as <code>CharArrayReader(buf, 0, buf.length)</code>.
   * <p>
   * Note that this array is not copied.  If its contents are changed
   * while this stream is being read, those changes will be reflected in the
   * chars supplied to the reader.  Please use caution in changing the
   * contents of the buffer while this stream is open.
   *
   * @param buffer The char array buffer this stream will read from.
  */
  public CharArrayReader(char[] buffer)
  {
    this(buffer, 0, buffer.length);
  }

  /**
   * Create a new CharArrayReader that will read chars from the passed
   * in char array.  This stream will read from position
   * <code>offset</code> in the array for a length of
   * <code>length</code> chars past <code>offset</code>.  If the
   * stream is reset to a position before <code>offset</code> then
   * more than <code>length</code> chars can be read from the stream.
   * The <code>length</code> value should be viewed as the array index
   * one greater than the last position in the buffer to read.
   * <p>
   * Note that this array is not copied.  If its contents are changed
   * while this stream is being read, those changes will be reflected in the
   * chars supplied to the reader.  Please use caution in changing the
   * contents of the buffer while this stream is open.
   *
   * @param buffer The char array buffer this stream will read from.
   * @param offset The index into the buffer to start reading chars from
   * @param length The number of chars to read from the buffer
   */
  public CharArrayReader(char[] buffer, int offset, int length)
  {
    super();
    if (offset < 0  || length < 0 || offset > buffer.length)
      throw new IllegalArgumentException();

    buf = buffer;

    count = offset + length;
    if (count > buf.length)
      count = buf.length;

    pos = offset;
    markedPos = pos;
  }

  /**
   * This method closes the stream.
   */
  public void close()
  {
    synchronized (lock)
      {
        buf = null;
      }
  }

  /**
   * This method sets the mark position in this stream to the current
   * position.  Note that the <code>readlimit</code> parameter in this
   * method does nothing as this stream is always capable of
   * remembering all the chars int it.
   * <p>
   * Note that in this class the mark position is set by default to
   * position 0 in the stream.  This is in constrast to some other
   * stream types where there is no default mark position.
   *
   * @param readAheadLimit The number of chars this stream must
   *                       remember.  This parameter is ignored.
   *
   * @exception IOException If an error occurs
   */
  public void mark(int readAheadLimit) throws IOException
  {
    synchronized (lock)
      {
        if (buf == null)
          throw new IOException("Stream closed");
        // readAheadLimit is ignored per Java Class Lib. book, p. 318.
        markedPos = pos;
      }
  }

  /**
   * This method overrides the <code>markSupported</code> method in
   * <code>Reader</code> in order to return <code>true</code> -
   * indicating that this stream class supports mark/reset
   * functionality.
   *
   * @return <code>true</code> to indicate that this class supports
   * mark/reset.
   */
  public boolean markSupported()
  {
    return true;
  }

  /**
   * This method reads one char from the stream.  The <code>pos</code>
   * counter is advanced to the next char to be read.  The char read
   * is returned as an int in the range of 0-65535.  If the stream
   * position is already at the end of the buffer, no char is read and
   * a -1 is returned in order to indicate the end of the stream.
   *
   * @return The char read, or -1 if end of stream
   */
  public int read() throws IOException
  {
    synchronized (lock)
      {
        if (buf == null)
          throw new IOException("Stream closed");

        if (pos < 0)
          throw new ArrayIndexOutOfBoundsException(pos);

        if (pos < count)
          return ((int) buf[pos++]) & 0xFFFF;
        return -1;
      }
  }

  /**
   * This method reads chars from the stream and stores them into a
   * caller supplied buffer.  It starts storing the data at index
   * <code>offset</code> into the buffer and attempts to read
   * <code>len</code> chars.  This method can return before reading
   * the number of chars requested if the end of the stream is
   * encountered first.  The actual number of chars read is returned.
   * If no chars can be read because the stream is already at the end
   * of stream position, a -1 is returned.
   * <p>
   * This method does not block.
   *
   * @param b The array into which the chars read should be stored.
   * @param off The offset into the array to start storing chars
   * @param len The requested number of chars to read
   *
   * @return The actual number of chars read, or -1 if end of stream.
   */
  public int read(char[] b, int off, int len) throws IOException
  {
    synchronized (lock)
      {
        if (buf == null)
          throw new IOException("Stream closed");

        /* Don't need to check pos value, arraycopy will check it. */
        if (off < 0 || len < 0 || off + len > b.length)
          throw new IndexOutOfBoundsException();

        if (pos >= count)
          return -1;

        int numChars = Math.min(count - pos, len);
        System.arraycopy(buf, pos, b, off, numChars);
        pos += numChars;
        return numChars;
      }
  }

  /**
   * Return true if more characters are available to be read.
   *
   * @return <code>true</code> to indicate that this stream is ready
   * to be read.
   *
   * @specnote The JDK 1.3 API docs are wrong here. This method will
   *           return false if there are no more characters available.
   */
  public boolean ready() throws IOException
  {
    if (buf == null)
      throw new IOException("Stream closed");

    return (pos < count);
  }

  /**
   * This method sets the read position in the stream to the mark
   * point by setting the <code>pos</code> variable equal to the
   * <code>mark</code> variable.  Since a mark can be set anywhere in
   * the array, the mark/reset methods int this class can be used to
   * provide random search capabilities for this type of stream.
   */
  public void reset() throws IOException
  {
    synchronized (lock)
      {
        if (buf == null)
          throw new IOException("Stream closed");

        pos = markedPos;
      }
  }

  /**
   * This method attempts to skip the requested number of chars in the
   * input stream.  It does this by advancing the <code>pos</code> value by the
   * specified number of chars.  It this would exceed the length of the
   * buffer, then only enough chars are skipped to position the stream at
   * the end of the buffer.  The actual number of chars skipped is returned.
   *
   * @param n The requested number of chars to skip
   *
   * @return The actual number of chars skipped.
   */
  public long skip(long n) throws IOException
  {
    synchronized (lock)
      {
        if (buf == null)
          throw new IOException("Stream closed");

        // Even though the var numChars is a long, in reality it can never
        // be larger than an int since the result of subtracting 2 positive
        // ints will always fit in an int.  Since we have to return a long
        // anyway, numChars might as well just be a long.
        long numChars = Math.min((long) (count - pos), n < 0 ? 0L : n);
        pos += numChars;
        return numChars;
      }
  }
}
