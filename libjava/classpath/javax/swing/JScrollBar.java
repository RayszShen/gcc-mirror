/* JScrollBar.java --
   Copyright (C) 2002, 2004, 2005, 2006,  Free Software Foundation, Inc.

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


package javax.swing;

import gnu.java.lang.CPStringBuilder;

import java.awt.Adjustable;
import java.awt.Dimension;
import java.awt.event.AdjustmentEvent;
import java.awt.event.AdjustmentListener;
import java.beans.PropertyChangeEvent;

import javax.accessibility.Accessible;
import javax.accessibility.AccessibleContext;
import javax.accessibility.AccessibleRole;
import javax.accessibility.AccessibleState;
import javax.accessibility.AccessibleStateSet;
import javax.accessibility.AccessibleValue;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.plaf.ScrollBarUI;

/**
 * The JScrollBar. Two buttons control how the values that the 
 * scroll bar can take. You can also drag the thumb or click the track
 * to move the scroll bar. Typically, the JScrollBar is used with
 * other components to translate the value of the bar to the viewable
 * contents of the other components.
 */
public class JScrollBar extends JComponent implements Adjustable, Accessible
{
  /**
   * Provides the accessibility features for the <code>JScrollBar</code>
   * component.
   */
  protected class AccessibleJScrollBar extends JComponent.AccessibleJComponent
    implements AccessibleValue
  {
    private static final long serialVersionUID = -7758162392045586663L;
    
    /**
     * Creates a new <code>AccessibleJScrollBar</code> instance.
     */
    protected AccessibleJScrollBar()
    {
      super();
    }

    /**
     * Returns a set containing the current state of the {@link JScrollBar} 
     * component.
     *
     * @return The accessible state set.
     */
    public AccessibleStateSet getAccessibleStateSet()
    {
      AccessibleStateSet result = super.getAccessibleStateSet();
      if (orientation == JScrollBar.HORIZONTAL)
        result.add(AccessibleState.HORIZONTAL);
      else if (orientation == JScrollBar.VERTICAL)
        result.add(AccessibleState.VERTICAL);
      return result;
    }

    /**
     * Returns the accessible role for the <code>JScrollBar</code> component.
     *
     * @return {@link AccessibleRole#SCROLL_BAR}.
     */
    public AccessibleRole getAccessibleRole()
    {
      return AccessibleRole.SCROLL_BAR;
    }

    /**
     * Returns an object that provides access to the current, minimum and 
     * maximum values.
     *
     * @return The accessible value.
     */
    public AccessibleValue getAccessibleValue()
    {
      return this;
    }

    /**
     * Returns the current value of the {@link JScrollBar} component, as an
     * {@link Integer}.
     *
     * @return The current value of the {@link JScrollBar} component.
     */
    public Number getCurrentAccessibleValue()
    {
      return new Integer(getValue());
    }

    /**
     * Sets the current value of the {@link JScrollBar} component and sends a
     * {@link PropertyChangeEvent} (with the property name 
     * {@link AccessibleContext#ACCESSIBLE_VALUE_PROPERTY}) to all registered
     * listeners.  If the supplied value is <code>null</code>, this method 
     * does nothing and returns <code>false</code>.
     *
     * @param value  the new slider value (<code>null</code> permitted).
     *
     * @return <code>true</code> if the slider value is updated, and 
     *     <code>false</code> otherwise.
     */
    public boolean setCurrentAccessibleValue(Number value)
    {
      if (value == null)
        return false;
      Number oldValue = getCurrentAccessibleValue();
      setValue(value.intValue());
      firePropertyChange(AccessibleContext.ACCESSIBLE_VALUE_PROPERTY, oldValue, 
                         new Integer(getValue()));
      return true;
    }

    /**
     * Returns the minimum value of the {@link JScrollBar} component, as an
     * {@link Integer}.
     *
     * @return The minimum value of the {@link JScrollBar} component.
     */
    public Number getMinimumAccessibleValue()
    {
      return new Integer(getMinimum());
    }

    /**
     * Returns the maximum value of the {@link JScrollBar} component, as an
     * {@link Integer}.
     *
     * @return The maximum value of the {@link JScrollBar} component.
     */
    public Number getMaximumAccessibleValue()
    {
      return new Integer(getMaximum() - model.getExtent());
    }
  }

