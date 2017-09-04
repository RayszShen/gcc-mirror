dnl Copyright (C) 1994-2017 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

dnl
dnl LIBGUPC_GCC_TLS_SUPPORTED
dnl
AC_DEFUN([LIBGUPC_GCC_TLS_SUPPORTED], [
AC_CACHE_CHECK([whether the GCC __threads extension is supported.],
               upc_cv_gcc_tls_supported,
[SAVE_LIBS="$LIBS"
LIBS="$LIBS -lpthread"
AC_TRY_RUN(
changequote(<<,>>)dnl
<<#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>

#define NTHREADS 5

pthread_t p[NTHREADS];

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

__thread long tlocal = 0;

void *
thread_start (void *arg)
{
  int id = *((int *)arg);
  int *return_val = malloc(sizeof(int));
  if (pthread_mutex_lock (&lock))
    { exit (2); }
  /* if the value is truly thread-local, this assignment
     will yield the value 1, for each thread. If tlocal
     is instead a process global static value then tlocal
     will be incremented by each thread, and its final
     value will be the number of threads. */
  tlocal += 1;
  if (pthread_mutex_unlock (&lock))
    { exit (2); }
  *return_val = tlocal;
  return return_val;
}

int
main()
{
  int i;
  for (i = 0; i < NTHREADS; ++i)
    {
      int *id = (int *)malloc(sizeof(int));
      *id = i;
      if (pthread_create(&p[i], NULL, thread_start, id))
        { exit (2); }
    }
  for (i = 0; i < NTHREADS; ++i)
    {
      int *rc;
      if (pthread_join (p[i], (void **)&rc))
        { exit (2); }
      if (*rc != 1)
        { exit (1); }
    }
  return 0;
}
>>,
changequote([, ])dnl
  [upc_cv_gcc_tls_supported="yes"],
  [upc_cv_gcc_tls_supported="no"],
  [upc_cv_gcc_tls_supported="no"])
  LIBS="$SAVE_LIBS"]
)
dnl if test "$upc_cv_gcc_tls_supported"x = "yes"x ; then
dnl   AC_DEFINE([HAVE_GCC_TLS_SUPPORT], 1)
dnl fi
])

dnl ----------------------------------------------------------------------
dnl The following (to the end of file) is copied from libgomp.
dnl ----------------------------------------------------------------------
dnl This whole bit snagged from libgfortran.

dnl Check whether the target supports __sync_*_compare_and_swap.
AC_DEFUN([LIBGUPC_CHECK_SYNC_BUILTINS], [
  AC_CACHE_CHECK([whether the target supports __sync_*_compare_and_swap],
		 libgupc_cv_have_sync_builtins, [
  AC_TRY_LINK([], [int foo; (void) __sync_val_compare_and_swap(&foo, 0, 1);],
	      libgupc_cv_have_sync_builtins=yes,
	      libgupc_cv_have_sync_builtins=no)])
  if test $libgupc_cv_have_sync_builtins = yes; then
    AC_DEFINE(HAVE_SYNC_BUILTINS, 1,
	      [Define to 1 if the target supports __sync_*_compare_and_swap.])
  fi])

dnl Check whether the target supports hidden visibility.
AC_DEFUN([LIBGUPC_CHECK_ATTRIBUTE_VISIBILITY], [
  AC_CACHE_CHECK([whether the target supports hidden visibility],
		 libgupc_cv_have_attribute_visibility, [
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Werror"
  AC_TRY_COMPILE([void __attribute__((visibility("hidden"))) foo(void) { }],
		 [], libgupc_cv_have_attribute_visibility=yes,
		 libgupc_cv_have_attribute_visibility=no)
  CFLAGS="$save_CFLAGS"])
  if test $libgupc_cv_have_attribute_visibility = yes; then
    AC_DEFINE(HAVE_ATTRIBUTE_VISIBILITY, 1,
      [Define to 1 if the target supports __attribute__((visibility(...))).])
  fi])

dnl Check whether the target supports dllexport
AC_DEFUN([LIBGUPC_CHECK_ATTRIBUTE_DLLEXPORT], [
  AC_CACHE_CHECK([whether the target supports dllexport],
		 libgupc_cv_have_attribute_dllexport, [
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Werror"
  AC_TRY_COMPILE([void __attribute__((dllexport)) foo(void) { }],
		 [], libgupc_cv_have_attribute_dllexport=yes,
		 libgupc_cv_have_attribute_dllexport=no)
  CFLAGS="$save_CFLAGS"])
  if test $libgupc_cv_have_attribute_dllexport = yes; then
    AC_DEFINE(HAVE_ATTRIBUTE_DLLEXPORT, 1,
      [Define to 1 if the target supports __attribute__((dllexport)).])
  fi])

