# Makefile.am -- Backtrace Makefile.
# Copyright (C) 2012-2019 Free Software Foundation, Inc.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:

#     (1) Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.

#     (2) Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.

#     (3) The name of the author may not be used to
#     endorse or promote products derived from this software without
#     specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

ACLOCAL_AMFLAGS = -I .. -I ../config

AM_CPPFLAGS = -I $(top_srcdir)/../include -I $(top_srcdir)/../libgcc \
	-I ../libgcc

AM_CFLAGS = $(EXTRA_FLAGS) $(WARN_FLAGS) $(PIC_FLAG)

noinst_LTLIBRARIES = libbacktrace.la

libbacktrace_la_SOURCES = \
	backtrace.h \
	atomic.c \
	dwarf.c \
	fileline.c \
	internal.h \
	posix.c \
	print.c \
	sort.c \
	state.c

BACKTRACE_FILES = \
	backtrace.c \
	simple.c \
	nounwind.c

FORMAT_FILES = \
	elf.c \
	pecoff.c \
	unknown.c \
	xcoff.c

VIEW_FILES = \
	read.c \
	mmapio.c

ALLOC_FILES = \
	alloc.c \
	mmap.c

EXTRA_libbacktrace_la_SOURCES = \
	$(BACKTRACE_FILES) \
	$(FORMAT_FILES) \
	$(VIEW_FILES) \
	$(ALLOC_FILES)

libbacktrace_la_LIBADD = \
	$(BACKTRACE_FILE) \
	$(FORMAT_FILE) \
	$(VIEW_FILE) \
	$(ALLOC_FILE)

libbacktrace_la_DEPENDENCIES = $(libbacktrace_la_LIBADD)

# Testsuite.

# Add test to this variable, if you want it to be build.
check_PROGRAMS =

# Add test to this variable, if you want it to be run.
TESTS =

# Add test to this variable, if you want it to be build and run.
BUILDTESTS =

if NATIVE
check_LTLIBRARIES = libbacktrace_alloc.la

libbacktrace_alloc_la_SOURCES = $(libbacktrace_la_SOURCES)
libbacktrace_alloc_la_LIBADD = $(BACKTRACE_FILE) $(FORMAT_FILE) read.lo alloc.lo

libbacktrace_alloc_la_DEPENDENCIES = $(libbacktrace_alloc_la_LIBADD)

check_LTLIBRARIES += libbacktrace_noformat.la

libbacktrace_noformat_la_SOURCES = $(libbacktrace_la_SOURCES)
libbacktrace_noformat_la_LIBADD = $(BACKTRACE_FILE) $(VIEW_FILE) $(ALLOC_FILE)

libbacktrace_noformat_la_DEPENDENCIES = $(libbacktrace_noformat_la_LIBADD)

if HAVE_ELF
if HAVE_OBJCOPY_DEBUGLINK

TEST_BUILD_ID_DIR=$(abs_builddir)/usr/lib/debug/.build-id/

check_LTLIBRARIES += libbacktrace_elf_for_test.la

libbacktrace_elf_for_test_la_SOURCES = $(libbacktrace_la_SOURCES)
libbacktrace_elf_for_test_la_LIBADD = $(BACKTRACE_FILE) elf_for_test.lo \
	$(VIEW_FILE) $(ALLOC_FILE)

elf_for_test.c: elf.c
	SEARCH='^#define SYSTEM_BUILD_ID_DIR.*$$'; \
	REPLACE="#define SYSTEM_BUILD_ID_DIR \"$(TEST_BUILD_ID_DIR)\""; \
	$(SED) "s%$$SEARCH%$$REPLACE%" \
		$< \
		> $@.tmp
	mv $@.tmp $@

endif HAVE_OBJCOPY_DEBUGLINK
endif HAVE_ELF

elf_%.c: elf.c
	SEARCH='#error "Unknown BACKTRACE_ELF_SIZE"'; \
	REPLACE='#undef BACKTRACE_ELF_SIZE\
	#define BACKTRACE_ELF_SIZE'; \
	$(SED) "s/^$$SEARCH\$$/$$REPLACE $*/" \
		$< \
		> $@.tmp
	mv $@.tmp $@

xcoff_%.c: xcoff.c
	SEARCH='#error "Unknown BACKTRACE_XCOFF_SIZE"'; \
	REPLACE='#undef BACKTRACE_XCOFF_SIZE\
	#define BACKTRACE_XCOFF_SIZE'; \
	$(SED) "s/^$$SEARCH\$$/$$REPLACE $*/" \
		$< \
		> $@.tmp
	mv $@.tmp $@

test_elf_32_SOURCES = test_format.c testlib.c
test_elf_32_LDADD = libbacktrace_noformat.la elf_32.lo

BUILDTESTS += test_elf_32

test_elf_64_SOURCES = test_format.c testlib.c
test_elf_64_LDADD = libbacktrace_noformat.la elf_64.lo

BUILDTESTS += test_elf_64

