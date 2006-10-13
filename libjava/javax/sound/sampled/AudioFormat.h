
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_sound_sampled_AudioFormat__
#define __javax_sound_sampled_AudioFormat__

#pragma interface

#include <java/lang/Object.h>
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
      }
    }
  }
}

class javax::sound::sampled::AudioFormat : public ::java::lang::Object
{

public:
  AudioFormat(::javax::sound::sampled::AudioFormat$Encoding *, jfloat, jint, jint, jint, jfloat, jboolean);
  AudioFormat(::javax::sound::sampled::AudioFormat$Encoding *, jfloat, jint, jint, jint, jfloat, jboolean, ::java::util::Map *);
  AudioFormat(jfloat, jint, jint, jboolean, jboolean);
  virtual jint getChannels();
  virtual ::javax::sound::sampled::AudioFormat$Encoding * getEncoding();
  virtual jfloat getFrameRate();
  virtual jint getFrameSize();
  virtual ::java::lang::Object * getProperty(::java::lang::String *);
  virtual jfloat getSampleRate();
  virtual jint getSampleSizeInBits();
  virtual jboolean isBigEndian();
  virtual jboolean matches(::javax::sound::sampled::AudioFormat *);
  virtual ::java::util::Map * properties();
  virtual ::java::lang::String * toString();
public: // actually protected
  jboolean __attribute__((aligned(__alignof__( ::java::lang::Object)))) bigEndian;
  jint channels;
  ::javax::sound::sampled::AudioFormat$Encoding * encoding;
  jfloat frameRate;
  jint frameSize;
  jfloat sampleRate;
  jint sampleSizeInBits;
private:
  ::java::util::Map * properties__;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_sound_sampled_AudioFormat__