  /**
   * Listens for changes on the model and fires them to interested
   * listeners on the JScrollBar, after re-sourcing them.
   */
  private class ScrollBarChangeListener
    implements ChangeListener
  {

    public void stateChanged(ChangeEvent event)
    {
      Object o = event.getSource();
      if (o instanceof BoundedRangeModel)
        {
          BoundedRangeModel m = (BoundedRangeModel) o;
          fireAdjustmentValueChanged(AdjustmentEvent.ADJUSTMENT_VALUE_CHANGED,
                                     AdjustmentEvent.TRACK, m.getValue(),
                                     m.getValueIsAdjusting());
        }
    }
    
  }

  private static final long serialVersionUID = -8195169869225066566L;
  
  /** How much the thumb moves when moving in a block. */
  protected int blockIncrement = 10;

  /** The model that holds the scroll bar's data. */
  protected BoundedRangeModel model;

  /** The orientation of the scroll bar. */
  protected int orientation = SwingConstants.VERTICAL;

  /** How much the thumb moves when moving in a unit. */
  protected int unitIncrement = 1;

  /**
   * This ChangeListener forwards events fired from the model and re-sources
   * them to originate from this JScrollBar.
   */
  private ChangeListener sbChangeListener;

  /** 
   * Creates a new horizontal JScrollBar object with a minimum
   * of 0, a maxmium of 100, a value of 0 and an extent of 10.
   */
  public JScrollBar()
  {
    this(SwingConstants.VERTICAL, 0, 10, 0, 100);
  }

  /**
   * Creates a new JScrollBar object with a minimum of 0, a 
   * maximum of 100, a value of 0, an extent of 10 and the given
   * orientation.
   *
   * @param orientation The orientation of the JScrollBar.
   */
  public JScrollBar(int orientation)
  {
    this(orientation, 0, 10, 0, 100);
  }

  /**
   * Creates a new JScrollBar object with the given orientation, 
   * value, min, max, and extent.
   *
   * @param orientation The orientation to use.
   * @param value The value to use.
   * @param extent The extent to use.
   * @param min The minimum value of the scrollbar.
   * @param max The maximum value of the scrollbar.
   */
  public JScrollBar(int orientation, int value, int extent, int min, int max)
  {
    model = new DefaultBoundedRangeModel(value, extent, min, max);
    sbChangeListener = new ScrollBarChangeListener();
    model.addChangeListener(sbChangeListener);
    if (orientation != SwingConstants.HORIZONTAL
        && orientation != SwingConstants.VERTICAL)
      throw new IllegalArgumentException(orientation
                                         + " is not a legal orientation");
    this.orientation = orientation;
    updateUI();
  }

  /**
   * This method sets the UI of this scrollbar to
   * the given UI.
   *
   * @param ui The UI to use with this scrollbar.
   */
  public void setUI(ScrollBarUI ui)
  {
    super.setUI(ui);
  }

  /**
   * This method returns the UI that is being used
   * with this scrollbar.
   *
   * @return The scrollbar's current UI.
   */
  public ScrollBarUI getUI()
  {
    return (ScrollBarUI) ui;
  }

  /**
   * This method changes the UI to be the
   * default for the current look and feel.
   */
  public void updateUI()
  {
    setUI((ScrollBarUI) UIManager.getUI(this));
  }

  /**
   * This method returns an identifier to 
   * choose the correct UI delegate for the
   * scrollbar.
   *
   * @return The identifer to choose the UI delegate; "ScrollBarUI"
   */
  public String getUIClassID()
  {
    return "ScrollBarUI";
  }

  /**
   * This method returns the orientation of the scrollbar.
   *
   * @return The orientation of the scrollbar.
   */
  public int getOrientation()
  {
    return orientation;
  }

  /**
   * This method sets the orientation of the scrollbar.
   *
   * @param orientation The orientation of the scrollbar.
   */
  public void setOrientation(int orientation)
  {
    if (orientation != SwingConstants.HORIZONTAL
        && orientation != SwingConstants.VERTICAL)
      throw new IllegalArgumentException("orientation must be one of HORIZONTAL or VERTICAL");
    if (orientation != this.orientation)
      {
	int oldOrientation = this.orientation;
	this.orientation = orientation;
	firePropertyChange("orientation", oldOrientation,
	                   this.orientation);
      }
  }

  /**
   * This method returns the model being used with 
   * the scrollbar.
   *
   * @return The scrollbar's model.
   */
  public BoundedRangeModel getModel()
  {
    return model;
  }

