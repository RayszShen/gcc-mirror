/* TMMH16Spi.java -- 
   Copyright (C) 2002, 2006  Free Software Foundation, Inc.

This file is a part of GNU Classpath.

GNU Classpath is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

GNU Classpath is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Classpath; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

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
exception statement from your version.  */


package gnu.javax.crypto.jce.mac;

import gnu.java.security.Registry;
import gnu.javax.crypto.mac.TMMH16;
import gnu.javax.crypto.jce.spec.TMMHParameterSpec;

import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.spec.AlgorithmParameterSpec;

/**
 * The implementation of the TMMH16 <i>Service Provider Interface</i>
 * (<b>SPI</b>) adapter.
 */
public final class TMMH16Spi extends MacAdapter
{

  // Constructors.
  // -----------------------------------------------------------------------

  public TMMH16Spi()
  {
    super(Registry.TMMH16);
  }

  // Instance methods overriding MacAdapter.
  // -----------------------------------------------------------------------

  protected void engineInit(Key key, AlgorithmParameterSpec params)
      throws InvalidKeyException, InvalidAlgorithmParameterException
  {
    if (!(params instanceof TMMHParameterSpec))
      {
        throw new InvalidAlgorithmParameterException();
      }
    TMMHParameterSpec spec = (TMMHParameterSpec) params;
    attributes.put(TMMH16.TAG_LENGTH, spec.getTagLength());
    attributes.put(TMMH16.KEYSTREAM, spec.getKeystream());
    attributes.put(TMMH16.PREFIX, spec.getPrefix());
    try
      {
        mac.reset();
        mac.init(attributes);
      }
    catch (IllegalArgumentException iae)
      {
        throw new InvalidAlgorithmParameterException(iae.getMessage());
      }
  }
}
