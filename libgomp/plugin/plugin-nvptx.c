/* Plugin for NVPTX execution.

   Copyright (C) 2013-2019 Free Software Foundation, Inc.

   Contributed by Mentor Embedded.

   This file is part of the GNU Offloading and Multi Processing Library
   (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* Nvidia PTX-specific parts of OpenACC support.  The cuda driver
   library appears to hold some implicit state, but the documentation
   is not clear as to what that state might be.  Or how one might
   propagate it from one thread to another.  */

#define _GNU_SOURCE
#include "openacc.h"
#include "config.h"
#include "libgomp-plugin.h"
#include "oacc-plugin.h"
#include "gomp-constants.h"
#include "oacc-int.h"

#include <pthread.h>
#include <cuda.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#if CUDA_VERSION < 6000
extern CUresult cuGetErrorString (CUresult, const char **);
#define CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_MULTIPROCESSOR 82
#endif

#if CUDA_VERSION >= 6050
#undef cuLinkCreate
#undef cuLinkAddData
CUresult cuLinkAddData (CUlinkState, CUjitInputType, void *, size_t,
			const char *, unsigned, CUjit_option *, void **);
CUresult cuLinkCreate (unsigned, CUjit_option *, void **, CUlinkState *);
#else
typedef size_t (*CUoccupancyB2DSize)(int);
CUresult cuLinkAddData_v2 (CUlinkState, CUjitInputType, void *, size_t,
			   const char *, unsigned, CUjit_option *, void **);
CUresult cuLinkCreate_v2 (unsigned, CUjit_option *, void **, CUlinkState *);
CUresult cuOccupancyMaxPotentialBlockSize(int *, int *, CUfunction,
					  CUoccupancyB2DSize, size_t, int);
#endif

#define DO_PRAGMA(x) _Pragma (#x)

#if PLUGIN_NVPTX_DYNAMIC
# include <dlfcn.h>

struct cuda_lib_s {

# define CUDA_ONE_CALL(call)			\
  __typeof (call) *call;
# define CUDA_ONE_CALL_MAYBE_NULL(call)		\
  CUDA_ONE_CALL (call)
#include "cuda-lib.def"
# undef CUDA_ONE_CALL
# undef CUDA_ONE_CALL_MAYBE_NULL

} cuda_lib;

/* -1 if init_cuda_lib has not been called yet, false
   if it has been and failed, true if it has been and succeeded.  */
static signed char cuda_lib_inited = -1;

/* Dynamically load the CUDA runtime library and initialize function
   pointers, return false if unsuccessful, true if successful.  */