  /**
   * This method sets the model to use with
   * the scrollbar.
   *
   * @param newModel The new model to use with the scrollbar.
   */
  public void setModel(BoundedRangeModel newModel)
  {
    BoundedRangeModel oldModel = model;
    if (oldModel != null)
      oldModel.removeChangeListener(sbChangeListener);
    model = newModel;
    if (model != null)
      model.addChangeListener(sbChangeListener);
    firePropertyChange("model", oldModel, model);
  }

  /**
   * This method returns how much the scrollbar's value
   * should change for a unit increment depending on the 
   * given direction.
   *
   * @param direction The direction to scroll in.
   *
   * @return The amount the scrollbar's value will change given the direction.
   */
  public int getUnitIncrement(int direction)
  {
    return unitIncrement;
  }

  /**
   * This method sets the unitIncrement property.
   *
   * @param unitIncrement The new unitIncrement.
   */
  public void setUnitIncrement(int unitIncrement)
  {
    if (unitIncrement != this.unitIncrement)
      {
	int oldInc = this.unitIncrement;
	this.unitIncrement = unitIncrement;
	firePropertyChange("unitIncrement", oldInc,
	                   this.unitIncrement);
      }
  }

  /**
   * The method returns how much the scrollbar's value
   * should change for a block increment depending on
   * the given direction.
   *
   * @param direction The direction to scroll in.
   *
   * @return The amount the scrollbar's value will change given the direction.
   */
  public int getBlockIncrement(int direction)
  {
    return blockIncrement;
  }

  /**
   * This method sets the blockIncrement property.
   *
   * @param blockIncrement The new blockIncrement.
   */
  public void setBlockIncrement(int blockIncrement)
  {
    if (blockIncrement != this.blockIncrement)
      {
	int oldInc = this.blockIncrement;
	this.blockIncrement = blockIncrement;
	firePropertyChange("blockIncrement", oldInc,
	                   this.blockIncrement);
      }
  }

  /**
   * This method returns the unitIncrement.
   *
   * @return The unitIncrement.
   */
  public int getUnitIncrement()
  {
    return unitIncrement;
  }

  /**
   * This method returns the blockIncrement.
   *
   * @return The blockIncrement.
   */
  public int getBlockIncrement()
  {
    return blockIncrement;
  }

  /**
   * This method returns the value of the scrollbar.
   *
   * @return The value of the scrollbar.
   */
  public int getValue()
  {
    return model.getValue();
  }

  /**
   * This method changes the value of the scrollbar.
   *
   * @param value The new value of the scrollbar.
   */
  public void setValue(int value)
  {
    model.setValue(value);
  }

  /**
   * This method returns the visible amount (AKA extent). 
   * The visible amount can be used by UI delegates to 
   * determine the size of the thumb.
   *
   * @return The visible amount (AKA extent).
   */
  public int getVisibleAmount()
  {
    return model.getExtent();
  }

  /**
   * This method sets the visible amount (AKA extent).
   *
   * @param extent The visible amount (AKA extent).
   */
  public void setVisibleAmount(int extent)
  {
    model.setExtent(extent);
  }

  /**
   * This method returns the minimum value of the scrollbar.
   *
   * @return The minimum value of the scrollbar.
   */
  public int getMinimum()
  {
    return model.getMinimum();
  }

  /**
   * This method sets the minimum value of the scrollbar.
   *
   * @param minimum The minimum value of the scrollbar.
   */
  public void setMinimum(int minimum)
  {
    model.setMinimum(minimum);
  }

  /**
   * This method returns the maximum value of the scrollbar.
   *
   * @return The maximum value of the scrollbar.
   */
  public int getMaximum()
  {
    return model.getMaximum();
  }

  /**
   * This method sets the maximum value of the scrollbar.
   *
   * @param maximum The maximum value of the scrollbar.
   */
  public void setMaximum(int maximum)
  {
    model.setMaximum(maximum);
  }

  /**
   * This method returns the model's isAjusting value.
   *
   * @return The model's isAdjusting value.
   */
  public boolean getValueIsAdjusting()
  {
    return model.getValueIsAdjusting();
  }

  /**
   * This method sets the model's isAdjusting value.
   *
   * @param b The new isAdjusting value.
   */
  public void setValueIsAdjusting(boolean b)
  {
    model.setValueIsAdjusting(b);
  }

