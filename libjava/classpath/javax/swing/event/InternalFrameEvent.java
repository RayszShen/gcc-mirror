/* InternalFrameEvent.java --
   Copyright (C) 2002, 2004, 2006,  Free Software Foundation, Inc.

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


package javax.swing.event;

import java.awt.AWTEvent;

import javax.swing.JInternalFrame;

/**
 * An event that indicates a change to a {@link JInternalFrame} component.
 *
 * @author Andrew Selkirk
 */
public class InternalFrameEvent extends AWTEvent
{
  private static final long serialVersionUID = -5204823611874873183L;

  /**
   * Internal frame activated event.
   */
  public static final int INTERNAL_FRAME_ACTIVATED = 25554;

  /**
   * Internal frame closed event.
   */
  public static final int INTERNAL_FRAME_CLOSED = 25551;

  /**
   * Internal frame closing event.
   */
  public static final int INTERNAL_FRAME_CLOSING = 25550;

  /**
   * Internal frame deactivated event.
   */
  public static final int INTERNAL_FRAME_DEACTIVATED = 25555;

  /**
   * Internal frame deiconifed event.
   */
  public static final int INTERNAL_FRAME_DEICONIFIED = 25553;

  /**
   * Internal frame frame first event.
   */
  public static final int INTERNAL_FRAME_FIRST = 25549;

  /**
   * Internal frame iconified event.
   */
  public static final int INTERNAL_FRAME_ICONIFIED = 25552;

  /**
   * Internal frame last event.
   */
  public static final int INTERNAL_FRAME_LAST = 25555;

  /**
   * Internal frame opened event.
   */
  public static final int INTERNAL_FRAME_OPENED = 25549;

  /**
   * Creates a new <code>JInternalFrameEvent</code> instance.
   *
   * @param source  the source of this event (<code>null</code> not permitted).
   * @param id  the event ID of this event (see the constants defined by this
   *     class).
   *
   * @throws IllegalArgumentException if <code>source</code> is
   *     <code>null</code>.
   */
  public InternalFrameEvent(JInternalFrame source, int id)
  {
    super(source, id);
  }

  /**
   * Returns the <code>JInternalFrame</code> component that is the source for
   * this event.
   *
   * @return The source.
   *
   * @since 1.3
   */
  public JInternalFrame getInternalFrame()
  {
    return (JInternalFrame) source;
  }

  /**
   * Returns a string that indicates the event id.  This is used by the
   * {@link #toString()} method.
   *
   * @return A string that indicates the event id.
   */
  public String paramString()
  {
    switch (id) {
      case INTERNAL_FRAME_ACTIVATED:
        return "INTERNAL_FRAME_ACTIVATED";
      case INTERNAL_FRAME_CLOSED:
        return "INTERNAL_FRAME_CLOSED";
      case INTERNAL_FRAME_CLOSING:
        return "INTERNAL_FRAME_CLOSING";
      case INTERNAL_FRAME_DEACTIVATED:
        return "INTERNAL_FRAME_DEACTIVATED";
      case INTERNAL_FRAME_DEICONIFIED:
        return "INTERNAL_FRAME_DEICONIFIED";
      case INTERNAL_FRAME_ICONIFIED:
        return "INTERNAL_FRAME_ICONIFIED";
      case INTERNAL_FRAME_OPENED:
        return "INTERNAL_FRAME_OPENED";
      default:
        return "unknown type";
    }
  }
}