static bool
init_cuda_lib (void)
{
  if (cuda_lib_inited != -1)
    return cuda_lib_inited;
  const char *cuda_runtime_lib = "libcuda.so.1";
  void *h = dlopen (cuda_runtime_lib, RTLD_LAZY);
  cuda_lib_inited = false;
  if (h == NULL)
    return false;

# define CUDA_ONE_CALL(call) CUDA_ONE_CALL_1 (call, false)
# define CUDA_ONE_CALL_MAYBE_NULL(call) CUDA_ONE_CALL_1 (call, true)
# define CUDA_ONE_CALL_1(call, allow_null)		\
  cuda_lib.call = dlsym (h, #call);	\
  if (!allow_null && cuda_lib.call == NULL)		\
    return false;
#include "cuda-lib.def"
# undef CUDA_ONE_CALL
# undef CUDA_ONE_CALL_1
# undef CUDA_ONE_CALL_MAYBE_NULL

  cuda_lib_inited = true;
  return true;
}
# define CUDA_CALL_PREFIX cuda_lib.
#else

# define CUDA_ONE_CALL(call)
# define CUDA_ONE_CALL_MAYBE_NULL(call) DO_PRAGMA (weak call)
#include "cuda-lib.def"
#undef CUDA_ONE_CALL_MAYBE_NULL
#undef CUDA_ONE_CALL

# define CUDA_CALL_PREFIX
# define init_cuda_lib() true
#endif

#include "secure_getenv.h"

#undef MIN
#undef MAX
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

/* Convenience macros for the frequently used CUDA library call and
   error handling sequence as well as CUDA library calls that
   do the error checking themselves or don't do it at all.  */

#define CUDA_CALL_ERET(ERET, FN, ...)		\
  do {						\
    unsigned __r				\
      = CUDA_CALL_PREFIX FN (__VA_ARGS__);	\
    if (__r != CUDA_SUCCESS)			\
      {						\
	GOMP_PLUGIN_error (#FN " error: %s",	\
			   cuda_error (__r));	\
	return ERET;				\
      }						\
  } while (0)

#define CUDA_CALL(FN, ...)			\
  CUDA_CALL_ERET (false, FN, __VA_ARGS__)

#define CUDA_CALL_ASSERT(FN, ...)		\
  do {						\
    unsigned __r				\
      = CUDA_CALL_PREFIX FN (__VA_ARGS__);	\
    if (__r != CUDA_SUCCESS)			\
      {						\
	GOMP_PLUGIN_fatal (#FN " error: %s",	\
			   cuda_error (__r));	\
      }						\
  } while (0)

#define CUDA_CALL_NOCHECK(FN, ...)		\
  CUDA_CALL_PREFIX FN (__VA_ARGS__)

#define CUDA_CALL_EXISTS(FN)			\
  CUDA_CALL_PREFIX FN

static const char *
cuda_error (CUresult r)
{
  const char *fallback = "unknown cuda error";
  const char *desc;

  if (!CUDA_CALL_EXISTS (cuGetErrorString))
    return fallback;

  r = CUDA_CALL_NOCHECK (cuGetErrorString, r, &desc);
  if (r == CUDA_SUCCESS)
    return desc;

  return fallback;
}

static unsigned int instantiated_devices = 0;
static pthread_mutex_t ptx_dev_lock = PTHREAD_MUTEX_INITIALIZER;

/* NVPTX/CUDA specific definition of asynchronous queues.  */
struct goacc_asyncqueue
{
  CUstream cuda_stream;
};

struct nvptx_callback
{
  void (*fn) (void *);
  void *ptr;
  struct goacc_asyncqueue *aq;
  struct nvptx_callback *next;
};

/* Thread-specific data for PTX.  */

struct nvptx_thread
{
  /* We currently have this embedded inside the plugin because libgomp manages
     devices through integer target_ids.  This might be better if using an
     opaque target-specific pointer directly from gomp_device_descr.  */
  struct ptx_device *ptx_dev;
};

/* Target data function launch information.  */

struct targ_fn_launch
{
  const char *fn;
  unsigned short dim[GOMP_DIM_MAX];
};

/* Target PTX object information.  */

struct targ_ptx_obj
{
  const char *code;
  size_t size;
};

/* Target data image information.  */

typedef struct nvptx_tdata
{
  const struct targ_ptx_obj *ptx_objs;
  unsigned ptx_num;

  const char *const *var_names;
  unsigned var_num;

  const struct targ_fn_launch *fn_descs;
  unsigned fn_num;
} nvptx_tdata_t;

/* Descriptor of a loaded function.  */

struct targ_fn_descriptor
{
  CUfunction fn;
  const struct targ_fn_launch *launch;
  int regs_per_thread;
  int max_threads_per_block;
};

/* A loaded PTX image.  */
struct ptx_image_data
{
  const void *target_data;
  CUmodule module;

  struct targ_fn_descriptor *fns;  /* Array of functions.  */
  
  struct ptx_image_data *next;
};

struct ptx_free_block
{
  void *ptr;
  struct ptx_free_block *next;
};

struct ptx_device
{
  CUcontext ctx;
  bool ctx_shared;
  CUdevice dev;

  int ord;
  bool overlap;
  bool map;
  bool concur;
  bool mkern;
  int  mode;
  int clock_khz;
  int num_sms;
  int regs_per_block;
  int regs_per_sm;
  int warp_size;
  int max_threads_per_block;
  int max_threads_per_multiprocessor;
  int default_dims[GOMP_DIM_MAX];

  struct ptx_image_data *images;  /* Images loaded on device.  */
  pthread_mutex_t image_lock;     /* Lock for above list.  */

  struct ptx_free_block *free_blocks;
  pthread_mutex_t free_blocks_lock;

  struct ptx_device *next;
};

static struct ptx_device **ptx_devices;

static inline struct nvptx_thread *
nvptx_thread (void)
{
  return (struct nvptx_thread *) GOMP_PLUGIN_acc_thread ();
}

/* Initialize the device.  Return TRUE on success, else FALSE.  PTX_DEV_LOCK
   should be locked on entry and remains locked on exit.  */

static bool
nvptx_init (void)
{
  int ndevs;

  if (instantiated_devices != 0)
    return true;

  if (!init_cuda_lib ())
    return false;

  CUDA_CALL (cuInit, 0);

  CUDA_CALL (cuDeviceGetCount, &ndevs);
  ptx_devices = GOMP_PLUGIN_malloc_cleared (sizeof (struct ptx_device *)
					    * ndevs);
  return true;
}

/* Select the N'th PTX device for the current host thread.  The device must
   have been previously opened before calling this function.  */

static bool
nvptx_attach_host_thread_to_device (int n)
{
  CUdevice dev;
  CUresult r;
  struct ptx_device *ptx_dev;
  CUcontext thd_ctx;

  r = CUDA_CALL_NOCHECK (cuCtxGetDevice, &dev);
  if (r == CUDA_ERROR_NOT_PERMITTED)
    {
      /* Assume we're in a CUDA callback, just return true.  */
      return true;
    }
  if (r != CUDA_SUCCESS && r != CUDA_ERROR_INVALID_CONTEXT)
    {
      GOMP_PLUGIN_error ("cuCtxGetDevice error: %s", cuda_error (r));
      return false;
    }

  if (r != CUDA_ERROR_INVALID_CONTEXT && dev == n)
    return true;
  else
    {
      CUcontext old_ctx;

      ptx_dev = ptx_devices[n];
      if (!ptx_dev)
	{
	  GOMP_PLUGIN_error ("device %d not found", n);
	  return false;
	}

      CUDA_CALL (cuCtxGetCurrent, &thd_ctx);

      /* We don't necessarily have a current context (e.g. if it has been
         destroyed.  Pop it if we do though.  */
      if (thd_ctx != NULL)
	CUDA_CALL (cuCtxPopCurrent, &old_ctx);

      CUDA_CALL (cuCtxPushCurrent, ptx_dev->ctx);
    }
  return true;
}

static struct ptx_device *
nvptx_open_device (int n)
{
  struct ptx_device *ptx_dev;
  CUdevice dev, ctx_dev;
  CUresult r;
  int async_engines, pi;

  CUDA_CALL_ERET (NULL, cuDeviceGet, &dev, n);

  ptx_dev = GOMP_PLUGIN_malloc (sizeof (struct ptx_device));

  ptx_dev->ord = n;
  ptx_dev->dev = dev;
  ptx_dev->ctx_shared = false;

  r = CUDA_CALL_NOCHECK (cuCtxGetDevice, &ctx_dev);
  if (r != CUDA_SUCCESS && r != CUDA_ERROR_INVALID_CONTEXT)
    {
      GOMP_PLUGIN_error ("cuCtxGetDevice error: %s", cuda_error (r));
      return NULL;
    }
  
  if (r != CUDA_ERROR_INVALID_CONTEXT && ctx_dev != dev)
    {
      /* The current host thread has an active context for a different device.
         Detach it.  */
      CUcontext old_ctx;
      CUDA_CALL_ERET (NULL, cuCtxPopCurrent, &old_ctx);
    }

  CUDA_CALL_ERET (NULL, cuCtxGetCurrent, &ptx_dev->ctx);

  if (!ptx_dev->ctx)
    CUDA_CALL_ERET (NULL, cuCtxCreate, &ptx_dev->ctx, CU_CTX_SCHED_AUTO, dev);
  else
    ptx_dev->ctx_shared = true;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_GPU_OVERLAP, dev);
  ptx_dev->overlap = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_CAN_MAP_HOST_MEMORY, dev);
  ptx_dev->map = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_CONCURRENT_KERNELS, dev);
  ptx_dev->concur = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_COMPUTE_MODE, dev);
  ptx_dev->mode = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_INTEGRATED, dev);
  ptx_dev->mkern = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, dev);
  ptx_dev->clock_khz = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, dev);
  ptx_dev->num_sms = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_BLOCK, dev);
  ptx_dev->regs_per_block = pi;

  /* CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_MULTIPROCESSOR is defined only
     in CUDA 6.0 and newer.  */
  r = CUDA_CALL_NOCHECK (cuDeviceGetAttribute, &pi,
			 CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_MULTIPROCESSOR,
			 dev);
  /* Fallback: use limit of registers per block, which is usually equal.  */
  if (r == CUDA_ERROR_INVALID_VALUE)
    pi = ptx_dev->regs_per_block;
  else if (r != CUDA_SUCCESS)
    {
      GOMP_PLUGIN_error ("cuDeviceGetAttribute error: %s", cuda_error (r));
      return NULL;
    }
  ptx_dev->regs_per_sm = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute,
		  &pi, CU_DEVICE_ATTRIBUTE_WARP_SIZE, dev);
  if (pi != 32)
    {
      GOMP_PLUGIN_error ("Only warp size 32 is supported");
      return NULL;
    }
  ptx_dev->warp_size = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute, &pi,
		  CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK, dev);
  ptx_dev->max_threads_per_block = pi;

  CUDA_CALL_ERET (NULL, cuDeviceGetAttribute, &pi,
		  CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR, dev);
  ptx_dev->max_threads_per_multiprocessor = pi;

  r = CUDA_CALL_NOCHECK (cuDeviceGetAttribute, &async_engines,
			 CU_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT, dev);
  if (r != CUDA_SUCCESS)
    async_engines = 1;

  for (int i = 0; i != GOMP_DIM_MAX; i++)
    ptx_dev->default_dims[i] = 0;

  ptx_dev->images = NULL;
  pthread_mutex_init (&ptx_dev->image_lock, NULL);

  ptx_dev->free_blocks = NULL;
  pthread_mutex_init (&ptx_dev->free_blocks_lock, NULL);

  return ptx_dev;
}

static bool
nvptx_close_device (struct ptx_device *ptx_dev)
{
  if (!ptx_dev)
    return true;

  for (struct ptx_free_block *b = ptx_dev->free_blocks; b;)
    {
      struct ptx_free_block *b_next = b->next;
      CUDA_CALL (cuMemFree, (CUdeviceptr) b->ptr);
      free (b);
      b = b_next;
    }

  pthread_mutex_destroy (&ptx_dev->free_blocks_lock);
  pthread_mutex_destroy (&ptx_dev->image_lock);

  if (!ptx_dev->ctx_shared)
    CUDA_CALL (cuCtxDestroy, ptx_dev->ctx);

  free (ptx_dev);
  return true;
}

static int
nvptx_get_num_devices (void)
{
  int n;

  /* PR libgomp/65099: Currently, we only support offloading in 64-bit
     configurations.  */
  if (sizeof (void *) != 8)
    {
      GOMP_PLUGIN_debug (0, "Disabling nvptx offloading;"
			 " only 64-bit configurations are supported\n");
      return 0;
    }

  /* This function will be called before the plugin has been initialized in
     order to enumerate available devices, but CUDA API routines can't be used
     until cuInit has been called.  Just call it now (but don't yet do any
     further initialization).  */
  if (instantiated_devices == 0)
    {
      if (!init_cuda_lib ())
	return 0;
      CUresult r = CUDA_CALL_NOCHECK (cuInit, 0);
      /* This is not an error: e.g. we may have CUDA libraries installed but
         no devices available.  */
      if (r != CUDA_SUCCESS)
	{
	  GOMP_PLUGIN_debug (0, "Disabling nvptx offloading; cuInit: %s\n",
			     cuda_error (r));
	  return 0;
	}
    }

  CUDA_CALL_ERET (-1, cuDeviceGetCount, &n);
  return n;
}

