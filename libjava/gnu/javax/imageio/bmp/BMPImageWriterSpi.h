
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_imageio_bmp_BMPImageWriterSpi__
#define __gnu_javax_imageio_bmp_BMPImageWriterSpi__

#pragma interface

#include <javax/imageio/spi/ImageWriterSpi.h>
#include <gcj/array.h>

extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace imageio
      {
        namespace bmp
        {
            class BMPImageWriter;
            class BMPImageWriterSpi;
        }
      }
    }
  }
  namespace javax
  {
    namespace imageio
    {
        class ImageTypeSpecifier;
        class ImageWriter;
    }
  }
}

class gnu::javax::imageio::bmp::BMPImageWriterSpi : public ::javax::imageio::spi::ImageWriterSpi
{

public:
  BMPImageWriterSpi();
  virtual jboolean canEncodeImage(::javax::imageio::ImageTypeSpecifier *);
  virtual ::javax::imageio::ImageWriter * createWriterInstance(::java::lang::Object *);
  virtual ::gnu::javax::imageio::bmp::BMPImageWriter * getWriterInstance();
  virtual ::java::lang::String * getDescription(::java::util::Locale *);
public: // actually package-private
  static ::java::lang::String * vendorName;
  static ::java::lang::String * version;
  static ::java::lang::String * writerClassName;
  static JArray< ::java::lang::String * > * names;
  static JArray< ::java::lang::String * > * suffixes;
  static JArray< ::java::lang::String * > * MIMETypes;
  static JArray< ::java::lang::String * > * readerSpiNames;
  static const jboolean supportsStandardStreamMetadataFormat = 0;
  static ::java::lang::String * nativeStreamMetadataFormatName;
  static ::java::lang::String * nativeStreamMetadataFormatClassName;
  static JArray< ::java::lang::String * > * extraStreamMetadataFormatNames;
  static JArray< ::java::lang::String * > * extraStreamMetadataFormatClassNames;
  static const jboolean supportsStandardImageMetadataFormat = 0;
  static ::java::lang::String * nativeImageMetadataFormatName;
  static ::java::lang::String * nativeImageMetadataFormatClassName;
  static JArray< ::java::lang::String * > * extraImageMetadataFormatNames;
  static JArray< ::java::lang::String * > * extraImageMetadataFormatClassNames;
private:
  ::gnu::javax::imageio::bmp::BMPImageWriter * __attribute__((aligned(__alignof__( ::javax::imageio::spi::ImageWriterSpi)))) writerInstance;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_imageio_bmp_BMPImageWriterSpi__
