
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_sound_sampled_spi_FormatConversionProvider__
#define __javax_sound_sampled_spi_FormatConversionProvider__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace javax
  {
    namespace sound
    {
      namespace sampled
      {
          class AudioFormat;
          class AudioFormat$Encoding;
          class AudioInputStream;
        namespace spi
        {
            class FormatConversionProvider;
        }
      }
    }
  }
}

class javax::sound::sampled::spi::FormatConversionProvider : public ::java::lang::Object
{

public:
  FormatConversionProvider();
  virtual ::javax::sound::sampled::AudioInputStream * getAudioInputStream(::javax::sound::sampled::AudioFormat$Encoding *, ::javax::sound::sampled::AudioInputStream *) = 0;
  virtual ::javax::sound::sampled::AudioInputStream * getAudioInputStream(::javax::sound::sampled::AudioFormat *, ::javax::sound::sampled::AudioInputStream *) = 0;
  virtual JArray< ::javax::sound::sampled::AudioFormat$Encoding * > * getSourceEncodings() = 0;
  virtual JArray< ::javax::sound::sampled::AudioFormat$Encoding * > * getTargetEncodings() = 0;
  virtual JArray< ::javax::sound::sampled::AudioFormat$Encoding * > * getTargetEncodings(::javax::sound::sampled::AudioFormat *) = 0;
  virtual JArray< ::javax::sound::sampled::AudioFormat * > * getTargetFormats(::javax::sound::sampled::AudioFormat$Encoding *, ::javax::sound::sampled::AudioFormat *) = 0;
  virtual jboolean isConversionSupported(::javax::sound::sampled::AudioFormat$Encoding *, ::javax::sound::sampled::AudioFormat *);
  virtual jboolean isConversionSupported(::javax::sound::sampled::AudioFormat *, ::javax::sound::sampled::AudioFormat *);
  virtual jboolean isSourceEncodingSupported(::javax::sound::sampled::AudioFormat$Encoding *);
  virtual jboolean isTargetEncodingSupported(::javax::sound::sampled::AudioFormat$Encoding *);
  static ::java::lang::Class class$;
};

#endif // __javax_sound_sampled_spi_FormatConversionProvider__
