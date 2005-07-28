/* EventModifier.java -- tool for converting modifier bits to 1.4 syle
   Copyright (C) 2002 Free Software Foundation, Inc.

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


package gnu.java.awt;

import java.awt.event.InputEvent;

public class EventModifier
{
  /** The mask for old events. */
  public static final int OLD_MASK = 0x3f;

  /** The mask for new events. */
  public static final int NEW_MASK = 0x3fc0;

  /**
   * Non-instantiable.
   */
  private EventModifier()
  {
    throw new InternalError();
  }

  /**
   * Converts the old style modifiers (0x3f) to the new style (0xffffffc0).
   *
   * @param mod the modifiers to convert
   * @return the adjusted modifiers
   */
  public static int extend(int mod)
  {
    // Favor what we hope will be the common case.
    if ((mod & OLD_MASK) == 0)
      return mod;
    if ((mod & InputEvent.SHIFT_MASK) != 0)
      mod |= InputEvent.SHIFT_DOWN_MASK;
    if ((mod & InputEvent.CTRL_MASK) != 0)
      mod |= InputEvent.CTRL_DOWN_MASK;
    if ((mod & InputEvent.META_MASK) != 0)
      mod |= InputEvent.META_DOWN_MASK;
    if ((mod & InputEvent.ALT_MASK) != 0)
      mod |= InputEvent.ALT_DOWN_MASK;
    if ((mod & InputEvent.BUTTON1_MASK) != 0)
      mod |= InputEvent.BUTTON1_DOWN_MASK;
    if ((mod & InputEvent.ALT_GRAPH_MASK) != 0)
      mod |= InputEvent.ALT_GRAPH_DOWN_MASK;
    return mod & ~OLD_MASK;
  }

  /**
   * Converts the new style modifiers (0xffffffc0) to the old style (0x3f).
   *
   * @param mod the modifiers to convert
   * @return the adjusted modifiers
   */
  public static int revert(int mod)
  {
    if ((mod & InputEvent.SHIFT_DOWN_MASK) != 0)
      mod |= InputEvent.SHIFT_MASK;
    if ((mod & InputEvent.CTRL_DOWN_MASK) != 0)
      mod |= InputEvent.CTRL_MASK;
    if ((mod & InputEvent.META_DOWN_MASK) != 0)
      mod |= InputEvent.META_MASK;
    if ((mod & InputEvent.ALT_DOWN_MASK) != 0)
      mod |= InputEvent.ALT_MASK;
    if ((mod & InputEvent.ALT_GRAPH_DOWN_MASK) != 0)
      mod |= InputEvent.ALT_GRAPH_MASK;
    if ((mod & InputEvent.BUTTON1_DOWN_MASK) != 0)
      mod |= InputEvent.BUTTON1_MASK;
    return mod & OLD_MASK;
  }
} // class EventModifier