static void
notify_var (const char *var_name, const char *env_var)
{
  if (env_var == NULL)
    GOMP_PLUGIN_debug (0, "%s: <Not defined>\n", var_name);
  else
    GOMP_PLUGIN_debug (0, "%s: '%s'\n", var_name, env_var);
}

static void
process_GOMP_NVPTX_JIT (intptr_t *gomp_nvptx_o)
{
  const char *var_name = "GOMP_NVPTX_JIT";
  const char *env_var = secure_getenv (var_name);
  notify_var (var_name, env_var);

  if (env_var == NULL)
    return;

  const char *c = env_var;
  while (*c != '\0')
    {
      while (*c == ' ')
	c++;

      if (c[0] == '-' && c[1] == 'O'
	  && '0' <= c[2] && c[2] <= '4'
	  && (c[3] == '\0' || c[3] == ' '))
	{
	  *gomp_nvptx_o = c[2] - '0';
	  c += 3;
	  continue;
	}

      GOMP_PLUGIN_error ("Error parsing %s", var_name);
      break;
    }
}

static bool
link_ptx (CUmodule *module, const struct targ_ptx_obj *ptx_objs,
	  unsigned num_objs)
{
  CUjit_option opts[7];
  void *optvals[7];
  float elapsed = 0.0;
  char elog[1024];
  char ilog[16384];
  CUlinkState linkstate;
  CUresult r;
  void *linkout;
  size_t linkoutsize __attribute__ ((unused));

  opts[0] = CU_JIT_WALL_TIME;
  optvals[0] = &elapsed;

  opts[1] = CU_JIT_INFO_LOG_BUFFER;
  optvals[1] = &ilog[0];

  opts[2] = CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES;
  optvals[2] = (void *) sizeof ilog;

  opts[3] = CU_JIT_ERROR_LOG_BUFFER;
  optvals[3] = &elog[0];

  opts[4] = CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES;
  optvals[4] = (void *) sizeof elog;

  opts[5] = CU_JIT_LOG_VERBOSE;
  optvals[5] = (void *) 1;

  static intptr_t gomp_nvptx_o = -1;

  static bool init_done = false;
  if (!init_done)
    {
      process_GOMP_NVPTX_JIT (&gomp_nvptx_o);
      init_done = true;
  }

  int nopts = 6;
  if (gomp_nvptx_o != -1)
    {
      opts[nopts] = CU_JIT_OPTIMIZATION_LEVEL;
      optvals[nopts] = (void *) gomp_nvptx_o;
      nopts++;
    }

  if (CUDA_CALL_EXISTS (cuLinkCreate_v2))
    CUDA_CALL (cuLinkCreate_v2, nopts, opts, optvals, &linkstate);
  else
    CUDA_CALL (cuLinkCreate, nopts, opts, optvals, &linkstate);

  for (; num_objs--; ptx_objs++)
    {
      /* cuLinkAddData's 'data' argument erroneously omits the const
	 qualifier.  */
      GOMP_PLUGIN_debug (0, "Loading:\n---\n%s\n---\n", ptx_objs->code);
      if (CUDA_CALL_EXISTS (cuLinkAddData_v2))
	r = CUDA_CALL_NOCHECK (cuLinkAddData_v2, linkstate, CU_JIT_INPUT_PTX,
			       (char *) ptx_objs->code, ptx_objs->size,
			       0, 0, 0, 0);
      else
	r = CUDA_CALL_NOCHECK (cuLinkAddData, linkstate, CU_JIT_INPUT_PTX,
			       (char *) ptx_objs->code, ptx_objs->size,
			       0, 0, 0, 0);
      if (r != CUDA_SUCCESS)
	{
	  GOMP_PLUGIN_error ("Link error log %s\n", &elog[0]);
	  GOMP_PLUGIN_error ("cuLinkAddData (ptx_code) error: %s",
			     cuda_error (r));
	  return false;
	}
    }

  GOMP_PLUGIN_debug (0, "Linking\n");
  r = CUDA_CALL_NOCHECK (cuLinkComplete, linkstate, &linkout, &linkoutsize);

  GOMP_PLUGIN_debug (0, "Link complete: %fms\n", elapsed);
  GOMP_PLUGIN_debug (0, "Link log %s\n", &ilog[0]);

  if (r != CUDA_SUCCESS)
    {
      GOMP_PLUGIN_error ("cuLinkComplete error: %s", cuda_error (r));
      return false;
    }

  CUDA_CALL (cuModuleLoadData, module, linkout);
  CUDA_CALL (cuLinkDestroy, linkstate);
  return true;
}

