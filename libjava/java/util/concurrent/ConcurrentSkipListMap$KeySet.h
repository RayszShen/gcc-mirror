
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_concurrent_ConcurrentSkipListMap$KeySet__
#define __java_util_concurrent_ConcurrentSkipListMap$KeySet__

#pragma interface

#include <java/util/AbstractSet.h>
#include <gcj/array.h>


class java::util::concurrent::ConcurrentSkipListMap$KeySet : public ::java::util::AbstractSet
{

public: // actually package-private
  ConcurrentSkipListMap$KeySet(::java::util::concurrent::ConcurrentNavigableMap *);
public:
  jint size();
  jboolean isEmpty();
  jboolean contains(::java::lang::Object *);
  jboolean remove(::java::lang::Object *);
  void clear();
  ::java::lang::Object * lower(::java::lang::Object *);
  ::java::lang::Object * floor(::java::lang::Object *);
  ::java::lang::Object * ceiling(::java::lang::Object *);
  ::java::lang::Object * higher(::java::lang::Object *);
  ::java::util::Comparator * comparator();
  ::java::lang::Object * first();
  ::java::lang::Object * last();
  ::java::lang::Object * pollFirst();
  ::java::lang::Object * pollLast();
  ::java::util::Iterator * iterator();
  jboolean equals(::java::lang::Object *);
  JArray< ::java::lang::Object * > * toArray();
  JArray< ::java::lang::Object * > * toArray(JArray< ::java::lang::Object * > *);
  ::java::util::Iterator * descendingIterator();
  ::java::util::NavigableSet * subSet(::java::lang::Object *, jboolean, ::java::lang::Object *, jboolean);
  ::java::util::NavigableSet * headSet(::java::lang::Object *, jboolean);
  ::java::util::NavigableSet * tailSet(::java::lang::Object *, jboolean);
  ::java::util::NavigableSet * target$subSet(::java::lang::Object *, ::java::lang::Object *);
  ::java::util::NavigableSet * target$headSet(::java::lang::Object *);
  ::java::util::NavigableSet * target$tailSet(::java::lang::Object *);
  ::java::util::NavigableSet * descendingSet();
  ::java::util::SortedSet * subSet(::java::lang::Object *, ::java::lang::Object *);
  ::java::util::SortedSet * tailSet(::java::lang::Object *);
  ::java::util::SortedSet * headSet(::java::lang::Object *);
private:
  ::java::util::concurrent::ConcurrentNavigableMap * __attribute__((aligned(__alignof__( ::java::util::AbstractSet)))) m;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_concurrent_ConcurrentSkipListMap$KeySet__
