/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 1996, 2003 Cygnus Solutions
   
   Sparc Foreign Function Interface 

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL CYGNUS SOLUTIONS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------- */

#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>

#ifdef SPARC64
extern void ffi_closure_v9(void);
#else
extern void ffi_closure_v8(void);
#endif

/* ffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments */

void ffi_prep_args_v8(char *stack, extended_cif *ecif)
{
  int i;
  void **p_argv;
  char *argp;
  ffi_type **p_arg;

  /* Skip 16 words for the window save area */
  argp = stack + 16*sizeof(int);

  /* This should only really be done when we are returning a structure,
     however, it's faster just to do it all the time...

  if ( ecif->cif->rtype->type == FFI_TYPE_STRUCT ) */
  *(int *) argp = (long)ecif->rvalue;

  /* And 1 word for the  structure return value. */
  argp += sizeof(int);

#ifdef USING_PURIFY
  /* Purify will probably complain in our assembly routine, unless we
     zero out this memory. */

  ((int*)argp)[0] = 0;
  ((int*)argp)[1] = 0;
  ((int*)argp)[2] = 0;
  ((int*)argp)[3] = 0;
  ((int*)argp)[4] = 0;
  ((int*)argp)[5] = 0;
#endif

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types; i; i--, p_arg++)
    {
      size_t z;

	  if ((*p_arg)->type == FFI_TYPE_STRUCT
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	      || (*p_arg)->type == FFI_TYPE_LONGDOUBLE
#endif
	      )
	    {
	      *(unsigned int *) argp = (unsigned long)(* p_argv);
	      z = sizeof(int);
	    }
	  else
	    {
	      z = (*p_arg)->size;
	      if (z < sizeof(int))
		{
		  z = sizeof(int);
		  switch ((*p_arg)->type)
		    {
		    case FFI_TYPE_SINT8:
		      *(signed int *) argp = *(SINT8 *)(* p_argv);
		      break;
		      
		    case FFI_TYPE_UINT8:
		      *(unsigned int *) argp = *(UINT8 *)(* p_argv);
		      break;
		      
		    case FFI_TYPE_SINT16:
		      *(signed int *) argp = *(SINT16 *)(* p_argv);
		      break;
		      
		    case FFI_TYPE_UINT16:
		      *(unsigned int *) argp = *(UINT16 *)(* p_argv);
		      break;

		    default:
		      FFI_ASSERT(0);
		    }
		}
	      else
		{
		  memcpy(argp, *p_argv, z);
		}
	    }
	  p_argv++;
	  argp += z;
    }
  
  return;
}

int ffi_prep_args_v9(char *stack, extended_cif *ecif)
{
  int i, ret = 0;
  int tmp;
  void **p_argv;
  char *argp;
  ffi_type **p_arg;

  tmp = 0;

  /* Skip 16 words for the window save area */
  argp = stack + 16*sizeof(long long);

#ifdef USING_PURIFY
  /* Purify will probably complain in our assembly routine, unless we
     zero out this memory. */

  ((long long*)argp)[0] = 0;
  ((long long*)argp)[1] = 0;
  ((long long*)argp)[2] = 0;
  ((long long*)argp)[3] = 0;
  ((long long*)argp)[4] = 0;
  ((long long*)argp)[5] = 0;
#endif

  p_argv = ecif->avalue;

  if (ecif->cif->rtype->type == FFI_TYPE_STRUCT &&
      ecif->cif->rtype->size > 32)
    {
      *(unsigned long long *) argp = (unsigned long)ecif->rvalue;
      tmp = 1;
    }

  for (i = 0, p_arg = ecif->cif->arg_types; i < ecif->cif->nargs;
       i++, p_arg++)
    {
      size_t z;

      z = (*p_arg)->size;
      switch ((*p_arg)->type)
	{
	case FFI_TYPE_STRUCT:
	  if (z > 16)
	    {
	      /* For structures larger than 16 bytes we pass reference.  */
	      *(unsigned long long *) argp = (unsigned long)* p_argv;
	      argp += sizeof(long long);
	      tmp++;
	      p_argv++;
	      continue;
	    }
	  /* FALLTHROUGH */
	case FFI_TYPE_FLOAT:
	case FFI_TYPE_DOUBLE:
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
#endif
	  ret = 1; /* We should promote into FP regs as well as integer.  */
	  break;
	}
      if (z < sizeof(long long))
	{
	  switch ((*p_arg)->type)
	    {
	    case FFI_TYPE_SINT8:
	      *(signed long long *) argp = *(SINT8 *)(* p_argv);
	      break;

	    case FFI_TYPE_UINT8:
	      *(unsigned long long *) argp = *(UINT8 *)(* p_argv);
	      break;

	    case FFI_TYPE_SINT16:
	      *(signed long long *) argp = *(SINT16 *)(* p_argv);
	      break;

	    case FFI_TYPE_UINT16:
	      *(unsigned long long *) argp = *(UINT16 *)(* p_argv);
	      break;

	    case FFI_TYPE_SINT32:
	      *(signed long long *) argp = *(SINT32 *)(* p_argv);
	      break;

	    case FFI_TYPE_UINT32:
	      *(unsigned long long *) argp = *(UINT32 *)(* p_argv);
	      break;

	    case FFI_TYPE_FLOAT:
	      *(float *) (argp + 4) = *(FLOAT32 *)(* p_argv); /* Right justify */
	      break;

	    case FFI_TYPE_STRUCT:
	      memcpy(argp, *p_argv, z);
	      break;

	    default:
	      FFI_ASSERT(0);
	    }
	  z = sizeof(long long);
	  tmp++;
	}
      else if (z == sizeof(long long))
	{
	  memcpy(argp, *p_argv, z);
	  z = sizeof(long long);
	  tmp++;
	}
      else
	{
	  if ((tmp & 1) && (*p_arg)->alignment > 8)
	    {
	      tmp++;
	      argp += sizeof(long long);
	    }
	  memcpy(argp, *p_argv, z);
	  z = 2 * sizeof(long long);
	  tmp += 2;
	}
      p_argv++;
      argp += z;
    }

  return ret;
}