dnl Check whether the target supports symbol aliases.
AC_DEFUN([LIBGUPC_CHECK_ATTRIBUTE_ALIAS], [
  AC_CACHE_CHECK([whether the target supports symbol aliases],
		 libgupc_cv_have_attribute_alias, [
  AC_TRY_LINK([
void foo(void) { }
extern void bar(void) __attribute__((alias("foo")));],
    [bar();], libgupc_cv_have_attribute_alias=yes, libgupc_cv_have_attribute_alias=no)])
  if test $libgupc_cv_have_attribute_alias = yes; then
    AC_DEFINE(HAVE_ATTRIBUTE_ALIAS, 1,
      [Define to 1 if the target supports __attribute__((alias(...))).])
  fi])

sinclude(../libtool.m4)
dnl The lines below arrange for aclocal not to bring an installed
dnl libtool.m4 into aclocal.m4, while still arranging for automake to
dnl add a definition of LIBTOOL to Makefile.in.
ifelse(,,,[AC_SUBST(LIBTOOL)
AC_DEFUN([AM_PROG_LIBTOOL])
AC_DEFUN([AC_LIBTOOL_DLOPEN])
AC_DEFUN([AC_PROG_LD])
])

dnl ----------------------------------------------------------------------
dnl This whole bit snagged from libstdc++-v3.

dnl
dnl LIBGUPC_ENABLE
dnl    (FEATURE, DEFAULT, HELP-ARG, HELP-STRING)
dnl    (FEATURE, DEFAULT, HELP-ARG, HELP-STRING, permit a|b|c)
dnl    (FEATURE, DEFAULT, HELP-ARG, HELP-STRING, SHELL-CODE-HANDLER)
dnl
dnl See docs/html/17_intro/configury.html#enable for documentation.
dnl
m4_define([LIBGUPC_ENABLE],[dnl
m4_define([_g_switch],[--enable-$1])dnl
m4_define([_g_help],[AC_HELP_STRING(_g_switch$3,[$4 @<:@default=$2@:>@])])dnl
 AC_ARG_ENABLE($1,_g_help,
  m4_bmatch([$5],
   [^permit ],
     [[
      case "$enableval" in
       m4_bpatsubst([$5],[permit ])) ;;
       *) AC_MSG_ERROR(Unknown argument to enable/disable $1) ;;
          dnl Idea for future:  generate a URL pointing to
          dnl "onlinedocs/configopts.html#whatever"
      esac
     ]],
   [^$],
     [[
      case "$enableval" in
       yes|no) ;;
       *) AC_MSG_ERROR(Argument to enable/disable $1 must be yes or no) ;;
      esac
     ]],
   [[$5]]),
  [enable_]m4_bpatsubst([$1],-,_)[=][$2])
m4_undefine([_g_switch])dnl
m4_undefine([_g_help])dnl
])


