
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __javax_swing_filechooser_FileSystemView__
#define __javax_swing_filechooser_FileSystemView__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace javax
  {
    namespace swing
    {
        class Icon;
      namespace filechooser
      {
          class FileSystemView;
      }
    }
  }
}

class javax::swing::filechooser::FileSystemView : public ::java::lang::Object
{

public:
  FileSystemView();
  virtual ::java::io::File * createFileObject(::java::io::File *, ::java::lang::String *);
  virtual ::java::io::File * createFileObject(::java::lang::String *);
public: // actually protected
  virtual ::java::io::File * createFileSystemRoot(::java::io::File *);
public:
  virtual ::java::io::File * createNewFolder(::java::io::File *) = 0;
  virtual ::java::io::File * getChild(::java::io::File *, ::java::lang::String *);
  virtual ::java::io::File * getDefaultDirectory();
  virtual JArray< ::java::io::File * > * getFiles(::java::io::File *, jboolean);
  static ::javax::swing::filechooser::FileSystemView * getFileSystemView();
  virtual ::java::io::File * getHomeDirectory();
  virtual ::java::io::File * getParentDirectory(::java::io::File *);
  virtual JArray< ::java::io::File * > * getRoots();
  virtual ::java::lang::String * getSystemDisplayName(::java::io::File *);
  virtual ::javax::swing::Icon * getSystemIcon(::java::io::File *);
  virtual ::java::lang::String * getSystemTypeDescription(::java::io::File *);
  virtual jboolean isComputerNode(::java::io::File *);
  virtual jboolean isDrive(::java::io::File *);
  virtual jboolean isFileSystem(::java::io::File *);
  virtual jboolean isFileSystemRoot(::java::io::File *);
  virtual jboolean isFloppyDrive(::java::io::File *);
  virtual jboolean isHiddenFile(::java::io::File *);
  virtual jboolean isParent(::java::io::File *, ::java::io::File *);
  virtual jboolean isRoot(::java::io::File *);
  virtual ::java::lang::Boolean * isTraversable(::java::io::File *);
private:
  static ::javax::swing::filechooser::FileSystemView * defaultFileSystemView;
public:
  static ::java::lang::Class class$;
};

#endif // __javax_swing_filechooser_FileSystemView__
