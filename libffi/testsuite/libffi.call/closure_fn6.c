/* Area:	closure_call
   Purpose:	Check multiple values passing from different type.
		Also, exceed the limit of gpr and fpr registers on PowerPC.
   Limitations:	none.
   PR:		PR23404
   Originator:	<andreast@gcc.gnu.org> 20050830	 */

/* { dg-do run { xfail mips*-*-* arm*-*-* strongarm*-*-* xscale*-*-* } } */
#include "ffitest.h"

static void
closure_test_fn0(ffi_cif* cif,void* resp,void** args, void* userdata)
{
  *(ffi_arg*)resp =
    (int)*(unsigned long long *)args[0] +
    (int)(*(unsigned long long *)args[1]) +
    (int)(*(unsigned long long *)args[2]) +
    (int)*(unsigned long long *)args[3] +
    (int)(*(int *)args[4]) + (int)(*(double *)args[5]) +
    (int)*(double *)args[6] + (int)(*(float *)args[7]) +
    (int)(*(double *)args[8]) + (int)*(double *)args[9] +
    (int)(*(int *)args[10]) + (int)(*(float *)args[11]) +
    (int)*(int *)args[12] + (int)(*(int *)args[13]) +
    (int)(*(double *)args[14]) +  (int)*(double *)args[15] +
    (int)(long)userdata;

  printf("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d: %d\n",
	 (int)*(unsigned long long  *)args[0],
	 (int)(*(unsigned long long  *)args[1]),
	 (int)(*(unsigned long long  *)args[2]),
	 (int)*(unsigned long long  *)args[3],
	 (int)(*(int *)args[4]), (int)(*(double *)args[5]),
	 (int)*(double *)args[6], (int)(*(float *)args[7]),
	 (int)(*(double *)args[8]), (int)*(double *)args[9],
	 (int)(*(int *)args[10]), (int)(*(float *)args[11]),
	 (int)*(int *)args[12], (int)(*(int *)args[13]),
	 (int)(*(double *)args[14]), (int)(*(double *)args[15]),
	 (int)(long)userdata, (int)*(ffi_arg *)resp);

}

typedef int (*closure_test_type0)(unsigned long long,
				  unsigned long long,
				  unsigned long long,
				  unsigned long long,
				  int, double, double, float, double, double,
				  int, float, int, int, double, double);

int main (void)
{
  ffi_cif cif;
#ifndef USING_MMAP
  static ffi_closure cl;
#endif
  ffi_closure *pcl;
  ffi_type * cl_arg_types[17];
  int res;

#ifdef USING_MMAP
  pcl = allocate_mmap (sizeof(ffi_closure));
#else
  pcl = &cl;
#endif

  cl_arg_types[0] = &ffi_type_uint64;
  cl_arg_types[1] = &ffi_type_uint64;
  cl_arg_types[2] = &ffi_type_uint64;
  cl_arg_types[3] = &ffi_type_uint64;
  cl_arg_types[4] = &ffi_type_uint;
  cl_arg_types[5] = &ffi_type_double;
  cl_arg_types[6] = &ffi_type_double;
  cl_arg_types[7] = &ffi_type_float;
  cl_arg_types[8] = &ffi_type_double;
  cl_arg_types[9] = &ffi_type_double;
  cl_arg_types[10] = &ffi_type_uint;
  cl_arg_types[11] = &ffi_type_float;
  cl_arg_types[12] = &ffi_type_uint;
  cl_arg_types[13] = &ffi_type_uint;
  cl_arg_types[14] = &ffi_type_double;
  cl_arg_types[15] = &ffi_type_double;
  cl_arg_types[16] = NULL;

  /* Initialize the cif */
  CHECK(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 16,
		     &ffi_type_sint, cl_arg_types) == FFI_OK);

  CHECK(ffi_prep_closure(pcl, &cif, closure_test_fn0,
			 (void *) 3 /* userdata */) == FFI_OK);

  res = (*((closure_test_type0)pcl))
    (1, 2, 3, 4, 127, 429., 7., 8., 9.5, 10., 11, 12., 13,
     19, 21., 1.);
  /* { dg-output "1 2 3 4 127 429 7 8 9 10 11 12 13 19 21 1 3: 680" } */
  printf("res: %d\n",res);
  /* { dg-output "\nres: 680" } */
  exit(0);
}