dnl
dnl If GNU ld is in use, check to see if tricky linker opts can be used.  If
dnl the native linker is in use, all variables will be defined to something
dnl safe (like an empty string).
dnl
dnl Defines:
dnl  SECTION_LDFLAGS='-Wl,--gc-sections' if possible
dnl  OPT_LDFLAGS='-Wl,-O1' if possible
dnl  LD (as a side effect of testing)
dnl Sets:
dnl  with_gnu_ld
dnl  libgupc_ld_is_gold (possibly)
dnl  libgupc_gnu_ld_version (possibly)
dnl
dnl The last will be a single integer, e.g., version 1.23.45.0.67.89 will
dnl set libgupc_gnu_ld_version to 12345.  Zeros cause problems.
dnl
AC_DEFUN([LIBGUPC_CHECK_LINKER_FEATURES], [
  # If we're not using GNU ld, then there's no point in even trying these
  # tests.  Check for that first.  We should have already tested for gld
  # by now (in libtool), but require it now just to be safe...
  test -z "$SECTION_LDFLAGS" && SECTION_LDFLAGS=''
  test -z "$OPT_LDFLAGS" && OPT_LDFLAGS=''
  AC_REQUIRE([AC_PROG_LD])
  AC_REQUIRE([AC_PROG_AWK])

  # The name set by libtool depends on the version of libtool.  Shame on us
  # for depending on an impl detail, but c'est la vie.  Older versions used
  # ac_cv_prog_gnu_ld, but now it's lt_cv_prog_gnu_ld, and is copied back on
  # top of with_gnu_ld (which is also set by --with-gnu-ld, so that actually
  # makes sense).  We'll test with_gnu_ld everywhere else, so if that isn't
  # set (hence we're using an older libtool), then set it.
  if test x${with_gnu_ld+set} != xset; then
    if test x${ac_cv_prog_gnu_ld+set} != xset; then
      # We got through "ac_require(ac_prog_ld)" and still not set?  Huh?
      with_gnu_ld=no
    else
      with_gnu_ld=$ac_cv_prog_gnu_ld
    fi
  fi

  # Start by getting the version number.  I think the libtool test already
  # does some of this, but throws away the result.
  libgupc_ld_is_gold=no
  if $LD --version 2>/dev/null | grep 'GNU gold'> /dev/null 2>&1; then
    libgupc_ld_is_gold=yes
  fi
  changequote(,)
  ldver=`$LD --version 2>/dev/null |
         sed -e 's/GNU gold /GNU ld /;s/GNU ld version /GNU ld /;s/GNU ld ([^)]*) /GNU ld /;s/GNU ld \([0-9.][0-9.]*\).*/\1/; q'`
  changequote([,])
  libgupc_gnu_ld_version=`echo $ldver | \
         $AWK -F. '{ if (NF<3) [$]3=0; print ([$]1*100+[$]2)*100+[$]3 }'`

  # Set --gc-sections.
  if test "$with_gnu_ld" = "notbroken"; then
    # GNU ld it is!  Joy and bunny rabbits!

    # All these tests are for C++; save the language and the compiler flags.
    # Need to do this so that g++ won't try to link in libstdc++
    ac_test_CFLAGS="${CFLAGS+set}"
    ac_save_CFLAGS="$CFLAGS"
    CFLAGS='-x c++  -Wl,--gc-sections'

    # Check for -Wl,--gc-sections
    # XXX This test is broken at the moment, as symbols required for linking
    # are now in libsupc++ (not built yet).  In addition, this test has
    # cored on solaris in the past.  In addition, --gc-sections doesn't
    # really work at the moment (keeps on discarding used sections, first
    # .eh_frame and now some of the glibc sections for iconv).
    # Bzzzzt.  Thanks for playing, maybe next time.
    AC_MSG_CHECKING([for ld that supports -Wl,--gc-sections])
    AC_TRY_RUN([
     int main(void)
     {
       try { throw 1; }
       catch (...) { };
       return 0;
     }
    ], [ac_sectionLDflags=yes],[ac_sectionLDflags=no], [ac_sectionLDflags=yes])
    if test "$ac_test_CFLAGS" = set; then
      CFLAGS="$ac_save_CFLAGS"
    else
      # this is the suspicious part
      CFLAGS=''
    fi
    if test "$ac_sectionLDflags" = "yes"; then
      SECTION_LDFLAGS="-Wl,--gc-sections $SECTION_LDFLAGS"
    fi
    AC_MSG_RESULT($ac_sectionLDflags)
  fi

  # Set linker optimization flags.
  if test x"$with_gnu_ld" = x"yes"; then
    OPT_LDFLAGS="-Wl,-O1 $OPT_LDFLAGS"
  fi

  AC_SUBST(SECTION_LDFLAGS)
  AC_SUBST(OPT_LDFLAGS)
])