  /**
   * This method sets the value, extent, minimum and 
   * maximum.
   *
   * @param newValue The new value.
   * @param newExtent The new extent.
   * @param newMin The new minimum.
   * @param newMax The new maximum.
   */
  public void setValues(int newValue, int newExtent, int newMin, int newMax)
  {
    model.setRangeProperties(newValue, newExtent, newMin, newMax,
                             model.getValueIsAdjusting());
  }

  /**
   * This method adds an AdjustmentListener to the scroll bar.
   *
   * @param listener The listener to add.
   */
  public void addAdjustmentListener(AdjustmentListener listener)
  {
    listenerList.add(AdjustmentListener.class, listener);
  }

  /**
   * This method removes an AdjustmentListener from the scroll bar. 
   *
   * @param listener The listener to remove.
   */
  public void removeAdjustmentListener(AdjustmentListener listener)
  {
    listenerList.remove(AdjustmentListener.class, listener);
  }

  /**
   * This method returns an arry of all AdjustmentListeners listening to 
   * this scroll bar.
   *
   * @return An array of AdjustmentListeners listening to this scroll bar.
   */
  public AdjustmentListener[] getAdjustmentListeners()
  {
    return (AdjustmentListener[]) listenerList.getListeners(AdjustmentListener.class);
  }

  /**
   * This method is called to fired AdjustmentEvents to the listeners
   * of this scroll bar. All AdjustmentEvents that are fired
   * will have an ID of ADJUSTMENT_VALUE_CHANGED and a type of
   * TRACK. 
   *
   * @param id The ID of the adjustment event.
   * @param type The Type of change.
   * @param value The new value for the property that was changed..
   */
  protected void fireAdjustmentValueChanged(int id, int type, int value)
  {
    fireAdjustmentValueChanged(id, type, value, getValueIsAdjusting());
  }

  /**
   * Helper method for firing adjustment events that can have their
   * isAdjusting field modified.
   *
   * This is package private to avoid an accessor method.
   *
   * @param id the ID of the event
   * @param type the type of the event
   * @param value the value
   * @param isAdjusting if the scrollbar is adjusting or not
   */
  void fireAdjustmentValueChanged(int id, int type, int value,
                                          boolean isAdjusting)
  {
    Object[] adjustmentListeners = listenerList.getListenerList();
    AdjustmentEvent adjustmentEvent = new AdjustmentEvent(this, id, type,
                                                          value, isAdjusting);
    for (int i = adjustmentListeners.length - 2; i >= 0; i -= 2)
      {
        if (adjustmentListeners[i] == AdjustmentListener.class)
          ((AdjustmentListener) adjustmentListeners[i + 1]).adjustmentValueChanged(adjustmentEvent);
      }
  }

  /**
   * This method returns the minimum size for this scroll bar.
   *
   * @return The minimum size.
   */
  public Dimension getMinimumSize()
  {
    return ui.getMinimumSize(this);
  }

  /**
   * This method returns the maximum size for this scroll bar.
   *
   * @return The maximum size.
   */
  public Dimension getMaximumSize()
  {
    return ui.getMaximumSize(this);
  }

  /**
   * This method overrides the setEnabled in JComponent.
   * When the scroll bar is disabled, the knob cannot
   * be moved.
   *
   * @param x Whether the scrollbar is enabled.
   */
  public void setEnabled(boolean x)
  {
    // nothing special needs to be done here since we 
    // just check the enabled setting before changing the value.
    super.setEnabled(x);
  }

  /**
   * Returns a string describing the attributes for the <code>JScrollBar</code>
   * component, for use in debugging.  The return value is guaranteed to be 
   * non-<code>null</code>, but the format of the string may vary between
   * implementations.
   *
   * @return A string describing the attributes of the <code>JScrollBar</code>.
   */
  protected String paramString()
  {
    CPStringBuilder sb = new CPStringBuilder(super.paramString());
    sb.append(",blockIncrement=").append(blockIncrement);
    sb.append(",orientation=");
    if (this.orientation == JScrollBar.HORIZONTAL)
      sb.append("HORIZONTAL");
    else 
      sb.append("VERTICAL");
    sb.append(",unitIncrement=").append(unitIncrement);
    return sb.toString();
  }

  /**
   * Returns the object that provides accessibility features for this
   * <code>JScrollBar</code> component.
   *
   * @return The accessible context (an instance of 
   *     {@link AccessibleJScrollBar}).
   */
  public AccessibleContext getAccessibleContext()
  {
    if (accessibleContext == null)
      accessibleContext = new AccessibleJScrollBar();
    return accessibleContext;
  }
}
