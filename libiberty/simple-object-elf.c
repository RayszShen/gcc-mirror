/* simple-object-elf.c -- routines to manipulate ELF object files.
   Copyright (C) 2010-2017 Free Software Foundation, Inc.
   Written by Ian Lance Taylor, Google.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 51 Franklin Street - Fifth Floor,
Boston, MA 02110-1301, USA.  */

#include "config.h"
#include "libiberty.h"
#include "simple-object.h"

#include <errno.h>
#include <stddef.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "simple-object-common.h"

/* ELF structures and constants.  */

/* 32-bit ELF file header.  */

typedef struct {
  unsigned char	e_ident[16];		/* ELF "magic number" */
  unsigned char	e_type[2];		/* Identifies object file type */
  unsigned char	e_machine[2];		/* Specifies required architecture */
  unsigned char	e_version[4];		/* Identifies object file version */
  unsigned char	e_entry[4];		/* Entry point virtual address */
  unsigned char	e_phoff[4];		/* Program header table file offset */
  unsigned char	e_shoff[4];		/* Section header table file offset */
  unsigned char	e_flags[4];		/* Processor-specific flags */
  unsigned char	e_ehsize[2];		/* ELF header size in bytes */
  unsigned char	e_phentsize[2];		/* Program header table entry size */
  unsigned char	e_phnum[2];		/* Program header table entry count */
  unsigned char	e_shentsize[2];		/* Section header table entry size */
  unsigned char	e_shnum[2];		/* Section header table entry count */
  unsigned char	e_shstrndx[2];		/* Section header string table index */
} Elf32_External_Ehdr;

/* 64-bit ELF file header.  */

typedef struct {
  unsigned char	e_ident[16];		/* ELF "magic number" */
  unsigned char	e_type[2];		/* Identifies object file type */
  unsigned char	e_machine[2];		/* Specifies required architecture */
  unsigned char	e_version[4];		/* Identifies object file version */
  unsigned char	e_entry[8];		/* Entry point virtual address */
  unsigned char	e_phoff[8];		/* Program header table file offset */
  unsigned char	e_shoff[8];		/* Section header table file offset */
  unsigned char	e_flags[4];		/* Processor-specific flags */
  unsigned char	e_ehsize[2];		/* ELF header size in bytes */
  unsigned char	e_phentsize[2];		/* Program header table entry size */
  unsigned char	e_phnum[2];		/* Program header table entry count */
  unsigned char	e_shentsize[2];		/* Section header table entry size */
  unsigned char	e_shnum[2];		/* Section header table entry count */
  unsigned char	e_shstrndx[2];		/* Section header string table index */
} Elf64_External_Ehdr;

/* Indexes and values in e_ident field of Ehdr.  */

#define EI_MAG0		0	/* File identification byte 0 index */
#define ELFMAG0		   0x7F	/* Magic number byte 0 */

#define EI_MAG1		1	/* File identification byte 1 index */
#define ELFMAG1		    'E'	/* Magic number byte 1 */

#define EI_MAG2		2	/* File identification byte 2 index */
#define ELFMAG2		    'L'	/* Magic number byte 2 */

#define EI_MAG3		3	/* File identification byte 3 index */
#define ELFMAG3		    'F'	/* Magic number byte 3 */

#define EI_CLASS	4	/* File class */
#define ELFCLASSNONE	      0	/* Invalid class */
#define ELFCLASS32	      1	/* 32-bit objects */
#define ELFCLASS64	      2	/* 64-bit objects */

#define EI_DATA		5	/* Data encoding */
#define ELFDATANONE	      0	/* Invalid data encoding */
#define ELFDATA2LSB	      1	/* 2's complement, little endian */
#define ELFDATA2MSB	      2	/* 2's complement, big endian */

#define EI_VERSION	6	/* File version */
#define EV_CURRENT	1		/* Current version */

#define EI_OSABI	7	/* Operating System/ABI indication */

/* Values for e_type field of Ehdr.  */

#define ET_REL		1	/* Relocatable file */

/* Values for e_machine field of Ehdr.  */

#define EM_SPARC	  2	/* SUN SPARC */
#define EM_SPARC32PLUS	 18	/* Sun's "v8plus" */

/* Special section index values.  */

#define SHN_LORESERVE	0xFF00		/* Begin range of reserved indices */
#define SHN_XINDEX	0xFFFF		/* Section index is held elsewhere */