static void
nvptx_exec (void (*fn), size_t mapnum, void **hostaddrs, void **devaddrs,
	    unsigned *dims, void *targ_mem_desc,
	    CUdeviceptr dp, CUstream stream)
{
  struct targ_fn_descriptor *targ_fn = (struct targ_fn_descriptor *) fn;
  CUfunction function;
  int i;
  void *kargs[1];
  struct nvptx_thread *nvthd = nvptx_thread ();
  int warp_size = nvthd->ptx_dev->warp_size;

  function = targ_fn->fn;

  /* Initialize the launch dimensions.  Typically this is constant,
     provided by the device compiler, but we must permit runtime
     values.  */
  int seen_zero = 0;
  for (i = 0; i != GOMP_DIM_MAX; i++)
    {
      if (targ_fn->launch->dim[i])
       dims[i] = targ_fn->launch->dim[i];
      if (!dims[i])
       seen_zero = 1;
    }

  if (seen_zero)
    {
      pthread_mutex_lock (&ptx_dev_lock);

      static int gomp_openacc_dims[GOMP_DIM_MAX];
      if (!gomp_openacc_dims[0])
	{
	  /* See if the user provided GOMP_OPENACC_DIM environment
	     variable to specify runtime defaults.  */
	  for (int i = 0; i < GOMP_DIM_MAX; ++i)
	    gomp_openacc_dims[i] = GOMP_PLUGIN_acc_default_dim (i);
	}

      if (!nvthd->ptx_dev->default_dims[0])
	{
	  int default_dims[GOMP_DIM_MAX];
	  for (int i = 0; i < GOMP_DIM_MAX; ++i)
	    default_dims[i] = gomp_openacc_dims[i];

	  int gang, worker, vector;
	  {
	    int block_size = nvthd->ptx_dev->max_threads_per_block;
	    int cpu_size = nvthd->ptx_dev->max_threads_per_multiprocessor;
	    int dev_size = nvthd->ptx_dev->num_sms;
	    GOMP_PLUGIN_debug (0, " warp_size=%d, block_size=%d,"
			       " dev_size=%d, cpu_size=%d\n",
			       warp_size, block_size, dev_size, cpu_size);

	    gang = (cpu_size / block_size) * dev_size;
	    worker = block_size / warp_size;
	    vector = warp_size;
	  }

	  /* There is no upper bound on the gang size.  The best size
	     matches the hardware configuration.  Logical gangs are
	     scheduled onto physical hardware.  To maximize usage, we
	     should guess a large number.  */
	  if (default_dims[GOMP_DIM_GANG] < 1)
	    default_dims[GOMP_DIM_GANG] = gang ? gang : 1024;
	  /* The worker size must not exceed the hardware.  */
	  if (default_dims[GOMP_DIM_WORKER] < 1
	      || (default_dims[GOMP_DIM_WORKER] > worker && gang))
	    default_dims[GOMP_DIM_WORKER] = worker;
	  /* The vector size must exactly match the hardware.  */
	  if (default_dims[GOMP_DIM_VECTOR] < 1
	      || (default_dims[GOMP_DIM_VECTOR] != vector && gang))
	    default_dims[GOMP_DIM_VECTOR] = vector;

	  GOMP_PLUGIN_debug (0, " default dimensions [%d,%d,%d]\n",
			     default_dims[GOMP_DIM_GANG],
			     default_dims[GOMP_DIM_WORKER],
			     default_dims[GOMP_DIM_VECTOR]);

	  for (i = 0; i != GOMP_DIM_MAX; i++)
	    nvthd->ptx_dev->default_dims[i] = default_dims[i];
	}
      pthread_mutex_unlock (&ptx_dev_lock);

      {
	bool default_dim_p[GOMP_DIM_MAX];
	for (i = 0; i != GOMP_DIM_MAX; i++)
	  default_dim_p[i] = !dims[i];

	if (!CUDA_CALL_EXISTS (cuOccupancyMaxPotentialBlockSize))
	  {
	    for (i = 0; i != GOMP_DIM_MAX; i++)
	      if (default_dim_p[i])
		dims[i] = nvthd->ptx_dev->default_dims[i];

	    if (default_dim_p[GOMP_DIM_VECTOR])
	      dims[GOMP_DIM_VECTOR]
		= MIN (dims[GOMP_DIM_VECTOR],
		       (targ_fn->max_threads_per_block / warp_size
			* warp_size));

	    if (default_dim_p[GOMP_DIM_WORKER])
	      dims[GOMP_DIM_WORKER]
		= MIN (dims[GOMP_DIM_WORKER],
		       targ_fn->max_threads_per_block / dims[GOMP_DIM_VECTOR]);
	  }
	else
	  {
	    /* Handle the case that the compiler allows the runtime to choose
	       the vector-length conservatively, by ignoring
	       gomp_openacc_dims[GOMP_DIM_VECTOR].  TODO: actually handle
	       it.  */
	    int vectors = 0;
	    /* TODO: limit gomp_openacc_dims[GOMP_DIM_WORKER] such that that
	       gomp_openacc_dims[GOMP_DIM_WORKER] * actual_vectors does not
	       exceed targ_fn->max_threads_per_block. */
	    int workers = gomp_openacc_dims[GOMP_DIM_WORKER];
	    int gangs = gomp_openacc_dims[GOMP_DIM_GANG];
	    int grids, blocks;

	    CUDA_CALL_ASSERT (cuOccupancyMaxPotentialBlockSize, &grids,
			      &blocks, function, NULL, 0,
			      dims[GOMP_DIM_WORKER] * dims[GOMP_DIM_VECTOR]);
	    GOMP_PLUGIN_debug (0, "cuOccupancyMaxPotentialBlockSize: "
			       "grid = %d, block = %d\n", grids, blocks);

	    /* Keep the num_gangs proportional to the block size.  In
	       the case were a block size is limited by shared-memory
	       or the register file capacity, the runtime will not
	       excessively over assign gangs to the multiprocessor
	       units if their state is going to be swapped out even
	       more than necessary. The constant factor 2 is there to
	       prevent threads from idling when there is insufficient
	       work for them.  */
	    if (gangs == 0)
	      gangs = 2 * grids * (blocks / warp_size);

	    if (vectors == 0)
	      vectors = warp_size;

	    if (workers == 0)
	      {
		int actual_vectors = (default_dim_p[GOMP_DIM_VECTOR]
				      ? vectors
				      : dims[GOMP_DIM_VECTOR]);
		workers = blocks / actual_vectors;
		workers = MAX (workers, 1);
		/* If we need a per-worker barrier ... .  */
		if (actual_vectors > 32)
		  /* Don't use more barriers than available.  */
		  workers = MIN (workers, 15);
	      }

	    for (i = 0; i != GOMP_DIM_MAX; i++)
	      if (default_dim_p[i])
		switch (i)
		  {
		  case GOMP_DIM_GANG: dims[i] = gangs; break;
		  case GOMP_DIM_WORKER: dims[i] = workers; break;
		  case GOMP_DIM_VECTOR: dims[i] = vectors; break;
		  default: GOMP_PLUGIN_fatal ("invalid dim");
		  }
	  }
      }
    }

  /* Check if the accelerator has sufficient hardware resources to
     launch the offloaded kernel.  */
  if (dims[GOMP_DIM_WORKER] * dims[GOMP_DIM_VECTOR]
      > targ_fn->max_threads_per_block)
    {
      const char *msg
	= ("The Nvidia accelerator has insufficient resources to launch '%s'"
	   " with num_workers = %d and vector_length = %d"
	   "; "
	   "recompile the program with 'num_workers = x and vector_length = y'"
	   " on that offloaded region or '-fopenacc-dim=:x:y' where"
	   " x * y <= %d"
	   ".\n");
      GOMP_PLUGIN_fatal (msg, targ_fn->launch->fn, dims[GOMP_DIM_WORKER],
			 dims[GOMP_DIM_VECTOR], targ_fn->max_threads_per_block);
    }

  /* Check if the accelerator has sufficient barrier resources to
     launch the offloaded kernel.  */
  if (dims[GOMP_DIM_WORKER] > 15 && dims[GOMP_DIM_VECTOR] > 32)
    {
      const char *msg
	= ("The Nvidia accelerator has insufficient barrier resources to launch"
	   " '%s' with num_workers = %d and vector_length = %d"
	   "; "
	   "recompile the program with 'num_workers = x' on that offloaded"
	   " region or '-fopenacc-dim=:x:' where x <= 15"
	   "; "
	   "or, recompile the program with 'vector_length = 32' on that"
	   " offloaded region or '-fopenacc-dim=::32'"
	   ".\n");
	GOMP_PLUGIN_fatal (msg, targ_fn->launch->fn, dims[GOMP_DIM_WORKER],
			   dims[GOMP_DIM_VECTOR]);
    }

  GOMP_PLUGIN_debug (0, "  %s: kernel %s: launch"
		     " gangs=%u, workers=%u, vectors=%u\n",
		     __FUNCTION__, targ_fn->launch->fn, dims[GOMP_DIM_GANG],
		     dims[GOMP_DIM_WORKER], dims[GOMP_DIM_VECTOR]);

  // OpenACC		CUDA
  //
  // num_gangs		nctaid.x
  // num_workers	ntid.y
  // vector length	ntid.x

  struct goacc_thread *thr = GOMP_PLUGIN_goacc_thread ();
  acc_prof_info *prof_info = thr->prof_info;
  acc_event_info enqueue_launch_event_info;
  acc_api_info *api_info = thr->api_info;
  bool profiling_p = __builtin_expect (prof_info != NULL, false);
  if (profiling_p)
    {
      prof_info->event_type = acc_ev_enqueue_launch_start;

      enqueue_launch_event_info.launch_event.event_type
	= prof_info->event_type;
      enqueue_launch_event_info.launch_event.valid_bytes
	= _ACC_LAUNCH_EVENT_INFO_VALID_BYTES;
      enqueue_launch_event_info.launch_event.parent_construct
	= acc_construct_parallel;
      enqueue_launch_event_info.launch_event.implicit = 1;
      enqueue_launch_event_info.launch_event.tool_info = NULL;
      enqueue_launch_event_info.launch_event.kernel_name = targ_fn->launch->fn;
      enqueue_launch_event_info.launch_event.num_gangs
	= dims[GOMP_DIM_GANG];
      enqueue_launch_event_info.launch_event.num_workers
	= dims[GOMP_DIM_WORKER];
      enqueue_launch_event_info.launch_event.vector_length
	= dims[GOMP_DIM_VECTOR];

      api_info->device_api = acc_device_api_cuda;

      GOMP_PLUGIN_goacc_profiling_dispatch (prof_info, &enqueue_launch_event_info,
					    api_info);
    }

  kargs[0] = &dp;
  CUDA_CALL_ASSERT (cuLaunchKernel, function,
		    dims[GOMP_DIM_GANG], 1, 1,
		    dims[GOMP_DIM_VECTOR], dims[GOMP_DIM_WORKER], 1,
		    0, stream, kargs, 0);

  if (profiling_p)
    {
      prof_info->event_type = acc_ev_enqueue_launch_end;
      enqueue_launch_event_info.launch_event.event_type
	= prof_info->event_type;
      GOMP_PLUGIN_goacc_profiling_dispatch (prof_info, &enqueue_launch_event_info,
					    api_info);
    }

  GOMP_PLUGIN_debug (0, "  %s: kernel %s: finished\n", __FUNCTION__,
		     targ_fn->launch->fn);
}