/* Perform machine dependent cif processing */
ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  int wordsize;

  if (cif->abi != FFI_V9)
    {
      wordsize = 4;

      /* If we are returning a struct, this will already have been added.
	 Otherwise we need to add it because it's always got to be there! */

      if (cif->rtype->type != FFI_TYPE_STRUCT)
	cif->bytes += wordsize;

      /* sparc call frames require that space is allocated for 6 args,
	 even if they aren't used. Make that space if necessary. */
  
      if (cif->bytes < 4*6+4)
	cif->bytes = 4*6+4;
    }
  else
    {
      wordsize = 8;

      /* sparc call frames require that space is allocated for 6 args,
	 even if they aren't used. Make that space if necessary. */
  
      if (cif->bytes < 8*6)
	cif->bytes = 8*6;
    }

  /* Adjust cif->bytes. to include 16 words for the window save area,
     and maybe the struct/union return pointer area, */

  cif->bytes += 16 * wordsize;

  /* The stack must be 2 word aligned, so round bytes up
     appropriately. */

  cif->bytes = ALIGN(cif->bytes, 2 * wordsize);

  /* Set the return type flag */
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
    case FFI_TYPE_LONGDOUBLE:
#endif
      cif->flags = cif->rtype->type;
      break;

    case FFI_TYPE_STRUCT:
      if (cif->abi == FFI_V9 && cif->rtype->size > 32)
	cif->flags = FFI_TYPE_VOID;
      else
	cif->flags = FFI_TYPE_STRUCT;
      break;

    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      if (cif->abi != FFI_V9)
	{
	  cif->flags = FFI_TYPE_SINT64;
	  break;
	}
      /* FALLTHROUGH */
    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }
  return FFI_OK;
}

int ffi_V9_return_struct(ffi_type *arg, int off, char *ret, char *intg, char *flt)
{
  ffi_type **ptr = &arg->elements[0];

  while (*ptr != NULL)
    {
      if (off & ((*ptr)->alignment - 1))
	off = ALIGN(off, (*ptr)->alignment);

      switch ((*ptr)->type)
	{
	case FFI_TYPE_STRUCT:
	  off = ffi_V9_return_struct(*ptr, off, ret, intg, flt);
	  break;
	case FFI_TYPE_FLOAT:
	case FFI_TYPE_DOUBLE:
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
	case FFI_TYPE_LONGDOUBLE:
#endif
	  memcpy(ret + off, flt + off, (*ptr)->size);
	  off += (*ptr)->size;
	  break;
	default:
	  memcpy(ret + off, intg + off, (*ptr)->size);
	  off += (*ptr)->size;
	  break;
	}
      ptr++;
    }
  return off;
}

extern int ffi_call_V8(void *, extended_cif *, unsigned, 
		       unsigned, unsigned *, void (*fn)());
extern int ffi_call_V9(void *, extended_cif *, unsigned, 
		       unsigned, unsigned *, void (*fn)());