/* 32-bit ELF program header.  */

typedef struct {
  unsigned char	p_type[4];		/* Identifies program segment type */
  unsigned char	p_offset[4];		/* Segment file offset */
  unsigned char	p_vaddr[4];		/* Segment virtual address */
  unsigned char	p_paddr[4];		/* Segment physical address */
  unsigned char	p_filesz[4];		/* Segment size in file */
  unsigned char	p_memsz[4];		/* Segment size in memory */
  unsigned char	p_flags[4];		/* Segment flags */
  unsigned char	p_align[4];		/* Segment alignment, file & memory */
} Elf32_External_Phdr;

/* 64-bit ELF program header.  */

typedef struct {
  unsigned char	p_type[4];		/* Identifies program segment type */
  unsigned char	p_flags[4];		/* Segment flags */
  unsigned char	p_offset[8];		/* Segment file offset */
  unsigned char	p_vaddr[8];		/* Segment virtual address */
  unsigned char	p_paddr[8];		/* Segment physical address */
  unsigned char	p_filesz[8];		/* Segment size in file */
  unsigned char	p_memsz[8];		/* Segment size in memory */
  unsigned char	p_align[8];		/* Segment alignment, file & memory */
} Elf64_External_Phdr;

/* 32-bit ELF section header */

typedef struct {
  unsigned char	sh_name[4];		/* Section name, index in string tbl */
  unsigned char	sh_type[4];		/* Type of section */
  unsigned char	sh_flags[4];		/* Miscellaneous section attributes */
  unsigned char	sh_addr[4];		/* Section virtual addr at execution */
  unsigned char	sh_offset[4];		/* Section file offset */
  unsigned char	sh_size[4];		/* Size of section in bytes */
  unsigned char	sh_link[4];		/* Index of another section */
  unsigned char	sh_info[4];		/* Additional section information */
  unsigned char	sh_addralign[4];	/* Section alignment */
  unsigned char	sh_entsize[4];		/* Entry size if section holds table */
} Elf32_External_Shdr;

/* 64-bit ELF section header.  */

typedef struct {
  unsigned char	sh_name[4];		/* Section name, index in string tbl */
  unsigned char	sh_type[4];		/* Type of section */
  unsigned char	sh_flags[8];		/* Miscellaneous section attributes */
  unsigned char	sh_addr[8];		/* Section virtual addr at execution */
  unsigned char	sh_offset[8];		/* Section file offset */
  unsigned char	sh_size[8];		/* Size of section in bytes */
  unsigned char	sh_link[4];		/* Index of another section */
  unsigned char	sh_info[4];		/* Additional section information */
  unsigned char	sh_addralign[8];	/* Section alignment */
  unsigned char	sh_entsize[8];		/* Entry size if section holds table */
} Elf64_External_Shdr;

/* Values for sh_type field.  */

#define SHT_PROGBITS	1		/* Program data */
#define SHT_STRTAB	3		/* A string table */

/* Functions to fetch and store different ELF types, depending on the
   endianness and size.  */

struct elf_type_functions
{
  unsigned short (*fetch_Elf_Half) (const unsigned char *);
  unsigned int (*fetch_Elf_Word) (const unsigned char *);
  ulong_type (*fetch_Elf_Addr) (const unsigned char *);
  void (*set_Elf_Half) (unsigned char *, unsigned short);
  void (*set_Elf_Word) (unsigned char *, unsigned int);
  void (*set_Elf_Addr) (unsigned char *, ulong_type);
};

static const struct elf_type_functions elf_big_32_functions =
{
  simple_object_fetch_big_16,
  simple_object_fetch_big_32,
  simple_object_fetch_big_32_ulong,
  simple_object_set_big_16,
  simple_object_set_big_32,
  simple_object_set_big_32_ulong
};

static const struct elf_type_functions elf_little_32_functions =
{
  simple_object_fetch_little_16,
  simple_object_fetch_little_32,
  simple_object_fetch_little_32_ulong,
  simple_object_set_little_16,
  simple_object_set_little_32,
  simple_object_set_little_32_ulong
};

#ifdef UNSIGNED_64BIT_TYPE

static const struct elf_type_functions elf_big_64_functions =
{
  simple_object_fetch_big_16,
  simple_object_fetch_big_32,
  simple_object_fetch_big_64,
  simple_object_set_big_16,
  simple_object_set_big_32,
  simple_object_set_big_64
};