void * openacc_get_current_cuda_context (void);

static void
goacc_profiling_acc_ev_alloc (struct goacc_thread *thr, void *dp, size_t s)
{
  acc_prof_info *prof_info = thr->prof_info;
  acc_event_info data_event_info;
  acc_api_info *api_info = thr->api_info;

  prof_info->event_type = acc_ev_alloc;

  data_event_info.data_event.event_type = prof_info->event_type;
  data_event_info.data_event.valid_bytes = _ACC_DATA_EVENT_INFO_VALID_BYTES;
  data_event_info.data_event.parent_construct = acc_construct_parallel;
  data_event_info.data_event.implicit = 1;
  data_event_info.data_event.tool_info = NULL;
  data_event_info.data_event.var_name = NULL;
  data_event_info.data_event.bytes = s;
  data_event_info.data_event.host_ptr = NULL;
  data_event_info.data_event.device_ptr = dp;

  api_info->device_api = acc_device_api_cuda;

  GOMP_PLUGIN_goacc_profiling_dispatch (prof_info, &data_event_info, api_info);
}

static void *
nvptx_alloc (size_t s)
{
  CUdeviceptr d;

  CUDA_CALL_ERET (NULL, cuMemAlloc, &d, s);
  struct goacc_thread *thr = GOMP_PLUGIN_goacc_thread ();
  bool profiling_p
    = __builtin_expect (thr != NULL && thr->prof_info != NULL, false);
  if (profiling_p)
    goacc_profiling_acc_ev_alloc (thr, (void *) d, s);

  return (void *) d;
}

static void
goacc_profiling_acc_ev_free (struct goacc_thread *thr, void *p)
{
  acc_prof_info *prof_info = thr->prof_info;
  acc_event_info data_event_info;
  acc_api_info *api_info = thr->api_info;

  prof_info->event_type = acc_ev_free;

  data_event_info.data_event.event_type = prof_info->event_type;
  data_event_info.data_event.valid_bytes = _ACC_DATA_EVENT_INFO_VALID_BYTES;
  data_event_info.data_event.parent_construct = acc_construct_parallel;
  data_event_info.data_event.implicit = 1;
  data_event_info.data_event.tool_info = NULL;
  data_event_info.data_event.var_name = NULL;
  data_event_info.data_event.bytes = -1;
  data_event_info.data_event.host_ptr = NULL;
  data_event_info.data_event.device_ptr = p;

  api_info->device_api = acc_device_api_cuda;

  GOMP_PLUGIN_goacc_profiling_dispatch (prof_info, &data_event_info, api_info);
}

static bool
nvptx_free (void *p, struct ptx_device *ptx_dev)
{
  /* Assume callback context if this is null.  */
  if (GOMP_PLUGIN_acc_thread () == NULL)
    {
      struct ptx_free_block *n
	= GOMP_PLUGIN_malloc (sizeof (struct ptx_free_block));
      n->ptr = p;
      pthread_mutex_lock (&ptx_dev->free_blocks_lock);
      n->next = ptx_dev->free_blocks;
      ptx_dev->free_blocks = n;
      pthread_mutex_unlock (&ptx_dev->free_blocks_lock);
      return true;
    }

  CUdeviceptr pb;
  size_t ps;

  CUDA_CALL (cuMemGetAddressRange, &pb, &ps, (CUdeviceptr) p);
  if ((CUdeviceptr) p != pb)
    {
      GOMP_PLUGIN_error ("invalid device address");
      return false;
    }

  CUDA_CALL (cuMemFree, (CUdeviceptr) p);
  struct goacc_thread *thr = GOMP_PLUGIN_goacc_thread ();
  bool profiling_p
    = __builtin_expect (thr != NULL && thr->prof_info != NULL, false);
  if (profiling_p)
    goacc_profiling_acc_ev_free (thr, p);

  return true;
}

static void *
nvptx_get_current_cuda_device (void)
{
  struct nvptx_thread *nvthd = nvptx_thread ();

  if (!nvthd || !nvthd->ptx_dev)
    return NULL;

  return &nvthd->ptx_dev->dev;
}

static void *
nvptx_get_current_cuda_context (void)
{
  struct nvptx_thread *nvthd = nvptx_thread ();

  if (!nvthd || !nvthd->ptx_dev)
    return NULL;

  return nvthd->ptx_dev->ctx;
}

/* Plugin entry points.  */

const char *
GOMP_OFFLOAD_get_name (void)
{
  return "nvptx";
}

unsigned int
GOMP_OFFLOAD_get_caps (void)
{
  return GOMP_OFFLOAD_CAP_OPENACC_200 | GOMP_OFFLOAD_CAP_OPENMP_400;
}

int
GOMP_OFFLOAD_get_type (void)
{
  return OFFLOAD_TARGET_TYPE_NVIDIA_PTX;
}

int
GOMP_OFFLOAD_get_num_devices (void)
{
  return nvptx_get_num_devices ();
}

bool
GOMP_OFFLOAD_init_device (int n)
{
  struct ptx_device *dev;

  pthread_mutex_lock (&ptx_dev_lock);

  if (!nvptx_init () || ptx_devices[n] != NULL)
    {
      pthread_mutex_unlock (&ptx_dev_lock);
      return false;
    }

  dev = nvptx_open_device (n);
  if (dev)
    {
      ptx_devices[n] = dev;
      instantiated_devices++;
    }

  pthread_mutex_unlock (&ptx_dev_lock);

  return dev != NULL;
}

bool
GOMP_OFFLOAD_fini_device (int n)
{
  pthread_mutex_lock (&ptx_dev_lock);

  if (ptx_devices[n] != NULL)
    {
      if (!nvptx_attach_host_thread_to_device (n)
	  || !nvptx_close_device (ptx_devices[n]))
	{
	  pthread_mutex_unlock (&ptx_dev_lock);
	  return false;
	}
      ptx_devices[n] = NULL;
      instantiated_devices--;
    }

  if (instantiated_devices == 0)
    {
      free (ptx_devices);
      ptx_devices = NULL;
    }

  pthread_mutex_unlock (&ptx_dev_lock);
  return true;
}

/* Return the libgomp version number we're compatible with.  There is
   no requirement for cross-version compatibility.  */

unsigned
GOMP_OFFLOAD_version (void)
{
  return GOMP_VERSION;
}

/* Initialize __nvptx_clocktick, if present in MODULE.  */

static void
nvptx_set_clocktick (CUmodule module, struct ptx_device *dev)
{
  CUdeviceptr dptr;
  CUresult r = CUDA_CALL_NOCHECK (cuModuleGetGlobal, &dptr, NULL,
				  module, "__nvptx_clocktick");
  if (r == CUDA_ERROR_NOT_FOUND)
    return;
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuModuleGetGlobal error: %s", cuda_error (r));
  double __nvptx_clocktick = 1e-3 / dev->clock_khz;
  r = CUDA_CALL_NOCHECK (cuMemcpyHtoD, dptr, &__nvptx_clocktick,
			 sizeof (__nvptx_clocktick));
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuMemcpyHtoD error: %s", cuda_error (r));
}

/* Load the (partial) program described by TARGET_DATA to device
   number ORD.  Allocate and return TARGET_TABLE.  */

int
GOMP_OFFLOAD_load_image (int ord, unsigned version, const void *target_data,
			 struct addr_pair **target_table)
{
  CUmodule module;
  const char *const *var_names;
  const struct targ_fn_launch *fn_descs;
  unsigned int fn_entries, var_entries, i, j;
  struct targ_fn_descriptor *targ_fns;
  struct addr_pair *targ_tbl;
  const nvptx_tdata_t *img_header = (const nvptx_tdata_t *) target_data;
  struct ptx_image_data *new_image;
  struct ptx_device *dev;