test_xcoff_32_SOURCES = test_format.c testlib.c
test_xcoff_32_LDADD = libbacktrace_noformat.la xcoff_32.lo

BUILDTESTS += test_xcoff_32

test_xcoff_64_SOURCES = test_format.c testlib.c
test_xcoff_64_LDADD = libbacktrace_noformat.la xcoff_64.lo

BUILDTESTS += test_xcoff_64

test_pecoff_SOURCES = test_format.c testlib.c
test_pecoff_LDADD = libbacktrace_noformat.la pecoff.lo

BUILDTESTS += test_pecoff

test_unknown_SOURCES = test_format.c testlib.c
test_unknown_LDADD = libbacktrace_noformat.la unknown.lo

BUILDTESTS += test_unknown

unittest_SOURCES = unittest.c testlib.c
unittest_LDADD = libbacktrace.la

BUILDTESTS += unittest

unittest_alloc_SOURCES = $(unittest_SOURCES)
unittest_alloc_LDADD = libbacktrace_alloc.la

BUILDTESTS += unittest_alloc

check_LTLIBRARIES += libbacktrace_instrumented_alloc.la

libbacktrace_instrumented_alloc_la_SOURCES = $(libbacktrace_la_SOURCES)
libbacktrace_instrumented_alloc_la_LIBADD = $(BACKTRACE_FILE) $(FORMAT_FILE) \
	read.lo instrumented_alloc.lo

libbacktrace_instrumented_alloc_la_DEPENDENCIES = \
	$(libbacktrace_instrumented_alloc_la_LIBADD)

instrumented_alloc.lo: alloc.c

allocfail_SOURCES = allocfail.c testlib.c
allocfail_LDADD = libbacktrace_instrumented_alloc.la

check_PROGRAMS += allocfail

allocfail.sh: allocfail

TESTS += allocfail.sh

if HAVE_ELF
if HAVE_OBJCOPY_DEBUGLINK

b2test_SOURCES = $(btest_SOURCES)
b2test_CFLAGS = $(btest_CFLAGS)
b2test_LDFLAGS = -Wl,--build-id
b2test_LDADD = libbacktrace_elf_for_test.la

check_PROGRAMS += b2test
TESTS += b2test_buildid

if HAVE_DWZ

b3test_SOURCES = $(btest_SOURCES)
b3test_CFLAGS = $(btest_CFLAGS)
b3test_LDFLAGS = -Wl,--build-id
b3test_LDADD = libbacktrace_elf_for_test.la

check_PROGRAMS += b3test
TESTS += b3test_dwz_buildid

endif HAVE_DWZ

endif HAVE_OBJCOPY_DEBUGLINK
endif HAVE_ELF

btest_SOURCES = btest.c testlib.c
btest_CFLAGS = $(AM_CFLAGS) -g -O
btest_LDADD = libbacktrace.la

BUILDTESTS += btest

if HAVE_ELF

btest_lto_SOURCES = btest.c testlib.c
btest_lto_CFLAGS = $(AM_CFLAGS) -g -O -flto
btest_lto_LDADD = libbacktrace.la

BUILDTESTS += btest_lto

endif HAVE_ELF

btest_alloc_SOURCES = $(btest_SOURCES)
btest_alloc_CFLAGS = $(btest_CFLAGS)
btest_alloc_LDADD = libbacktrace_alloc.la

BUILDTESTS += btest_alloc

if HAVE_DWZ

%_dwz: %
	rm -f $@ $@_common.debug
	cp $< $@_1
	cp $< $@_2
	$(DWZ) -m $@_common.debug $@_1 $@_2
	rm -f $@_2
	mv $@_1 $@

TESTS += btest_dwz

if HAVE_OBJCOPY_DEBUGLINK

TESTS += btest_dwz_gnudebuglink

endif HAVE_OBJCOPY_DEBUGLINK

endif HAVE_DWZ

stest_SOURCES = stest.c
stest_LDADD = libbacktrace.la

BUILDTESTS += stest

stest_alloc_SOURCES = $(stest_SOURCES)
stest_alloc_LDADD = libbacktrace_alloc.la

BUILDTESTS += stest_alloc

if HAVE_ELF

ztest_SOURCES = ztest.c testlib.c
ztest_CFLAGS = -DSRCDIR=\"$(srcdir)\"
ztest_LDADD = libbacktrace.la
ztest_alloc_LDADD = libbacktrace_alloc.la

if HAVE_ZLIB
ztest_LDADD += -lz
ztest_alloc_LDADD += -lz
endif
ztest_LDADD += $(CLOCK_GETTIME_LINK)
ztest_alloc_LDADD += $(CLOCK_GETTIME_LINK)

BUILDTESTS += ztest

ztest_alloc_SOURCES = $(ztest_SOURCES)
ztest_alloc_CFLAGS = $(ztest_CFLAGS)

BUILDTESTS += ztest_alloc

endif HAVE_ELF