static const struct elf_type_functions elf_little_64_functions =
{
  simple_object_fetch_little_16,
  simple_object_fetch_little_32,
  simple_object_fetch_little_64,
  simple_object_set_little_16,
  simple_object_set_little_32,
  simple_object_set_little_64
};

#endif

/* Hideous macro to fetch the value of a field from an external ELF
   struct of some sort.  TYPEFUNCS is the set of type functions.
   BUFFER points to the external data.  STRUCTTYPE is the appropriate
   struct type.  FIELD is a field within the struct.  TYPE is the type
   of the field in the struct: Elf_Half, Elf_Word, or Elf_Addr.  */

#define ELF_FETCH_STRUCT_FIELD(TYPEFUNCS, STRUCTTYPE, FIELD, BUFFER, TYPE) \
  ((TYPEFUNCS)->fetch_ ## TYPE ((BUFFER) + offsetof (STRUCTTYPE, FIELD)))

/* Even more hideous macro to fetch the value of FIELD from BUFFER.
   SIZE is 32 or 64.  STRUCTTYPE is the name of the struct from
   elf/external.h: Ehdr, Shdr, etc.  FIELD is the name of a field in
   the struct.  TYPE is the type of the field in the struct: Elf_Half,
   Elf_Word, or Elf_Addr.  */

#define ELF_FETCH_SIZED_FIELD(TYPEFUNCS, SIZE, STRUCTTYPE, BUFFER,	\
			      FIELD, TYPE)				\
  ELF_FETCH_STRUCT_FIELD (TYPEFUNCS,					\
			  Elf ## SIZE ## _External_ ## STRUCTTYPE,	\
			  FIELD, BUFFER, TYPE)

/* Like ELF_FETCH_SIZED_FIELD but taking an ELFCLASS value.  */

#define ELF_FETCH_FIELD(TYPEFUNCS, CLASS, STRUCTTYPE, BUFFER,		\
			FIELD, TYPE)					\
  ((CLASS) == ELFCLASS32						\
    ? ELF_FETCH_SIZED_FIELD (TYPEFUNCS, 32, STRUCTTYPE, BUFFER, FIELD,	\
			     TYPE)					\
    : ELF_FETCH_SIZED_FIELD (TYPEFUNCS, 64, STRUCTTYPE, BUFFER, FIELD,	\
			     TYPE))

/* Hideous macro to set the value of a field in an external ELF
   structure to VAL.  TYPEFUNCS is the set of type functions.  BUFFER
   points to the external data.  STRUCTTYPE is the appropriate
   structure type.  FIELD is a field within the struct.  TYPE is the
   type of the field in the struct: Elf_Half, Elf_Word, or
   Elf_Addr.  */

#define ELF_SET_STRUCT_FIELD(TYPEFUNCS, STRUCTTYPE, FIELD, BUFFER, TYPE, VAL) \
  (TYPEFUNCS)->set_ ## TYPE ((BUFFER) + offsetof (STRUCTTYPE, FIELD), (VAL))

/* Even more hideous macro to set the value of FIELD in BUFFER to VAL.
   SIZE is 32 or 64.  STRUCTTYPE is the name of the struct from
   elf/external.h: Ehdr, Shdr, etc.  FIELD is the name of a field in
   the struct.  TYPE is the type of the field in the struct: Elf_Half,
   Elf_Word, or Elf_Addr.  */

#define ELF_SET_SIZED_FIELD(TYPEFUNCS, SIZE, STRUCTTYPE, BUFFER, FIELD, \
			    TYPE, VAL)					\
  ELF_SET_STRUCT_FIELD (TYPEFUNCS,					\
			Elf ## SIZE ## _External_ ## STRUCTTYPE,	\
			FIELD, BUFFER, TYPE, VAL)

/* Like ELF_SET_SIZED_FIELD but taking an ELFCLASS value.  */

#define ELF_SET_FIELD(TYPEFUNCS, CLASS, STRUCTTYPE, BUFFER, FIELD,	\
		      TYPE, VAL)					\
  ((CLASS) == ELFCLASS32						\
    ? ELF_SET_SIZED_FIELD (TYPEFUNCS, 32, STRUCTTYPE, BUFFER, FIELD,	\
			   TYPE, VAL)					\
    : ELF_SET_SIZED_FIELD (TYPEFUNCS, 64, STRUCTTYPE, BUFFER, FIELD,	\
			   TYPE, VAL))

