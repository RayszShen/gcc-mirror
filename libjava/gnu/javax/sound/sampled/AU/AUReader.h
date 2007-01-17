
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_javax_sound_sampled_AU_AUReader__
#define __gnu_javax_sound_sampled_AU_AUReader__

#pragma interface

#include <javax/sound/sampled/spi/AudioFileReader.h>
extern "Java"
{
  namespace gnu
  {
    namespace javax
    {
      namespace sound
      {
        namespace sampled
        {
          namespace AU
          {
              class AUReader;
          }
        }
      }
    }
  }
  namespace java
  {
    namespace net
    {
        class URL;
    }
  }
  namespace javax
  {
    namespace sound
    {
      namespace sampled
      {
          class AudioFileFormat;
          class AudioInputStream;
      }
    }
  }
}

class gnu::javax::sound::sampled::AU::AUReader : public ::javax::sound::sampled::spi::AudioFileReader
{

public:
  AUReader();
  virtual ::javax::sound::sampled::AudioFileFormat * getAudioFileFormat(::java::io::File *);
  virtual ::javax::sound::sampled::AudioFileFormat * getAudioFileFormat(::java::io::InputStream *);
  virtual ::javax::sound::sampled::AudioFileFormat * getAudioFileFormat(::java::net::URL *);
  virtual ::javax::sound::sampled::AudioInputStream * getAudioInputStream(::java::io::File *);
  virtual ::javax::sound::sampled::AudioInputStream * getAudioInputStream(::java::io::InputStream *);
  virtual ::javax::sound::sampled::AudioInputStream * getAudioInputStream(::java::net::URL *);
  static ::java::lang::Class class$;
};

#endif // __gnu_javax_sound_sampled_AU_AUReader__
