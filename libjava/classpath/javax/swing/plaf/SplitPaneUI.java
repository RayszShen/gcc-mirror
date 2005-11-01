/* SplitPaneUI.java --
   Copyright (C) 2002, 2003, 2004  Free Software Foundation, Inc.

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


package javax.swing.plaf;

import java.awt.Graphics;

import javax.swing.JSplitPane;

/**
 * An abstract base class for delegates that implement the pluggable
 * look and feel for a <code>JSplitPane</code>.
 *
 * @see javax.swing.JSplitPane
 *
 * @author Andrew Selkirk (aselkirk@sympatico.ca)
 * @author Sascha Brawer (brawer@dandelis.ch)
 */
public abstract class SplitPaneUI
  extends ComponentUI
{
  /**
   * Constructs a new <code>SplitPaneUI</code>.
   */
  public SplitPaneUI()
  {
  }


  /**
   * Moves the divider to the location which best respects
   * the preferred sizes of the children.
   *
   * @param pane the <code>JSplitPane</code> for thich this
   *        delegate provides the look and feel.
   */
  public abstract void resetToPreferredSizes(JSplitPane pane);


  /**
   * Moves the divider to the specified location.
   *
   * @param pane the <code>JSplitPane</code> for thich this
   *        delegate provides the look and feel.
   *
   * @param location the new location of the divider.
   */
  public abstract void setDividerLocation(JSplitPane pane,
                                          int location);


  /**
   * Determines the current location of the divider.
   *
   * @param pane the <code>JSplitPane</code> for thich this
   *        delegate provides the look and feel.
   *
   * @return the current location of the divider.
   */
  public abstract int getDividerLocation(JSplitPane pane);
  
  
  /**
   * Determines the minimum location of the divider.
   *
   * @param pane the <code>JSplitPane</code> for thich this
   *        delegate provides the look and feel.
   *
   * @return the leftmost (or topmost) possible location
   *         of the divider.
   */
  public abstract int getMinimumDividerLocation(JSplitPane pane);


  /**
   * Determines the maximum location of the divider.
   *
   * @param pane the <code>JSplitPane</code> for thich this
   *        delegate provides the look and feel.
   *
   * @return the bottommost (or rightmost) possible location
   *         of the divider.
   */
  public abstract int getMaximumDividerLocation(JSplitPane pane);


  /**
   * Called by the <code>JSplitPane</code> after it has finished
   * painting its children.
   *
   * @param pane the <code>JSplitPane</code> for thich this
   *        delegate provides the look and feel.
   *
   * @param g the Graphics used for painting.
   */
  public abstract void finishedPaintingChildren(JSplitPane pane,
                                                Graphics g);
}