/* Private data for an simple_object_read.  */

struct simple_object_elf_read
{
  /* Type functions.  */
  const struct elf_type_functions* type_functions;
  /* Elf data.  */
  unsigned char ei_data;
  /* Elf class.  */
  unsigned char ei_class;
  /* ELF OS ABI.  */
  unsigned char ei_osabi;
  /* Elf machine number.  */
  unsigned short machine;
  /* Processor specific flags.  */
  unsigned int flags;
  /* File offset of section headers.  */
  ulong_type shoff;
  /* Number of sections.  */
  unsigned int shnum;
  /* Index of string table section header.  */
  unsigned int shstrndx;
};

/* Private data for an simple_object_attributes.  */

struct simple_object_elf_attributes
{
  /* Type functions.  */
  const struct elf_type_functions* type_functions;
  /* Elf data.  */
  unsigned char ei_data;
  /* Elf class.  */
  unsigned char ei_class;
  /* ELF OS ABI.  */
  unsigned char ei_osabi;
  /* Elf machine number.  */
  unsigned short machine;
  /* Processor specific flags.  */
  unsigned int flags;
};

/* See if we have an ELF file.  */

static void *
simple_object_elf_match (unsigned char header[SIMPLE_OBJECT_MATCH_HEADER_LEN],
			 int descriptor, off_t offset,
			 const char *segment_name ATTRIBUTE_UNUSED,
			 const char **errmsg, int *err)
{
  unsigned char ei_data;
  unsigned char ei_class;
  const struct elf_type_functions *type_functions;
  unsigned char ehdr[sizeof (Elf64_External_Ehdr)];
  struct simple_object_elf_read *eor;

  if (header[EI_MAG0] != ELFMAG0
      || header[EI_MAG1] != ELFMAG1
      || header[EI_MAG2] != ELFMAG2
      || header[EI_MAG3] != ELFMAG3
      || header[EI_VERSION] != EV_CURRENT)
    {
      *errmsg = NULL;
      *err = 0;
      return NULL;
    }

  ei_data = header[EI_DATA];
  if (ei_data != ELFDATA2LSB && ei_data != ELFDATA2MSB)
    {
      *errmsg = "unknown ELF endianness";
      *err = 0;
      return NULL;
    }

  ei_class = header[EI_CLASS];
  switch (ei_class)
    {
    case ELFCLASS32:
      type_functions = (ei_data == ELFDATA2LSB
			? &elf_little_32_functions
			: &elf_big_32_functions);
      break;

    case ELFCLASS64:
#ifndef UNSIGNED_64BIT_TYPE
      *errmsg = "64-bit ELF objects not supported";
      *err = 0;
      return NULL;
#else
      type_functions = (ei_data == ELFDATA2LSB
			? &elf_little_64_functions
			: &elf_big_64_functions);
      break;
#endif

    default:
      *errmsg = "unrecognized ELF size";
      *err = 0;
      return NULL;
    }

  if (!simple_object_internal_read (descriptor, offset, ehdr, sizeof ehdr,
				    errmsg, err))
    return NULL;

  eor = XNEW (struct simple_object_elf_read);
  eor->type_functions = type_functions;
  eor->ei_data = ei_data;
  eor->ei_class = ei_class;
  eor->ei_osabi = header[EI_OSABI];
  eor->machine = ELF_FETCH_FIELD (type_functions, ei_class, Ehdr, ehdr,
				  e_machine, Elf_Half);
  eor->flags = ELF_FETCH_FIELD (type_functions, ei_class, Ehdr, ehdr,
				e_flags, Elf_Word);
  eor->shoff = ELF_FETCH_FIELD (type_functions, ei_class, Ehdr, ehdr,
				e_shoff, Elf_Addr);
  eor->shnum = ELF_FETCH_FIELD (type_functions, ei_class, Ehdr, ehdr,
				e_shnum, Elf_Half);
  eor->shstrndx = ELF_FETCH_FIELD (type_functions, ei_class, Ehdr, ehdr,
				   e_shstrndx, Elf_Half);

  if ((eor->shnum == 0 || eor->shstrndx == SHN_XINDEX)
      && eor->shoff != 0)
    {
      unsigned char shdr[sizeof (Elf64_External_Shdr)];

      /* Object file has more than 0xffff sections.  */

      if (!simple_object_internal_read (descriptor, offset + eor->shoff, shdr,
					(ei_class == ELFCLASS32
					 ? sizeof (Elf32_External_Shdr)
					 : sizeof (Elf64_External_Shdr)),
					errmsg, err))
	{
	  XDELETE (eor);
	  return NULL;
	}

      if (eor->shnum == 0)
	eor->shnum = ELF_FETCH_FIELD (type_functions, ei_class, Shdr,
				      shdr, sh_size, Elf_Addr);

      if (eor->shstrndx == SHN_XINDEX)
	{
	  eor->shstrndx = ELF_FETCH_FIELD (type_functions, ei_class, Shdr,
					   shdr, sh_link, Elf_Word);

	  /* Versions of the GNU binutils between 2.12 and 2.18 did
	     not handle objects with more than SHN_LORESERVE sections
	     correctly.  All large section indexes were offset by
	     0x100.  There is more information at
	     http://sourceware.org/bugzilla/show_bug.cgi?id-5900 .
	     Fortunately these object files are easy to detect, as the
	     GNU binutils always put the section header string table
	     near the end of the list of sections.  Thus if the
	     section header string table index is larger than the
	     number of sections, then we know we have to subtract
	     0x100 to get the real section index.  */
	  if (eor->shstrndx >= eor->shnum
	      && eor->shstrndx >= SHN_LORESERVE + 0x100)
	    eor->shstrndx -= 0x100;
	}
    }

  if (eor->shstrndx >= eor->shnum)
    {
      *errmsg = "invalid ELF shstrndx >= shnum";
      *err = 0;
      XDELETE (eor);
      return NULL;
    }

  return (void *) eor;
}