void ffi_call(ffi_cif *cif, void (*fn)(), void *rvalue, void **avalue)
{
  extended_cif ecif;
  void *rval = rvalue;

  ecif.cif = cif;
  ecif.avalue = avalue;

  /* If the return value is a struct and we don't have a return	*/
  /* value address then we need to make one		        */

  ecif.rvalue = rvalue;
  if (cif->rtype->type == FFI_TYPE_STRUCT)
    {
      if (cif->rtype->size <= 32)
	rval = alloca(64);
      else
	{
	  rval = NULL;
	  if (rvalue == NULL)
	    ecif.rvalue = alloca(cif->rtype->size);
	}
    }

  switch (cif->abi) 
    {
    case FFI_V8:
#ifdef SPARC64
      /* We don't yet support calling 32bit code from 64bit */
      FFI_ASSERT(0);
#else
      ffi_call_V8(ffi_prep_args_v8, &ecif, cif->bytes, 
		  cif->flags, rvalue, fn);
#endif
      break;
    case FFI_V9:
#ifdef SPARC64
      ffi_call_V9(ffi_prep_args_v9, &ecif, cif->bytes,
		  cif->flags, rval, fn);
      if (rvalue && rval && cif->rtype->type == FFI_TYPE_STRUCT)
	ffi_V9_return_struct(cif->rtype, 0, (char *)rvalue, (char *)rval, ((char *)rval)+32);
#else
      /* And vice versa */
      FFI_ASSERT(0);
#endif
      break;
    default:
      FFI_ASSERT(0);
      break;
    }

}

ffi_status
ffi_prep_closure (ffi_closure* closure,
		  ffi_cif* cif,
		  void (*fun)(ffi_cif*, void*, void**, void*),
		  void *user_data)
{
  unsigned int *tramp = (unsigned int *) &closure->tramp[0];
  unsigned long fn;
  unsigned long ctx = (unsigned long) closure;

#ifdef SPARC64
  /* Trampoline address is equal to the closure address.  We take advantage
     of that to reduce the trampoline size by 8 bytes. */
  FFI_ASSERT (cif->abi == FFI_V9);
  fn = (unsigned long) ffi_closure_v9;
  tramp[0] = 0x83414000;	/* rd	%pc, %g1	*/
  tramp[1] = 0xca586010;	/* ldx	[%g1+16], %g5	*/
  tramp[2] = 0x81c14000;	/* jmp	%g5		*/
  tramp[3] = 0x01000000;	/* nop			*/
  *((unsigned long *) &tramp[4]) = fn;
#else
  FFI_ASSERT (cif->abi == FFI_V8);
  fn = (unsigned long) ffi_closure_v8;
  tramp[0] = 0x03000000 | fn >> 10;	/* sethi %hi(fn), %g1	*/
  tramp[1] = 0x05000000 | ctx >> 10;	/* sethi %hi(ctx), %g2	*/
  tramp[2] = 0x81c06000 | (fn & 0x3ff);	/* jmp   %g1+%lo(fn)	*/
  tramp[3] = 0x8410a000 | (ctx & 0x3ff);/* or    %g2, %lo(ctx)	*/
#endif

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  /* Flush the Icache.  FIXME: alignment isn't certain, assume 8 bytes */
#ifdef SPARC64
  asm volatile ("flush	%0" : : "r" (closure) : "memory");
  asm volatile ("flush	%0" : : "r" (((char *) closure) + 8) : "memory");
#else
  asm volatile ("iflush	%0" : : "r" (closure) : "memory");
  asm volatile ("iflush	%0" : : "r" (((char *) closure) + 8) : "memory");
#endif

  return FFI_OK;
}

int
ffi_closure_sparc_inner(ffi_closure *closure,
  void *rvalue, unsigned long *gpr, double *fpr)
{
  ffi_cif *cif;
  void **avalue;
  ffi_type **arg_types;
  int i, avn, argn;

  cif = closure->cif;
  avalue = alloca(cif->nargs * sizeof(void *));

  argn = 0;

  /* Copy the caller's structure return address to that the closure
     returns the data directly to the caller.  */
  if (cif->flags == FFI_TYPE_STRUCT)
    {
      rvalue = (void *) gpr[0];
      argn = 1;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;
  
  /* Grab the addresses of the arguments from the stack frame.  */
  while (i < avn)
    {
      /* Assume big-endian.  FIXME */
      argn += ALIGN(arg_types[i]->size, SIZEOF_ARG) / SIZEOF_ARG;

#ifdef SPARC64
      if (i < 16 && (arg_types[i]->type == FFI_TYPE_FLOAT
		 || arg_types[i]->type == FFI_TYPE_DOUBLE
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
		 || arg_types[i]->type == FFI_TYPE_LONGDOUBLE
#endif
		))
        avalue[i] = ((char *) &fpr[argn]) - arg_types[i]->size;
      else
#endif
        avalue[i] = ((char *) &gpr[argn]) - arg_types[i]->size;
      i++;
    }

  /* Invoke the closure.  */
  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  /* Tell ffi_closure_sparc how to perform return type promotions.  */
  return cif->rtype->type;
}