dnl
dnl Add version tags to symbols in shared library (or not), additionally
dnl marking other symbols as private/local (or not).
dnl
dnl --enable-symvers=style adds a version script to the linker call when
dnl       creating the shared library.  The choice of version script is
dnl       controlled by 'style'.
dnl --disable-symvers does not.
dnl  +  Usage:  LIBGUPC_ENABLE_SYMVERS[(DEFAULT)]
dnl       Where DEFAULT is either 'yes' or 'no'.  Passing `yes' tries to
dnl       choose a default style based on linker characteristics.  Passing
dnl       'no' disables versioning.
dnl
AC_DEFUN([LIBGUPC_ENABLE_SYMVERS], [

LIBGUPC_ENABLE(symvers,yes,[=STYLE],
  [enables symbol versioning of the shared library],
  [permit yes|no|gnu])

# If we never went through the LIBGUPC_CHECK_LINKER_FEATURES macro, then we
# don't know enough about $LD to do tricks...
AC_REQUIRE([LIBGUPC_CHECK_LINKER_FEATURES])
# FIXME  The following test is too strict, in theory.
if test $enable_shared = no ||
        test "x$LD" = x ||
        test x$libgupc_gnu_ld_version = x; then
  enable_symvers=no
fi

# Check to see if libgcc_s exists, indicating that shared libgcc is possible.
if test $enable_symvers != no; then
  AC_MSG_CHECKING([for shared libgcc])
  ac_save_CFLAGS="$CFLAGS"
  CFLAGS=' -lgcc_s'
  AC_TRY_LINK(, [return 0;], libgupc_shared_libgcc=yes, libgupc_shared_libgcc=no)
  CFLAGS="$ac_save_CFLAGS"
  if test $libgupc_shared_libgcc = no; then
    cat > conftest.c <<EOF
int main (void) { return 0; }
EOF
changequote(,)dnl
    libgupc_libgcc_s_suffix=`${CC-cc} $CFLAGS $CPPFLAGS $LDFLAGS \
			     -shared -shared-libgcc -o conftest.so \
			     conftest.c -v 2>&1 >/dev/null \
			     | sed -n 's/^.* -lgcc_s\([^ ]*\) .*$/\1/p'`
changequote([,])dnl
    rm -f conftest.c conftest.so
    if test x${libgupc_libgcc_s_suffix+set} = xset; then
      CFLAGS=" -lgcc_s$libgupc_libgcc_s_suffix"
      AC_TRY_LINK(, [return 0;], libgupc_shared_libgcc=yes)
      CFLAGS="$ac_save_CFLAGS"
    fi
  fi
  AC_MSG_RESULT($libgupc_shared_libgcc)
fi

# For GNU ld, we need at least this version.  The format is described in
# LIBGUPC_CHECK_LINKER_FEATURES above.
libgupc_min_gnu_ld_version=21400
# XXXXXXXXXXX libgupc_gnu_ld_version=21390

# Check to see if unspecified "yes" value can win, given results above.
# Change "yes" into either "no" or a style name.
if test $enable_symvers = yes; then
  if test $with_gnu_ld = yes &&
     test $libgupc_shared_libgcc = yes;
  then
    if test $libgupc_gnu_ld_version -ge $libgupc_min_gnu_ld_version ; then
      enable_symvers=gnu
    elif test $libgupc_ld_is_gold = yes ; then
      enable_symvers=gnu
    else
      # The right tools, the right setup, but too old.  Fallbacks?
      AC_MSG_WARN(=== Linker version $libgupc_gnu_ld_version is too old for)
      AC_MSG_WARN(=== full symbol versioning support in this release of GCC.)
      AC_MSG_WARN(=== You would need to upgrade your binutils to version)
      AC_MSG_WARN(=== $libgupc_min_gnu_ld_version or later and rebuild GCC.)
      if test $libgupc_gnu_ld_version -ge 21200 ; then
        # Globbing fix is present, proper block support is not.
        dnl AC_MSG_WARN([=== Dude, you are soooo close.  Maybe we can fake it.])
        dnl enable_symvers=???
        AC_MSG_WARN([=== Symbol versioning will be disabled.])
        enable_symvers=no
      else
        # 2.11 or older.
        AC_MSG_WARN([=== Symbol versioning will be disabled.])
        enable_symvers=no
      fi
    fi
  else
    # just fail for now
    AC_MSG_WARN([=== You have requested some kind of symbol versioning, but])
    AC_MSG_WARN([=== either you are not using a supported linker, or you are])
    AC_MSG_WARN([=== not building a shared libgcc_s (which is required).])
    AC_MSG_WARN([=== Symbol versioning will be disabled.])
    enable_symvers=no
  fi
fi

AC_CACHE_CHECK([whether the target supports .symver directive],
	       libgupc_cv_have_as_symver_directive, [
  AC_TRY_COMPILE([void foo (void); __asm (".symver foo, bar@SYMVER");],
		 [], libgupc_cv_have_as_symver_directive=yes,
		 libgupc_cv_have_as_symver_directive=no)])
if test $libgupc_cv_have_as_symver_directive = yes; then
  AC_DEFINE(HAVE_AS_SYMVER_DIRECTIVE, 1,
    [Define to 1 if the target assembler supports .symver directive.])
fi

AM_CONDITIONAL(LIBGUPC_BUILD_VERSIONED_SHLIB, test $enable_symvers != no)
AC_MSG_NOTICE(versioning on shared library symbols is $enable_symvers)
])