/* Find all sections in an ELF file.  */

static const char *
simple_object_elf_find_sections (simple_object_read *sobj,
				 int (*pfn) (void *, const char *,
					     off_t offset, off_t length),
				 void *data,
				 int *err)
{
  struct simple_object_elf_read *eor =
    (struct simple_object_elf_read *) sobj->data;
  const struct elf_type_functions *type_functions = eor->type_functions;
  unsigned char ei_class = eor->ei_class;
  size_t shdr_size;
  unsigned int shnum;
  unsigned char *shdrs;
  const char *errmsg;
  unsigned char *shstrhdr;
  size_t name_size;
  off_t shstroff;
  unsigned char *names;
  unsigned int i;

  shdr_size = (ei_class == ELFCLASS32
	       ? sizeof (Elf32_External_Shdr)
	       : sizeof (Elf64_External_Shdr));

  /* Read the section headers.  We skip section 0, which is not a
     useful section.  */

  shnum = eor->shnum;
  shdrs = XNEWVEC (unsigned char, shdr_size * (shnum - 1));

  if (!simple_object_internal_read (sobj->descriptor,
				    sobj->offset + eor->shoff + shdr_size,
				    shdrs,
				    shdr_size * (shnum - 1),
				    &errmsg, err))
    {
      XDELETEVEC (shdrs);
      return errmsg;
    }

  /* Read the section names.  */

  shstrhdr = shdrs + (eor->shstrndx - 1) * shdr_size;
  name_size = ELF_FETCH_FIELD (type_functions, ei_class, Shdr,
			       shstrhdr, sh_size, Elf_Addr);
  shstroff = ELF_FETCH_FIELD (type_functions, ei_class, Shdr,
			      shstrhdr, sh_offset, Elf_Addr);
  names = XNEWVEC (unsigned char, name_size);
  if (!simple_object_internal_read (sobj->descriptor,
				    sobj->offset + shstroff,
				    names, name_size, &errmsg, err))
    {
      XDELETEVEC (names);
      XDELETEVEC (shdrs);
      return errmsg;
    }

  for (i = 1; i < shnum; ++i)
    {
      unsigned char *shdr;
      unsigned int sh_name;
      const char *name;
      off_t offset;
      off_t length;

      shdr = shdrs + (i - 1) * shdr_size;
      sh_name = ELF_FETCH_FIELD (type_functions, ei_class, Shdr,
				 shdr, sh_name, Elf_Word);
      if (sh_name >= name_size)
	{
	  *err = 0;
	  XDELETEVEC (names);
	  XDELETEVEC (shdrs);
	  return "ELF section name out of range";
	}

      name = (const char *) names + sh_name;
      offset = ELF_FETCH_FIELD (type_functions, ei_class, Shdr,
				shdr, sh_offset, Elf_Addr);
      length = ELF_FETCH_FIELD (type_functions, ei_class, Shdr,
				shdr, sh_size, Elf_Addr);

      if (!(*pfn) (data, name, offset, length))
	break;
    }

  XDELETEVEC (names);
  XDELETEVEC (shdrs);

  return NULL;
}

