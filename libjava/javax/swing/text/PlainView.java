/* PlainView.java -- 
   Copyright (C) 2004  Free Software Foundation, Inc.

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
exception statement from your version. */


package javax.swing.text;

import java.awt.Color;
import java.awt.Component;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Rectangle;
import java.awt.Shape;


public class PlainView extends View
  implements TabExpander
{
  private Color selectedColor;
  private Color unselectedColor;
  private Font font;
  
  protected FontMetrics metrics;

  public PlainView(Element elem)
  {
    super(elem);
  }

  /**
   * @since 1.4
   */
  protected void updateMetrics()
  {
    Component component = getContainer();
    Font font = component.getFont();

    if (this.font != font)
      {
	this.font = font;
	metrics = component.getFontMetrics(font);
      }
  }
  
  /**
   * @since 1.4
   */
  protected Rectangle lineToRect(Shape a, int line)
  {
    // Ensure metrics are up-to-date.
    updateMetrics();
    
    Rectangle rect = a.getBounds();
    int fontHeight = metrics.getHeight();
    return new Rectangle(rect.x, rect.y + (line * fontHeight),
			 rect.width, fontHeight);
  }

  protected Shape modelToView(int position, Shape a, Position.Bias b)
    throws BadLocationException
  {
    Document document = getDocument();

    // Get rectangle of the line containing position.
    int lineIndex = getElement().getElementIndex(position);
    Rectangle rect = lineToRect(a, lineIndex);

    // Get the rectangle for position.
    Element line = getElement().getElement(lineIndex);
    int lineStart = line.getStartOffset();
    Segment segment = new Segment();
    document.getText(lineStart, position - lineStart, segment);
    int xoffset = Utilities.getTabbedTextWidth(segment, metrics, rect.x,
					       this, lineStart);

    // Calc the real rectangle.
    rect.x += xoffset;
    rect.width = 1;
    rect.height = metrics.getHeight();

    return rect;
  }
  
  public void drawLine(int lineIndex, Graphics g, int x, int y)
  {
    try
      {
	metrics = g.getFontMetrics();
	// FIXME: Selected text are not drawn yet.
	drawUnselectedText(g, x, y, 0, getDocument().getLength());
	//drawSelectedText(g, , , , );
      }
    catch (BadLocationException e)
      {
	// This should never happen.
      }
  }

  public int drawSelectedText(Graphics g, int x, int y, int p0, int p1)
    throws BadLocationException
  {
    g.setColor(selectedColor);
    Segment segment = new Segment();
    getDocument().getText(p0, p1 - p0, segment);
    return Utilities.drawTabbedText(segment, x, y, g, this, 0);
  }

  public int drawUnselectedText(Graphics g, int x, int y, int p0, int p1)
    throws BadLocationException
  {
    g.setColor(unselectedColor);
    Segment segment = new Segment();
    getDocument().getText(p0, p1 - p0, segment);
    return Utilities.drawTabbedText(segment, x, y, g, this, 0);
  }

  public void paint(Graphics g, Shape s)
  {
    JTextComponent textComponent = (JTextComponent) getContainer();

    g.setFont(textComponent.getFont());
    selectedColor = textComponent.getSelectedTextColor();
    unselectedColor = textComponent.getForeground();
    
    Rectangle rect = s.getBounds();

    g.setColor(Color.WHITE);
    g.fillRect(rect.x, rect.y, rect.width, rect.height);
    
    // FIXME: Text may be scrolled.
    drawLine(0, g, rect.x, rect.y);
  }

  public int getTabSize()
  {
    return 8;
  }

  public float nextTabStop(float x, int tabStop)
  {
    System.out.println("Michael: PlainView.nextTabpStop: missing implementation");
    return x;
  }

  public float getPreferredSpan(int axis)
  {
    if (axis != X_AXIS && axis != Y_AXIS)
      throw new IllegalArgumentException();

    return 10;
  }
}

