/*
  FileURLConnection.java -- URLConnection class for "file" protocol
  Copyright (C) 1998, 1999, 2003 Free Software Foundation, Inc.

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
  exception statement from your version.
*/
package gnu.java.net.protocol.file;

import java.net.URL;
import java.net.URLConnection;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.ProtocolException;
import java.util.Map;
import java.util.Vector;
import java.util.Hashtable;
import java.util.Enumeration;

/**
 * @author Warren Levy <warrenl@cygnus.com>
 * @date April 13, 1999.
 */
public class Connection extends URLConnection
{
  private Hashtable hdrHash = new Hashtable();
  private Vector hdrVec = new Vector();
  private boolean gotHeaders = false;
  private File fileIn;
  private InputStream inputStream;
  private OutputStream outputStream;

  public Connection(URL url)
  {
    super (url);
  }
  
  /**
   * "Connects" to the file by opening it.
   */
  public void connect() throws IOException
  {
    // Call is ignored if already connected.
    if (connected)
      return;
    
    // If not connected, then file needs to be openned.
    String fname = url.getFile();
    fileIn = new File(fname);
    if (doInput)
      inputStream = new BufferedInputStream(new FileInputStream(fileIn));
    if (doOutput)
      outputStream = new BufferedOutputStream(new FileOutputStream(fileIn));
    
    connected = true;
  }
  
  /**
   * Opens the file for reading and returns a stream for it.
   *
   * @return An InputStream for this connection.
   *
   * @exception IOException If an error occurs
   */
  public InputStream getInputStream()
    throws IOException
  {
    if (! doInput)
      throw new ProtocolException("Can't open InputStream if doInput is false");
    if (!connected)
      connect();
    
    return inputStream;
  }

  /**
   * Opens the file for writing and returns a stream for it.
   *
   * @return An OutputStream for this connection.
   *
   * @exception IOException If an error occurs.
   */
  public OutputStream getOutputStream()
    throws IOException
  {
    if (! doOutput)
      throw new
	ProtocolException("Can't open OutputStream if doOutput is false");

    if (!connected)
      connect();
    
    return outputStream;
  }

  // Override default method in URLConnection.
  public String getHeaderField(String name)
  {
    try
      {
	getHeaders();
      }
    catch (IOException e)
      {
	return null;
      }
    return (String) hdrHash.get(name.toLowerCase());
  }

  // Override default method in URLConnection.
  public Map getHeaderFields()
  {
    try
      {
	getHeaders();
      }
    catch (IOException e)
      {
	return null;
      }
    return hdrHash;
  }

  // Override default method in URLConnection.
  public String getHeaderField(int n)
  {
    try
      {
	getHeaders();
      }
    catch (IOException x)
      {
	return null;
      }
    if (n < hdrVec.size())
      return getField ((String) hdrVec.elementAt(n));

    return null;
  }

  // Override default method in URLConnection.
  public String getHeaderFieldKey(int n)
  {
    try
      {
	getHeaders();
      }
    catch (IOException x)
      {
	return null;
      }
    if (n < hdrVec.size())
      return getKey ((String) hdrVec.elementAt(n));

    return null;
  }

  private String getKey(String str)
  {
    if (str == null)
      return null;
    int index = str.indexOf(':');
    if (index >= 0)
      return str.substring(0, index);
    else
      return null;
  }

  private String getField(String str)
  {
    if (str == null)
      return null;
    int index = str.indexOf(':');
    if (index >= 0)
      return str.substring(index + 1).trim();
    else
      return str;
  }

  private void getHeaders() throws IOException
  {
    if (gotHeaders)
      return;
    gotHeaders = true;

    connect();

    // Yes, it is overkill to use the hash table and vector here since
    // we're only putting one header in the file, but in case we need
    // to add others later and for consistency, we'll implement it this way.

    // Add the only header we know about right now:  Content-length.
    long len = fileIn.length();
    String line = "Content-length: " + len;
    hdrVec.addElement(line);

    // The key will never be null in this scenario since we build up the
    // headers ourselves.  If we ever rely on getting a header from somewhere
    // else, then we may have to check if the result of getKey() is null.
    String key = getKey(line);
    hdrHash.put(key.toLowerCase(), Long.toString(len));
  }
  
} // class FileURLConnection