/* Fetch the attributes for an simple_object_read.  */

static void *
simple_object_elf_fetch_attributes (simple_object_read *sobj,
				    const char **errmsg ATTRIBUTE_UNUSED,
				    int *err ATTRIBUTE_UNUSED)
{
  struct simple_object_elf_read *eor =
    (struct simple_object_elf_read *) sobj->data;
  struct simple_object_elf_attributes *ret;

  ret = XNEW (struct simple_object_elf_attributes);
  ret->type_functions = eor->type_functions;
  ret->ei_data = eor->ei_data;
  ret->ei_class = eor->ei_class;
  ret->ei_osabi = eor->ei_osabi;
  ret->machine = eor->machine;
  ret->flags = eor->flags;
  return ret;
}

/* Release the privata data for an simple_object_read.  */

static void
simple_object_elf_release_read (void *data)
{
  XDELETE (data);
}

/* Compare two attributes structures.  */

static const char *
simple_object_elf_attributes_merge (void *todata, void *fromdata, int *err)
{
  struct simple_object_elf_attributes *to =
    (struct simple_object_elf_attributes *) todata;
  struct simple_object_elf_attributes *from =
    (struct simple_object_elf_attributes *) fromdata;

  if (to->ei_data != from->ei_data || to->ei_class != from->ei_class)
    {
      *err = 0;
      return "ELF object format mismatch";
    }

  if (to->machine != from->machine)
    {
      int ok;

      /* EM_SPARC and EM_SPARC32PLUS are compatible and force an
	 output of EM_SPARC32PLUS.  */
      ok = 0;
      switch (to->machine)
	{
	case EM_SPARC:
	  if (from->machine == EM_SPARC32PLUS)
	    {
	      to->machine = from->machine;
	      ok = 1;
	    }
	  break;

	case EM_SPARC32PLUS:
	  if (from->machine == EM_SPARC)
	    ok = 1;
	  break;

	default:
	  break;
	}

      if (!ok)
	{
	  *err = 0;
	  return "ELF machine number mismatch";
	}
    }

  return NULL;
}

/* Release the private data for an attributes structure.  */

static void
simple_object_elf_release_attributes (void *data)
{
  XDELETE (data);
}

/* Prepare to write out a file.  */

static void *
simple_object_elf_start_write (void *attributes_data,
			       const char **errmsg ATTRIBUTE_UNUSED,
			       int *err ATTRIBUTE_UNUSED)
{
  struct simple_object_elf_attributes *attrs =
    (struct simple_object_elf_attributes *) attributes_data;
  struct simple_object_elf_attributes *ret;

  /* We're just going to record the attributes, but we need to make a
     copy because the user may delete them.  */
  ret = XNEW (struct simple_object_elf_attributes);
  *ret = *attrs;
  return ret;
}

/* Write out an ELF ehdr.  */