  if (GOMP_VERSION_DEV (version) > GOMP_VERSION_NVIDIA_PTX)
    {
      GOMP_PLUGIN_error ("Offload data incompatible with PTX plugin"
			 " (expected %u, received %u)",
			 GOMP_VERSION_NVIDIA_PTX, GOMP_VERSION_DEV (version));
      return -1;
    }

  if (!nvptx_attach_host_thread_to_device (ord)
      || !link_ptx (&module, img_header->ptx_objs, img_header->ptx_num))
    return -1;

  dev = ptx_devices[ord];

  /* The mkoffload utility emits a struct of pointers/integers at the
     start of each offload image.  The array of kernel names and the
     functions addresses form a one-to-one correspondence.  */

  var_entries = img_header->var_num;
  var_names = img_header->var_names;
  fn_entries = img_header->fn_num;
  fn_descs = img_header->fn_descs;

  targ_tbl = GOMP_PLUGIN_malloc (sizeof (struct addr_pair)
				 * (fn_entries + var_entries));
  targ_fns = GOMP_PLUGIN_malloc (sizeof (struct targ_fn_descriptor)
				 * fn_entries);

  *target_table = targ_tbl;

  new_image = GOMP_PLUGIN_malloc (sizeof (struct ptx_image_data));
  new_image->target_data = target_data;
  new_image->module = module;
  new_image->fns = targ_fns;

  pthread_mutex_lock (&dev->image_lock);
  new_image->next = dev->images;
  dev->images = new_image;
  pthread_mutex_unlock (&dev->image_lock);

  for (i = 0; i < fn_entries; i++, targ_fns++, targ_tbl++)
    {
      CUfunction function;
      int nregs, mthrs;

      CUDA_CALL_ERET (-1, cuModuleGetFunction, &function, module,
		      fn_descs[i].fn);
      CUDA_CALL_ERET (-1, cuFuncGetAttribute, &nregs,
		      CU_FUNC_ATTRIBUTE_NUM_REGS, function);
      CUDA_CALL_ERET (-1, cuFuncGetAttribute, &mthrs,
		      CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK, function);

      targ_fns->fn = function;
      targ_fns->launch = &fn_descs[i];
      targ_fns->regs_per_thread = nregs;
      targ_fns->max_threads_per_block = mthrs;

      targ_tbl->start = (uintptr_t) targ_fns;
      targ_tbl->end = targ_tbl->start + 1;
    }

  for (j = 0; j < var_entries; j++, targ_tbl++)
    {
      CUdeviceptr var;
      size_t bytes;

      CUDA_CALL_ERET (-1, cuModuleGetGlobal,
		      &var, &bytes, module, var_names[j]);

      targ_tbl->start = (uintptr_t) var;
      targ_tbl->end = targ_tbl->start + bytes;
    }

  nvptx_set_clocktick (module, dev);

  return fn_entries + var_entries;
}

/* Unload the program described by TARGET_DATA.  DEV_DATA is the
   function descriptors allocated by G_O_load_image.  */

bool
GOMP_OFFLOAD_unload_image (int ord, unsigned version, const void *target_data)
{
  struct ptx_image_data *image, **prev_p;
  struct ptx_device *dev = ptx_devices[ord];

  if (GOMP_VERSION_DEV (version) > GOMP_VERSION_NVIDIA_PTX)
    {
      GOMP_PLUGIN_error ("Offload data incompatible with PTX plugin"
			 " (expected %u, received %u)",
			 GOMP_VERSION_NVIDIA_PTX, GOMP_VERSION_DEV (version));
      return false;
    }

  bool ret = true;
  pthread_mutex_lock (&dev->image_lock);
  for (prev_p = &dev->images; (image = *prev_p) != 0; prev_p = &image->next)
    if (image->target_data == target_data)
      {
	*prev_p = image->next;
	if (CUDA_CALL_NOCHECK (cuModuleUnload, image->module) != CUDA_SUCCESS)
	  ret = false;
	free (image->fns);
	free (image);
	break;
      }
  pthread_mutex_unlock (&dev->image_lock);
  return ret;
}

void *
GOMP_OFFLOAD_alloc (int ord, size_t size)
{
  if (!nvptx_attach_host_thread_to_device (ord))
    return NULL;

  struct ptx_device *ptx_dev = ptx_devices[ord];
  struct ptx_free_block *blocks, *tmp;

  pthread_mutex_lock (&ptx_dev->free_blocks_lock);
  blocks = ptx_dev->free_blocks;
  ptx_dev->free_blocks = NULL;
  pthread_mutex_unlock (&ptx_dev->free_blocks_lock);

  while (blocks)
    {
      tmp = blocks->next;
      nvptx_free (blocks->ptr, ptx_dev);
      free (blocks);
      blocks = tmp;
    }

  return nvptx_alloc (size);
}

bool
GOMP_OFFLOAD_free (int ord, void *ptr)
{
  return (nvptx_attach_host_thread_to_device (ord)
	  && nvptx_free (ptr, ptx_devices[ord]));
}

void
GOMP_OFFLOAD_openacc_exec (void (*fn) (void *), size_t mapnum,
			   void **hostaddrs, void **devaddrs,
			   unsigned *dims, void *targ_mem_desc)
{
  GOMP_PLUGIN_debug (0, "  %s: prepare mappings\n", __FUNCTION__);

  struct goacc_thread *thr = GOMP_PLUGIN_goacc_thread ();
  acc_prof_info *prof_info = thr->prof_info;
  acc_event_info data_event_info;
  acc_api_info *api_info = thr->api_info;
  bool profiling_p = __builtin_expect (prof_info != NULL, false);

  void **hp = NULL;
  CUdeviceptr dp = 0;

  if (mapnum > 0)
    {
      size_t s = mapnum * sizeof (void *);
      hp = alloca (s);
      for (int i = 0; i < mapnum; i++)
	hp[i] = (devaddrs[i] ? devaddrs[i] : hostaddrs[i]);
      CUDA_CALL_ASSERT (cuMemAlloc, &dp, s);
      if (profiling_p)
	goacc_profiling_acc_ev_alloc (thr, (void *) dp, s);
    }

  /* Copy the (device) pointers to arguments to the device (dp and hp might in
     fact have the same value on a unified-memory system).  */
  if (mapnum > 0)
    {
      if (profiling_p)
	{
	  prof_info->event_type = acc_ev_enqueue_upload_start;

	  data_event_info.data_event.event_type = prof_info->event_type;
	  data_event_info.data_event.valid_bytes
	    = _ACC_DATA_EVENT_INFO_VALID_BYTES;
	  data_event_info.data_event.parent_construct
	    = acc_construct_parallel;
	  data_event_info.data_event.implicit = 1; /* Always implicit.  */
	  data_event_info.data_event.tool_info = NULL;
	  data_event_info.data_event.var_name = NULL;
	  data_event_info.data_event.bytes = mapnum * sizeof (void *);
	  data_event_info.data_event.host_ptr = hp;
	  data_event_info.data_event.device_ptr = (const void *) dp;

	  api_info->device_api = acc_device_api_cuda;

	  GOMP_PLUGIN_goacc_profiling_dispatch (prof_info, &data_event_info,
						api_info);
	}
      CUDA_CALL_ASSERT (cuMemcpyHtoD, dp, (void *) hp,
			mapnum * sizeof (void *));
      if (profiling_p)
	{
	  prof_info->event_type = acc_ev_enqueue_upload_end;
	  data_event_info.data_event.event_type = prof_info->event_type;
	  GOMP_PLUGIN_goacc_profiling_dispatch (prof_info, &data_event_info,
						api_info);
	}
    }

  nvptx_exec (fn, mapnum, hostaddrs, devaddrs, dims, targ_mem_desc,
	      dp, NULL);

  CUresult r = CUDA_CALL_NOCHECK (cuStreamSynchronize, NULL);
  const char *maybe_abort_msg = "(perhaps abort was called)";
  if (r == CUDA_ERROR_LAUNCH_FAILED)
    GOMP_PLUGIN_fatal ("cuStreamSynchronize error: %s %s\n", cuda_error (r),
		       maybe_abort_msg);
  else if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuStreamSynchronize error: %s", cuda_error (r));

  CUDA_CALL_ASSERT (cuMemFree, dp);
  if (profiling_p)
    goacc_profiling_acc_ev_free (thr, (void *) dp);
}

