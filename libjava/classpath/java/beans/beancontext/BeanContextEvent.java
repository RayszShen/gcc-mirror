/* java.beans.beancontext.BeanContextEvent
   Copyright (C) 1999 Free Software Foundation, Inc.

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


package java.beans.beancontext;

import java.util.EventObject;

/**
 * Generic superclass for events fired by <code>BeanContext</code>s.
 *
 * @author John Keiser
 * @since 1.2
 */

public abstract class BeanContextEvent extends EventObject
{
  private static final long serialVersionUID = 7267998073569045052L;
  
  /**
   * The <code>BeanContext</code> that most recently passed this
   * event on.
   */
  protected BeanContext propagatedFrom;

  /**
   * Create a new event, from the specified <code>BeanContext</code>.
   * <code>propagatedFrom</code> will be initialized to
   * <code>null</code>.
   *
   * @param source the source of the event.
   */
  protected BeanContextEvent(BeanContext source)
  {
    super(source);
  }

  /**
   * Get the <code>BeanContext</code> that originated this event.
   * @return the originator of this event.
   */
  public BeanContext getBeanContext()
  {
    return (BeanContext)getSource();
  }

  /**
   * Get the most recent propagator of this event.
   * If this value is <code>null</code>, you have received the event
   * straight from the source.
   *
   * @return the most recent propagator of this event.
   */
  public BeanContext getPropagatedFrom()
  {
    return propagatedFrom;
  }

  /**
   * Tell whether this event has been propagated.
   * @return <code>true</code> iff <code>getPropagatedFrom() != null</code>.
   */
  public boolean isPropagated()
  {
    return propagatedFrom != null;
  }

  /**
   * Set the most recent propagator of this event.
   * @param propagator the most recent propagator of this event.
   */
  public void setPropagatedFrom(BeanContext propagator)
  {
    propagatedFrom = propagator;
  }
}
