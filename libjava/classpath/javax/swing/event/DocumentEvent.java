/* DocumentEvent.java --
   Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.

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

import javax.swing.text.Document;
import javax.swing.text.Element;

/**
 * DocumentEvent public interface
 * @author Andrew Selkirk
 * @author Ronald Veldema
 */
public interface DocumentEvent
{
  /**
   * ElementChange public interface
   */
  public static interface ElementChange
  {
    /**
     * getIndex
     * @returns int
     */
    int getIndex();

    /**
     * getElement
     * @returns Element
     */
    Element getElement();

    /**
     * getChildrenRemoved
     * @returns Element[]
     */
    Element[] getChildrenRemoved();

    /**
     * getChildrenAdded
     * @returns Element[]
     */
    Element[] getChildrenAdded();
    
  }

  /**
   * EventType
   */
  class EventType
  {
    /**
     * INSERT
     */
    public static final EventType INSERT = new EventType("INSERT"); // TODO

    /**
     * REMOVE
     */
    public static final EventType REMOVE = new EventType("REMOVE"); // TODO

    /**
     * CHANGE
     */
    public static final EventType CHANGE = new EventType("CHANGE"); // TODO

    /**
     * typeString
     */
    private String type;

    /**
     * Constructor EventType
     * @param type TODO
     */
    private EventType(String type)
    {
      this.type = type;
    }

    /**
     * toString
     * @returns String
     */
    public String toString()
    {
      return type;
    }
  }

  /**
   * getType
   * @returns EventType
   */
  EventType getType();

  /**
   * getOffset
   * @returns int
   */
  int getOffset();

  /**
   * getLength
   * @returns int
   */
  int getLength();

  /**
   * getDocument
   * @returns Document
   */
  Document getDocument();

  /**
   * getChange
   * @param element TODO
   * @returns ElementChange
   */
  ElementChange getChange(Element element);

}