static void
cuda_free_argmem (void *ptr)
{
  void **block = (void **) ptr;
  nvptx_free (block[0], (struct ptx_device *) block[1]);
  free (block);
}

void
GOMP_OFFLOAD_openacc_async_exec (void (*fn) (void *), size_t mapnum,
				 void **hostaddrs, void **devaddrs,
				 unsigned *dims, void *targ_mem_desc,
				 struct goacc_asyncqueue *aq)
{
  GOMP_PLUGIN_debug (0, "  %s: prepare mappings\n", __FUNCTION__);

  struct goacc_thread *thr = GOMP_PLUGIN_goacc_thread ();
  acc_prof_info *prof_info = thr->prof_info;
  acc_event_info data_event_info;
  acc_api_info *api_info = thr->api_info;
  bool profiling_p = __builtin_expect (prof_info != NULL, false);

  void **hp = NULL;
  CUdeviceptr dp = 0;
  void **block = NULL;

  if (mapnum > 0)
    {
      size_t s = mapnum * sizeof (void *);
      block = (void **) GOMP_PLUGIN_malloc (2 * sizeof (void *) + s);
      hp = block + 2;
      for (int i = 0; i < mapnum; i++)
	hp[i] = (devaddrs[i] ? devaddrs[i] : hostaddrs[i]);
      CUDA_CALL_ASSERT (cuMemAlloc, &dp, s);
      if (profiling_p)
	goacc_profiling_acc_ev_alloc (thr, (void *) dp, s);
    }

  /* Copy the (device) pointers to arguments to the device (dp and hp might in
     fact have the same value on a unified-memory system).  */
  if (mapnum > 0)
    {
      if (profiling_p)
	{
	  prof_info->event_type = acc_ev_enqueue_upload_start;

	  data_event_info.data_event.event_type = prof_info->event_type;
	  data_event_info.data_event.valid_bytes
	    = _ACC_DATA_EVENT_INFO_VALID_BYTES;
	  data_event_info.data_event.parent_construct
	    = acc_construct_parallel;
	  data_event_info.data_event.implicit = 1; /* Always implicit.  */
	  data_event_info.data_event.tool_info = NULL;
	  data_event_info.data_event.var_name = NULL;
	  data_event_info.data_event.bytes = mapnum * sizeof (void *);
	  data_event_info.data_event.host_ptr = hp;
	  data_event_info.data_event.device_ptr = (const void *) dp;

	  api_info->device_api = acc_device_api_cuda;

	  GOMP_PLUGIN_goacc_profiling_dispatch (prof_info, &data_event_info,
						api_info);
	}

      CUDA_CALL_ASSERT (cuMemcpyHtoDAsync, dp, (void *) hp,
			mapnum * sizeof (void *), aq->cuda_stream);
      block[0] = (void *) dp;

      struct nvptx_thread *nvthd =
	(struct nvptx_thread *) GOMP_PLUGIN_acc_thread ();
      block[1] = (void *) nvthd->ptx_dev;

      if (profiling_p)
	{
	  prof_info->event_type = acc_ev_enqueue_upload_end;
	  data_event_info.data_event.event_type = prof_info->event_type;
	  GOMP_PLUGIN_goacc_profiling_dispatch (prof_info, &data_event_info,
						api_info);
	}
    }

  nvptx_exec (fn, mapnum, hostaddrs, devaddrs, dims, targ_mem_desc,
	      dp, aq->cuda_stream);

  if (mapnum > 0)
    GOMP_OFFLOAD_openacc_async_queue_callback (aq, cuda_free_argmem, block);
}

void *
GOMP_OFFLOAD_openacc_create_thread_data (int ord)
{
  struct ptx_device *ptx_dev;
  struct nvptx_thread *nvthd
    = GOMP_PLUGIN_malloc (sizeof (struct nvptx_thread));
  CUcontext thd_ctx;

  ptx_dev = ptx_devices[ord];

  assert (ptx_dev);

  CUDA_CALL_ASSERT (cuCtxGetCurrent, &thd_ctx);

  assert (ptx_dev->ctx);

  if (!thd_ctx)
    CUDA_CALL_ASSERT (cuCtxPushCurrent, ptx_dev->ctx);

  nvthd->ptx_dev = ptx_dev;

  return (void *) nvthd;
}

void
GOMP_OFFLOAD_openacc_destroy_thread_data (void *data)
{
  free (data);
}

void *
GOMP_OFFLOAD_openacc_cuda_get_current_device (void)
{
  return nvptx_get_current_cuda_device ();
}

void *
GOMP_OFFLOAD_openacc_cuda_get_current_context (void)
{
  return nvptx_get_current_cuda_context ();
}

/* This returns a CUstream.  */
void *
GOMP_OFFLOAD_openacc_cuda_get_stream (struct goacc_asyncqueue *aq)
{
  return (void *) aq->cuda_stream;
}

/* This takes a CUstream.  */
int
GOMP_OFFLOAD_openacc_cuda_set_stream (struct goacc_asyncqueue *aq, void *stream)
{
  if (aq->cuda_stream)
    {
      CUDA_CALL_ASSERT (cuStreamSynchronize, aq->cuda_stream);
      CUDA_CALL_ASSERT (cuStreamDestroy, aq->cuda_stream);
    }

  aq->cuda_stream = (CUstream) stream;
  return 1;
}

struct goacc_asyncqueue *
GOMP_OFFLOAD_openacc_async_construct (void)
{
  CUstream stream = NULL;
  CUDA_CALL_ERET (NULL, cuStreamCreate, &stream, CU_STREAM_DEFAULT);

  struct goacc_asyncqueue *aq
    = GOMP_PLUGIN_malloc (sizeof (struct goacc_asyncqueue));
  aq->cuda_stream = stream;
  return aq;
}

bool
GOMP_OFFLOAD_openacc_async_destruct (struct goacc_asyncqueue *aq)
{
  CUDA_CALL_ERET (false, cuStreamDestroy, aq->cuda_stream);
  free (aq);
  return true;
}

int
GOMP_OFFLOAD_openacc_async_test (struct goacc_asyncqueue *aq)
{
  CUresult r = CUDA_CALL_NOCHECK (cuStreamQuery, aq->cuda_stream);
  if (r == CUDA_SUCCESS)
    return 1;
  if (r == CUDA_ERROR_NOT_READY)
    return 0;

  GOMP_PLUGIN_error ("cuStreamQuery error: %s", cuda_error (r));
  return -1;
}

bool
GOMP_OFFLOAD_openacc_async_synchronize (struct goacc_asyncqueue *aq)
{
  CUDA_CALL_ERET (false, cuStreamSynchronize, aq->cuda_stream);
  return true;
}

bool
GOMP_OFFLOAD_openacc_async_serialize (struct goacc_asyncqueue *aq1,
				      struct goacc_asyncqueue *aq2)
{
  CUevent e;
  CUDA_CALL_ERET (false, cuEventCreate, &e, CU_EVENT_DISABLE_TIMING);
  CUDA_CALL_ERET (false, cuEventRecord, e, aq1->cuda_stream);
  CUDA_CALL_ERET (false, cuStreamWaitEvent, aq2->cuda_stream, e, 0);
  return true;
}

