/* GraphicAttribute.java
   Copyright (C) 2003 Free Software Foundation, Inc.

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


package java.awt.font;

import java.awt.Graphics2D;
import java.awt.geom.Rectangle2D;

/**
 * @author Michael Koch
 */
public abstract class GraphicAttribute
{
  public static final int BOTTOM_ALIGNMENT = -2;
  public static final int CENTER_BASELINE = 1;
  public static final int HANGING_BASELINE = 2;
  public static final int ROMAN_BASELINE = 0;
  public static final int TOP_ALIGNMENT = -1;

  private int alignment;
  
  protected GraphicAttribute (int alignment)
  {
    this.alignment = alignment;
  }

  public abstract void draw (Graphics2D graphics, float x, float y);

  public abstract float getAdvance ();

  public final int getAlignment ()
  {
    return alignment;
  }

  public abstract float getAscent ();

  public Rectangle2D getBounds ()
  {
    throw new Error ("not implemented");
  }

  public abstract float getDescent ();

  public GlyphJustificationInfo getJustificationInfo ()
  {
    throw new Error ("not implemented");
  }
}
