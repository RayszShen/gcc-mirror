
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_plaf_metal_OceanTheme__
#define __javax_swing_plaf_metal_OceanTheme__

#pragma interface

#include <javax/swing/plaf/metal/DefaultMetalTheme.h>
extern "Java"
{
  namespace javax
  {
    namespace swing
    {
        class UIDefaults;
      namespace plaf
      {
          class ColorUIResource;
        namespace metal
        {
            class OceanTheme;
        }
      }
    }
  }
}

class javax::swing::plaf::metal::OceanTheme : public ::javax::swing::plaf::metal::DefaultMetalTheme
{

public:
  OceanTheme();
  virtual ::java::lang::String * getName();
  virtual ::javax::swing::plaf::ColorUIResource * getControlTextColor();
  virtual ::javax::swing::plaf::ColorUIResource * getDesktopColor();
  virtual ::javax::swing::plaf::ColorUIResource * getInactiveControlTextColor();
  virtual ::javax::swing::plaf::ColorUIResource * getMenuDisabledForeground();
public: // actually protected
  virtual ::javax::swing::plaf::ColorUIResource * getBlack();
  virtual ::javax::swing::plaf::ColorUIResource * getPrimary1();
  virtual ::javax::swing::plaf::ColorUIResource * getPrimary2();
  virtual ::javax::swing::plaf::ColorUIResource * getPrimary3();
  virtual ::javax::swing::plaf::ColorUIResource * getSecondary1();
  virtual ::javax::swing::plaf::ColorUIResource * getSecondary2();
  virtual ::javax::swing::plaf::ColorUIResource * getSecondary3();
public:
  virtual void addCustomEntriesToTable(::javax::swing::UIDefaults *);
public: // actually package-private
  static ::javax::swing::plaf::ColorUIResource * BLACK;
  static ::javax::swing::plaf::ColorUIResource * PRIMARY1;
  static ::javax::swing::plaf::ColorUIResource * PRIMARY2;
  static ::javax::swing::plaf::ColorUIResource * PRIMARY3;
  static ::javax::swing::plaf::ColorUIResource * SECONDARY1;
  static ::javax::swing::plaf::ColorUIResource * SECONDARY2;
  static ::javax::swing::plaf::ColorUIResource * SECONDARY3;
  static ::javax::swing::plaf::ColorUIResource * INACTIVE_CONTROL_TEXT;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_plaf_metal_OceanTheme__