static int
simple_object_elf_write_ehdr (simple_object_write *sobj, int descriptor,
			      const char **errmsg, int *err)
{
  struct simple_object_elf_attributes *attrs =
    (struct simple_object_elf_attributes *) sobj->data;
  const struct elf_type_functions* fns;
  unsigned char cl;
  size_t ehdr_size;
  unsigned char buf[sizeof (Elf64_External_Ehdr)];
  simple_object_write_section *section;
  unsigned int shnum;
  unsigned int shstrndx;

  fns = attrs->type_functions;
  cl = attrs->ei_class;

  shnum = 0;
  for (section = sobj->sections; section != NULL; section = section->next)
    ++shnum;
  if (shnum > 0)
    {
      /* Add a section header for the dummy section and one for
	 .shstrtab.  */
      shnum += 2;
    }

  ehdr_size = (cl == ELFCLASS32
	       ? sizeof (Elf32_External_Ehdr)
	       : sizeof (Elf64_External_Ehdr));
  memset (buf, 0, sizeof (Elf64_External_Ehdr));

  buf[EI_MAG0] = ELFMAG0;
  buf[EI_MAG1] = ELFMAG1;
  buf[EI_MAG2] = ELFMAG2;
  buf[EI_MAG3] = ELFMAG3;
  buf[EI_CLASS] = cl;
  buf[EI_DATA] = attrs->ei_data;
  buf[EI_VERSION] = EV_CURRENT;
  buf[EI_OSABI] = attrs->ei_osabi;

  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_type, Elf_Half, ET_REL);
  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_machine, Elf_Half, attrs->machine);
  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_version, Elf_Word, EV_CURRENT);
  /* e_entry left as zero.  */
  /* e_phoff left as zero.  */
  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_shoff, Elf_Addr, ehdr_size);
  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_flags, Elf_Word, attrs->flags);
  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_ehsize, Elf_Half, ehdr_size);
  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_phentsize, Elf_Half,
		 (cl == ELFCLASS32
		  ? sizeof (Elf32_External_Phdr)
		  : sizeof (Elf64_External_Phdr)));
  /* e_phnum left as zero.  */
  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_shentsize, Elf_Half,
		 (cl == ELFCLASS32
		  ? sizeof (Elf32_External_Shdr)
		  : sizeof (Elf64_External_Shdr)));
  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_shnum, Elf_Half,
		 shnum >= SHN_LORESERVE ? 0 : shnum);
  if (shnum == 0)
    shstrndx = 0;
  else
    {
      shstrndx = shnum - 1;
      if (shstrndx >= SHN_LORESERVE)
	shstrndx = SHN_XINDEX;
    }
  ELF_SET_FIELD (fns, cl, Ehdr, buf, e_shstrndx, Elf_Half, shstrndx);

  return simple_object_internal_write (descriptor, 0, buf, ehdr_size,
				       errmsg, err);
}

/* Write out an ELF shdr.  */

static int
simple_object_elf_write_shdr (simple_object_write *sobj, int descriptor,
			      off_t offset, unsigned int sh_name,
			      unsigned int sh_type, unsigned int sh_flags,
			      unsigned int sh_offset, unsigned int sh_size,
			      unsigned int sh_link, unsigned int sh_addralign,
			      const char **errmsg, int *err)
{
  struct simple_object_elf_attributes *attrs =
    (struct simple_object_elf_attributes *) sobj->data;
  const struct elf_type_functions* fns;
  unsigned char cl;
  size_t shdr_size;
  unsigned char buf[sizeof (Elf64_External_Shdr)];

  fns = attrs->type_functions;
  cl = attrs->ei_class;

  shdr_size = (cl == ELFCLASS32
	       ? sizeof (Elf32_External_Shdr)
	       : sizeof (Elf64_External_Shdr));
  memset (buf, 0, sizeof (Elf64_External_Shdr));

  ELF_SET_FIELD (fns, cl, Shdr, buf, sh_name, Elf_Word, sh_name);
  ELF_SET_FIELD (fns, cl, Shdr, buf, sh_type, Elf_Word, sh_type);
  ELF_SET_FIELD (fns, cl, Shdr, buf, sh_flags, Elf_Addr, sh_flags);
  ELF_SET_FIELD (fns, cl, Shdr, buf, sh_offset, Elf_Addr, sh_offset);
  ELF_SET_FIELD (fns, cl, Shdr, buf, sh_size, Elf_Addr, sh_size);
  ELF_SET_FIELD (fns, cl, Shdr, buf, sh_link, Elf_Word, sh_link);
  /* sh_info left as zero.  */
  ELF_SET_FIELD (fns, cl, Shdr, buf, sh_addralign, Elf_Addr, sh_addralign);
  /* sh_entsize left as zero.  */

  return simple_object_internal_write (descriptor, offset, buf, shdr_size,
				       errmsg, err);
}

/* Write out a complete ELF file.
   Ehdr
   initial dummy Shdr
   user-created Shdrs
   .shstrtab Shdr
   user-created section data
   .shstrtab data  */