edtest_SOURCES = edtest.c edtest2_build.c testlib.c
edtest_LDADD = libbacktrace.la

BUILDTESTS += edtest

edtest_alloc_SOURCES = $(edtest_SOURCES)
edtest_alloc_LDADD = libbacktrace_alloc.la

BUILDTESTS += edtest_alloc

edtest2_build.c: gen_edtest2_build; @true
gen_edtest2_build: $(srcdir)/edtest2.c
	cat $(srcdir)/edtest2.c > tmp-edtest2_build.c
	$(SHELL) $(srcdir)/../move-if-change tmp-edtest2_build.c edtest2_build.c
	echo timestamp > $@

if HAVE_PTHREAD

BUILDTESTS += ttest

ttest_SOURCES = ttest.c testlib.c
ttest_CFLAGS = $(AM_CFLAGS) -pthread
ttest_LDADD = libbacktrace.la

BUILDTESTS += ttest_alloc

ttest_alloc_SOURCES = $(ttest_SOURCES)
ttest_alloc_CFLAGS = $(ttest_CFLAGS)
ttest_alloc_LDADD = libbacktrace_alloc.la

endif HAVE_PTHREAD

if HAVE_OBJCOPY_DEBUGLINK

TESTS += btest_gnudebuglink

%_gnudebuglink: %
	$(OBJCOPY) --only-keep-debug $< $@.debug
	$(OBJCOPY) --strip-debug --add-gnu-debuglink=$@.debug $< $@

endif HAVE_OBJCOPY_DEBUGLINK

%_buildid: %
	./install-debuginfo-for-buildid.sh \
	  "$(TEST_BUILD_ID_DIR)" \
	  $<
	$(OBJCOPY) --strip-debug $< $@

if HAVE_COMPRESSED_DEBUG

ctestg_SOURCES = btest.c testlib.c
ctestg_CFLAGS = $(AM_CFLAGS) -g
ctestg_LDFLAGS = -Wl,--compress-debug-sections=zlib-gnu
ctestg_LDADD = libbacktrace.la

ctesta_SOURCES = btest.c testlib.c
ctesta_CFLAGS = $(AM_CFLAGS) -g
ctesta_LDFLAGS = -Wl,--compress-debug-sections=zlib-gabi
ctesta_LDADD = libbacktrace.la

BUILDTESTS += ctestg ctesta

ctestg_alloc_SOURCES = $(ctestg_SOURCES)
ctestg_alloc_CFLAGS = $(ctestg_CFLAGS)
ctestg_alloc_LDFLAGS = $(ctestg_LDFLAGS)
ctestg_alloc_LDADD = libbacktrace_alloc.la

ctesta_alloc_SOURCES = $(ctesta_SOURCES)
ctesta_alloc_CFLAGS = $(ctesta_CFLAGS)
ctesta_alloc_LDFLAGS = $(ctesta_LDFLAGS)
ctesta_alloc_LDADD = libbacktrace_alloc.la

BUILDTESTS += ctestg_alloc ctesta_alloc

endif

endif NATIVE

check_PROGRAMS += $(BUILDTESTS)

TESTS += $(BUILDTESTS)

# We can't use automake's automatic dependency tracking, because it
# breaks when using bootstrap-lean.  Automatic dependency tracking
# with GCC bootstrap will cause some of the objects to depend on
# header files in prev-gcc/include, e.g., stddef.h and stdarg.h.  When
# using bootstrap-lean, prev-gcc is removed after each stage.  When
# running "make install", those header files will be gone, causing the
# library to be rebuilt at install time.  That may not succeed.

# These manual dependencies do not include dependencies on unwind.h,
# even though that is part of GCC, because where to find it depends on
# whether we are being built as a host library or a target library.

INCDIR = $(top_srcdir)/../include
alloc.lo: config.h backtrace.h internal.h
backtrace.lo: config.h backtrace.h internal.h
btest.lo: (INCDIR)/filenames.h backtrace.h backtrace-supported.h
dwarf.lo: config.h $(INCDIR)/dwarf2.h $(INCDIR)/dwarf2.def \
	$(INCDIR)/filenames.h backtrace.h internal.h
elf.lo: config.h backtrace.h internal.h
fileline.lo: config.h backtrace.h internal.h
mmap.lo: config.h backtrace.h internal.h
mmapio.lo: config.h backtrace.h internal.h
nounwind.lo: config.h internal.h
pecoff.lo: config.h backtrace.h internal.h
posix.lo: config.h backtrace.h internal.h
print.lo: config.h backtrace.h internal.h
read.lo: config.h backtrace.h internal.h
simple.lo: config.h backtrace.h internal.h
sort.lo: config.h backtrace.h internal.h
stest.lo: config.h backtrace.h internal.h
state.lo: config.h backtrace.h backtrace-supported.h internal.h
unknown.lo: config.h backtrace.h internal.h
xcoff.lo: config.h backtrace.h internal.h

include $(top_srcdir)/../multilib.am