static void
cuda_callback_wrapper (CUstream stream, CUresult res, void *ptr)
{
  if (res != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("%s error: %s", __FUNCTION__, cuda_error (res));
  struct nvptx_callback *cb = (struct nvptx_callback *) ptr;
  cb->fn (cb->ptr);
  free (ptr);
}

void
GOMP_OFFLOAD_openacc_async_queue_callback (struct goacc_asyncqueue *aq,
					   void (*callback_fn)(void *),
					   void *userptr)
{
  struct nvptx_callback *b = GOMP_PLUGIN_malloc (sizeof (*b));
  b->fn = callback_fn;
  b->ptr = userptr;
  b->aq = aq;
  CUDA_CALL_ASSERT (cuStreamAddCallback, aq->cuda_stream,
		    cuda_callback_wrapper, (void *) b, 0);
}

static bool
cuda_memcpy_sanity_check (const void *h, const void *d, size_t s)
{
  CUdeviceptr pb;
  size_t ps;
  if (!s)
    return true;
  if (!d)
    {
      GOMP_PLUGIN_error ("invalid device address");
      return false;
    }
  CUDA_CALL (cuMemGetAddressRange, &pb, &ps, (CUdeviceptr) d);
  if (!pb)
    {
      GOMP_PLUGIN_error ("invalid device address");
      return false;
    }
  if (!h)
    {
      GOMP_PLUGIN_error ("invalid host address");
      return false;
    }
  if (d == h)
    {
      GOMP_PLUGIN_error ("invalid host or device address");
      return false;
    }
  if ((void *)(d + s) > (void *)(pb + ps))
    {
      GOMP_PLUGIN_error ("invalid size");
      return false;
    }
  return true;
}

bool
GOMP_OFFLOAD_host2dev (int ord, void *dst, const void *src, size_t n)
{
  if (!nvptx_attach_host_thread_to_device (ord)
      || !cuda_memcpy_sanity_check (src, dst, n))
    return false;
  CUDA_CALL (cuMemcpyHtoD, (CUdeviceptr) dst, src, n);
  return true;
}

bool
GOMP_OFFLOAD_dev2host (int ord, void *dst, const void *src, size_t n)
{
  if (!nvptx_attach_host_thread_to_device (ord)
      || !cuda_memcpy_sanity_check (dst, src, n))
    return false;
  CUDA_CALL (cuMemcpyDtoH, dst, (CUdeviceptr) src, n);
  return true;
}

bool
GOMP_OFFLOAD_dev2dev (int ord, void *dst, const void *src, size_t n)
{
  CUDA_CALL (cuMemcpyDtoDAsync, (CUdeviceptr) dst, (CUdeviceptr) src, n, NULL);
  return true;
}

bool
GOMP_OFFLOAD_openacc_async_host2dev (int ord, void *dst, const void *src,
				     size_t n, struct goacc_asyncqueue *aq)
{
  if (!nvptx_attach_host_thread_to_device (ord)
      || !cuda_memcpy_sanity_check (src, dst, n))
    return false;
  CUDA_CALL (cuMemcpyHtoDAsync, (CUdeviceptr) dst, src, n, aq->cuda_stream);
  return true;
}

bool
GOMP_OFFLOAD_openacc_async_dev2host (int ord, void *dst, const void *src,
				     size_t n, struct goacc_asyncqueue *aq)
{
  if (!nvptx_attach_host_thread_to_device (ord)
      || !cuda_memcpy_sanity_check (dst, src, n))
    return false;
  CUDA_CALL (cuMemcpyDtoHAsync, dst, (CUdeviceptr) src, n, aq->cuda_stream);
  return true;
}

/* Adjust launch dimensions: pick good values for number of blocks and warps
   and ensure that number of warps does not exceed CUDA limits as well as GCC's
   own limits.  */

static void
nvptx_adjust_launch_bounds (struct targ_fn_descriptor *fn,
			    struct ptx_device *ptx_dev,
			    int *teams_p, int *threads_p)
{
  int max_warps_block = fn->max_threads_per_block / 32;
  /* Maximum 32 warps per block is an implementation limit in NVPTX backend
     and libgcc, which matches documented limit of all GPUs as of 2015.  */
  if (max_warps_block > 32)
    max_warps_block = 32;
  if (*threads_p <= 0)
    *threads_p = 8;
  if (*threads_p > max_warps_block)
    *threads_p = max_warps_block;

  int regs_per_block = fn->regs_per_thread * 32 * *threads_p;
  /* This is an estimate of how many blocks the device can host simultaneously.
     Actual limit, which may be lower, can be queried with "occupancy control"
     driver interface (since CUDA 6.0).  */
  int max_blocks = ptx_dev->regs_per_sm / regs_per_block * ptx_dev->num_sms;
  if (*teams_p <= 0 || *teams_p > max_blocks)
    *teams_p = max_blocks;
}

/* Return the size of per-warp stacks (see gcc -msoft-stack) to use for OpenMP
   target regions.  */

static size_t
nvptx_stacks_size ()
{
  return 128 * 1024;
}

/* Return contiguous storage for NUM stacks, each SIZE bytes.  */

static void *
nvptx_stacks_alloc (size_t size, int num)
{
  CUdeviceptr stacks;
  CUresult r = CUDA_CALL_NOCHECK (cuMemAlloc, &stacks, size * num);
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuMemAlloc error: %s", cuda_error (r));
  return (void *) stacks;
}

/* Release storage previously allocated by nvptx_stacks_alloc.  */

static void
nvptx_stacks_free (void *p, int num)
{
  CUresult r = CUDA_CALL_NOCHECK (cuMemFree, (CUdeviceptr) p);
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuMemFree error: %s", cuda_error (r));
}

void
GOMP_OFFLOAD_run (int ord, void *tgt_fn, void *tgt_vars, void **args)
{
  CUfunction function = ((struct targ_fn_descriptor *) tgt_fn)->fn;
  CUresult r;
  struct ptx_device *ptx_dev = ptx_devices[ord];
  const char *maybe_abort_msg = "(perhaps abort was called)";
  int teams = 0, threads = 0;

  if (!args)
    GOMP_PLUGIN_fatal ("No target arguments provided");
  while (*args)
    {
      intptr_t id = (intptr_t) *args++, val;
      if (id & GOMP_TARGET_ARG_SUBSEQUENT_PARAM)
	val = (intptr_t) *args++;
      else
        val = id >> GOMP_TARGET_ARG_VALUE_SHIFT;
      if ((id & GOMP_TARGET_ARG_DEVICE_MASK) != GOMP_TARGET_ARG_DEVICE_ALL)
	continue;
      val = val > INT_MAX ? INT_MAX : val;
      id &= GOMP_TARGET_ARG_ID_MASK;
      if (id == GOMP_TARGET_ARG_NUM_TEAMS)
	teams = val;
      else if (id == GOMP_TARGET_ARG_THREAD_LIMIT)
	threads = val;
    }
  nvptx_adjust_launch_bounds (tgt_fn, ptx_dev, &teams, &threads);

  size_t stack_size = nvptx_stacks_size ();
  void *stacks = nvptx_stacks_alloc (stack_size, teams * threads);
  void *fn_args[] = {tgt_vars, stacks, (void *) stack_size};
  size_t fn_args_size = sizeof fn_args;
  void *config[] = {
    CU_LAUNCH_PARAM_BUFFER_POINTER, fn_args,
    CU_LAUNCH_PARAM_BUFFER_SIZE, &fn_args_size,
    CU_LAUNCH_PARAM_END
  };
  r = CUDA_CALL_NOCHECK (cuLaunchKernel, function, teams, 1, 1,
			 32, threads, 1, 0, NULL, NULL, config);
  if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuLaunchKernel error: %s", cuda_error (r));

  r = CUDA_CALL_NOCHECK (cuCtxSynchronize, );
  if (r == CUDA_ERROR_LAUNCH_FAILED)
    GOMP_PLUGIN_fatal ("cuCtxSynchronize error: %s %s\n", cuda_error (r),
		       maybe_abort_msg);
  else if (r != CUDA_SUCCESS)
    GOMP_PLUGIN_fatal ("cuCtxSynchronize error: %s", cuda_error (r));
  nvptx_stacks_free (stacks, teams * threads);
}

void
GOMP_OFFLOAD_async_run (int ord, void *tgt_fn, void *tgt_vars, void **args,
			void *async_data)
{
  GOMP_PLUGIN_fatal ("GOMP_OFFLOAD_async_run unimplemented");
}