static const char *
simple_object_elf_write_to_file (simple_object_write *sobj, int descriptor,
				 int *err)
{
  struct simple_object_elf_attributes *attrs =
    (struct simple_object_elf_attributes *) sobj->data;
  unsigned char cl;
  size_t ehdr_size;
  size_t shdr_size;
  const char *errmsg;
  simple_object_write_section *section;
  unsigned int shnum;
  size_t shdr_offset;
  size_t sh_offset;
  unsigned int first_sh_size;
  unsigned int first_sh_link;
  size_t sh_name;
  unsigned char zero;

  if (!simple_object_elf_write_ehdr (sobj, descriptor, &errmsg, err))
    return errmsg;

  cl = attrs->ei_class;
  if (cl == ELFCLASS32)
    {
      ehdr_size = sizeof (Elf32_External_Ehdr);
      shdr_size = sizeof (Elf32_External_Shdr);
    }
  else
    {
      ehdr_size = sizeof (Elf64_External_Ehdr);
      shdr_size = sizeof (Elf64_External_Shdr);
    }

  shnum = 0;
  for (section = sobj->sections; section != NULL; section = section->next)
    ++shnum;
  if (shnum == 0)
    return NULL;

  /* Add initial dummy Shdr and .shstrtab.  */
  shnum += 2;

  shdr_offset = ehdr_size;
  sh_offset = shdr_offset + shnum * shdr_size;

  if (shnum < SHN_LORESERVE)
    first_sh_size = 0;
  else
    first_sh_size = shnum;
  if (shnum - 1 < SHN_LORESERVE)
    first_sh_link = 0;
  else
    first_sh_link = shnum - 1;
  if (!simple_object_elf_write_shdr (sobj, descriptor, shdr_offset,
				     0, 0, 0, 0, first_sh_size, first_sh_link,
				     0, &errmsg, err))
    return errmsg;

  shdr_offset += shdr_size;

  sh_name = 1;
  for (section = sobj->sections; section != NULL; section = section->next)
    {
      size_t mask;
      size_t new_sh_offset;
      size_t sh_size;
      struct simple_object_write_section_buffer *buffer;

      mask = (1U << section->align) - 1;
      new_sh_offset = sh_offset + mask;
      new_sh_offset &= ~ mask;
      while (new_sh_offset > sh_offset)
	{
	  unsigned char zeroes[16];
	  size_t write;

	  memset (zeroes, 0, sizeof zeroes);
	  write = new_sh_offset - sh_offset;
	  if (write > sizeof zeroes)
	    write = sizeof zeroes;
	  if (!simple_object_internal_write (descriptor, sh_offset, zeroes,
					     write, &errmsg, err))
	    return errmsg;
	  sh_offset += write;
	}

      sh_size = 0;
      for (buffer = section->buffers; buffer != NULL; buffer = buffer->next)
	{
	  if (!simple_object_internal_write (descriptor, sh_offset + sh_size,
					     ((const unsigned char *)
					      buffer->buffer),
					     buffer->size, &errmsg, err))
	    return errmsg;
	  sh_size += buffer->size;
	}

      if (!simple_object_elf_write_shdr (sobj, descriptor, shdr_offset,
					 sh_name, SHT_PROGBITS, 0, sh_offset,
					 sh_size, 0, 1U << section->align,
					 &errmsg, err))
	return errmsg;

      shdr_offset += shdr_size;
      sh_name += strlen (section->name) + 1;
      sh_offset += sh_size;
    }

  if (!simple_object_elf_write_shdr (sobj, descriptor, shdr_offset,
				     sh_name, SHT_STRTAB, 0, sh_offset,
				     sh_name + strlen (".shstrtab") + 1, 0,
				     1, &errmsg, err))
    return errmsg;

  /* .shstrtab has a leading zero byte.  */
  zero = 0;
  if (!simple_object_internal_write (descriptor, sh_offset, &zero, 1,
				     &errmsg, err))
    return errmsg;
  ++sh_offset;

  for (section = sobj->sections; section != NULL; section = section->next)
    {
      size_t len;

      len = strlen (section->name) + 1;
      if (!simple_object_internal_write (descriptor, sh_offset,
					 (const unsigned char *) section->name,
					 len, &errmsg, err))
	return errmsg;
      sh_offset += len;
    }

  if (!simple_object_internal_write (descriptor, sh_offset,
				     (const unsigned char *) ".shstrtab",
				     strlen (".shstrtab") + 1, &errmsg, err))
    return errmsg;

  return NULL;
}

/* Release the private data for an simple_object_write structure.  */

static void
simple_object_elf_release_write (void *data)
{
  XDELETE (data);
}

/* The ELF functions.  */

const struct simple_object_functions simple_object_elf_functions =
{
  simple_object_elf_match,
  simple_object_elf_find_sections,
  simple_object_elf_fetch_attributes,
  simple_object_elf_release_read,
  simple_object_elf_attributes_merge,
  simple_object_elf_release_attributes,
  simple_object_elf_start_write,
  simple_object_elf_write_to_file,
  simple_object_elf_release_write
};
