/* Exception handling and frame unwind runtime interface routines.
   Copyright (C) 2001 Free Software Foundation, Inc.

   This file is part of GNU CC.

   GNU CC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU CC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU CC; see the file COPYING.  If not, write to
   the Free Software Foundation, 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* @@@ Really this should be out of line, but this also causes link
   compatibility problems with the base ABI.  This is slightly better
   than duplicating code, however.  */

/* Pointer encodings, from dwarf2.h.  */
#define DW_EH_PE_absptr         0x00
#define DW_EH_PE_omit           0xff

#define DW_EH_PE_uleb128        0x01
#define DW_EH_PE_udata2         0x02
#define DW_EH_PE_udata4         0x03
#define DW_EH_PE_udata8         0x04
#define DW_EH_PE_sleb128        0x09
#define DW_EH_PE_sdata2         0x0A
#define DW_EH_PE_sdata4         0x0B
#define DW_EH_PE_sdata8         0x0C
#define DW_EH_PE_signed         0x08

#define DW_EH_PE_pcrel          0x10
#define DW_EH_PE_textrel        0x20
#define DW_EH_PE_datarel        0x30
#define DW_EH_PE_funcrel        0x40

#define DW_EH_PE_indirect	0x80


/* Given an encoding, return the number of bytes the format occupies.
   This is only defined for fixed-size encodings, and so does not 
   include leb128.  */

static unsigned int
size_of_encoded_value (unsigned char encoding)
{
  if (encoding == DW_EH_PE_omit)
    return 0;

  switch (encoding & 0x07)
    {
    case DW_EH_PE_absptr:
      return sizeof (void *);
    case DW_EH_PE_udata2:
      return 2;
    case DW_EH_PE_udata4:
      return 4;
    case DW_EH_PE_udata8:
      return 8;
    }
  abort ();
}

/* Given an encoding and an _Unwind_Context, return the base to which
   the encoding is relative.  This base may then be passed to 
   read_encoded_value_with_base for use when the _Unwind_Context is
   not available.  */

static _Unwind_Ptr
base_of_encoded_value (unsigned char encoding, _Unwind_Context *context)
{
  if (encoding == DW_EH_PE_omit)
    return 0;

  switch (encoding & 0x70)
    {
    case DW_EH_PE_absptr:
    case DW_EH_PE_pcrel:
      return 0;

    case DW_EH_PE_textrel:
      return _Unwind_GetTextRelBase (context);
    case DW_EH_PE_datarel:
      return _Unwind_GetDataRelBase (context);
    case DW_EH_PE_funcrel:
      return _Unwind_GetRegionStart (context);
    }
  abort ();
}

/* Load an encoded value from memory at P.  The value is returned in VAL;
   The function returns P incremented past the value.  BASE is as given
   by base_of_encoded_value for this encoding in the appropriate context.  */

static const unsigned char *
read_encoded_value_with_base (unsigned char encoding, _Unwind_Ptr base,
			      const unsigned char *p, _Unwind_Ptr *val)
{
  union unaligned
    {
      void *ptr;
      unsigned u2 __attribute__ ((mode (HI)));
      unsigned u4 __attribute__ ((mode (SI)));
      unsigned u8 __attribute__ ((mode (DI)));
      signed s2 __attribute__ ((mode (HI)));
      signed s4 __attribute__ ((mode (SI)));
      signed s8 __attribute__ ((mode (DI)));
    } __attribute__((__packed__));

  union unaligned *u = (union unaligned *) p;
  _Unwind_Ptr result;

  switch (encoding & 0x0f)
    {
    case DW_EH_PE_absptr:
      result = (_Unwind_Ptr) u->ptr;
      p += sizeof (void *);
      break;

    case DW_EH_PE_uleb128:
      {
	unsigned int shift = 0;
	unsigned char byte;

	result = 0;
	do
	  {
	    byte = *p++;
	    result |= (_Unwind_Ptr)(byte & 0x7f) << shift;
	    shift += 7;
	  }
	while (byte & 0x80);
      }
      break;

    case DW_EH_PE_sleb128:
      {
	unsigned int shift = 0;
	unsigned char byte;

	result = 0;
	do
	  {
	    byte = *p++;
	    result |= (_Unwind_Ptr)(byte & 0x7f) << shift;
	    shift += 7;
	  }
	while (byte & 0x80);

	if (shift < 8 * sizeof(result) && (byte & 0x40) != 0)
	  result |= -(1L << shift);
      }
      break;

    case DW_EH_PE_udata2:
      result = u->u2;
      p += 2;
      break;
    case DW_EH_PE_udata4:
      result = u->u4;
      p += 4;
      break;
    case DW_EH_PE_udata8:
      result = u->u8;
      p += 8;
      break;

    case DW_EH_PE_sdata2:
      result = u->s2;
      p += 2;
      break;
    case DW_EH_PE_sdata4:
      result = u->s4;
      p += 4;
      break;
    case DW_EH_PE_sdata8:
      result = u->s8;
      p += 8;
      break;

    default:
      abort ();
    }

  if (result != 0)
    {
      result += ((encoding & 0x70) == DW_EH_PE_pcrel ? (_Unwind_Ptr)u : base);
      if (encoding & DW_EH_PE_indirect)
	result = *(_Unwind_Ptr *)result;
    }

  *val = result;
  return p;
}

/* Like read_encoded_value_with_base, but get the base from the context
   rather than providing it directly.  */

static inline const unsigned char *
read_encoded_value (_Unwind_Context *context, unsigned char encoding,
		    const unsigned char *p, _Unwind_Ptr *val)
{
  return read_encoded_value_with_base (encoding, 
		base_of_encoded_value (encoding, context),
		p, val);
}

/* Read an unsigned leb128 value from P, store the value in VAL, return
   P incremented past the value.  */

static inline const unsigned char *
read_uleb128 (const unsigned char *p, _Unwind_Ptr *val)
{
  return read_encoded_value_with_base (DW_EH_PE_uleb128, 0, p, val);
}

/* Similar, but read a signed leb128 value.  */

static inline const unsigned char *
read_sleb128 (const unsigned char *p, _Unwind_Ptr *val)
{
  return read_encoded_value_with_base (DW_EH_PE_sleb128, 0, p, val);
}
