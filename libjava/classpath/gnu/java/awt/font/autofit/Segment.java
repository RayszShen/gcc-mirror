/* Segment.java -- FIXME: briefly describe file purpose
   Copyright (C) 2006 Free Software Foundation, Inc.

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


package gnu.java.awt.font.autofit;

import gnu.java.awt.font.opentype.truetype.Point;

class Segment
{

  static final int FLAG_EDGE_NORMAL = 0;
  static final int FLAG_EDGE_ROUND = 1;
  static final int FLAG_EDGE_SERIF = 2;
  static final int FLAG_EDGE_DONE = 4;
  int dir;
  int flags;
  Segment link;
  Segment serif;
  int numLinked;
  int pos;
  Point first;
  Point last;
  Point contour;
  int minPos;
  int maxPos;
  int score;
  int len;
  Segment edgeNext;
  Edge edge;

  public String toString()
  {
    StringBuilder s = new StringBuilder();
    s.append("[Segment] id: ");
    s.append(hashCode());
    s.append(", len:");
    s.append(len);
    s.append(", round: ");
    s.append(((flags & FLAG_EDGE_ROUND) != 0));
    s.append(", dir: ");
    s.append(dir);
    s.append(", pos: ");
    s.append(pos);
    s.append(", minPos: ");
    s.append(minPos);
    s.append(", maxPos: ");
    s.append(maxPos);
    s.append(", first: ");
    s.append(first);
    s.append(", last: ");
    s.append(last);
    s.append(", contour: ");
    s.append(contour);
    s.append(", link: ");
    s.append(link == null ? "null" : link.hashCode());
    s.append(", serif: ");
    s.append(serif == null ? "null" : serif.hashCode());
    return s.toString();
  }
}
