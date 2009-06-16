/* Basile's static analysis (should have a better name) basilys.c
   Copyright (C) 2008, 2009 Free Software Foundation, Inc.
   Contributed by Basile Starynkevitch <basile@starynkevitch.net>
   Indented with GNU indent 

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.   If not see
<http://www.gnu.org/licenses/>.   
  */

/* for debugging -fmelt-debug is useful */


#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "obstack.h"
#include "tm.h"
#include "tree.h"
#include "gimple.h"
#include "intl.h"
#include "filenames.h"
#include "tree-pass.h"
#include "tree-dump.h"
#include "tree-flow.h"
#include "tree-iterator.h"
#include "tree-inline.h"
#include "basic-block.h"
#include "timevar.h"
#include "ggc.h"
#include "cgraph.h"
#include "diagnostic.h"
#include "flags.h"
#include "toplev.h"
#include "options.h"
#include "params.h"
#include "real.h"
#include "prefix.h"
#include "md5.h"
#include "plugin.h"

/* executable_checksum is declared in c-common.h which we can't
   include here.. */
extern const unsigned char executable_checksum[16];

#include "cppdefault.h"



#include <dirent.h>

#if HAVE_PARMAPOLY
#include <ppl_c.h>
#else
#error required parma polyedral library PPL
#endif /*HAVE_PARMAPOLY */

#if HAVE_LIBTOOLDYNL
#include <ltdl.h>
#else
#error required libtool dynamic loader library LTDL
#endif /*HAVE_LIBTOOLDYNL */

/* basilysgc_sort_multiple needs setjmp */
#include <setjmp.h>

/* we need GDBM here */
#include <gdbm.h>

#include "basilys.h"


#define MINOR_SIZE_KILOWORD PARAM_VALUE(PARAM_BASILYS_MINOR_ZONE)
#define FULL_THRESHOLD PARAM_VALUE(PARAM_BASILYS_FULL_THRESHOLD)

#ifndef MELT_PRIVATE_INCLUDE_DIR
#error MELT_PRIVATE_INCLUDE_DIR is not defined thru compile flags
#endif

#ifndef MELT_SOURCE_DIR
#error MELT_SOURCE_DIR is not defined thru compile flags
#endif

#ifndef MELT_GENERATED_DIR
#error MELT_GENERATED_DIR is not defined thru compile flags
#endif

#ifndef MELT_DYNLIB_DIR
#error MELT_DYNLIB_DIR is not defined thru compile flags
#endif

#ifndef MELT_COMPILE_SCRIPT
#error MELT_COMPILE_SCRIPT  is not defined thru compile flags
#endif

#ifndef MELT_DEFAULT_MODLIS
#error MELT_DEFAULT_MODLIS is not defined thru compile flags
#endif

/* we use the plugin registration facilities, so this is the plugin
   name in use */
#define MELT_PLUGIN_NAME "__MELT"


/* *INDENT-OFF* */
static const char melt_private_include_dir[] = MELT_PRIVATE_INCLUDE_DIR;
static const char melt_source_dir[] = MELT_SOURCE_DIR;
static const char melt_generated_dir[] = MELT_GENERATED_DIR;
static const char melt_dynlib_dir[] = MELT_DYNLIB_DIR;
static const char melt_compile_script[] = MELT_COMPILE_SCRIPT;

basilys_ptr_t basilys_globarr[BGLOB__LASTGLOB];
void* basilys_startalz=NULL;
void* basilys_endalz;
char* basilys_curalz;
void** basilys_storalz;


typedef struct basilys_module_info_st
{
  void *dlh;			/* tldl handle */
  void **iniframp;		/* initial frame pointer adress */
  void (*marker_rout) (void *);	/* marking routine of initial frame */
    basilys_ptr_t (*start_rout) (basilys_ptr_t);	/* start routine */
} basilys_module_info_t;

DEF_VEC_O (basilys_module_info_t);
DEF_VEC_ALLOC_O (basilys_module_info_t, heap);

static VEC (basilys_module_info_t, heap) *modinfvec = 0;

struct callframe_basilys_st* basilys_topframe; 
struct basilyslocalsptr_st* basilys_localtab; 
struct basilysspecial_st* basilys_newspeclist;
struct basilysspecial_st* basilys_oldspeclist;
unsigned long basilys_kilowords_sincefull;
/* number of full & any basilys garbage collections */
unsigned long basilys_nb_full_garbcoll;
unsigned long basilys_nb_garbcoll;
void* basilys_touched_cache[BASILYS_TOUCHED_CACHE_SIZE];
bool basilys_prohibit_garbcoll;

long basilys_dbgcounter;
long basilys_debugskipcount;


int basilys_last_global_ix = BGLOB__LASTGLOB;

/* our copying garbage collector needs a vector of basilys_ptr_t to
   scan and an hashtable of basilys_ptr_t which are local variables
   copied into GGC heap;  */
static GTY(()) VEC(basilys_ptr_t,gc) *bscanvec;


struct GTY(())  basilocalsptr_st {
  unsigned char lenix;			/* length is prime, this is the index of length */
  int nbent;
  basilys_ptr_t  GTY((length("basilys_primtab[%h.lenix]"))) ptrtab[FLEXIBLE_DIM];
};

static GTY(()) struct basilocalsptr_st* blocaltab; 

static lt_dlhandle proghandle;


/* *INDENT-ON* */

/* to code case ALL_OBMAG_SPECIAL_CASES: */
#define ALL_OBMAG_SPECIAL_CASES			\
         OBMAG_SPEC_FILE:			\
    case OBMAG_SPECPPL_COEFFICIENT:   		\
    case OBMAG_SPECPPL_LINEAR_EXPRESSION:	\
    case OBMAG_SPECPPL_CONSTRAINT:		\
    case OBMAG_SPECPPL_CONSTRAINT_SYSTEM:	\
    case OBMAG_SPECPPL_GENERATOR:		\
    case OBMAG_SPECPPL_GENERATOR_SYSTEM:	\
    case OBMAG_SPECPPL_POLYHEDRON:		\
    case OBMAG_SPEC_MPFR

/* Obstack used for reading names */
static struct obstack bname_obstack;




static inline void
delete_special (struct basilysspecial_st *sp)
{
  switch (sp->discr->object_magic)
    {
    case OBMAG_SPEC_FILE:
      if (sp->val.sp_file)
	{
	  fclose (sp->val.sp_file);
	  sp->val.sp_file = NULL;
	};
      break;
    case OBMAG_SPEC_MPFR:
      if (sp->val.sp_mpfr)
	{
	  mpfr_clear ((mpfr_ptr) (sp->val.sp_mpfr));
	  free (sp->val.sp_mpfr);
	  sp->val.sp_mpfr = NULL;
	};
      break;
    case OBMAG_SPECPPL_COEFFICIENT:
      if (sp->val.sp_coefficient)
	ppl_delete_Coefficient (sp->val.sp_coefficient);
      sp->val.sp_coefficient = NULL;
      break;
    case OBMAG_SPECPPL_LINEAR_EXPRESSION:
      if (sp->val.sp_linear_expression)
	ppl_delete_Linear_Expression (sp->val.sp_linear_expression);
      sp->val.sp_linear_expression = NULL;
      break;
    case OBMAG_SPECPPL_CONSTRAINT:
      if (sp->val.sp_constraint)
	ppl_delete_Constraint (sp->val.sp_constraint);
      sp->val.sp_constraint = NULL;
      break;
    case OBMAG_SPECPPL_CONSTRAINT_SYSTEM:
      if (sp->val.sp_constraint_system)
	ppl_delete_Constraint_System (sp->val.sp_constraint_system);
      sp->val.sp_constraint_system = NULL;
      break;
    case OBMAG_SPECPPL_GENERATOR:
      if (sp->val.sp_generator)
	ppl_delete_Generator (sp->val.sp_generator);
      sp->val.sp_generator = NULL;
      break;
    case OBMAG_SPECPPL_GENERATOR_SYSTEM:
      if (sp->val.sp_generator_system)
	ppl_delete_Generator_System (sp->val.sp_generator_system);
      sp->val.sp_generator_system = NULL;
      break;
    case OBMAG_SPECPPL_POLYHEDRON:
      if (sp->val.sp_polyhedron)
	ppl_delete_Polyhedron (sp->val.sp_polyhedron);
      sp->val.sp_polyhedron = NULL;
      break;
    default:
      break;
    }
}

#define FORWARDED_DISCR (basilysobject_ptr_t)1
static basilys_ptr_t forwarded_copy (basilys_ptr_t);

#ifdef ENABLE_CHECKING
/* only for debugging, to be set from the debugger */
static void *bstrangelocal;
static long nbaddlocalptr;

static FILE *debughack_file;
FILE *basilys_dbgtracefile;
void *basilys_checkedp_ptr1;
void *basilys_checkedp_ptr2;
#endif /*ENABLE_CHECKING */

/*** GDBM state ****/
static GDBM_FILE gdbm_basilys;


static inline void *
forwarded (void *ptr)
{
  basilys_ptr_t p = (basilys_ptr_t) ptr;
  if (p && basilys_is_young (p))
    {
      if (p->u_discr == FORWARDED_DISCR)
	p = ((struct basilysforward_st *) p)->forward;
      else
	p = forwarded_copy (p);
    }
  return p;
}

#if GCC_VERSION > 4000
#define FORWARDED(P) do {if (P) { \
  (P) = (__typeof__(P))forwarded((void*)(P));} } while(0)
#else
#define FORWARDED(P) do {if (P) { 		       		\
       (P) = (basilys_ptr_t)forwarded((basilys_ptr_t)(P));} }  while(0)
#endif
static void scanning (basilys_ptr_t);


static void
add_localptr (basilys_ptr_t p)
{
  HOST_WIDE_INT ix;
  int h, k;
  long primsiz = basilys_primtab[blocaltab->lenix];
  if (!p)
    return;
#ifdef ENABLE_CHECKING
  nbaddlocalptr++;
  if (p == bstrangelocal)
    {
      debugeprintf ("adding #%ld bstrangelocal %p", nbaddlocalptr,
		    (void *) p);
    }
#endif
  gcc_assert ((void *) p != (void *) FORWARDED_DISCR);
  gcc_assert (primsiz > 0);
  ix = (HOST_WIDE_INT) p;
  ix ^= ((HOST_WIDE_INT) p) >> 11;
  ix &= 0x3fffffff;
  h = (int) ix % primsiz;
  for (k = h; k < primsiz; k++)
    {
      if (!blocaltab->ptrtab[k])
	{
	  blocaltab->ptrtab[k] = p;
	  blocaltab->nbent++;
	  return;
	}
      else if (blocaltab->ptrtab[k] == p)
	return;
    }
  for (k = 0; k < h; k++)
    {
      if (!blocaltab->ptrtab[k])
	{
	  blocaltab->ptrtab[k] = p;
	  blocaltab->nbent++;
	  return;
	}
      else if (blocaltab->ptrtab[k] == p)
	return;
    }
  /* the only way to reach this point is that blocaltab is
     full; this should never happen, since it was allocated bigger
     than the number of locals! */
  fatal_error ("corrupted bloctab foradd_localptr p=%p", (void *) p);
}


#if ENABLE_CHECKING
/***
 * check our call frames
 ***/
static inline void
check_pointer_at (const char msg[], long count, basilys_ptr_t * pptr,
		  const char *filenam, int lineno)
{
  basilys_ptr_t ptr = *pptr;
  if (!ptr)
    return;
  if (!ptr->u_discr)
    fatal_error
      ("<%s#%ld> corrupted pointer %p (at %p) without discr at %s:%d", msg,
       count, (void *) ptr, (void *) pptr, basename (filenam), lineno);
  switch (ptr->u_discr->object_magic)
    {
    case OBMAG_OBJECT:
    case OBMAG_DECAY:
    case OBMAG_BOX:
    case OBMAG_MULTIPLE:
    case OBMAG_CLOSURE:
    case OBMAG_ROUTINE:
    case OBMAG_LIST:
    case OBMAG_PAIR:
    case OBMAG_TRIPLE:
    case OBMAG_INT:
    case OBMAG_MIXINT:
    case OBMAG_MIXLOC:
    case OBMAG_MIXBIGINT:
    case OBMAG_REAL:
    case OBMAG_STRING:
    case OBMAG_STRBUF:
    case OBMAG_TREE:
    case OBMAG_GIMPLE:
    case OBMAG_GIMPLESEQ:
    case OBMAG_BASICBLOCK:
    case OBMAG_EDGE:
    case OBMAG_MAPOBJECTS:
    case OBMAG_MAPTREES:
    case OBMAG_MAPGIMPLES:
    case OBMAG_MAPGIMPLESEQS:
    case OBMAG_MAPSTRINGS:
    case OBMAG_MAPBASICBLOCKS:
    case OBMAG_MAPEDGES:
    case ALL_OBMAG_SPECIAL_CASES:
      break;
    default:
      fatal_error ("<%s#%ld> bad pointer %p (at %p) bad magic %d at %s:%d",
		   msg, count, (void *) ptr, (void *) pptr,
		   (int) ptr->u_discr->object_magic, basename (filenam),
		   lineno);
    }
}

static long nbcheckcallframes;
static long thresholdcheckcallframes;


void
basilys_check_call_frames_at (int noyoungflag, const char *msg,
			      const char *filenam, int lineno)
{
  struct callframe_basilys_st *cfram = NULL;
  int nbfram = 0, nbvar = 0;
  nbcheckcallframes++;
  if (!msg)
    msg = "/";
  if (thresholdcheckcallframes > 0
      && nbcheckcallframes > thresholdcheckcallframes)
    {
      debugeprintf
	("start check_call_frames#%ld {%s} from %s:%d",
	 nbcheckcallframes, msg, basename (filenam), lineno);
    }
  for (cfram = basilys_topframe; cfram != NULL; cfram = cfram->prev)
    {
      int varix = 0;
      nbfram++;
      if (cfram->clos)
	{
	  if (noyoungflag && basilys_is_young (cfram->clos))
	    fatal_error
	      ("bad frame <%s#%ld> unexpected young closure %p in frame %p at %s:%d",
	       msg, nbcheckcallframes,
	       (void *) cfram->clos, (void *) cfram, basename (filenam),
	       lineno);

	  check_pointer_at (msg, nbcheckcallframes,
			    (basilys_ptr_t *) (void *) &cfram->clos, filenam,
			    lineno);
	  if (cfram->clos->discr->object_magic != OBMAG_CLOSURE)
	    fatal_error
	      ("bad frame <%s#%ld> invalid closure %p in frame %p at %s:%d",
	       msg, nbcheckcallframes,
	       (void *) cfram->clos, (void *) cfram, basename (filenam),
	       lineno);
	}
      for (varix = ((int) cfram->nbvar) - 1; varix >= 0; varix--)
	{
	  nbvar++;
	  if (noyoungflag && cfram->varptr[varix] != NULL
	      && basilys_is_young (cfram->varptr[varix]))
	    fatal_error
	      ("bad frame <%s#%ld> unexpected young pointer %p in frame %p at %s:%d",
	       msg, nbcheckcallframes, (void *) cfram->varptr[varix],
	       (void *) cfram, basename (filenam), lineno);

	  check_pointer_at (msg, nbcheckcallframes, &cfram->varptr[varix],
			    filenam, lineno);
	}
    }
  if (thresholdcheckcallframes > 0
      && nbcheckcallframes > thresholdcheckcallframes)
    debugeprintf ("end check_call_frames#%ld {%s} %d frames/%d vars %s:%d",
		  nbcheckcallframes, msg, nbfram, nbvar, basename (filenam),
		  lineno);
  if (debughack_file)
    {
      fprintf (debughack_file,
	       "check_call_frames#%ld {%s} %d frames/%d vars %s:%d\n",
	       nbcheckcallframes, msg, nbfram, nbvar, basename (filenam),
	       lineno);
      fflush (debughack_file);
    }
}

void
basilys_caught_assign_at (void *ptr, const char *fil, int lin,
			  const char *msg)
{
  if (debughack_file)
    {
      fprintf (debughack_file, "caught assign %p at %s:%d /// %s\n", ptr,
	       basename (fil), lin, msg);
      fflush (debughack_file);
    }
  debugeprintf ("caught assign %p at %s:%d /// %s", ptr, basename (fil), lin,
		msg);
}

static long nbcbreak;

void
basilys_cbreak_at (const char *msg, const char *fil, int lin)
{
  nbcbreak++;
  if (debughack_file)
    {
      fprintf (debughack_file, "CBREAK#%ld AT %s:%d - %s\n", nbcbreak,
	       basename (fil), lin, msg);
      fflush (debughack_file);
    };
  debugeprintf_raw ("%s:%d: CBREAK#%ld %s\n", basename (fil), lin, nbcbreak,
		    msg);
}

#endif


/***
 * the marking routine is registered thru PLUGIN_GGC_MARKING
 * it makes GGC play nice with MELT.
 **/
static void
basilys_marking_callback (void *gcc_data ATTRIBUTE_UNUSED,
			  void* user_data ATTRIBUTE_UNUSED)
{
  int ix = 0;
  basilys_module_info_t *mi = 0;
  struct callframe_basilys_st *cf = 0;
  /* first, scan all the modules and mark their frame if it is non null */
  if (modinfvec) 
    for (ix = 0; VEC_iterate (basilys_module_info_t, modinfvec, ix, mi); ix++)
      {
        if ( !mi->marker_rout || !mi->iniframp || !*mi->iniframp) 
	  continue;
        (mi->marker_rout) (*mi->iniframp);
      };
  /* then scan all the MELT call frames */
  for (cf = (struct callframe_basilys_st*) basilys_topframe; cf; cf = cf->prev) {
    if (cf->clos) {
      basilysroutfun_t*funp = 0;
      gcc_assert(cf->clos->rout);
      funp = cf->clos->rout->routfunad;
      gcc_assert(funp);
      /* call the function specially with the MARKGCC special parameter descriptor */
      funp(cf->clos, (basilys_ptr_t)cf, BASILYSPAR_MARKGGC, 
	   (union basilysparam_un*)0, (char*)0, (union basilysparam_un*)0);
      continue;
    }
    else 
      /* if no closure, mark the local pointers */
      for (ix= 0; ix<(int) cf->nbvar; ix++) 
	if (cf->varptr[ix]) 
	  gt_ggc_mx_basilys_un((basilys_ptr_t)(cf->varptr[ix]));
  }
}

/***
 * our copying garbage collector 
 ***/
void
basilys_garbcoll (size_t wanted, bool needfull)
{
  long primix = 0;
  int locdepth = 0;
  int nbloc = 0;
  int nbglob = 0;
  int locsiz = 0;
  int ix = 0;
  struct callframe_basilys_st *cfram = NULL;
  basilys_ptr_t *storp = NULL;
  struct basilysspecial_st *specp = NULL;
  struct basilysspecial_st **prevspecptr = NULL;
  struct basilysspecial_st *nextspecp = NULL;
  if (basilys_prohibit_garbcoll)
    fatal_error ("basilys garbage collection prohibited");
  basilys_nb_garbcoll++;
  basilys_check_call_frames (BASILYS_ANYWHERE, "before garbage collection");
  gcc_assert ((char *) basilys_startalz < (char *) basilys_endalz);
  gcc_assert ((char *) basilys_curalz >= (char *) basilys_startalz
	      && (char *) basilys_curalz < (char *) basilys_storalz);
  gcc_assert ((char *) basilys_storalz < (char *) basilys_endalz);
  bscanvec = VEC_alloc (basilys_ptr_t, gc, 1024 + 32 * MINOR_SIZE_KILOWORD);
  wanted += wanted / 4 + MINOR_SIZE_KILOWORD * 1000;
  wanted |= 0x3fff;
  wanted++;
  if (wanted < MINOR_SIZE_KILOWORD * sizeof (void *) * 1024)
    wanted = MINOR_SIZE_KILOWORD * sizeof (void *) * 1024;
  /* compute number of locals and depth of call stack */
  nbglob = BGLOB__LASTGLOB;
  for (cfram = basilys_topframe; cfram != NULL; cfram = cfram->prev)
    {
      locdepth++;
      /* we should never have more than a few thousand locals in a
         call frame, so we check this */
      gcc_assert (cfram->nbvar < (int) BASILYS_MAXNBLOCALVAR);
      nbloc += cfram->nbvar;
    }
  locsiz = 200 + (5 * (locdepth + nbloc + nbglob + 100)) / 4;
  locsiz |= 0xff;
  for (primix = 5;
       basilys_primtab[primix] > 0
       && basilys_primtab[primix] <= locsiz; primix++);
  locsiz = basilys_primtab[primix];
  gcc_assert (locsiz > 10);
  blocaltab =
    (struct basilocalsptr_st *)
    ggc_alloc_cleared (sizeof (struct basilocalsptr_st) +
		       locsiz * sizeof (void *));
  blocaltab->lenix = primix;
  for (ix = 0; ix < BGLOB__LASTGLOB; ix++)
    FORWARDED (basilys_globarr[ix]);
  for (cfram = basilys_topframe; cfram != NULL; cfram = cfram->prev)
    {
      int varix;
      if (cfram->clos)
	{
	  FORWARDED (cfram->clos);
	  add_localptr ((basilys_ptr_t) (cfram->clos));
	}
      for (varix = ((int) cfram->nbvar) - 1; varix >= 0; varix--)
	{
	  if (!cfram->varptr[varix])
	    continue;
	  FORWARDED (cfram->varptr[varix]);
	  add_localptr (cfram->varptr[varix]);
	}
    };
  /* scan the store list */
  for (storp = (basilys_ptr_t *) basilys_storalz;
       (char *) storp < (char *) basilys_endalz; storp++)
    {
      if (*storp)
	scanning (*storp);
    }
  memset (basilys_touched_cache, 0, sizeof (basilys_touched_cache));
  /* sort of Cheney loop; http://en.wikipedia.org/wiki/Cheney%27s_algorithm */
  while (!VEC_empty (basilys_ptr_t, bscanvec))
    {
      basilys_ptr_t p = VEC_pop (basilys_ptr_t, bscanvec);
      if (!p)
	continue;
#if ENABLE_CHECKING
      if (debughack_file)
	fprintf (debughack_file, "cheney scan %p\n", (void *) p);
#endif
      scanning (p);
    }
  /* delete every unmarked special on the new list and clear it */
  for (specp = basilys_newspeclist; specp; specp = specp->nextspec)
    {
      gcc_assert (basilys_is_young (specp));
      if (specp->mark)
	continue;
      delete_special (specp);
    }
  basilys_newspeclist = NULL;
  /* free the previous young zone and allocate a new one */
#if ENABLE_CHECKING
  if (debughack_file)
    {
      fprintf (debughack_file,
	       "%s:%d free previous young %p - %p GC#%ld\n",
	       basename (__FILE__), __LINE__, basilys_startalz,
	       basilys_endalz, basilys_nb_garbcoll);
      fflush (debughack_file);
    }
  memset (basilys_startalz, 0,
	  (char *) basilys_endalz - (char *) basilys_startalz);
#endif
  free (basilys_startalz);
  basilys_startalz = basilys_endalz = basilys_curalz = NULL;
  basilys_storalz = NULL;
  basilys_kilowords_sincefull += wanted / (1024 * sizeof (void *));
  if (basilys_kilowords_sincefull >
      (unsigned long) MINOR_SIZE_KILOWORD * FULL_THRESHOLD)
    needfull = TRUE;
  basilys_startalz = basilys_curalz =
    (char *) xcalloc (sizeof (void *), wanted / sizeof (void *));
  basilys_endalz = (char *) basilys_curalz + wanted;
  basilys_storalz = ((void **) basilys_endalz) - 2;
  if (needfull)
    {
      bool wasforced = ggc_force_collect;
      basilys_nb_full_garbcoll++;
      debugeprintf ("basilys_garbcoll #%ld fullgarbcoll #%ld",
		    basilys_nb_garbcoll, basilys_nb_full_garbcoll);
      /* clear marks on the old spec list */
      for (specp = basilys_oldspeclist; specp; specp = specp->nextspec)
	specp->mark = 0;
      /* force major collection, with our callback */
      ggc_force_collect = true;
      ggc_collect ();
      ggc_force_collect = wasforced;
      /* delete the unmarked spec */
      prevspecptr = &basilys_oldspeclist;
      for (specp = basilys_oldspeclist; specp; specp = nextspecp)
	{
	  nextspecp = specp->nextspec;
	  if (specp->mark)
	    {
	      prevspecptr = &specp->nextspec;
	      continue;
	    }
	  delete_special (specp);
	  memset (specp, 0, sizeof (*specp));
	  ggc_free (specp);
	  *prevspecptr = nextspecp;
	}
      basilys_kilowords_sincefull = 0;
    }
  ggc_free (blocaltab);
  blocaltab = NULL;
  ggc_free (bscanvec);
  bscanvec = NULL;
  basilys_check_call_frames (BASILYS_NOYOUNG, "after garbage collection");
}


/* the inline function basilys_allocatereserved is the only one
   calling this basilys_reserved_allocation_failure function, which
   should never be called. If it is indeed called, you've been bitten
   by a severe bug. In principle basilys_allocatereserved should have
   been called with a suitable previous call to basilysgc_reserve such
   that all the reserved allocations fits into the reserved size */
void
basilys_reserved_allocation_failure (long siz)
{
  /* this function should never really be called */
  fatal_error ("memory corruption in basilys reserved allocation: "
	       "requiring %ld bytes but only %ld available in young zone",
	       siz,
	       (long) ((char *) basilys_storalz - (char *) basilys_curalz));
}

/* cheney like forwarding */
static basilys_ptr_t
forwarded_copy (basilys_ptr_t p)
{
  basilys_ptr_t n = 0;
  int mag = 0;
  gcc_assert (basilys_is_young (p));
  gcc_assert (p->u_discr && p->u_discr != FORWARDED_DISCR);
  if (p->u_discr->obj_class == FORWARDED_DISCR)
    mag =
      ((basilysobject_ptr_t)
       (((struct basilysforward_st *) p->u_discr)->forward))->object_magic;
  else
    mag = p->u_discr->object_magic;
  /***
   * we can copy *dst = *src only for structures which do not use
   * FLEXIBLE_DIM; for those that do and which are "empty" this is not
   * possible, since when FLEXIBLE_DIM is 1 it would overwrite
   * something else. 
   *
   * I really hate the C dialect which long time ago
   * prohibited zero-length arrays.
   ***/
  switch (mag)
    {
    case OBMAG_OBJECT:
      {
	struct basilysobject_st *src = (struct basilysobject_st *) p;
	unsigned oblen = src->obj_len;
	struct basilysobject_st *dst = (struct basilysobject_st *)
	  ggc_alloc_cleared (offsetof (struct basilysobject_st,
				       obj_vartab) +
			     oblen * sizeof (src->obj_vartab[0]));
	int ix;
	/* we cannot copy the whole src, because FLEXIBLE_DIM might be 1 */
	dst->obj_class = src->obj_class;
	dst->obj_hash = src->obj_hash;
	dst->obj_num = src->obj_num;
	dst->obj_len = src->obj_len;
#if ENABLE_CHECKING
	dst->obj_serial = src->obj_serial;
#endif
	if (oblen > 0)
	  for (ix = (int) oblen - 1; ix >= 0; ix--)
	    dst->obj_vartab[ix] = src->obj_vartab[ix];
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_DECAY:
      {
	struct basilysdecay_st *src = (struct basilysdecay_st *) p;
	struct basilysdecay_st *dst = (struct basilysdecay_st *)
	  ggc_alloc_cleared (sizeof (struct basilysdecay_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_BOX:
      {
	struct basilysbox_st *src = (struct basilysbox_st *) p;
	struct basilysbox_st *dst = (struct basilysbox_st *)
	  ggc_alloc_cleared (sizeof (struct basilysbox_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MULTIPLE:
      {
	struct basilysmultiple_st *src = (struct basilysmultiple_st *) p;
	unsigned nbv = src->nbval;
	int ix;
	struct basilysmultiple_st *dst = (struct basilysmultiple_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmultiple_st) +
			     nbv * sizeof (void *));
	/* we cannot copy the whole src, because FLEXIBLE_DIM might be
	   1 and nbval could be 0 */
	dst->discr = src->discr;
	dst->nbval = src->nbval;
	for (ix = (int) nbv; ix >= 0; ix--)
	  dst->tabval[ix] = src->tabval[ix];
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_CLOSURE:
      {
	struct basilysclosure_st *src = (struct basilysclosure_st *) p;
	unsigned nbv = src->nbval;
	int ix;
	struct basilysclosure_st *dst = (struct basilysclosure_st *)
	  ggc_alloc_cleared (sizeof (struct basilysclosure_st) +
			     nbv * sizeof (void *));
	dst->discr = src->discr;
	dst->rout = src->rout;
	dst->nbval = src->nbval;
	for (ix = (int) nbv; ix >= 0; ix--)
	  dst->tabval[ix] = src->tabval[ix];
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_ROUTINE:
      {
	struct basilysroutine_st *src = (struct basilysroutine_st *) p;
	unsigned nbv = src->nbval;
	int ix;
	struct basilysroutine_st *dst = (struct basilysroutine_st *)
	  ggc_alloc_cleared (sizeof (struct basilysroutine_st) +
			     nbv * sizeof (void *));
	dst->discr = src->discr;
	strncpy (dst->routdescr, src->routdescr, BASILYS_ROUTDESCR_LEN);
	dst->routdescr[BASILYS_ROUTDESCR_LEN - 1] = 0;
	dst->nbval = src->nbval;
	dst->routfunad = src->routfunad;
	for (ix = (int) nbv; ix >= 0; ix--)
	  dst->tabval[ix] = src->tabval[ix];
	dst->routdata = src->routdata;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_LIST:
      {
	struct basilyslist_st *src = (struct basilyslist_st *) p;
	struct basilyslist_st *dst = (struct basilyslist_st *)
	  ggc_alloc_cleared (sizeof (struct basilyslist_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_PAIR:
      {
	struct basilyspair_st *src = (struct basilyspair_st *) p;
	struct basilyspair_st *dst = (struct basilyspair_st *)
	  ggc_alloc_cleared (sizeof (struct basilyspair_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_TRIPLE:
      {
	struct basilystriple_st *src = (struct basilystriple_st *) p;
	struct basilystriple_st *dst = (struct basilystriple_st *)
	  ggc_alloc_cleared (sizeof (struct basilystriple_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_INT:
      {
	struct basilysint_st *src = (struct basilysint_st *) p;
	struct basilysint_st *dst = (struct basilysint_st *)
	  ggc_alloc_cleared (sizeof (struct basilysint_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MIXINT:
      {
	struct basilysmixint_st *src = (struct basilysmixint_st *) p;
	struct basilysmixint_st *dst = (struct basilysmixint_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmixint_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MIXLOC:
      {
	struct basilysmixloc_st *src = (struct basilysmixloc_st *) p;
	struct basilysmixloc_st *dst = (struct basilysmixloc_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmixloc_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MIXBIGINT:
      {
	struct basilysmixbigint_st *src = (struct basilysmixbigint_st *) p;
	unsigned blen = src->biglen;
	struct basilysmixbigint_st *dst = (struct basilysmixbigint_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmixbigint_st)
			     + blen * sizeof(src->tabig[0]));
	dst->discr = src->discr;
	dst->ptrval = src->ptrval;
	dst->negative = src->negative;
	dst->biglen = blen;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_REAL:
      {
	struct basilysreal_st *src = (struct basilysreal_st *) p;
	struct basilysreal_st *dst = (struct basilysreal_st *)
	  ggc_alloc_cleared (sizeof (struct basilysreal_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case ALL_OBMAG_SPECIAL_CASES:
      {
	struct basilysspecial_st *src = (struct basilysspecial_st *) p;
	struct basilysspecial_st *dst = (struct basilysspecial_st *)
	  ggc_alloc_cleared (sizeof (struct basilysspecial_st));
	*dst = *src;
	/* add the new copy to the old (major) special list */
	dst->nextspec = basilys_oldspeclist;
	basilys_oldspeclist = dst;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_STRING:
      {
	struct basilysstring_st *src = (struct basilysstring_st *) p;
	int srclen = strlen (src->val);
	struct basilysstring_st *dst = (struct basilysstring_st *)
	  ggc_alloc_cleared (sizeof (struct basilysstring_st) + srclen + 1);
	dst->discr = src->discr;
	memcpy (dst->val, src->val, srclen);
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_STRBUF:
      {
	struct basilysstrbuf_st *src = (struct basilysstrbuf_st *) p;
	unsigned blen = basilys_primtab[src->buflenix];
	struct basilysstrbuf_st *dst = (struct basilysstrbuf_st *)
	  ggc_alloc_cleared (sizeof (struct basilysstrbuf_st));
	dst->discr = src->discr;
	dst->bufstart = src->bufstart;
	dst->bufend = src->bufend;
	dst->buflenix = src->buflenix;
	if (blen > 0)
	  {
	    dst->bufzn = (char *) ggc_alloc_cleared (1 + blen);
	    memcpy (dst->bufzn, src->bufzn, blen);
	  }
	else
	  dst->bufzn = NULL;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_TREE:
      {
	struct basilystree_st *src = (struct basilystree_st *) p;
	struct basilystree_st *dst = (struct basilystree_st *)
	  ggc_alloc_cleared (sizeof (struct basilystree_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_GIMPLE:
      {
	struct basilysgimple_st *src = (struct basilysgimple_st *) p;
	struct basilysgimple_st *dst = (struct basilysgimple_st *)
	  ggc_alloc_cleared (sizeof (struct basilysgimple_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_GIMPLESEQ:
      {
	struct basilysgimpleseq_st *src = (struct basilysgimpleseq_st *) p;
	struct basilysgimpleseq_st *dst = (struct basilysgimpleseq_st *)
	  ggc_alloc_cleared (sizeof (struct basilysgimpleseq_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_BASICBLOCK:
      {
	struct basilysbasicblock_st *src = (struct basilysbasicblock_st *) p;
	struct basilysbasicblock_st *dst = (struct basilysbasicblock_st *)
	  ggc_alloc_cleared (sizeof (struct basilysbasicblock_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_EDGE:
      {
	struct basilysedge_st *src = (struct basilysedge_st *) p;
	struct basilysedge_st *dst = (struct basilysedge_st *)
	  ggc_alloc_cleared (sizeof (struct basilysedge_st));
	*dst = *src;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MAPOBJECTS:
      {
	struct basilysmapobjects_st *src = (struct basilysmapobjects_st *) p;
	int siz = basilys_primtab[src->lenix];
	struct basilysmapobjects_st *dst = (struct basilysmapobjects_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmapobjects_st));
	dst->discr = src->discr;
	dst->count = src->count;
	dst->lenix = src->lenix;
	if (siz > 0 && src->entab)
	  {
	    dst->entab =
	      (struct entryobjectsbasilys_st *) ggc_alloc_cleared (siz *
								   sizeof
								   (dst->entab
								    [0]));
	    memcpy (dst->entab, src->entab, siz * sizeof (dst->entab[0]));
	  }
	else
	  dst->entab = NULL;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MAPTREES:
      {
	struct basilysmaptrees_st *src = (struct basilysmaptrees_st *) p;
	int siz = basilys_primtab[src->lenix];
	struct basilysmaptrees_st *dst = (struct basilysmaptrees_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmaptrees_st));
	dst->discr = src->discr;
	dst->count = src->count;
	dst->lenix = src->lenix;
	if (siz > 0 && src->entab)
	  {
	    dst->entab =
	      (struct entrytreesbasilys_st *) ggc_alloc_cleared (siz *
								 sizeof
								 (dst->entab
								  [0]));
	    memcpy (dst->entab, src->entab, siz * sizeof (dst->entab[0]));
	  }
	else
	  dst->entab = NULL;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MAPGIMPLES:
      {
	struct basilysmapgimples_st *src = (struct basilysmapgimples_st *) p;
	int siz = basilys_primtab[src->lenix];
	struct basilysmapgimples_st *dst = (struct basilysmapgimples_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmapgimples_st));
	dst->discr = src->discr;
	dst->count = src->count;
	dst->lenix = src->lenix;
	if (siz > 0 && src->entab)
	  {
	    dst->entab =
	      (struct entrygimplesbasilys_st *)
	      ggc_alloc_cleared (siz * sizeof (dst->entab[0]));
	    memcpy (dst->entab, src->entab, siz * sizeof (dst->entab[0]));
	  }
	else
	  dst->entab = NULL;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MAPGIMPLESEQS:
      {
	struct basilysmapgimpleseqs_st *src =
	  (struct basilysmapgimpleseqs_st *) p;
	int siz = basilys_primtab[src->lenix];
	struct basilysmapgimpleseqs_st *dst =
	  (struct basilysmapgimpleseqs_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmapgimpleseqs_st));
	dst->discr = src->discr;
	dst->count = src->count;
	dst->lenix = src->lenix;
	if (siz > 0 && src->entab)
	  {
	    dst->entab =
	      (struct entrygimpleseqsbasilys_st *)
	      ggc_alloc_cleared (siz * sizeof (dst->entab[0]));
	    memcpy (dst->entab, src->entab, siz * sizeof (dst->entab[0]));
	  }
	else
	  dst->entab = NULL;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MAPSTRINGS:
      {
	struct basilysmapstrings_st *src = (struct basilysmapstrings_st *) p;
	int siz = basilys_primtab[src->lenix];
	struct basilysmapstrings_st *dst = (struct basilysmapstrings_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmapstrings_st));
	dst->discr = src->discr;
	dst->count = src->count;
	dst->lenix = src->lenix;
	if (siz > 0 && src->entab)
	  {
	    dst->entab =
	      (struct entrystringsbasilys_st *) ggc_alloc_cleared (siz *
								   sizeof
								   (dst->entab
								    [0]));
	    memcpy (dst->entab, src->entab, siz * sizeof (dst->entab[0]));
	  }
	else
	  dst->entab = NULL;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MAPBASICBLOCKS:
      {
	struct basilysmapbasicblocks_st *src =
	  (struct basilysmapbasicblocks_st *) p;
	int siz = basilys_primtab[src->lenix];
	struct basilysmapbasicblocks_st *dst =
	  (struct basilysmapbasicblocks_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmapbasicblocks_st));
	dst->discr = src->discr;
	dst->count = src->count;
	dst->lenix = src->lenix;
	if (siz > 0 && src->entab)
	  {
	    dst->entab =
	      (struct entrybasicblocksbasilys_st *) ggc_alloc_cleared (siz *
								       sizeof
								       (dst->
									entab
									[0]));
	    memcpy (dst->entab, src->entab, siz * sizeof (dst->entab[0]));
	  }
	else
	  dst->entab = NULL;
	n = (basilys_ptr_t) dst;
	break;
      }
    case OBMAG_MAPEDGES:
      {
	struct basilysmapedges_st *src = (struct basilysmapedges_st *) p;
	int siz = basilys_primtab[src->lenix];
	struct basilysmapedges_st *dst = (struct basilysmapedges_st *)
	  ggc_alloc_cleared (sizeof (struct basilysmapedges_st));
	dst->discr = src->discr;
	dst->count = src->count;
	dst->lenix = src->lenix;
	if (siz > 0 && src->entab)
	  {
	    dst->entab =
	      (struct entryedgesbasilys_st *) ggc_alloc_cleared (siz *
								 sizeof
								 (dst->entab
								  [0]));
	    memcpy (dst->entab, src->entab, siz * sizeof (dst->entab[0]));
	  }
	else
	  dst->entab = NULL;
	n = (basilys_ptr_t) dst;
	break;
      }
    default:
      fatal_error ("corruption: forward invalid p=%p discr=%p magic=%d",
		   (void *) p, (void *) p->u_discr, mag);
    }
  if (n)
    {
      p->u_forward.discr = FORWARDED_DISCR;
      p->u_forward.forward = n;
#ifdef ENABLE_CHECKING
      if (debughack_file)
	{
	  fprintf (debughack_file, "forwarded pushing %p to scan\n",
		   (void *) n);
	}
#endif
      VEC_safe_push (basilys_ptr_t, gc, bscanvec, n);
    }
  return n;
  /* end of forwarded_copy */
}



/* the scanning routine is mostly chesney like; however some types,
   including objects, strbuf, stringmaps, objectmaps, all the other
   *maps, contain a pointer to a non value; this pointer should be
   carefully updated if it was young */
static void
scanning (basilys_ptr_t p)
{
  unsigned omagic = 0;
  if (!p)
    return;
  gcc_assert (p != (void *) 1);
#if ENABLE_CHECKING
  if (debughack_file)
    {
      fprintf (debughack_file, "scanning %p\n", (void *) p);
    }
#endif
  gcc_assert (p->u_discr && p->u_discr != (basilysobject_ptr_t) 1);
  FORWARDED (p->u_discr);
  gcc_assert (!basilys_is_young (p));
  omagic = p->u_discr->object_magic;
  switch (omagic)
    {
    case OBMAG_OBJECT:
      {
	int ix;
	struct basilysobject_st *src = (basilysobject_ptr_t) p;
	for (ix = (int) (src->obj_len) - 1; ix >= 0; ix--)
	  FORWARDED (src->obj_vartab[ix]);
	break;
      }
    case OBMAG_DECAY:
      {
	struct basilysdecay_st *src = (struct basilysdecay_st *) p;
	FORWARDED (src->val);
	break;
      }
    case OBMAG_BOX:
      {
	struct basilysbox_st *src = (struct basilysbox_st *) p;
	FORWARDED (src->val);
	break;
      }
    case OBMAG_MULTIPLE:
      {
	struct basilysmultiple_st *src = (struct basilysmultiple_st *) p;
	unsigned nbval = src->nbval;
	int ix;
	for (ix = (int) nbval - 1; ix >= 0; ix--)
	  FORWARDED (src->tabval[ix]);
	break;
      }
    case OBMAG_CLOSURE:
      {
	struct basilysclosure_st *src = (struct basilysclosure_st *) p;
	unsigned nbval = src->nbval;
	int ix;
	FORWARDED (src->rout);
	for (ix = (int) nbval - 1; ix >= 0; ix--)
	  FORWARDED (src->tabval[ix]);
	break;
      }
    case OBMAG_ROUTINE:
      {
	struct basilysroutine_st *src = (struct basilysroutine_st *) p;
	unsigned nbval = src->nbval;
	int ix;
	for (ix = (int) nbval - 1; ix >= 0; ix--)
	  FORWARDED (src->tabval[ix]);
	break;
      }
    case OBMAG_LIST:
      {
	struct basilyslist_st *src = (struct basilyslist_st *) p;
	FORWARDED (src->first);
	FORWARDED (src->last);
	break;
      }
    case OBMAG_PAIR:
      {
	struct basilyspair_st *src = (struct basilyspair_st *) p;
	FORWARDED (src->hd);
	FORWARDED (src->tl);
	break;
      }
    case OBMAG_TRIPLE:
      {
	struct basilystriple_st *src = (struct basilystriple_st *) p;
	FORWARDED (src->hd);
	FORWARDED (src->mi);
	FORWARDED (src->tl);
	break;
      }
    case ALL_OBMAG_SPECIAL_CASES:
      {
	struct basilysspecial_st *src = (struct basilysspecial_st *) p;
	src->mark = 1;
	break;
      }
    case OBMAG_MAPOBJECTS:
      {
	struct basilysmapobjects_st *src = (struct basilysmapobjects_st *) p;
	int siz, ix;
	if (!src->entab)
	  break;
	siz = basilys_primtab[src->lenix];
	gcc_assert (siz > 0);
	if (basilys_is_young (src->entab))
	  {
	    struct entryobjectsbasilys_st *newtab =
	      (struct entryobjectsbasilys_st *) ggc_alloc_cleared (siz *
								   sizeof
								   (struct
								    entryobjectsbasilys_st));
	    memcpy (newtab, src->entab,
		    siz * sizeof (struct entryobjectsbasilys_st));
	    src->entab = newtab;
	  }
	for (ix = 0; ix < siz; ix++)
	  {
	    basilysobject_ptr_t at = src->entab[ix].e_at;
	    if (!at || at == (void *) 1)
	      {
		src->entab[ix].e_va = NULL;
		continue;
	      }
	    FORWARDED (at);
	    src->entab[ix].e_at = at;
	    FORWARDED (src->entab[ix].e_va);
	  }
	break;
      }
    case OBMAG_MAPTREES:
      {
	struct basilysmaptrees_st *src = (struct basilysmaptrees_st *) p;
	int ix, siz;
	if (!src->entab)
	  break;
	siz = basilys_primtab[src->lenix];
	gcc_assert (siz > 0);
	if (basilys_is_young (src->entab))
	  {
	    struct entrytreesbasilys_st *newtab =
	      (struct entrytreesbasilys_st *) ggc_alloc_cleared (siz *
								 sizeof
								 (struct
								  entrytreesbasilys_st));
	    memcpy (newtab, src->entab,
		    siz * sizeof (struct entrytreesbasilys_st));
	    src->entab = newtab;
	  }
	for (ix = 0; ix < siz; ix++)
	  {
	    tree at = src->entab[ix].e_at;
	    if (!at || at == (void *) 1)
	      {
		src->entab[ix].e_va = NULL;
		continue;
	      }
	    FORWARDED (src->entab[ix].e_va);
	  }
	break;
      }
    case OBMAG_MAPGIMPLES:
      {
	struct basilysmapgimples_st *src = (struct basilysmapgimples_st *) p;
	int ix, siz;
	if (!src->entab)
	  break;
	siz = basilys_primtab[src->lenix];
	gcc_assert (siz > 0);
	if (basilys_is_young (src->entab))
	  {
	    struct entrygimplesbasilys_st *newtab =
	      (struct entrygimplesbasilys_st *) ggc_alloc_cleared (siz *
								   sizeof
								   (struct
								    entrygimplesbasilys_st));
	    memcpy (newtab, src->entab,
		    siz * sizeof (struct entrygimplesbasilys_st));
	    src->entab = newtab;
	  }
	for (ix = 0; ix < siz; ix++)
	  {
	    gimple at = src->entab[ix].e_at;
	    if (!at || at == (void *) 1)
	      {
		src->entab[ix].e_va = NULL;
		continue;
	      }
	    FORWARDED (src->entab[ix].e_va);
	  }
	break;
      }
    case OBMAG_MAPGIMPLESEQS:
      {
	struct basilysmapgimpleseqs_st *src =
	  (struct basilysmapgimpleseqs_st *) p;
	int ix, siz;
	if (!src->entab)
	  break;
	siz = basilys_primtab[src->lenix];
	gcc_assert (siz > 0);
	if (basilys_is_young (src->entab))
	  {
	    struct entrygimpleseqsbasilys_st *newtab =
	      (struct entrygimpleseqsbasilys_st *)
	      ggc_alloc_cleared (siz *
				 sizeof (struct entrygimpleseqsbasilys_st));
	    memcpy (newtab, src->entab,
		    siz * sizeof (struct entrygimpleseqsbasilys_st));
	    src->entab = newtab;
	  }
	for (ix = 0; ix < siz; ix++)
	  {
	    gimple_seq at = src->entab[ix].e_at;
	    if (!at || at == (void *) 1)
	      {
		src->entab[ix].e_va = NULL;
		continue;
	      }
	    FORWARDED (src->entab[ix].e_va);
	  }
	break;
      }
    case OBMAG_MAPSTRINGS:
      {
	struct basilysmapstrings_st *src = (struct basilysmapstrings_st *) p;
	int ix, siz;
	if (!src->entab)
	  break;
	siz = basilys_primtab[src->lenix];
	gcc_assert (siz > 0);
	if (basilys_is_young (src->entab))
	  {
	    struct entrystringsbasilys_st *newtab
	      = (struct entrystringsbasilys_st *)
	      ggc_alloc_cleared (siz *
				 sizeof (struct entrystringsbasilys_st));
	    memcpy (newtab, src->entab,
		    siz * sizeof (struct entrystringsbasilys_st));
	    src->entab = newtab;
	  }
	for (ix = 0; ix < siz; ix++)
	  {
	    const char *at = src->entab[ix].e_at;
	    if (!at || at == (void *) 1)
	      {
		src->entab[ix].e_va = NULL;
		continue;
	      }
	    if (basilys_is_young ((const void *) at))
	      src->entab[ix].e_at = (const char *) ggc_strdup (at);
	    FORWARDED (src->entab[ix].e_va);
	  }
	break;
      }
    case OBMAG_MAPBASICBLOCKS:
      {
	struct basilysmapbasicblocks_st *src =
	  (struct basilysmapbasicblocks_st *) p;
	int ix, siz;
	if (!src->entab)
	  break;
	siz = basilys_primtab[src->lenix];
	gcc_assert (siz > 0);
	if (basilys_is_young (src->entab))
	  {
	    struct entrybasicblocksbasilys_st *newtab
	      = (struct entrybasicblocksbasilys_st *)
	      ggc_alloc_cleared (siz *
				 sizeof (struct entrybasicblocksbasilys_st));
	    memcpy (newtab, src->entab,
		    siz * sizeof (struct entrybasicblocksbasilys_st));
	    src->entab = newtab;
	  }
	for (ix = 0; ix < siz; ix++)
	  {
	    basic_block at = src->entab[ix].e_at;
	    if (!at || at == (void *) 1)
	      {
		src->entab[ix].e_va = NULL;
		continue;
	      }
	    FORWARDED (src->entab[ix].e_va);
	  }
	break;
      }
    case OBMAG_MAPEDGES:
      {
	struct basilysmapedges_st *src = (struct basilysmapedges_st *) p;
	int siz, ix;
	if (!src->entab)
	  break;
	siz = basilys_primtab[src->lenix];
	gcc_assert (siz > 0);
	if (basilys_is_young (src->entab))
	  {
	    struct entryedgesbasilys_st *newtab
	      = (struct entryedgesbasilys_st *)
	      ggc_alloc_cleared (siz * sizeof (struct entryedgesbasilys_st));
	    memcpy (newtab, src->entab,
		    siz * sizeof (struct entryedgesbasilys_st));
	    src->entab = newtab;
	  }
	for (ix = 0; ix < siz; ix++)
	  {
	    edge at = src->entab[ix].e_at;
	    if (!at || at == (void *) 1)
	      {
		src->entab[ix].e_va = NULL;
		continue;
	      }
	    FORWARDED (src->entab[ix].e_va);
	  }
	break;
      }
    case OBMAG_MIXINT:
      {
	struct basilysmixint_st *src = (struct basilysmixint_st *) p;
	FORWARDED (src->ptrval);
	break;
      }
    case OBMAG_MIXLOC:
      {
	struct basilysmixloc_st *src = (struct basilysmixloc_st *) p;
	FORWARDED (src->ptrval);
	break;
      }
    case OBMAG_MIXBIGINT:
      {
	struct basilysmixbigint_st *src = (struct basilysmixbigint_st *) p;
	FORWARDED (src->ptrval);
	break;
      }
    case OBMAG_STRBUF:
      {
	struct basilysstrbuf_st *src = (struct basilysstrbuf_st *) p;
	char *oldbufzn = src->bufzn;
	if (basilys_is_young (oldbufzn))
	  {
	    int bsiz = basilys_primtab[src->buflenix];
	    if (bsiz > 0)
	      {
		char *newbufzn = (char *) ggc_alloc_cleared (bsiz + 1);
		memcpy (newbufzn, oldbufzn, bsiz);
		src->bufzn = newbufzn;
		memset (oldbufzn, 0, bsiz);
	      }
	    else
	      src->bufzn = NULL;
	  }
	break;
      }
    case OBMAG_INT:
    case OBMAG_REAL:
    case OBMAG_STRING:
    case OBMAG_TREE:
    case OBMAG_GIMPLE:
    case OBMAG_GIMPLESEQ:
    case OBMAG_BASICBLOCK:
    case OBMAG_EDGE:
      break;
    default:
      /* gcc_unreachable (); */
      fatal_error ("basilys scanning GC: corrupted heap, p=%p omagic=%d\n",
		   (void *) p, (int) omagic);
    }
}






/** array of about 190 primes gotten by shell command 
    primes 3 2000000000 | awk '($1>p+p/8){print $1, ","; p=$1}'  **/
const long basilys_primtab[256] = {
  0,				/* the first entry indexed #0 is 0 to never be used */
  3, 5, 7, 11, 13, 17, 23, 29, 37, 43, 53, 61, 71, 83, 97, 113,
  131, 149, 173, 197, 223, 251, 283, 331, 373, 421, 479, 541,
  613, 691, 787, 887, 1009, 1151, 1297, 1471, 1657, 1867, 2111,
  2377, 2677, 3019, 3407, 3833, 4327, 4871, 5483, 6173, 6947,
  7817, 8803, 9907, 11149, 12547, 14143, 15913, 17903, 20143,
  22669, 25523, 28723, 32321, 36373, 40927, 46049, 51817,
  58309, 65599, 73819, 83047, 93463, 105167, 118343, 133153,
  149803, 168533, 189613, 213319, 239999, 270001, 303767,
  341743, 384469, 432539, 486617, 547453, 615887, 692893,
  779507, 876947, 986567, 1109891, 1248631, 1404721, 1580339,
  1777891, 2000143, 2250163, 2531443, 2847893, 3203909,
  3604417, 4054987, 4561877, 5132117, 5773679, 6495389,
  7307323, 8220743, 9248339, 10404403, 11704963, 13168091,
  14814103, 16665881, 18749123, 21092779, 23729411, 26695609,
  30032573, 33786659, 38010019, 42761287, 48106453, 54119761,
  60884741, 68495347, 77057297, 86689469, 97525661, 109716379,
  123430961, 138859837, 156217333, 175744531, 197712607,
  222426683, 250230023, 281508827, 316697431, 356284619,
  400820209, 450922753, 507288107, 570699121, 642036517,
  722291083, 812577517, 914149741,
#if HOST_BITS_PER_LONG >= 64
  1028418463, 1156970821, 1301592203,
  1464291239, 1647327679, 1853243677, 2084899139, 2345511541,
  2638700497, 2968538081, 3339605383, 3757056091, 4226688133,
  4755024167, 5349402193, 6018077509, 6770337239, 7616629399,
  8568708139, 9639796667, 10844771263, 12200367671,
  13725413633, 15441090347, 17371226651, 19542629983,
  21985458749, 24733641113, 27825346259, 31303514549,
  35216453869, 39618510629, 44570824481, 50142177559,
#endif
  0, 0
};

/* index of entry to get or add an attribute in an mapobject (or -1 on error) */
static inline int
unsafe_index_mapobject (struct entryobjectsbasilys_st *tab,
			basilysobject_ptr_t attr, int siz)
{
  int da = 0, ix = 0, frix = -1;
  unsigned h = 0;
#if 0 && ENABLE_CHECKING
  static long samehashcnt;
#endif
  if (!tab)
    return -1;
  da = attr->obj_class->object_magic;
  if (da == OBMAG_OBJECT)
    h = ((struct basilysobject_st *) attr)->obj_hash;
  else
    return -1;
  h = h % siz;
  for (ix = h; ix < siz; ix++)
    {
      basilysobject_ptr_t curat = tab[ix].e_at;
      if (curat == attr)
	return ix;
      else if (curat == (void *) HTAB_DELETED_ENTRY)
	{
	  if (frix < 0)
	    frix = ix;
	}
      else if (!curat)
	{
	  if (frix < 0)
	    frix = ix;
	  return frix;
	}
#if 0 && ENABLE_CHECKING
      /* this to help hunt bugs on wrongly homonymous objects of same hash & class */
      else if (curat->obj_hash == attr->obj_hash
	       && curat->obj_class == attr->obj_class)
	{
	  samehashcnt++;
	  if (flag_basilys_debug
	      || (samehashcnt < 16 || samehashcnt % 16 == 0))
	    {
	      basilys_checkmsg ("similar gotten & found attr", curat == attr);
	      dbgprintf
		("unprobable #%ld gotten %p ##%ld & found attr %p ##%ld of same hash %#x & class %p [%s]",
		 samehashcnt, (void *) attr, attr->obj_serial, (void *) curat,
		 curat->obj_serial, curat->obj_hash,
		 (void *) curat->obj_class,
		 basilys_string_str (curat->
				     obj_class->obj_vartab[FNAMED_NAME]));
	      if (basilys_is_instance_of
		  ((basilys_ptr_t) attr,
		   (basilys_ptr_t) BASILYSGOB (CLASS_NAMED)))
		dbgprintf ("gotten attr named %s found attr named %s",
			   basilys_string_str (attr->obj_vartab[FNAMED_NAME]),
			   basilys_string_str (curat->obj_vartab
					       [FNAMED_NAME]));
	      basilys_dbgshortbacktrace
		("gotten & found attr of same hash & class", 15);
	    }
	}
#endif
    }
  for (ix = 0; ix < (int) h; ix++)
    {
      basilysobject_ptr_t curat = tab[ix].e_at;
      if (curat == attr)
	return ix;
      else if (curat == (void *) HTAB_DELETED_ENTRY)
	{
	  if (frix < 0)
	    frix = ix;
	}
      else if (!curat)
	{
	  if (frix < 0)
	    frix = ix;
	  return frix;
	}
#if 0 && ENABLE_CHECKING
      else if (curat->obj_hash == attr->obj_hash
	       && curat->obj_class == attr->obj_class)
	{
	  samehashcnt++;
	  if (flag_basilys_debug
	      || (samehashcnt < 16 || samehashcnt % 16 == 0))
	    {
	      basilys_checkmsg ("similar gotten & found attr", curat == attr);
	      dbgprintf
		("unprobable #%ld gotten %p ##%ld & found attr %p ##%ld of same hash %#x & class %p [%s]",
		 samehashcnt, (void *) attr, attr->obj_serial, (void *) curat,
		 curat->obj_serial, curat->obj_hash,
		 (void *) curat->obj_class,
		 basilys_string_str (curat->
				     obj_class->obj_vartab[FNAMED_NAME]));
	      if (basilys_is_instance_of
		  ((basilys_ptr_t) attr,
		   (basilys_ptr_t) BASILYSGOB (CLASS_NAMED)))
		dbgprintf ("gotten attr named %s found attr named %s",
			   basilys_string_str (attr->obj_vartab[FNAMED_NAME]),
			   basilys_string_str (curat->obj_vartab
					       [FNAMED_NAME]));
	      basilys_dbgshortbacktrace
		("gotten & found attr of same hash & class", 15);
	    }
	}
#endif
    }
  if (frix >= 0)
    return frix;		/* found some place in a table with
				   deleted entries but no empty
				   entries */
  return -1;			/* entirely full, should not happen */
}


basilys_ptr_t
basilysgc_new_int (basilysobject_ptr_t discr_p, long num)
{
  BASILYS_ENTERFRAME (2, NULL);
#define newintv curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define int_newintv ((struct basilysint_st*)(newintv))
  discrv = (void *) discr_p;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_INT)
    goto end;
  newintv = basilysgc_allocate (sizeof (struct basilysint_st), 0);
  int_newintv->discr = object_discrv;
  int_newintv->val = num;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newintv;
#undef newintv
#undef discrv
#undef int_newintv
#undef object_discrv
}


basilys_ptr_t
basilysgc_new_mixint (basilysobject_ptr_t discr_p,
		      basilys_ptr_t val_p, long num)
{
  BASILYS_ENTERFRAME (3, NULL);
#define newmix  curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define valv    curfram__.varptr[2]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mix_newmix ((struct basilysmixint_st*)(newmix))
  newmix = NULL;
  discrv = (void *) discr_p;
  valv = val_p;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MIXINT)
    goto end;
  newmix = basilysgc_allocate (sizeof (struct basilysmixint_st), 0);
  mix_newmix->discr = object_discrv;
  mix_newmix->intval = num;
  mix_newmix->ptrval = (basilys_ptr_t) valv;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmix;
#undef newmix
#undef valv
#undef discrv
#undef mix_newmix
#undef object_discrv
}


basilys_ptr_t
basilysgc_new_mixloc (basilysobject_ptr_t discr_p,
		      basilys_ptr_t val_p, long num, location_t loc)
{
  BASILYS_ENTERFRAME (3, NULL);
#define newmix  curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define valv    curfram__.varptr[2]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mix_newmix ((struct basilysmixloc_st*)(newmix))
  newmix = NULL;
  discrv = (void *) discr_p;
  valv = val_p;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MIXLOC)
    goto end;
  newmix = basilysgc_allocate (sizeof (struct basilysmixloc_st), 0);
  mix_newmix->discr = object_discrv;
  mix_newmix->intval = num;
  mix_newmix->ptrval = (basilys_ptr_t) valv;
  mix_newmix->locval = loc;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmix;
#undef newmix
#undef valv
#undef discrv
#undef mix_newmix
#undef object_discrv
}


basilys_ptr_t
basilysgc_new_mixbigint_mpz (basilysobject_ptr_t discr_p,
			     basilys_ptr_t val_p, mpz_t mp)
{
  unsigned numb = 0, blen = 0;
  size_t wcnt = 0;
  BASILYS_ENTERFRAME (3, NULL);
#define newbig  curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define valv    curfram__.varptr[2]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mix_newbig ((struct basilysmixbigint_st*)(newbig))
  newbig = NULL;
  discrv = (void *) discr_p;
  if (!discrv)
    discrv = BASILYSGOB (DISCR_MIXBIGINT);
  valv = val_p;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MIXBIGINT)
    goto end;
  if (!mp) 
    goto end; 
  numb = 8*sizeof(mix_newbig->tabig[0]);
  blen = (mpz_sizeinbase (mp, 2) + numb-1) / numb;
  newbig = basilysgc_allocate (sizeof (struct basilysmixbigint_st),
			       blen*sizeof(mix_newbig->tabig[0]));
  mix_newbig->discr = object_discrv;
  mix_newbig->ptrval = (basilys_ptr_t) valv;
  mix_newbig->negative = (mpz_sgn (mp)<0);
  mix_newbig->biglen = blen;
  mpz_export (mix_newbig->tabig, &wcnt, 
	      /*most significant word first */ 1, 
	      sizeof(mix_newbig->tabig[0]), 
	      /*native endian*/ 0, 
	      /* no nails bits */ 0, 
	      mp);
  gcc_assert(wcnt <= blen);
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newbig;
#undef newbig
#undef valv
#undef discrv
#undef mix_newbig
#undef object_discrv
}


/* allocate a new routine object of given DISCR and of length LEN,
   with a DESCR-iptive string a a PROC-edure */
basilysroutine_ptr_t
basilysgc_new_routine (basilysobject_ptr_t discr_p,
		       unsigned len, const char *descr,
		       basilysroutfun_t * proc)
{
  BASILYS_ENTERFRAME (2, NULL);
#define newroutv curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define obj_discrv ((basilysobject_ptr_t)(discrv))
#define rou_newroutv ((basilysroutine_ptr_t)(newroutv))
  newroutv = NULL;
  discrv = discr_p;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT
      || obj_discrv->object_magic != OBMAG_ROUTINE || !descr || !descr[0]
      || !proc || len > BASILYS_MAXLEN)
    goto end;
  newroutv =
    basilysgc_allocate (sizeof (struct basilysroutine_st),
			len * sizeof (void *));
  rou_newroutv->discr = (basilysobject_ptr_t) discrv;
  rou_newroutv->nbval = len;
  rou_newroutv->routfunad = proc;
  strncpy (rou_newroutv->routdescr, descr, BASILYS_ROUTDESCR_LEN - 1);
  rou_newroutv->routdescr[BASILYS_ROUTDESCR_LEN - 1] = (char) 0;
end:
  BASILYS_EXITFRAME ();
  return (basilysroutine_ptr_t) newroutv;
#undef newroutv
#undef discrv
#undef obj_discrv
#undef rou_newroutv
}

void
basilysgc_set_routine_data (basilys_ptr_t rout_p, basilys_ptr_t data_p)
{
  BASILYS_ENTERFRAME (2, NULL);
#define routv curfram__.varptr[0]
#define datav  curfram__.varptr[1]
  routv = rout_p;
  datav = data_p;
  if (basilys_magic_discr ((basilys_ptr_t) routv) == OBMAG_ROUTINE)
    {
      ((basilysroutine_ptr_t) routv)->routdata = (basilys_ptr_t) datav;
      basilysgc_touch_dest (routv, datav);
    }
  BASILYS_EXITFRAME ();
#undef routv
#undef datav
}

basilysclosure_ptr_t
basilysgc_new_closure (basilysobject_ptr_t discr_p,
		       basilysroutine_ptr_t rout_p, unsigned len)
{
  BASILYS_ENTERFRAME (3, NULL);
#define newclosv  curfram__.varptr[0]
#define discrv    curfram__.varptr[1]
#define routv     curfram__.varptr[2]
#define clo_newclosv ((basilysclosure_ptr_t)(newclosv))
#define obj_discrv   ((basilysobject_ptr_t)(discrv))
#define rou_routv    ((basilysroutine_ptr_t)(routv))
  discrv = discr_p;
  routv = rout_p;
  newclosv = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT
      || obj_discrv->object_magic != OBMAG_CLOSURE
      || basilys_magic_discr ((basilys_ptr_t) (routv)) != OBMAG_ROUTINE
      || len > BASILYS_MAXLEN)
    goto end;
  newclosv =
    basilysgc_allocate (sizeof (struct basilysclosure_st),
			sizeof (void *) * len);
  clo_newclosv->discr = (basilysobject_ptr_t) discrv;
  clo_newclosv->rout = (basilysroutine_ptr_t) routv;
  clo_newclosv->nbval = len;
end:
  BASILYS_EXITFRAME ();
  return (basilysclosure_ptr_t) newclosv;
#undef newclosv
#undef discrv
#undef routv
#undef clo_newclosv
#undef obj_discrv
#undef rou_routv
}



struct basilysstrbuf_st *
basilysgc_new_strbuf (basilysobject_ptr_t discr_p, const char *str)
{
  int slen = 0, blen = 0, ix = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define newbufv  curfram__.varptr[0]
#define discrv   curfram__.varptr[1]
#define buf_newbufv ((struct basilysstrbuf_st*)(newbufv))
  discrv = discr_p;
  newbufv = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT)
    goto end;
  if (((basilysobject_ptr_t) (discrv))->object_magic != OBMAG_STRBUF)
    goto end;
  if (str)
    slen = strlen (str);
  gcc_assert (slen < BASILYS_MAXLEN);
  slen += slen / 5 + 40;
  for (ix = 2; (blen = basilys_primtab[ix]) != 0 && blen < slen; ix++);
  gcc_assert (blen != 0);
  newbufv =
    basilysgc_allocate (offsetof
			(struct basilysstrbuf_st, buf_space), blen + 1);
  buf_newbufv->discr = (basilysobject_ptr_t) discrv;
  buf_newbufv->bufzn = buf_newbufv->buf_space;
  buf_newbufv->buflenix = ix;
  buf_newbufv->bufstart = 0;
  if (str)
    {
      strcpy (buf_newbufv->bufzn, str);
      buf_newbufv->bufend = strlen (str);
    }
  else
    buf_newbufv->bufend = 0;
end:
  BASILYS_EXITFRAME ();
  return (struct basilysstrbuf_st *) newbufv;
#undef newbufv
#undef discrv
#undef buf_newbufv
}


void
basilysgc_add_strbuf_raw_len (basilys_ptr_t strbuf_p, const char *str, int slen)
{
#ifdef ENABLE_CHECKING
  static long addcount;
#endif
  int blen = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define strbufv  curfram__.varptr[0]
#define buf_strbufv  ((struct basilysstrbuf_st*)(strbufv))
  strbufv = strbuf_p;
  if (!str)
    goto end;
  if (basilys_magic_discr ((basilys_ptr_t) (strbufv)) != OBMAG_STRBUF)
    goto end;
  gcc_assert (!basilys_is_young (str));
  if (slen<0)
    slen = strlen (str);
  blen = basilys_primtab[buf_strbufv->buflenix];
  gcc_assert (blen > 0);
#ifdef ENABLE_CHECKING
  addcount++;
#endif
  gcc_assert (buf_strbufv->bufstart <= buf_strbufv->bufend
	      && buf_strbufv->bufend < (unsigned) blen);
  if ((int) buf_strbufv->bufend + slen + 2 < blen)
    {				/* simple case, just copy at end */
      strcpy (buf_strbufv->bufzn + buf_strbufv->bufend, str);
      buf_strbufv->bufend += slen;
      buf_strbufv->bufzn[buf_strbufv->bufend] = 0;
    }
  else
    if ((int) buf_strbufv->bufstart > (int) 0
	&& (int) buf_strbufv->bufend -
	(int) buf_strbufv->bufstart + (int) slen + 2 < (int) blen)
    {				/* should move the buffer to fit */
      int siz = buf_strbufv->bufend - buf_strbufv->bufstart;
      gcc_assert (siz > 0);
      memmove (buf_strbufv->bufzn,
	       buf_strbufv->bufzn + buf_strbufv->bufstart, siz);
      buf_strbufv->bufstart = 0;
      strcpy (buf_strbufv->bufzn + siz, str);
      buf_strbufv->bufend = siz + slen;
      buf_strbufv->bufzn[buf_strbufv->bufend] = 0;
    }
  else
    {				/* should grow the buffer to fit */
      int siz = buf_strbufv->bufend - buf_strbufv->bufstart;
      int newsiz = (siz + slen + 50 + siz / 8) | 0x1f;
      int newix = 0, newblen = 0;
      char *newb = NULL;
      int oldblen = basilys_primtab[buf_strbufv->buflenix];
      for (newix = buf_strbufv->buflenix + 1;
	   (newblen = basilys_primtab[newix]) != 0
	   && newblen < newsiz; newix++);
      gcc_assert (newblen >= newsiz);
      gcc_assert (siz >= 0);
      if (newblen > BASILYS_MAXLEN)
	fatal_error ("strbuf overflow to %d bytes", newblen);
      /* the newly grown buffer is allocated in young memory if the
         previous was young, or in old memory if it was already old;
         but we have to deal with the rare case when the allocation
         triggers a GC which migrate the strbuf from young to old */
      if (basilys_is_young (buf_strbufv->bufzn))
	{
	  /* bug to avoid: the strbuf was young, the allocation of
	     newb triggers a GC, and then the strbuf becomes old. we
	     cannot put newb inside it (this violate the GC invariant
	     of no unfollowed -on store list- old to young
	     pointers). So we reserve the required length to make sure
	     that the following newb allocation does not trigger a
	     GC */
	  basilysgc_reserve (newblen + 10 * sizeof (void *));
	  /* does the above reservation triggered a GC which moved buf_strbufv to old? */
	  if (!basilys_is_young (buf_strbufv->bufzn))
	    goto strbuf_in_old_memory;
	  gcc_assert (basilys_is_young (buf_strbufv));
	  newb = (char *) basilys_allocatereserved (newblen + 1, 0);
	  gcc_assert (basilys_is_young (buf_strbufv));
	  memcpy (newb, buf_strbufv->bufzn + buf_strbufv->bufstart, siz);
	  strcpy (newb + siz, str);
	  memset (buf_strbufv->bufzn, 0, oldblen);
	  buf_strbufv->bufzn = newb;
	}
      else
	{
	  /* we may come here if the strbuf was young but became old
	     by the basilysgc_reserve call above */
	strbuf_in_old_memory:
	  gcc_assert (!basilys_is_young (buf_strbufv));
	  newb = (char *) ggc_alloc_cleared (newblen + 1);
	  memcpy (newb, buf_strbufv->bufzn + buf_strbufv->bufstart, siz);
	  strcpy (newb + siz, str);
	  memset (buf_strbufv->bufzn, 0, oldblen);
	  ggc_free (buf_strbufv->bufzn);
	  buf_strbufv->bufzn = newb;
	}
      buf_strbufv->buflenix = newix;
      buf_strbufv->bufstart = 0;
      buf_strbufv->bufend = siz + slen;
      buf_strbufv->bufzn[buf_strbufv->bufend] = 0;
      /* touch the buffer so that it will be scanned if not young */
      basilysgc_touch (strbufv);
    }
end:
  BASILYS_EXITFRAME ();
#undef strbufv
#undef buf_strbufv
}

void
basilysgc_add_strbuf_raw (basilys_ptr_t strbuf_p, const char *str)
{
  basilysgc_add_strbuf_raw_len(strbuf_p, str, -1);
}

void
basilysgc_add_strbuf (basilys_ptr_t strbuf_p, const char *str)
{
  char sbuf[80];
  char *cstr = NULL;
  int slen = 0;
  if (str)
    slen = strlen (str);
  if (slen <= 0)
    return;
  if (slen < (int) sizeof (sbuf) - 1)
    {
      memset (sbuf, 0, sizeof (sbuf));
      strcpy (sbuf, str);
      basilysgc_add_strbuf_raw (strbuf_p, sbuf);
    }
  else
    {
      cstr = xstrdup (str);
      basilysgc_add_strbuf_raw (strbuf_p, cstr);
      free (cstr);
    }
}

void
basilysgc_add_strbuf_cstr (basilys_ptr_t strbuf_p, const char *str)
{
  int slen = str ? strlen (str) : 0;
  const char *ps = NULL;
  char *pd = NULL;
  char *cstr = NULL;
  if (!str || !str[0])
    return;
  cstr = (char *) xcalloc (slen + 5, 4);
  pd = cstr;
  for (ps = str; *ps; ps++)
    {
      switch (*ps)
	{
#define ADDS(S) strcpy(pd, S); pd+=sizeof(S)-1; break
	case '\n':
	  ADDS ("\\n");
	case '\r':
	  ADDS ("\\r");
	case '\t':
	  ADDS ("\\t");
	case '\v':
	  ADDS ("\\v");
	case '\f':
	  ADDS ("\\f");
	case '\'':
	  ADDS ("\\\'");
	case '\"':
	  ADDS ("\\\"");
	case '\\':
	  ADDS ("\\\\");
#undef ADDS
	default:
	  if (ISPRINT (*ps))
	    *(pd++) = *ps;
	  else
	    {
	      sprintf (pd, "\\%03o", (*ps) & 0xff);
	      pd += 4;
	    }
	}
    };
  basilysgc_add_strbuf_raw (strbuf_p, cstr);
  free (cstr);
}


void
basilysgc_add_strbuf_ccomment (basilys_ptr_t strbuf_p, const char *str)
{
  int slen = str ? strlen (str) : 0;
  const char *ps = NULL;
  char *pd = NULL;
  char *cstr = NULL;
  if (!str || !str[0])
    return;
  cstr = (char *) xcalloc (slen + 4, 4);
  pd = cstr;
  for (ps = str; *ps; ps++)
    {
      if (ps[0] == '/' && ps[1] == '*')
	{
	  pd[0] = '/';
	  pd[1] = '+';
	  pd += 2;
	  ps++;
	}
      else if (ps[0] == '*' && ps[1] == '/')
	{
	  pd[0] = '+';
	  pd[1] = '/';
	  pd += 2;
	  ps++;
	}
      else
	*(pd++) = *ps;
    };
  basilysgc_add_strbuf_raw (strbuf_p, cstr);
  free (cstr);
}

void
basilysgc_add_strbuf_cident (basilys_ptr_t strbuf_p, const char *str)
{
  int slen = str ? strlen (str) : 0;
  char *dupstr = 0;
  const char *ps = 0;
  char *pd = 0;
  char tinybuf[80];
  if (!str || !str[0])
    return;
  if (slen < (int) sizeof (tinybuf) - 2)
    {
      memset (tinybuf, 0, sizeof (tinybuf));
      dupstr = tinybuf;
    }
  else
    dupstr = (char *) xcalloc (slen + 2, 1);
  if (str)
    for (ps = (const char *) str, pd = dupstr; *ps; ps++)
      {
	if (ISALNUM (*ps))
	  *(pd++) = *ps;
	else if (pd > dupstr && pd[-1] != '_')
	  *(pd++) = '_';
	else
	  *pd = (char) 0;
	pd[1] = (char) 0;
      }
  basilysgc_add_strbuf_raw (strbuf_p, dupstr);
  if (dupstr && dupstr != tinybuf)
    free (dupstr);
}

void
basilysgc_add_strbuf_cidentprefix (basilys_ptr_t strbuf_p, const char *str, int preflen)
{
  const char *ps = 0;
  char *pd = 0;
  char tinybuf[80];
  if (str)
    {
      int lenst = strlen (str);
      if (lenst < preflen)
	preflen = lenst;
    }
  else
    return;
  if (preflen >= (int) sizeof (tinybuf) - 1)
    preflen = sizeof (tinybuf) - 2;
  if (preflen <= 0)
    return;
  memset (tinybuf, 0, sizeof (tinybuf));
  for (pd = tinybuf, ps = str; ps < str + preflen && *ps; ps++)
    {
      if (ISALNUM (*ps))
	*(pd++) = *ps;
      else if (pd > tinybuf && pd[-1] != '_')
	*(pd++) = '_';
    }
  basilysgc_add_strbuf_raw (strbuf_p, tinybuf);
}


void
basilysgc_add_strbuf_hex (basilys_ptr_t strbuf_p, unsigned long l)
{
  if (l == 0UL)
    basilysgc_add_strbuf_raw (strbuf_p, "0");
  else
    {
      int ix = 0, j = 0;
      char revbuf[80], thebuf[80];
      memset (revbuf, 0, sizeof (revbuf));
      memset (thebuf, 0, sizeof (thebuf));
      while (ix < (int) sizeof (revbuf) - 1 && l != 0UL)
	{
	  unsigned h = l & 15;
	  l >>= 4;
	  revbuf[ix++] = "0123456789abcdef"[h];
	}
      ix--;
      for (j = 0; j < (int) sizeof (thebuf) - 1 && ix >= 0; j++, ix--)
	thebuf[j] = revbuf[ix];
      basilysgc_add_strbuf_raw (strbuf_p, thebuf);
    }
}


void
basilysgc_add_strbuf_dec (basilys_ptr_t strbuf_p, long l)
{
  if (l == 0L)
    basilysgc_add_strbuf_raw (strbuf_p, "0");
  else
    {
      int ix = 0, j = 0, neg = 0;
      char revbuf[96], thebuf[96];
      memset (revbuf, 0, sizeof (revbuf));
      memset (thebuf, 0, sizeof (thebuf));
      if (l < 0)
	{
	  l = -l;
	  neg = 1;
	};
      while (ix < (int) sizeof (revbuf) - 1 && l != 0UL)
	{
	  unsigned h = l % 10;
	  l = l / 10;
	  revbuf[ix++] = "0123456789"[h];
	}
      ix--;
      if (neg)
	{
	  thebuf[0] = '-';
	  j = 1;
	};
      for (; j < (int) sizeof (thebuf) - 1 && ix >= 0; j++, ix--)
	thebuf[j] = revbuf[ix];
      basilysgc_add_strbuf_raw (strbuf_p, thebuf);
    }
}


void
basilysgc_strbuf_printf (basilys_ptr_t strbuf_p,
			 const char *fmt, ...)
{
  char *cstr = NULL;
  va_list ap;
  int l = 0;
  static char tinybuf[80];
  memset (tinybuf, 0, sizeof (tinybuf));
  va_start (ap, fmt);
  l = vsnprintf (tinybuf, sizeof (tinybuf) - 1, fmt, ap);
  va_end (ap);
  if (l < (int) sizeof (tinybuf) - 3)
    {
      basilysgc_add_strbuf_raw (strbuf_p, tinybuf);
      return;
    }
  va_start (ap, fmt);
  vasprintf (&cstr, fmt, ap);
  va_end (ap);
  basilysgc_add_strbuf_raw (strbuf_p, cstr);
  free (cstr);
}


/* add safely into STRBUF either a space or an indented newline if the current line is bigger than the threshold */
void
basilysgc_strbuf_add_indent (basilys_ptr_t strbuf_p, int depth, int linethresh)
{
  int llln = 0;			/* last line length */
  BASILYS_ENTERFRAME (2, NULL);
  /* we need a frame, because we have more than one call to
     basilysgc_add_strbuf_raw */
#define strbv   curfram__.varptr[0]
#define strbufv ((struct basilysstrbuf_st*)(strbv))
  strbv = strbuf_p;
  if (!strbufv
      || basilys_magic_discr ((basilys_ptr_t) (strbufv)) != OBMAG_STRBUF)
    goto end;
  if (linethresh > 0 && linethresh < 40)
    linethresh = 40;
  /* compute the last line length llln */
  {
    char *bs = 0, *be = 0, *nl = 0;
    bs = strbufv->bufzn + strbufv->bufstart;
    be = strbufv->bufzn + strbufv->bufend;
    for (nl = be - 1; nl > bs && *nl && *nl != '\n'; nl--);
    llln = be - nl;
    gcc_assert (llln >= 0);
  }
  if (linethresh > 0 && llln < linethresh)
    basilysgc_add_strbuf_raw ((basilys_ptr_t) strbv, " ");
  else
    {
      int nbsp = depth;
      static const char spaces32[] = "                                ";
      basilysgc_add_strbuf_raw ((basilys_ptr_t) strbv, "\n");
      if (nbsp < 0)
	nbsp = 0;
      if (nbsp > 0 && nbsp % 32 != 0)
	basilysgc_add_strbuf_raw ((basilys_ptr_t) strbv, spaces32 + (32 - nbsp % 32));
    }
end:
  BASILYS_EXITFRAME ();
#undef strbufv
#undef strbv
}




/***************/

#if ENABLE_CHECKING

/* just for debugging */
unsigned long basilys_serial_1, basilys_serial_2, basilys_serial_3;
unsigned long basilys_countserial;

void
basilys_object_set_serial (basilysobject_ptr_t ob)
{
  if (ob && ob->obj_serial == 0)
    ob->obj_serial = ++basilys_countserial;
  else
    return;
  if (basilys_serial_1 > 0 && basilys_countserial == basilys_serial_1)
    {
      debugeprintf ("set serial_1 ob=%p ##%ld", (void *) ob,
		    basilys_countserial);
      gcc_assert (ob);
    }
  else if (basilys_serial_2 > 0 && basilys_countserial == basilys_serial_1)
    {
      debugeprintf ("set serial_2 ob=%p ##%ld", (void *) ob,
		    basilys_countserial);
      gcc_assert (ob);
    }
  else if (basilys_serial_3 > 0 && basilys_countserial == basilys_serial_3)
    {
      debugeprintf ("set serial_3 ob=%p ##%ld", (void *) ob,
		    basilys_countserial);
      gcc_assert (ob);
    }
}
#endif

basilysobject_ptr_t
basilysgc_new_raw_object (basilysobject_ptr_t klass_p, unsigned len)
{
  unsigned h = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define newobjv   curfram__.varptr[0]
#define klassv    curfram__.varptr[1]
#define obj_newobjv  ((basilysobject_ptr_t)(newobjv))
#define obj_klassv   ((basilysobject_ptr_t)(klassv))
  newobjv = NULL;
  klassv = klass_p;
  if (basilys_magic_discr ((basilys_ptr_t) (klassv)) != OBMAG_OBJECT
      || obj_klassv->object_magic != OBMAG_OBJECT || len >= SHRT_MAX)
    goto end;
  /* the sizeof below could be the offsetof obj__tabfields */
  newobjv =
    basilysgc_allocate (sizeof (struct basilysobject_st),
			len * sizeof (void *));
  obj_newobjv->obj_class = (basilysobject_ptr_t) klassv;
  do
    {
      h = basilys_lrand () & BASILYS_MAXHASH;
    }
  while (h == 0);
  obj_newobjv->obj_hash = h;
  obj_newobjv->obj_len = len;
  basilys_object_set_serial (obj_newobjv);
end:
  BASILYS_EXITFRAME ();
  return (basilysobject_ptr_t) newobjv;
#undef newobjv
#undef klassv
#undef obj_newobjv
#undef obj_klassv
}


/* allocate a new multiple of given DISCR & length LEN */
basilys_ptr_t
basilysgc_new_multiple (basilysobject_ptr_t discr_p, unsigned len)
{
  BASILYS_ENTERFRAME (2, NULL);
#define newmul curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mult_newmul ((struct basilysmultiple_st*)(newmul))
  discrv = (void *) discr_p;
  newmul = NULL;
  gcc_assert (len < BASILYS_MAXLEN);
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MULTIPLE)
    goto end;
  newmul =
    basilysgc_allocate (sizeof (struct basilysmultiple_st),
			sizeof (void *) * len);
  mult_newmul->discr = object_discrv;
  mult_newmul->nbval = len;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmul;
#undef newmul
#undef discr
#undef mult_newmul
#undef object_discrv
}

/* make a subsequence of a given multiple OLDMUL_P from STARTIX to
   ENDIX; if either index is negative, take it from last.  return null
   if arguments are incorrect, or a fresh subsequence of same
   discriminant as source otherwise */
basilys_ptr_t
basilysgc_new_subseq_multiple (basilys_ptr_t oldmul_p, int startix, int endix)
{
  int oldlen=0, newlen=0, i=0;
  BASILYS_ENTERFRAME(3, NULL);
#define oldmulv   curfram__.varptr[0]
#define newmulv   curfram__.varptr[1]
#define mult_oldmulv   ((struct basilysmultiple_st*)(oldmulv))
#define mult_newmulv   ((struct basilysmultiple_st*)(newmulv))
  oldmulv = oldmul_p;
  newmulv = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) (oldmulv)) != OBMAG_MULTIPLE)
    goto end;
  oldlen = mult_oldmulv->nbval;
  if (startix < 0)
    startix += oldlen;
  if (endix < 0)
    endix += oldlen;
  if (startix < 0 || startix >= oldlen)
    goto end;
  if (endix < 0 || endix >= oldlen || endix < startix)
    goto end;
  newlen = endix - startix;
  newmulv =
    basilysgc_allocate (sizeof (struct basilysmultiple_st),
			sizeof (void *) * newlen);
  mult_newmulv->discr = mult_oldmulv->discr;
  mult_newmulv->nbval = newlen;
  for (i=0; i<newlen; i++)
    mult_newmulv->tabval[i] = mult_oldmulv->tabval[startix+i];
 end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmulv;
#undef oldmulv
#undef newmulv
#undef mult_oldmulv
#undef mult_newmulv
}

void
basilysgc_multiple_put_nth (basilys_ptr_t mul_p, int n, basilys_ptr_t val_p)
{
  int ln = 0;
  BASILYS_ENTERFRAME (3, NULL);
#define mulv    curfram__.varptr[0]
#define mult_mulv ((struct basilysmultiple_st*)(mulv))
#define discrv  curfram__.varptr[1]
#define valv    curfram__.varptr[2]
  mulv = mul_p;
  valv = val_p;
  if (basilys_magic_discr ((basilys_ptr_t) (mulv)) != OBMAG_MULTIPLE)
    goto end;
  ln = mult_mulv->nbval;
  if (n < 0)
    n += ln;
  if (n >= 0 && n < ln)
    {
      mult_mulv->tabval[n] = (basilys_ptr_t) valv;
      basilysgc_touch_dest (mulv, valv);
    }
end:
  BASILYS_EXITFRAME ();
#undef mulv
#undef mult_mulv
#undef discrv
#undef valv
}



/*** sort a multiple with a compare closure which should return a
     number; if it does not, the sort return nil, by longjmp-ing out
     of qsort
 ***/
static jmp_buf mulsort_escapjmp;
static basilys_ptr_t *mulsort_mult_ad;
static basilys_ptr_t *mulsort_clos_ad;
static int
mulsort_cmp (const void *p1, const void *p2)
{
  int ok = 0;
  int cmp = 0;
  int ix1 = -1, ix2 = -1;
  union basilysparam_un argtab[2];
  BASILYS_ENTERFRAME (5, NULL);
#define rescmpv curfram__.varptr[0]
#define val1v curfram__.varptr[1]
#define val2v curfram__.varptr[2]
#define clov  curfram__.varptr[3]
#define mulv  curfram__.varptr[4]
  mulv = *mulsort_mult_ad;
  clov = *mulsort_clos_ad;
  ix1 = *(const int *) p1;
  ix2 = *(const int *) p2;
  val1v = basilys_multiple_nth ((basilys_ptr_t) mulv, ix1);
  val2v = basilys_multiple_nth ((basilys_ptr_t) mulv, ix2);
  if (val1v == val2v)
    {
      ok = 1;
      cmp = 0;
      goto end;
    }
  memset (argtab, 0, sizeof (argtab));
  argtab[0].bp_aptr = (basilys_ptr_t *) & val2v;
  rescmpv =
    basilys_apply ((basilysclosure_ptr_t) clov, (basilys_ptr_t) val1v,
		   BPARSTR_PTR, argtab, "", NULL);
  if (basilys_magic_discr ((basilys_ptr_t) rescmpv) == OBMAG_INT)
    {
      ok = 1;
      cmp = basilys_get_int ((basilys_ptr_t) rescmpv);
    }
end:
  BASILYS_EXITFRAME ();
#undef rescmpv
#undef val1v
#undef val2v
#undef clov
  if (!ok)
    {
      longjmp (mulsort_escapjmp, 1);
    }
  return cmp;
}

basilys_ptr_t
basilysgc_sort_multiple (basilys_ptr_t mult_p, basilys_ptr_t clo_p,
			 basilys_ptr_t discrm_p)
{
  int *ixtab = 0;
  int i = 0;
  unsigned mulen = 0;
  BASILYS_ENTERFRAME (5, NULL);
#define multv      curfram__.varptr[0]
#define clov       curfram__.varptr[1]
#define discrmv    curfram__.varptr[2]
#define resv       curfram__.varptr[3]
  multv = mult_p;
  clov = clo_p;
  discrmv = discrm_p;
  resv = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) multv) != OBMAG_MULTIPLE)
    goto end;
  if (basilys_magic_discr ((basilys_ptr_t) clov) != OBMAG_CLOSURE)
    goto end;
  if (!discrmv)
    discrmv = BASILYSGOB (DISCR_MULTIPLE);
  if (basilys_magic_discr ((basilys_ptr_t) discrmv) != OBMAG_OBJECT)
    goto end;
  if (((basilysobject_ptr_t) discrmv)->obj_num != OBMAG_MULTIPLE)
    goto end;
  mulen = (int) (((basilysmultiple_ptr_t) multv)->nbval);
  /* allocate and fill ixtab with indexes */
  ixtab = (int *) xcalloc (mulen + 1, sizeof (ixtab[0]));
  for (i = 0; i < (int) mulen; i++)
    ixtab[i] = i;
  mulsort_mult_ad = (basilys_ptr_t *) & multv;
  mulsort_clos_ad = (basilys_ptr_t *) & clov;
  if (!setjmp (mulsort_escapjmp))
    {
      qsort (ixtab, (size_t) mulen, sizeof (ixtab[0]), mulsort_cmp);
      resv =
	basilysgc_new_multiple ((basilysobject_ptr_t) discrmv,
				(unsigned) mulen);
      for (i = 0; i < (int) mulen; i++)
	basilysgc_multiple_put_nth ((basilys_ptr_t) resv, i,
				    basilys_multiple_nth ((basilys_ptr_t)
							  multv, ixtab[i]));
    }
  else
    {
      resv = NULL;
    }
end:
  if (ixtab)
    free (ixtab);
  memset (&mulsort_escapjmp, 0, sizeof (mulsort_escapjmp));
  mulsort_mult_ad = 0;
  mulsort_clos_ad = 0;
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) resv;
#undef multv
#undef clov
#undef discrmv
#undef resv
}



/* allocate a new box of given DISCR & content VAL */
basilys_ptr_t
basilysgc_new_box (basilysobject_ptr_t discr_p, basilys_ptr_t val_p)
{
  BASILYS_ENTERFRAME (3, NULL);
#define boxv curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define valv   curfram__.varptr[2]
#define object_discrv ((basilysobject_ptr_t)(discrv))
  discrv = (void *) discr_p;
  valv = (void *) val_p;
  boxv = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_BOX)
    goto end;
  boxv = basilysgc_allocate (sizeof (struct basilysbox_st), 0);
  ((struct basilysbox_st *) (boxv))->discr = (basilysobject_ptr_t) discrv;
  ((struct basilysbox_st *) (boxv))->val = (basilys_ptr_t) valv;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) boxv;
#undef boxv
#undef discrv
#undef valv
#undef object_discrv
}

/* put inside a box */
void
basilysgc_box_put (basilys_ptr_t box_p, basilys_ptr_t val_p)
{
  BASILYS_ENTERFRAME (2, NULL);
#define boxv curfram__.varptr[0]
#define valv   curfram__.varptr[1]
  boxv = box_p;
  valv = val_p;
  if (basilys_magic_discr ((basilys_ptr_t) boxv) != OBMAG_BOX)
    goto end;
  ((basilysbox_ptr_t) boxv)->val = (basilys_ptr_t) valv;
  basilysgc_touch_dest (boxv, valv);
end:
  BASILYS_EXITFRAME ();
#undef boxv
#undef valv
}

/* safely return the content of a container - instance of CLASS_CONTAINER */
basilys_ptr_t
basilys_container_value (basilys_ptr_t cont)
{
  if (basilys_magic_discr (cont) != OBMAG_OBJECT
      || ((basilysobject_ptr_t) cont)->obj_len < FCONTAINER__LAST)
    return 0;
  if (!basilys_is_instance_of
      ((basilys_ptr_t) cont, (basilys_ptr_t) BASILYSGOB (CLASS_CONTAINER)))
    return 0;
  return ((basilysobject_ptr_t) cont)->obj_vartab[FCONTAINER_VALUE];
}

/* allocate a new boxed tree of given DISCR [or DISCR_TREE] & content
   VAL */
basilys_ptr_t
basilysgc_new_tree (basilysobject_ptr_t discr_p, tree tr)
{
  BASILYS_ENTERFRAME (2, NULL);
#define btreev  curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
  discrv = (void *) discr_p;
  if (!discrv)
    discrv = BASILYSG (DISCR_TREE);
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_TREE)
    goto end;
  btreev = basilysgc_allocate (sizeof (struct basilystree_st), 0);
  ((struct basilystree_st *) (btreev))->discr = (basilysobject_ptr_t) discrv;
  ((struct basilystree_st *) (btreev))->val = tr;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) btreev;
#undef btreev
#undef discrv
#undef object_discrv
}



/* allocate a new boxed gimple of given DISCR [or DISCR_GIMPLE] & content
   VAL */
basilys_ptr_t
basilysgc_new_gimple (basilysobject_ptr_t discr_p, gimple g)
{
  BASILYS_ENTERFRAME (2, NULL);
#define bgimplev  curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
  discrv = (void *) discr_p;
  if (!discrv)
    discrv = BASILYSG (DISCR_GIMPLE);
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_GIMPLE)
    goto end;
  bgimplev = basilysgc_allocate (sizeof (struct basilysgimple_st), 0);
  ((struct basilysgimple_st *) (bgimplev))->discr =
    (basilysobject_ptr_t) discrv;
  ((struct basilysgimple_st *) (bgimplev))->val = g;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) bgimplev;
#undef bgimplev
#undef discrv
#undef object_discrv
}


/* allocate a new boxed gimpleseq of given DISCR [or DISCR_GIMPLE] & content
   VAL */
basilys_ptr_t
basilysgc_new_gimpleseq (basilysobject_ptr_t discr_p, gimple_seq g)
{
  BASILYS_ENTERFRAME (2, NULL);
#define bgimplev  curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
  discrv = (void *) discr_p;
  if (!discrv)
    discrv = BASILYSG (DISCR_GIMPLESEQ);
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_GIMPLESEQ)
    goto end;
  bgimplev = basilysgc_allocate (sizeof (struct basilysgimpleseq_st), 0);
  ((struct basilysgimpleseq_st *) (bgimplev))->discr =
    (basilysobject_ptr_t) discrv;
  ((struct basilysgimpleseq_st *) (bgimplev))->val = g;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) bgimplev;
#undef bgimplev
#undef discrv
#undef object_discrv
}


/* allocate a new boxed basic_block of given DISCR [or DISCR_BASICBLOCK] & content
   VAL */
basilys_ptr_t
basilysgc_new_basicblock (basilysobject_ptr_t discr_p, basic_block bb)
{
  BASILYS_ENTERFRAME (2, NULL);
#define bbbv    curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
  discrv = (void *) discr_p;
  if (!discrv)
    discrv = BASILYSG (DISCR_BASICBLOCK);
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_BASICBLOCK)
    goto end;
  bbbv = basilysgc_allocate (sizeof (struct basilysbasicblock_st), 0);
  ((struct basilysbasicblock_st *) (bbbv))->discr =
    (basilysobject_ptr_t) discrv;
  ((struct basilysbasicblock_st *) (bbbv))->val = bb;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) bbbv;
#undef bbbv
#undef discrv
#undef object_discrv
}




/****** MULTIPLES ******/

/* allocate a multiple of arity 1 */
basilys_ptr_t
basilysgc_new_mult1 (basilysobject_ptr_t discr_p, basilys_ptr_t v0_p)
{
  BASILYS_ENTERFRAME (3, NULL);
#define newmul curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define v0   curfram__.varptr[2]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mult_newmul ((struct basilysmultiple_st*)(newmul))
  discrv = (void *) discr_p;
  v0 = v0_p;
  newmul = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MULTIPLE)
    goto end;
  newmul =
    basilysgc_allocate (sizeof (struct basilysmultiple_st),
			sizeof (void *) * 1);
  mult_newmul->discr = object_discrv;
  mult_newmul->nbval = 1;
  mult_newmul->tabval[0] = (basilys_ptr_t) v0;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmul;
#undef newmul
#undef discr
#undef v0
#undef mult_newmul
#undef object_discrv
}

basilys_ptr_t
basilysgc_new_mult2 (basilysobject_ptr_t discr_p,
		     basilys_ptr_t v0_p, basilys_ptr_t v1_p)
{
  BASILYS_ENTERFRAME (4, NULL);
#define newmul curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define v0   curfram__.varptr[2]
#define v1   curfram__.varptr[3]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mult_newmul ((struct basilysmultiple_st*)(newmul))
  discrv = (void *) discr_p;
  v0 = v0_p;
  v1 = v1_p;
  newmul = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MULTIPLE)
    goto end;
  newmul =
    basilysgc_allocate (sizeof (struct basilysmultiple_st),
			sizeof (void *) * 2);
  mult_newmul->discr = object_discrv;
  mult_newmul->nbval = 2;
  mult_newmul->tabval[0] = (basilys_ptr_t) v0;
  mult_newmul->tabval[1] = (basilys_ptr_t) v1;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmul;
#undef newmul
#undef discr
#undef v0
#undef v1
#undef mult_newmul
#undef object_discrv
}

basilys_ptr_t
basilysgc_new_mult3 (basilysobject_ptr_t discr_p,
		     basilys_ptr_t v0_p, basilys_ptr_t v1_p,
		     basilys_ptr_t v2_p)
{
  BASILYS_ENTERFRAME (5, NULL);
#define newmul curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define v0   curfram__.varptr[2]
#define v1   curfram__.varptr[3]
#define v2   curfram__.varptr[4]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mult_newmul ((struct basilysmultiple_st*)(newmul))
  discrv = (void *) discr_p;
  v0 = v0_p;
  v1 = v1_p;
  v2 = v2_p;
  newmul = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MULTIPLE)
    goto end;
  newmul =
    basilysgc_allocate (sizeof (struct basilysmultiple_st),
			sizeof (void *) * 3);
  mult_newmul->discr = object_discrv;
  mult_newmul->nbval = 3;
  mult_newmul->tabval[0] = (basilys_ptr_t) v0;
  mult_newmul->tabval[1] = (basilys_ptr_t) v1;
  mult_newmul->tabval[2] = (basilys_ptr_t) v2;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmul;
#undef newmul
#undef discrv
#undef v0
#undef v1
#undef v2
#undef mult_newmul
#undef object_discrv
}

basilys_ptr_t
basilysgc_new_mult4 (basilysobject_ptr_t discr_p,
		     basilys_ptr_t v0_p, basilys_ptr_t v1_p,
		     basilys_ptr_t v2_p, basilys_ptr_t v3_p)
{
  BASILYS_ENTERFRAME (6, NULL);
#define newmul curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define v0   curfram__.varptr[2]
#define v1   curfram__.varptr[3]
#define v2   curfram__.varptr[4]
#define v3   curfram__.varptr[5]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mult_newmul ((struct basilysmultiple_st*)(newmul))
  discrv = (void *) discr_p;
  v0 = v0_p;
  v1 = v1_p;
  v2 = v2_p;
  v3 = v3_p;
  newmul = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MULTIPLE)
    goto end;
  newmul =
    basilysgc_allocate (sizeof (struct basilysmultiple_st),
			sizeof (void *) * 4);
  mult_newmul->discr = object_discrv;
  mult_newmul->nbval = 4;
  mult_newmul->tabval[0] = (basilys_ptr_t) v0;
  mult_newmul->tabval[1] = (basilys_ptr_t) v1;
  mult_newmul->tabval[2] = (basilys_ptr_t) v2;
  mult_newmul->tabval[3] = (basilys_ptr_t) v3;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmul;
#undef newmul
#undef discrv
#undef v0
#undef v1
#undef v2
#undef v3
#undef mult_newmul
#undef object_discrv
}


basilys_ptr_t
basilysgc_new_mult5 (basilysobject_ptr_t discr_p,
		     basilys_ptr_t v0_p, basilys_ptr_t v1_p,
		     basilys_ptr_t v2_p, basilys_ptr_t v3_p,
		     basilys_ptr_t v4_p)
{
  BASILYS_ENTERFRAME (7, NULL);
#define newmul curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define v0   curfram__.varptr[2]
#define v1   curfram__.varptr[3]
#define v2   curfram__.varptr[4]
#define v3   curfram__.varptr[5]
#define v4   curfram__.varptr[6]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mult_newmul ((struct basilysmultiple_st*)(newmul))
  discrv = (void *) discr_p;
  v0 = v0_p;
  v1 = v1_p;
  v2 = v2_p;
  v3 = v3_p;
  v4 = v4_p;
  newmul = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MULTIPLE)
    goto end;
  newmul =
    basilysgc_allocate (sizeof (struct basilysmultiple_st),
			sizeof (void *) * 5);
  mult_newmul->discr = object_discrv;
  mult_newmul->nbval = 5;
  mult_newmul->tabval[0] = (basilys_ptr_t) v0;
  mult_newmul->tabval[1] = (basilys_ptr_t) v1;
  mult_newmul->tabval[2] = (basilys_ptr_t) v2;
  mult_newmul->tabval[3] = (basilys_ptr_t) v3;
  mult_newmul->tabval[4] = (basilys_ptr_t) v4;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmul;
#undef newmul
#undef discrv
#undef v0
#undef v1
#undef v2
#undef v3
#undef v4
#undef mult_newmul
#undef object_discrv
}


basilys_ptr_t
basilysgc_new_mult6 (basilysobject_ptr_t discr_p,
		     basilys_ptr_t v0_p, basilys_ptr_t v1_p,
		     basilys_ptr_t v2_p, basilys_ptr_t v3_p,
		     basilys_ptr_t v4_p, basilys_ptr_t v5_p)
{
  BASILYS_ENTERFRAME (8, NULL);
#define newmul curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define v0   curfram__.varptr[2]
#define v1   curfram__.varptr[3]
#define v2   curfram__.varptr[4]
#define v3   curfram__.varptr[5]
#define v4   curfram__.varptr[6]
#define v5   curfram__.varptr[7]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mult_newmul ((struct basilysmultiple_st*)(newmul))
  discrv = (void *) discr_p;
  v0 = v0_p;
  v1 = v1_p;
  v2 = v2_p;
  v3 = v3_p;
  v4 = v4_p;
  v5 = v5_p;
  newmul = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MULTIPLE)
    goto end;
  newmul =
    basilysgc_allocate (sizeof (struct basilysmultiple_st),
			sizeof (void *) * 6);
  mult_newmul->discr = object_discrv;
  mult_newmul->nbval = 6;
  mult_newmul->tabval[0] = (basilys_ptr_t) v0;
  mult_newmul->tabval[1] = (basilys_ptr_t) v1;
  mult_newmul->tabval[2] = (basilys_ptr_t) v2;
  mult_newmul->tabval[3] = (basilys_ptr_t) v3;
  mult_newmul->tabval[4] = (basilys_ptr_t) v4;
  mult_newmul->tabval[5] = (basilys_ptr_t) v5;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmul;
#undef newmul
#undef discrv
#undef v0
#undef v1
#undef v2
#undef v3
#undef v4
#undef v5
#undef mult_newmul
#undef object_discrv
}

basilys_ptr_t
basilysgc_new_mult7 (basilysobject_ptr_t discr_p,
		     basilys_ptr_t v0_p, basilys_ptr_t v1_p,
		     basilys_ptr_t v2_p, basilys_ptr_t v3_p,
		     basilys_ptr_t v4_p, basilys_ptr_t v5_p,
		     basilys_ptr_t v6_p)
{
  BASILYS_ENTERFRAME (9, NULL);
#define newmul curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define v0   curfram__.varptr[2]
#define v1   curfram__.varptr[3]
#define v2   curfram__.varptr[4]
#define v3   curfram__.varptr[5]
#define v4   curfram__.varptr[6]
#define v5   curfram__.varptr[7]
#define v6   curfram__.varptr[8]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mult_newmul ((struct basilysmultiple_st*)(newmul))
  discrv = (void *) discr_p;
  v0 = v0_p;
  v1 = v1_p;
  v2 = v2_p;
  v3 = v3_p;
  v4 = v4_p;
  v5 = v5_p;
  v6 = v6_p;
  newmul = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MULTIPLE)
    goto end;
  newmul =
    basilysgc_allocate (sizeof (struct basilysmultiple_st),
			sizeof (void *) * 7);
  mult_newmul->discr = object_discrv;
  mult_newmul->nbval = 7;
  mult_newmul->tabval[0] = (basilys_ptr_t) v0;
  mult_newmul->tabval[1] = (basilys_ptr_t) v1;
  mult_newmul->tabval[2] = (basilys_ptr_t) v2;
  mult_newmul->tabval[3] = (basilys_ptr_t) v3;
  mult_newmul->tabval[4] = (basilys_ptr_t) v4;
  mult_newmul->tabval[5] = (basilys_ptr_t) v5;
  mult_newmul->tabval[6] = (basilys_ptr_t) v6;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmul;
#undef newmul
#undef discrv
#undef v0
#undef v1
#undef v2
#undef v3
#undef v4
#undef v5
#undef v6
#undef mult_newmul
#undef object_discrv
}


basilys_ptr_t
basilysgc_new_list (basilysobject_ptr_t discr_p)
{
  BASILYS_ENTERFRAME (2, NULL);
#define discrv curfram__.varptr[0]
#define newlist curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define list_newlist ((struct basilyslist_st*)(newlist))
  discrv = (void *) discr_p;
  newlist = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_LIST)
    goto end;
  newlist = basilysgc_allocate (sizeof (struct basilyslist_st), 0);
  list_newlist->discr = object_discrv;
  list_newlist->first = NULL;
  list_newlist->last = NULL;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newlist;
#undef newlist
#undef discrv
#undef list_newlist
#undef object_discrv
}

/* allocate a pair of given head and tail */
basilys_ptr_t
basilysgc_new_pair (basilysobject_ptr_t discr_p, void *head_p, void *tail_p)
{
  BASILYS_ENTERFRAME (4, NULL);
#define pairv   curfram__.varptr[0]
#define discrv  curfram__.varptr[1]
#define headv   curfram__.varptr[2]
#define tailv   curfram__.varptr[3]
  discrv = discr_p;
  headv = head_p;
  tailv = tail_p;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT
      || ((basilysobject_ptr_t) (discrv))->object_magic != OBMAG_PAIR)
    goto end;
  if (basilys_magic_discr ((basilys_ptr_t) tailv) != OBMAG_PAIR)
    tailv = NULL;
  pairv = basilysgc_allocate (sizeof (struct basilyspair_st), 0);
  ((struct basilyspair_st *) (pairv))->discr = (basilysobject_ptr_t) discrv;
  ((struct basilyspair_st *) (pairv))->hd = (basilys_ptr_t) headv;
  ((struct basilyspair_st *) (pairv))->tl = (struct basilyspair_st *) tailv;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) pairv;
#undef pairv
#undef headv
#undef tailv
#undef discrv
}

/* change the head of a pair */
void
basilysgc_pair_set_head (basilys_ptr_t pair_p, void *head_p)
{
  BASILYS_ENTERFRAME (2, NULL);
#define pairv   curfram__.varptr[0]
#define headv   curfram__.varptr[1]
  pairv = pair_p;
  headv = head_p;
  if (basilys_magic_discr ((basilys_ptr_t) pairv) != OBMAG_PAIR)
    goto end;
  ((struct basilyspair_st *) pairv)->hd = (basilys_ptr_t) headv;
  basilysgc_touch_dest (pairv, headv);
end:
  BASILYS_EXITFRAME ();
#undef pairv
#undef headv
}


void
basilysgc_append_list (basilys_ptr_t list_p, basilys_ptr_t valu_p)
{
  BASILYS_ENTERFRAME (4, NULL);
#define list curfram__.varptr[0]
#define valu curfram__.varptr[1]
#define pairv curfram__.varptr[2]
#define lastv curfram__.varptr[3]
#define pai_pairv ((struct basilyspair_st*)(pairv))
#define list_list ((struct basilyslist_st*)(list))
  list = list_p;
  valu = valu_p;
  if (basilys_magic_discr ((basilys_ptr_t) list) != OBMAG_LIST
      || !BASILYSGOB (DISCR_PAIR))
    goto end;
  pairv = basilysgc_allocate (sizeof (struct basilyspair_st), 0);
  pai_pairv->discr = BASILYSGOB (DISCR_PAIR);
  pai_pairv->hd = (basilys_ptr_t) valu;
  pai_pairv->tl = NULL;
  gcc_assert (basilys_magic_discr ((basilys_ptr_t) pairv) == OBMAG_PAIR);
  lastv = list_list->last;
  if (basilys_magic_discr ((basilys_ptr_t) lastv) == OBMAG_PAIR)
    {
      gcc_assert (((struct basilyspair_st *) lastv)->tl == NULL);
      ((struct basilyspair_st *) lastv)->tl = (struct basilyspair_st *) pairv;
      basilysgc_touch_dest (lastv, pairv);
    }
  else
    list_list->first = (struct basilyspair_st *) pairv;
  list_list->last = (struct basilyspair_st *) pairv;
  basilysgc_touch_dest (list, pairv);
end:
  BASILYS_EXITFRAME ();
#undef list
#undef valu
#undef list_list
#undef pairv
#undef pai_pairv
#undef lastv
}

void
basilysgc_prepend_list (basilys_ptr_t list_p, basilys_ptr_t valu_p)
{
  BASILYS_ENTERFRAME (4, NULL);
#define list curfram__.varptr[0]
#define valu curfram__.varptr[1]
#define pairv curfram__.varptr[2]
#define firstv curfram__.varptr[3]
#define pai_pairv ((struct basilyspair_st*)(pairv))
#define list_list ((struct basilyslist_st*)(list))
  list = list_p;
  valu = valu_p;
  if (basilys_magic_discr ((basilys_ptr_t) list) != OBMAG_LIST
      || !BASILYSGOB (DISCR_PAIR))
    goto end;
  pairv = basilysgc_allocate (sizeof (struct basilyspair_st), 0);
  pai_pairv->discr = BASILYSGOB (DISCR_PAIR);
  pai_pairv->hd = (basilys_ptr_t) valu;
  pai_pairv->tl = NULL;
  gcc_assert (basilys_magic_discr ((basilys_ptr_t) pairv) == OBMAG_PAIR);
  firstv = (basilys_ptr_t) (list_list->first);
  if (basilys_magic_discr ((basilys_ptr_t) firstv) == OBMAG_PAIR)
    {
      pai_pairv->tl = (struct basilyspair_st *) firstv;
      basilysgc_touch_dest (pairv, firstv);
    }
  else
    list_list->last = (struct basilyspair_st *) pairv;
  list_list->first = (struct basilyspair_st *) pairv;
  basilysgc_touch_dest (list, pairv);
end:
  BASILYS_EXITFRAME ();
#undef list
#undef valu
#undef list_list
#undef pairv
#undef pai_pairv
}


basilys_ptr_t
basilysgc_popfirst_list (basilys_ptr_t list_p)
{
  BASILYS_ENTERFRAME (3, NULL);
#define list curfram__.varptr[0]
#define valu curfram__.varptr[1]
#define pairv curfram__.varptr[2]
#define pai_pairv ((struct basilyspair_st*)(pairv))
#define list_list ((struct basilyslist_st*)(list))
  list = list_p;
  if (basilys_magic_discr ((basilys_ptr_t) list) != OBMAG_LIST)
    goto end;
  pairv = list_list->first;
  if (basilys_magic_discr ((basilys_ptr_t) pairv) != OBMAG_PAIR)
    goto end;
  if (list_list->last == pairv)
    {
      valu = pai_pairv->hd;
      list_list->first = NULL;
      list_list->last = NULL;
    }
  else
    {
      valu = pai_pairv->hd;
      list_list->first = pai_pairv->tl;
    }
  basilysgc_touch (list);
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) valu;
#undef list
#undef value
#undef list_list
#undef pairv
#undef pai_pairv
}				/* enf of popfirst */


/* return the length of a list or -1 iff non list */
int
basilys_list_length (basilys_ptr_t list_p)
{
  struct basilyspair_st *pair = NULL;
  int ln = 0;
  if (!list_p)
    return 0;
  if (basilys_magic_discr (list_p) != OBMAG_LIST)
    return -1;
  for (pair = ((struct basilyslist_st *) list_p)->first;
       basilys_magic_discr ((basilys_ptr_t) pair) ==
       OBMAG_PAIR; pair = (struct basilyspair_st *) (pair->tl))
    ln++;
  return ln;
}


/* allocate a new empty mapobjects */
basilys_ptr_t
basilysgc_new_mapobjects (basilysobject_ptr_t discr_p, unsigned len)
{
  int maplen = 0;
  int lenix = 0, primlen = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define discrv curfram__.varptr[0]
#define newmapv curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mapobject_newmapv ((struct basilysmapobjects_st*)(newmapv))
  discrv = discr_p;
  if (!discrv || object_discrv->obj_class->object_magic != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MAPOBJECTS)
    goto end;
  if (len > 0)
    {
      gcc_assert (len < (unsigned) BASILYS_MAXLEN);
      for (lenix = 1;
	   (primlen = (int) basilys_primtab[lenix]) != 0
	   && primlen <= (int) len; lenix++);
      maplen = primlen;
    };
  newmapv =
    basilysgc_allocate (offsetof
			(struct basilysmapobjects_st, map_space),
			maplen * sizeof (struct entryobjectsbasilys_st));
  mapobject_newmapv->discr = object_discrv;
  if (len > 0)
    {
      mapobject_newmapv->entab = mapobject_newmapv->map_space;
      mapobject_newmapv->lenix = lenix;
    };
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmapv;
#undef discrv
#undef newmapv
#undef object_discrv
#undef mapobject_newmapv
}

/* get from a mapobject */
basilys_ptr_t
basilys_get_mapobjects (basilysmapobjects_ptr_t mapobject_p,
			basilysobject_ptr_t attrobject_p)
{
  long ix, len;
  basilys_ptr_t val = NULL;
  if (!mapobject_p || !attrobject_p
      || mapobject_p->discr->object_magic != OBMAG_MAPOBJECTS
      || !mapobject_p->entab
      || attrobject_p->obj_class->object_magic != OBMAG_OBJECT)
    return NULL;
  len = basilys_primtab[mapobject_p->lenix];
  if (len <= 0)
    return NULL;
  ix = unsafe_index_mapobject (mapobject_p->entab, attrobject_p, len);
  if (ix < 0)
    return NULL;
  if (mapobject_p->entab[ix].e_at == attrobject_p)
    val = mapobject_p->entab[ix].e_va;
  return val;
}

void
basilysgc_put_mapobjects (basilysmapobjects_ptr_t
			  mapobject_p,
			  basilysobject_ptr_t attrobject_p,
			  basilys_ptr_t valu_p)
{
  long ix = 0, len = 0, cnt = 0;
#if ENABLE_CHECKING
  static long callcount;
  long curcount = ++callcount;
#endif
  BASILYS_ENTERFRAME (4, NULL);
#define discrv curfram__.varptr[0]
#define mapobjectv curfram__.varptr[1]
#define attrobjectv curfram__.varptr[2]
#define valuv curfram__.varptr[3]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define object_attrobjectv ((basilysobject_ptr_t)(attrobjectv))
#define map_mapobjectv ((basilysmapobjects_ptr_t)(mapobjectv))
  mapobjectv = mapobject_p;
  attrobjectv = attrobject_p;
  valuv = valu_p;
  if (!mapobjectv || !attrobjectv || !valuv)
    goto end;
  discrv = map_mapobjectv->discr;
  if (!discrv || object_discrv->object_magic != OBMAG_MAPOBJECTS)
    goto end;
  discrv = object_attrobjectv->obj_class;
  if (!discrv || object_discrv->object_magic != OBMAG_OBJECT)
    goto end;
  if (!map_mapobjectv->entab)
    {
      /* fresh map without any entab; allocate it minimally */
      size_t lensiz = 0;
      len = basilys_primtab[1];	/* i.e. 3 */
      lensiz = len * sizeof (struct entryobjectsbasilys_st);
      if (basilys_is_young (mapobjectv))
	{
	  basilysgc_reserve (lensiz + 20);
	  if (!basilys_is_young (mapobjectv))
	    goto alloc_old_smallmapobj;
	  map_mapobjectv->entab =
	    (struct entryobjectsbasilys_st *)
	    basilys_allocatereserved (lensiz, 0);
	}
      else
	{
	alloc_old_smallmapobj:
	  map_mapobjectv->entab =
	    (struct entryobjectsbasilys_st *) ggc_alloc_cleared (lensiz);
	}
      map_mapobjectv->lenix = 1;
      basilysgc_touch (map_mapobjectv);
    }
  else
    if ((len = basilys_primtab[map_mapobjectv->lenix]) <=
	(5 * (cnt = map_mapobjectv->count)) / 4
	|| (len <= 5 && cnt + 1 >= len))
    {
      /* entab is nearly full so need to be resized */
      int ix, newcnt = 0;
      int newlen = basilys_primtab[map_mapobjectv->lenix + 1];
      size_t newlensiz = 0;
      struct entryobjectsbasilys_st *newtab = NULL;
      struct entryobjectsbasilys_st *oldtab = NULL;
      newlensiz = newlen * sizeof (struct entryobjectsbasilys_st);
      if (basilys_is_young (map_mapobjectv->entab))
	{
	  basilysgc_reserve (newlensiz + 100);
	  if (!basilys_is_young (map_mapobjectv))
	    goto alloc_old_mapobj;
	  newtab =
	    (struct entryobjectsbasilys_st *)
	    basilys_allocatereserved (newlensiz, 0);
	}
      else
	{
	alloc_old_mapobj:
	  newtab =
	    (struct entryobjectsbasilys_st *) ggc_alloc_cleared (newlensiz);
	};
      oldtab = map_mapobjectv->entab;
      for (ix = 0; ix < len; ix++)
	{
	  basilysobject_ptr_t curat = oldtab[ix].e_at;
	  int newix;
	  if (!curat || curat == (void *) HTAB_DELETED_ENTRY)
	    continue;
	  newix = unsafe_index_mapobject (newtab, curat, newlen);
	  gcc_assert (newix >= 0);
	  newtab[newix] = oldtab[ix];
	  newcnt++;
	}
      if (!basilys_is_young (oldtab))
	/* free oldtab since it is in old ggc space */
	ggc_free (oldtab);
      map_mapobjectv->entab = newtab;
      map_mapobjectv->count = newcnt;
      map_mapobjectv->lenix++;
      basilysgc_touch (map_mapobjectv);
      len = newlen;
    }
  ix =
    unsafe_index_mapobject (map_mapobjectv->entab, object_attrobjectv, len);
#if ENABLE_CHECKING
  if (ix < 0)
    debugeprintf
      ("put_mapobjects failed curcount %ld len %ld map %p count %d lenix %d",
       curcount, len, mapobjectv, map_mapobjectv->count,
       map_mapobjectv->lenix);
#endif
  gcc_assert (ix >= 0);
  if (map_mapobjectv->entab[ix].e_at != attrobjectv)
    {
      map_mapobjectv->entab[ix].e_at = (basilysobject_ptr_t) attrobjectv;
      map_mapobjectv->count++;
    }
  map_mapobjectv->entab[ix].e_va = (basilys_ptr_t) valuv;
  basilysgc_touch_dest (map_mapobjectv, attrobjectv);
  basilysgc_touch_dest (map_mapobjectv, valuv);
end:
  BASILYS_EXITFRAME ();
#undef discrv
#undef mapobjectv
#undef attrobjectv
#undef valuv
#undef object_discrv
#undef object_attrobjectv
#undef map_mapobjectv
}


basilys_ptr_t
basilysgc_remove_mapobjects (basilysmapobjects_ptr_t
			     mapobject_p, basilysobject_ptr_t attrobject_p)
{
  long ix = 0, len = 0, cnt = 0;
  BASILYS_ENTERFRAME (4, NULL);
#define discrv curfram__.varptr[0]
#define mapobjectv curfram__.varptr[1]
#define attrobjectv curfram__.varptr[2]
#define valuv curfram__.varptr[3]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define object_attrobjectv ((basilysobject_ptr_t)(attrobjectv))
#define map_mapobjectv ((basilysmapobjects_ptr_t)(mapobjectv))
  mapobjectv = mapobject_p;
  attrobjectv = attrobject_p;
  valuv = NULL;
  if (!mapobjectv || !attrobjectv)
    goto end;
  discrv = map_mapobjectv->discr;
  if (!discrv || object_discrv->object_magic != OBMAG_MAPOBJECTS)
    goto end;
  discrv = object_attrobjectv->obj_class;
  if (!discrv || object_discrv->object_magic != OBMAG_OBJECT)
    goto end;
  if (!map_mapobjectv->entab)
    goto end;
  len = basilys_primtab[map_mapobjectv->lenix];
  if (len <= 0)
    goto end;
  ix = unsafe_index_mapobject (map_mapobjectv->entab, attrobject_p, len);
  if (ix < 0 || map_mapobjectv->entab[ix].e_at != attrobjectv)
    goto end;
  map_mapobjectv->entab[ix].e_at = (basilysobject_ptr_t) HTAB_DELETED_ENTRY;
  valuv = map_mapobjectv->entab[ix].e_va;
  map_mapobjectv->entab[ix].e_va = NULL;
  map_mapobjectv->count--;
  cnt = map_mapobjectv->count;
  if (len >= 7 && cnt < len / 2 - 2)
    {
      int newcnt = 0, newlen = 0, newlenix = 0;
      size_t newlensiz = 0;
      struct entryobjectsbasilys_st *oldtab = NULL, *newtab = NULL;
      for (newlenix = map_mapobjectv->lenix;
	   (newlen = basilys_primtab[newlenix]) > 2 * cnt + 3; newlenix--);
      if (newlen >= len)
	goto end;
      newlensiz = newlen * sizeof (struct entryobjectsbasilys_st);
      if (basilys_is_young (map_mapobjectv->entab))
	{
	  /* reserve a zone; if a GC occurred, the mapobject & entab
	     could become old */
	  basilysgc_reserve (newlensiz + 10 * sizeof (void *));
	  if (!basilys_is_young (map_mapobjectv))
	    goto alloc_old_entries;
	  newtab =
	    (struct entryobjectsbasilys_st *)
	    basilys_allocatereserved (newlensiz, 0);
	}
      else
	{
	alloc_old_entries:
	  newtab =
	    (struct entryobjectsbasilys_st *) ggc_alloc_cleared (newlensiz);
	}
      oldtab = map_mapobjectv->entab;
      for (ix = 0; ix < len; ix++)
	{
	  basilysobject_ptr_t curat = oldtab[ix].e_at;
	  int newix;
	  if (!curat || curat == (void *) HTAB_DELETED_ENTRY)
	    continue;
	  newix = unsafe_index_mapobject (newtab, curat, newlen);
	  gcc_assert (newix >= 0);
	  newtab[newix] = oldtab[ix];
	  newcnt++;
	}
      if (!basilys_is_young (oldtab))
	/* free oldtab since it is in old ggc space */
	ggc_free (oldtab);
      map_mapobjectv->entab = newtab;
      map_mapobjectv->count = newcnt;
      map_mapobjectv->lenix = newlenix;
    }
  basilysgc_touch (map_mapobjectv);
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) valuv;
#undef discrv
#undef mapobjectv
#undef attrobjectv
#undef valuv
#undef object_discrv
#undef object_attrobjectv
#undef map_mapobjectv
}



/* index of entry to get or add an attribute in an mapstring (or -1 on error) */
static inline int
unsafe_index_mapstring (struct entrystringsbasilys_st *tab,
			const char *attr, int siz)
{
  int ix = 0, frix = -1;
  unsigned h = 0;
  if (!tab || !attr || siz <= 0)
    return -1;
  h = (unsigned) htab_hash_string (attr) & BASILYS_MAXHASH;
  h = h % siz;
  for (ix = h; ix < siz; ix++)
    {
      const char *curat = tab[ix].e_at;
      if (curat == (void *) HTAB_DELETED_ENTRY)
	{
	  if (frix < 0)
	    frix = ix;
	}
      else if (!curat)
	{
	  if (frix < 0)
	    frix = ix;
	  return frix;
	}
      else if (!strcmp (curat, attr))
	return ix;
    }
  for (ix = 0; ix < (int) h; ix++)
    {
      const char *curat = tab[ix].e_at;
      if (curat == (void *) HTAB_DELETED_ENTRY)
	{
	  if (frix < 0)
	    frix = ix;
	}
      else if (!curat)
	{
	  if (frix < 0)
	    frix = ix;
	  return frix;
	}
      else if (!strcmp (curat, attr))
	return ix;
    }
  if (frix >= 0)		/* found a place in a table with deleted entries */
    return frix;
  return -1;			/* entirely full, should not happen */
}

/* allocate a new empty mapstrings */
basilys_ptr_t
basilysgc_new_mapstrings (basilysobject_ptr_t discr_p, unsigned len)
{
  BASILYS_ENTERFRAME (2, NULL);
#define discrv curfram__.varptr[0]
#define newmapv curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define mapstring_newmapv ((struct basilysmapstrings_st*)(newmapv))
  discrv = discr_p;
  if (!discrv || object_discrv->obj_class->object_magic != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_MAPSTRINGS)
    goto end;
  newmapv = basilysgc_allocate (sizeof (struct basilysmapstrings_st), 0);
  mapstring_newmapv->discr = object_discrv;
  if (len > 0)
    {
      int lenix, primlen;
      gcc_assert (len < (unsigned) BASILYS_MAXLEN);
      for (lenix = 1;
	   (primlen = (int) basilys_primtab[lenix]) != 0
	   && primlen <= (int) len; lenix++);
      /* the newmapv is always young */
      mapstring_newmapv->entab = (struct entrystringsbasilys_st *)
	basilysgc_allocate (primlen *
			    sizeof (struct entrystringsbasilys_st), 0);
      mapstring_newmapv->lenix = lenix;
      basilysgc_touch_dest (newmapv, mapstring_newmapv->entab);
    }
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) newmapv;
#undef discrv
#undef newmapv
#undef object_discrv
#undef mapstring_newmapv
}


void
basilysgc_put_mapstrings (struct basilysmapstrings_st
			  *mapstring_p, const char *attr,
			  basilys_ptr_t valu_p)
{
  long ix = 0, len = 0, cnt = 0, atlen = 0;
  char *attrdup = 0;
  char tinybuf[130];
  BASILYS_ENTERFRAME (3, NULL);
#define discrv curfram__.varptr[0]
#define mapstringv curfram__.varptr[1]
#define valuv curfram__.varptr[2]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define map_mapstringv ((struct basilysmapstrings_st*)(mapstringv))
  mapstringv = mapstring_p;
  valuv = valu_p;
  if (!mapstringv || !attr || !attr[0] || !valuv)
    goto end;
  discrv = map_mapstringv->discr;
  if (!discrv || object_discrv->object_magic != OBMAG_MAPSTRINGS)
    goto end;
  atlen = strlen (attr);
  if (atlen < (int) sizeof (tinybuf) - 1)
    {
      memset (tinybuf, 0, sizeof (tinybuf));
      attrdup = strcpy (tinybuf, attr);
    }
  else
    attrdup = strcpy ((char *) xcalloc (atlen + 1, 1), attr);
  if (!map_mapstringv->entab)
    {
      size_t lensiz = 0;
      len = basilys_primtab[1];	/* i.e. 3 */
      lensiz = len * sizeof (struct entrystringsbasilys_st);
      if (basilys_is_young (mapstringv))
	{
	  basilysgc_reserve (lensiz + 16 * sizeof (void *));
	  if (!basilys_is_young (mapstringv))
	    goto alloc_old_small_mapstring;
	  map_mapstringv->entab =
	    (struct entrystringsbasilys_st *)
	    basilys_allocatereserved (lensiz, 0);
	}
      else
	{
	alloc_old_small_mapstring:
	  map_mapstringv->entab =
	    (struct entrystringsbasilys_st *) ggc_alloc_cleared (lensiz);
	}
      map_mapstringv->lenix = 1;
      basilysgc_touch (map_mapstringv);
    }
  else
    if ((len = basilys_primtab[map_mapstringv->lenix]) <=
	(5 * (cnt = map_mapstringv->count)) / 4
	|| (len <= 5 && cnt + 1 >= len))
    {
      int ix, newcnt = 0;
      int newlen = basilys_primtab[map_mapstringv->lenix + 1];
      struct entrystringsbasilys_st *oldtab = NULL;
      struct entrystringsbasilys_st *newtab = NULL;
      size_t newlensiz = newlen * sizeof (struct entrystringsbasilys_st);
      if (basilys_is_young (mapstringv))
	{
	  basilysgc_reserve (newlensiz + 10 * sizeof (void *));
	  if (!basilys_is_young (mapstringv))
	    goto alloc_old_mapstring;
	  newtab =
	    (struct entrystringsbasilys_st *)
	    basilys_allocatereserved (newlensiz, 0);
	}
      else
	{
	alloc_old_mapstring:
	  newtab =
	    (struct entrystringsbasilys_st *) ggc_alloc_cleared (newlensiz);
	};
      oldtab = map_mapstringv->entab;
      for (ix = 0; ix < len; ix++)
	{
	  const char *curat = oldtab[ix].e_at;
	  int newix;
	  if (!curat || curat == (void *) HTAB_DELETED_ENTRY)
	    continue;
	  newix = unsafe_index_mapstring (newtab, curat, newlen);
	  gcc_assert (newix >= 0);
	  newtab[newix] = oldtab[ix];
	  newcnt++;
	}
      if (!basilys_is_young (oldtab))
	/* free oldtab since it is in old ggc space */
	ggc_free (oldtab);
      map_mapstringv->entab = newtab;
      map_mapstringv->count = newcnt;
      map_mapstringv->lenix++;
      basilysgc_touch (map_mapstringv);
      len = newlen;
    }
  ix = unsafe_index_mapstring (map_mapstringv->entab, attrdup, len);
  gcc_assert (ix >= 0);
  if (!map_mapstringv->entab[ix].e_at
      || map_mapstringv->entab[ix].e_at == HTAB_DELETED_ENTRY)
    {
      char *newat = (char *) basilysgc_allocate (atlen + 1, 0);
      strcpy (newat, attrdup);
      map_mapstringv->entab[ix].e_at = newat;
      map_mapstringv->count++;
    }
  map_mapstringv->entab[ix].e_va = (basilys_ptr_t) valuv;
  basilysgc_touch_dest (map_mapstringv, valuv);
end:
  if (attrdup && attrdup != tinybuf)
    free (attrdup);
  BASILYS_EXITFRAME ();
#undef discrv
#undef mapstringv
#undef attrobjectv
#undef valuv
#undef object_discrv
#undef object_attrobjectv
#undef map_mapstringv
}

basilys_ptr_t
basilys_get_mapstrings (struct basilysmapstrings_st
			*mapstring_p, const char *attr)
{
  long ix = 0, len = 0;
  const char *oldat = NULL;
  if (!mapstring_p || !attr)
    return NULL;
  if (mapstring_p->discr->object_magic != OBMAG_MAPSTRINGS)
    return NULL;
  if (!mapstring_p->entab)
    return NULL;
  len = basilys_primtab[mapstring_p->lenix];
  if (len <= 0)
    return NULL;
  ix = unsafe_index_mapstring (mapstring_p->entab, attr, len);
  if (ix < 0 || !(oldat = mapstring_p->entab[ix].e_at)
      || oldat == HTAB_DELETED_ENTRY)
    return NULL;
  return mapstring_p->entab[ix].e_va;
}

basilys_ptr_t
basilysgc_remove_mapstrings (struct basilysmapstrings_st *
			     mapstring_p, const char *attr)
{
  long ix = 0, len = 0, cnt = 0, atlen = 0;
  const char *oldat = NULL;
  char *attrdup = 0;
  char tinybuf[130];
  BASILYS_ENTERFRAME (3, NULL);
#define discrv curfram__.varptr[0]
#define mapstringv curfram__.varptr[1]
#define valuv curfram__.varptr[2]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define map_mapstringv ((struct basilysmapstrings_st*)(mapstringv))
  mapstringv = mapstring_p;
  valuv = NULL;
  if (!mapstringv || !attr || !valuv || !attr[0])
    goto end;
  atlen = strlen (attr);
  discrv = map_mapstringv->discr;
  if (!discrv || object_discrv->object_magic != OBMAG_MAPSTRINGS)
    goto end;
  if (!map_mapstringv->entab)
    goto end;
  len = basilys_primtab[map_mapstringv->lenix];
  if (len <= 0)
    goto end;
  if (atlen < (int) sizeof (tinybuf) - 1)
    {
      memset (tinybuf, 0, sizeof (tinybuf));
      attrdup = strcpy (tinybuf, attr);
    }
  else
    attrdup = strcpy ((char *) xcalloc (atlen + 1, 1), attr);
  ix = unsafe_index_mapstring (map_mapstringv->entab, attrdup, len);
  if (ix < 0 || !(oldat = map_mapstringv->entab[ix].e_at)
      || oldat == HTAB_DELETED_ENTRY)
    goto end;
  if (!basilys_is_young (oldat))
    ggc_free (CONST_CAST(char *, oldat));
  map_mapstringv->entab[ix].e_at = (char *) HTAB_DELETED_ENTRY;
  valuv = map_mapstringv->entab[ix].e_va;
  map_mapstringv->entab[ix].e_va = NULL;
  map_mapstringv->count--;
  cnt = map_mapstringv->count;
  if (len > 7 && 2 * cnt + 2 < len)
    {
      int newcnt = 0, newlen = 0, newlenix = 0;
      size_t newlensiz = 0;
      struct entrystringsbasilys_st *oldtab = NULL, *newtab = NULL;
      for (newlenix = map_mapstringv->lenix;
	   (newlen = basilys_primtab[newlenix]) > 2 * cnt + 3; newlenix--);
      if (newlen >= len)
	goto end;
      newlensiz = newlen * sizeof (struct entrystringsbasilys_st);
      if (basilys_is_young (mapstringv))
	{
	  basilysgc_reserve (newlensiz + 10 * sizeof (void *));
	  if (!basilys_is_young (mapstringv))
	    goto alloc_old_mapstring_newtab;
	  newtab =
	    (struct entrystringsbasilys_st *)
	    basilys_allocatereserved (newlensiz, 0);
	}
      else
	{
	alloc_old_mapstring_newtab:
	  newtab =
	    (struct entrystringsbasilys_st *) ggc_alloc_cleared (newlensiz);
	}
      oldtab = map_mapstringv->entab;
      for (ix = 0; ix < len; ix++)
	{
	  const char *curat = oldtab[ix].e_at;
	  int newix;
	  if (!curat || curat == (void *) HTAB_DELETED_ENTRY)
	    continue;
	  newix = unsafe_index_mapstring (newtab, curat, newlen);
	  gcc_assert (newix >= 0);
	  newtab[newix] = oldtab[ix];
	  newcnt++;
	}
      if (!basilys_is_young (oldtab))
	/* free oldtab since it is in ol<d ggc space */
	ggc_free (oldtab);
      map_mapstringv->entab = newtab;
      map_mapstringv->count = newcnt;
    }
  basilysgc_touch (map_mapstringv);
end:
  if (attrdup && attrdup != tinybuf)
    free (attrdup);
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) valuv;
#undef discrv
#undef mapstringv
#undef valuv
#undef object_discrv
#undef map_mapstringv
}







/* index of entry to get or add an attribute in an mappointer (or -1 on error) */
struct entrypointerbasilys_st
{
  const void *e_at;
  basilys_ptr_t e_va;
};
static inline int
unsafe_index_mappointer (struct entrypointerbasilys_st *tab,
			 const void *attr, int siz)
{
  int ix = 0, frix = -1;
  unsigned h = 0;
  if (!tab || !attr || siz <= 0)
    return -1;
  h = ((unsigned) (((long) (attr)) >> 3)) & BASILYS_MAXHASH;
  h = h % siz;
  for (ix = h; ix < siz; ix++)
    {
      const void *curat = tab[ix].e_at;
      if (curat == (void *) HTAB_DELETED_ENTRY)
	{
	  if (frix < 0)
	    frix = ix;
	}
      else if (!curat)
	{
	  if (frix < 0)
	    frix = ix;
	  return frix;
	}
      else if (curat == attr)
	return ix;
    }
  for (ix = 0; ix < (int) h; ix++)
    {
      const void *curat = tab[ix].e_at;
      if (curat == (void *) HTAB_DELETED_ENTRY)
	{
	  if (frix < 0)
	    frix = ix;
	}
      else if (!curat)
	{
	  if (frix < 0)
	    frix = ix;
	  return frix;
	}
      else if (curat == attr)
	return ix;
    }
  if (frix >= 0)		/* found some place in a table with deleted entries */
    return frix;
  return -1;			/* entirely full, should not happen */
}


/* this should be the same as basilysmaptrees_st, basilysmapedges_st,
   basilysmapbasicblocks_st, .... */
struct basilysmappointers_st
{
  basilysobject_ptr_t discr;
  unsigned count;
  unsigned char lenix;
  struct entrypointerbasilys_st *entab;
  /* the following field is usually the value of entab (for
     objects in the young zone), to allocate the object and its fields
     at once */
  struct entrypointerbasilys_st map_space[FLEXIBLE_DIM];
};
/* allocate a new empty mappointers without checks */
void *
basilysgc_raw_new_mappointers (basilysobject_ptr_t discr_p, unsigned len)
{
  int lenix = 0, primlen = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define discrv curfram__.varptr[0]
#define newmapv curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define map_newmapv ((struct basilysmappointers_st*)(newmapv))
  discrv = discr_p;
  if (len > 0)
    {
      gcc_assert (len < (unsigned) BASILYS_MAXLEN);
      for (lenix = 1;
	   (primlen = (int) basilys_primtab[lenix]) != 0
	   && primlen <= (int) len; lenix++);
    };
  gcc_assert (sizeof (struct entrypointerbasilys_st) ==
	      sizeof (struct entrytreesbasilys_st));
  gcc_assert (sizeof (struct entrypointerbasilys_st) ==
	      sizeof (struct entrygimplesbasilys_st));
  gcc_assert (sizeof (struct entrypointerbasilys_st) ==
	      sizeof (struct entryedgesbasilys_st));
  gcc_assert (sizeof (struct entrypointerbasilys_st) ==
	      sizeof (struct entrybasicblocksbasilys_st));
  newmapv =
    basilysgc_allocate (offsetof
			(struct basilysmappointers_st,
			 map_space),
			primlen * sizeof (struct entrypointerbasilys_st));
  map_newmapv->discr = object_discrv;
  map_newmapv->count = 0;
  map_newmapv->lenix = lenix;
  if (len > 0)
    map_newmapv->entab = map_newmapv->map_space;
  else
    map_newmapv->entab = NULL;
  BASILYS_EXITFRAME ();
  return newmapv;
#undef discrv
#undef newmapv
#undef object_discrv
#undef map_newmapv
}


void
basilysgc_raw_put_mappointers (void *mappointer_p,
			       const void *attr, basilys_ptr_t valu_p)
{
  long ix = 0, len = 0, cnt = 0;
  size_t lensiz = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define mappointerv curfram__.varptr[0]
#define valuv curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define map_mappointerv ((struct basilysmappointers_st*)(mappointerv))
  mappointerv = mappointer_p;
  valuv = valu_p;
  if (!map_mappointerv->entab)
    {
      len = basilys_primtab[1];	/* i.e. 3 */
      lensiz = len * sizeof (struct entrypointerbasilys_st);
      if (basilys_is_young (mappointerv))
	{
	  basilysgc_reserve (lensiz + 10 * sizeof (void *));
	  if (!basilys_is_young (mappointerv))
	    goto alloc_old_mappointer_small_entab;
	  map_mappointerv->entab =
	    (struct entrypointerbasilys_st *)
	    basilys_allocatereserved (lensiz, 0);
	}
      else
	{
	alloc_old_mappointer_small_entab:
	  map_mappointerv->entab = (struct entrypointerbasilys_st *)
	    ggc_alloc_cleared (len * sizeof (struct entrypointerbasilys_st));
	}
      map_mappointerv->lenix = 1;
      basilysgc_touch (map_mappointerv);
    }
  else
    if ((len = basilys_primtab[map_mappointerv->lenix]) <=
	(5 * (cnt = map_mappointerv->count)) / 4
	|| (len <= 5 && cnt + 1 >= len))
    {
      int ix, newcnt = 0;
      int newlen = basilys_primtab[map_mappointerv->lenix + 1];
      struct entrypointerbasilys_st *oldtab = NULL;
      struct entrypointerbasilys_st *newtab = NULL;
      size_t newlensiz = newlen * sizeof (struct entrypointerbasilys_st);
      if (basilys_is_young (mappointerv))
	{
	  basilysgc_reserve (newlensiz + 10 * sizeof (void *));
	  if (!basilys_is_young (mappointerv))
	    goto alloc_old_mappointer_entab;
	  newtab =
	    (struct entrypointerbasilys_st *)
	    basilys_allocatereserved (newlensiz, 0);
	}
      else
	{
	alloc_old_mappointer_entab:
	  newtab = (struct entrypointerbasilys_st *)
	    ggc_alloc_cleared (newlen *
			       sizeof (struct entrypointerbasilys_st));
	}
      oldtab = map_mappointerv->entab;
      for (ix = 0; ix < len; ix++)
	{
	  const void *curat = oldtab[ix].e_at;
	  int newix;
	  if (!curat || curat == (void *) HTAB_DELETED_ENTRY)
	    continue;
	  newix = unsafe_index_mappointer (newtab, curat, newlen);
	  gcc_assert (newix >= 0);
	  newtab[newix] = oldtab[ix];
	  newcnt++;
	}
      if (!basilys_is_young (oldtab))
	/* free oldtab since it is in old ggc space */
	ggc_free (oldtab);
      map_mappointerv->entab = newtab;
      map_mappointerv->count = newcnt;
      map_mappointerv->lenix++;
      basilysgc_touch (map_mappointerv);
      len = newlen;
    }
  ix = unsafe_index_mappointer (map_mappointerv->entab, attr, len);
  gcc_assert (ix >= 0);
  if (!map_mappointerv->entab[ix].e_at
      || map_mappointerv->entab[ix].e_at == HTAB_DELETED_ENTRY)
    {
      map_mappointerv->entab[ix].e_at = attr;
      map_mappointerv->count++;
    }
  map_mappointerv->entab[ix].e_va = (basilys_ptr_t) valuv;
  basilysgc_touch_dest (map_mappointerv, valuv);
  BASILYS_EXITFRAME ();
#undef discrv
#undef mappointerv
#undef valuv
#undef object_discrv
#undef map_mappointerv
}

basilys_ptr_t
basilys_raw_get_mappointers (void *map, const void *attr)
{
  long ix = 0, len = 0;
  const void *oldat = NULL;
  struct basilysmappointers_st *mappointer_p =
    (struct basilysmappointers_st *) map;
  if (!mappointer_p->entab)
    return NULL;
  len = basilys_primtab[mappointer_p->lenix];
  if (len <= 0)
    return NULL;
  ix = unsafe_index_mappointer (mappointer_p->entab, attr, len);
  if (ix < 0 || !(oldat = mappointer_p->entab[ix].e_at)
      || oldat == HTAB_DELETED_ENTRY)
    return NULL;
  return mappointer_p->entab[ix].e_va;
}

basilys_ptr_t
basilysgc_raw_remove_mappointers (void *mappointer_p, const void *attr)
{
  long ix = 0, len = 0, cnt = 0;
  const char *oldat = NULL;
  BASILYS_ENTERFRAME (3, NULL);
#define discrv curfram__.varptr[0]
#define mappointerv curfram__.varptr[1]
#define valuv curfram__.varptr[2]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define map_mappointerv ((struct basilysmappointers_st*)(mappointerv))
  mappointerv = mappointer_p;
  valuv = NULL;
  if (!map_mappointerv->entab)
    goto end;
  len = basilys_primtab[map_mappointerv->lenix];
  if (len <= 0)
    goto end;
  ix = unsafe_index_mappointer (map_mappointerv->entab, attr, len);
  if (ix < 0 || !(oldat = (const char *) map_mappointerv->entab[ix].e_at)
      || oldat == HTAB_DELETED_ENTRY)
    goto end;
  map_mappointerv->entab[ix].e_at = (void *) HTAB_DELETED_ENTRY;
  valuv = map_mappointerv->entab[ix].e_va;
  map_mappointerv->entab[ix].e_va = NULL;
  map_mappointerv->count--;
  cnt = map_mappointerv->count;
  if (len > 7 && 2 * cnt + 2 < len)
    {
      int newcnt = 0, newlen = 0, newlenix = 0;
      struct entrypointerbasilys_st *oldtab = NULL, *newtab = NULL;
      size_t newlensiz = 0;
      for (newlenix = map_mappointerv->lenix;
	   (newlen = basilys_primtab[newlenix]) > 2 * cnt + 3; newlenix--);
      if (newlen >= len)
	goto end;
      newlensiz = newlen * sizeof (struct entrypointerbasilys_st);
      if (basilys_is_young (mappointerv))
	{
	  basilysgc_reserve (newlensiz + 10 * sizeof (void *));
	  if (!basilys_is_young (mappointerv))
	    goto allocate_old_newtab_mappointer;
	  newtab =
	    (struct entrypointerbasilys_st *)
	    basilys_allocatereserved (newlensiz, 0);
	}
      else
	{
	allocate_old_newtab_mappointer:
	  newtab = (struct entrypointerbasilys_st *)
	    ggc_alloc_cleared (newlen *
			       sizeof (struct entrypointerbasilys_st));
	};
      oldtab = map_mappointerv->entab;
      for (ix = 0; ix < len; ix++)
	{
	  const void *curat = oldtab[ix].e_at;
	  int newix;
	  if (!curat || curat == (void *) HTAB_DELETED_ENTRY)
	    continue;
	  newix = unsafe_index_mappointer (newtab, curat, newlen);
	  gcc_assert (newix >= 0);
	  newtab[newix] = oldtab[ix];
	  newcnt++;
	}
      if (!basilys_is_young (oldtab))
	/* free oldtab since it is in old ggc space */
	ggc_free (oldtab);
      map_mappointerv->entab = newtab;
      map_mappointerv->count = newcnt;
    }
  basilysgc_touch (map_mappointerv);
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) valuv;
#undef discrv
#undef mappointerv
#undef valuv
#undef object_discrv
#undef map_mappointerv
}


/***************** objvlisp test of strict subclassing */
bool
basilys_is_subclass_of (basilysobject_ptr_t subclass_p,
			basilysobject_ptr_t superclass_p)
{
  struct basilysmultiple_st *subanc = NULL;
  struct basilysmultiple_st *superanc = NULL;
  unsigned subdepth = 0, superdepth = 0;
  if (basilys_magic_discr ((basilys_ptr_t) subclass_p) !=
      OBMAG_OBJECT || subclass_p->object_magic != OBMAG_OBJECT
      || basilys_magic_discr ((basilys_ptr_t) superclass_p) !=
      OBMAG_OBJECT || superclass_p->object_magic != OBMAG_OBJECT)
    {
      return FALSE;
    }
  if (subclass_p->obj_len < FCLASS__LAST
      || !subclass_p->obj_vartab
      || superclass_p->obj_len < FCLASS__LAST || !superclass_p->obj_vartab)
    {
      return FALSE;
    }
  if (superclass_p == BASILYSGOB (CLASS_ROOT))
    return TRUE;
  subanc =
    (struct basilysmultiple_st *) subclass_p->obj_vartab[FCLASS_ANCESTORS];
  superanc =
    (struct basilysmultiple_st *) superclass_p->obj_vartab[FCLASS_ANCESTORS];
  if (basilys_magic_discr ((basilys_ptr_t) subanc) !=
      OBMAG_MULTIPLE || subanc->discr != BASILYSGOB (DISCR_SEQCLASS))
    {
      return FALSE;
    }
  if (basilys_magic_discr ((basilys_ptr_t) superanc) !=
      OBMAG_MULTIPLE || superanc->discr != BASILYSGOB (DISCR_SEQCLASS))
    {
      return FALSE;
    }
  subdepth = subanc->nbval;
  superdepth = superanc->nbval;
  if (subdepth <= superdepth)
    return FALSE;
  if ((basilys_ptr_t) subanc->tabval[superdepth] ==
      (basilys_ptr_t) superclass_p)
    return TRUE;
  return FALSE;
}


basilys_ptr_t
basilysgc_new_string_raw_len (basilysobject_ptr_t discr_p, const char *str, int slen)
{
  BASILYS_ENTERFRAME (2, NULL);
#define discrv     curfram__.varptr[0]
#define strv       curfram__.varptr[1]
#define obj_discrv  ((struct basilysobject_st*)(discrv))
#define str_strv  ((struct basilysstring_st*)(strv))
  strv = 0;
  if (!str)
    goto end;
  if (slen<0)
    slen = strlen (str);
  discrv = discr_p;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (obj_discrv->object_magic != OBMAG_STRING)
    goto end;
  strv = basilysgc_allocate (sizeof (struct basilysstring_st), slen + 1);
  str_strv->discr = obj_discrv;
  memcpy (str_strv->val, str, slen);
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) strv;
#undef discrv
#undef strv
#undef obj_discrv
#undef str_strv
}

basilys_ptr_t
basilysgc_new_string (basilysobject_ptr_t discr_p, const char *str)
{
  return basilysgc_new_string_raw_len(discr_p, str, -1);
}

basilys_ptr_t
basilysgc_new_stringdup (basilysobject_ptr_t discr_p, const char *str)
{
  int slen = 0;
  char tinybuf[80];
  char *strcop = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define discrv     curfram__.varptr[0]
#define strv       curfram__.varptr[1]
#define obj_discrv  ((struct basilysobject_st*)(discrv))
#define str_strv  ((struct basilysstring_st*)(strv))
  strv = 0;
  if (!str)
    goto end;
  discrv = discr_p;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (obj_discrv->object_magic != OBMAG_STRING)
    goto end;
  slen = strlen (str);
  if (slen < (int) sizeof (tinybuf) - 1)
    {
      memset (tinybuf, 0, sizeof (tinybuf));
      strcop = strcpy (tinybuf, str);
    }
  else
    strcop = strcpy ((char *) xcalloc (1, slen + 1), str);
  strv = basilysgc_allocate (sizeof (struct basilysstring_st), slen + 1);
  str_strv->discr = obj_discrv;
  strcpy (str_strv->val, strcop);
end:
  if (strcop && strcop != tinybuf)
    free (strcop);
  memset (tinybuf, 0, sizeof (tinybuf));
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) strv;
#undef discrv
#undef strv
#undef obj_discrv
#undef str_strv
}

basilys_ptr_t
basilysgc_new_string_nakedbasename (basilysobject_ptr_t
				    discr_p, const char *str)
{
  int slen = 0;
  char tinybuf[120];
  char *strcop = 0;
  const char *basestr = 0;
  char *dot = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define discrv     curfram__.varptr[0]
#define strv       curfram__.varptr[1]
#define obj_discrv  ((struct basilysobject_st*)(discrv))
#define str_strv  ((struct basilysstring_st*)(strv))
  strv = 0;
  if (!str)
    goto end;
  discrv = discr_p;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (obj_discrv->object_magic != OBMAG_STRING)
    goto end;
  slen = strlen (str);
  if (slen < (int) sizeof (tinybuf) - 1)
    {
      memset (tinybuf, 0, sizeof (tinybuf));
      strcop = strcpy (tinybuf, str);
    }
  else
    strcop = strcpy ((char *) xcalloc (1, slen + 1), str);
  basestr = (const char *) lbasename (strcop);
  dot = strchr (basestr, '.');
  if (dot)
    *dot = 0;
  strv =
    basilysgc_allocate (sizeof (struct basilysstring_st),
			strlen (basestr) + 1);
  str_strv->discr = obj_discrv;
  strcpy (str_strv->val, basestr);
end:
  if (strcop && strcop != tinybuf)
    free (strcop);
  memset (tinybuf, 0, sizeof (tinybuf));
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) strv;
#undef discrv
#undef strv
#undef obj_discrv
#undef str_strv
}


basilys_ptr_t
basilysgc_new_string_tempname_suffixed (basilysobject_ptr_t
					discr_p, const char *namstr, const char *suffstr)
{
  int slen = 0;
  char suffix[16];
  const char *basestr = xstrdup(lbasename(namstr));
  const char* tempnampath = 0;
  char *dot = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define discrv     curfram__.varptr[0]
#define strv       curfram__.varptr[1]
#define obj_discrv  ((struct basilysobject_st*)(discrv))
#define str_strv  ((struct basilysstring_st*)(strv))
  memset(suffix, 0, sizeof(suffix));
  if (suffstr) strncpy(suffix, suffstr, sizeof(suffix)-1);
  if (basestr)
    dot = strrchr(basestr, '.');
  if (dot)
    *dot=0;
  tempnampath = basilys_tempdir_path(basestr, suffix);
  dbgprintf ("new_string_tempbasename basestr='%s' tempnampath='%s'", basestr, tempnampath);
  free(CONST_CAST(char*,basestr));
  basestr = 0;
  strv = 0;
  if (!tempnampath)
    goto end;
  discrv = discr_p;
  if (basilys_magic_discr ((basilys_ptr_t) discrv) != OBMAG_OBJECT)
    goto end;
  if (obj_discrv->object_magic != OBMAG_STRING)
    goto end;
  slen = strlen (tempnampath);
  strv =
    basilysgc_allocate (sizeof (struct basilysstring_st),
			slen + 1);
  str_strv->discr = obj_discrv;
  strcpy (str_strv->val, tempnampath);
end:
  if (tempnampath)
    free (CONST_CAST (char*,tempnampath));
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) strv;
#undef discrv
#undef strv
#undef obj_discrv
#undef str_strv
}




#if ENABLE_CHECKING
static long applcount_basilys;
static int appldepth_basilys;
#define MAXDEPTH_APPLY_BASILYS 2000
#endif
/*************** closure application ********************/
basilys_ptr_t
basilys_apply (basilysclosure_ptr_t clos_p,
	       basilys_ptr_t arg1_p,
	       const char *xargdescr_,
	       union basilysparam_un *xargtab_,
	       const char *xresdescr_, union basilysparam_un *xrestab_)
{
  basilys_ptr_t res = NULL;
  basilysroutfun_t*routfun = 0;
#if ENABLE_CHECKING
  applcount_basilys++;
  appldepth_basilys++;
  if (appldepth_basilys > MAXDEPTH_APPLY_BASILYS)
    {
      basilys_dbgshortbacktrace ("too deep applications", 200);
      fatal_error ("too deep (%d) basilys applications", appldepth_basilys);
    }
#endif
  if (basilys_magic_discr ((basilys_ptr_t) clos_p) != OBMAG_CLOSURE)
    goto end;
  if (basilys_magic_discr ((basilys_ptr_t) (clos_p->rout)) !=
      OBMAG_ROUTINE || !(routfun = clos_p->rout->routfunad))
    goto end;
  res = (*routfun) (clos_p, arg1_p, xargdescr_, xargtab_, xresdescr_, xrestab_);
 end:
#if ENABLE_CHECKING
  appldepth_basilys--;
#endif
  return res;
}



/************** method sending ***************/
basilys_ptr_t
basilysgc_send (basilys_ptr_t recv_p,
		basilys_ptr_t sel_p,
		const char *xargdescr_,
		union basilysparam_un * xargtab_,
		const char *xresdescr_, union basilysparam_un * xrestab_)
{
  /* NAUGHTY TRICK here: message sending is very common, and we want
     to avoid having the current frame (the frame declared by the
     B*ENTERFRAME macro call below) to be active when the application
     for the sending is performed. This should make our call frames'
     linked list shorter. To do so, we put the closure to apply and
     the reciever in the two variables below. Yes this is dirty, but
     it works! 

     We should be very careful when modifying this routine */
  /* never assign to these if a GC could happen */
  basilysclosure_ptr_t closure_dirtyptr = NULL;
  basilys_ptr_t recv_dirtyptr = NULL;

#ifdef ENABLE_CHECKING
  static long sendcount;
  long sendnum = ++sendcount;
#endif
  BASILYS_ENTERFRAME (9, NULL);
#define recv    curfram__.varptr[0]
#define selv    curfram__.varptr[1]
#define argv    curfram__.varptr[2]
#define closv   curfram__.varptr[3]
#define discrv  curfram__.varptr[4]
#define mapv    curfram__.varptr[5]
#define superv  curfram__.varptr[6]
#define resv    curfram__.varptr[7]
#define ancv    curfram__.varptr[8]
#define obj_discrv ((basilysobject_ptr_t)(discrv))
#define obj_selv ((basilysobject_ptr_t)(selv))
#define clo_closv ((basilysclosure_ptr_t)(closv))
#define mul_ancv  ((struct basilysmultiple_st*)(ancv))
  recv = recv_p;
  selv = sel_p;
  /* the reciever can be null, using DISCR_NULLRECV */
#ifdef ENABLE_CHECKING
  (void) sendnum;		/* to use it */
#endif
  if (basilys_magic_discr ((basilys_ptr_t) selv) != OBMAG_OBJECT)
    goto end;
  if (!basilys_is_instance_of
      ((basilys_ptr_t) selv, (basilys_ptr_t) BASILYSGOB (CLASS_SELECTOR)))
    goto end;
#if 0 && ENABLE_CHECKING
  debugeprintf ("send #%ld recv %p", sendnum, (void *) recv);
  debugeprintf ("send #%ld selv %p <%s>", sendnum,
		(void *) obj_selv,
		basilys_string_str (obj_selv->obj_vartab[FNAMED_NAME]));
#endif
  if (recv != NULL)
    {
      discrv = ((basilys_ptr_t) recv)->u_discr;
      gcc_assert (discrv != NULL);
    }
  else
    {
      discrv = (BASILYSGOB (DISCR_NULLRECV));
      gcc_assert (discrv != NULL);
    };
  while (discrv)
    {
      gcc_assert (basilys_magic_discr ((basilys_ptr_t) discrv) ==
		  OBMAG_OBJECT);
      gcc_assert (obj_discrv->obj_len >= FDISCR__LAST);
#if 0 && ENABLE_CHECKING
      debugeprintf ("send #%ld discrv %p <%s>",
		    sendnum, discrv,
		    basilys_string_str (obj_discrv->obj_vartab[FNAMED_NAME]));
#endif
      mapv = obj_discrv->obj_vartab[FDISCR_METHODICT];
      if (basilys_magic_discr ((basilys_ptr_t) mapv) == OBMAG_MAPOBJECTS)
	{
	  closv =
	    (basilys_ptr_t) basilys_get_mapobjects ((basilysmapobjects_ptr_t)
						    mapv,
						    (basilysobject_ptr_t)
						    selv);
	}
      else
	{
	  closv = obj_discrv->obj_vartab[FDISCR_SENDER];
	  if (basilys_magic_discr ((basilys_ptr_t) closv) == OBMAG_CLOSURE)
	    {
	      union basilysparam_un pararg[1];
	      pararg[0].bp_aptr = (basilys_ptr_t *) & selv;
	      resv =
		basilys_apply ((basilysclosure_ptr_t) closv,
			       (basilys_ptr_t) recv, BPARSTR_PTR, pararg, "",
			       NULL);
	      closv = resv;
	    }
	}
      if (basilys_magic_discr ((basilys_ptr_t) closv) == OBMAG_CLOSURE)
	{
	  /* NAUGHTY TRICK: assign to dirty (see comments near start of function) */
	  closure_dirtyptr = (basilysclosure_ptr_t) closv;
	  recv_dirtyptr = (basilys_ptr_t) recv;
	  /*** OLD CODE: 
	  resv =
	    basilys_apply (closv, recv, xargdescr_, xargtab_,
			     xresdescr_, xrestab_);
	  ***/
	  goto end;
	}
      discrv = obj_discrv->obj_vartab[FDISCR_SUPER];
    }				/* end while discrv */
  resv = NULL;
end:
#if 0 && ENABLE_CHECKING
  debugeprintf ("endsend #%ld recv %p resv %p selv %p <%s>",
		sendnum, recv, resv, (void *) obj_selv,
		basilys_string_str (obj_selv->obj_vartab[FNAMED_NAME]));
#endif
  BASILYS_EXITFRAME ();
  /* NAUGHTY TRICK  (see comments near start of function) */
  if (closure_dirtyptr)
    return basilys_apply (closure_dirtyptr, recv_dirtyptr, xargdescr_,
			  xargtab_, xresdescr_, xrestab_);
  return (basilys_ptr_t) resv;
#undef recv
#undef selv
#undef closv
#undef discrv
#undef argv
#undef mapv
#undef superv
#undef resv
#undef ancv
#undef obj_discrv
#undef obj_selv
#undef clo_closv
}

/* our temporary directory */
/* maybe it should not be static, or have a bigger length */
static char tempdir_basilys[1024];
static bool made_tempdir_basilys;
/* returns malloc-ed path inside a temporary directory, with a given basename  */
char *
basilys_tempdir_path (const char *srcnam, const char* suffix)
{
  int loopcnt = 0;
  const char *basnam = 0;
  extern char *choose_tmpdir (void);	/* from libiberty/choose-temp.c */
  basnam = lbasename (CONST_CAST (char*,srcnam));
  debugeprintf ("basilys_tempdir_path srcnam %s basnam %s suffix %s", srcnam, basnam, suffix);
  gcc_assert (basnam && (ISALNUM (basnam[0]) || basnam[0] == '_'));
  if (basilys_tempdir_string && basilys_tempdir_string[0])
    {
      if (access (basilys_tempdir_string, F_OK))
	{
	  if (mkdir (basilys_tempdir_string, 0700))
	    fatal_error ("failed to mkdir basilys_tempdir %s - %m",
			 basilys_tempdir_string);
	  made_tempdir_basilys = true;
	}
      return concat (basilys_tempdir_string, "/", basnam, suffix, NULL);
    }
  if (!tempdir_basilys[0])
    {
      /* usually this loop runs only once */
      for (loopcnt = 0; loopcnt < 1000; loopcnt++)
	{
	  int n = basilys_lrand () & 0xfffffff;
	  memset(tempdir_basilys, 0, sizeof(tempdir_basilys));
	  snprintf (tempdir_basilys, sizeof(tempdir_basilys)-1, 
		     "%s/GccMeltmpd%d-%d", choose_tmpdir (), (int) getpid (),
		    n);
	  if (!mkdir (tempdir_basilys, 0700))
	    {
	      made_tempdir_basilys = true;
	      break;
	    }
	  tempdir_basilys[0] = '\0';
	}
      if (!tempdir_basilys[0])
	fatal_error ("failed to create temporary directory for MELT in %s",
		     choose_tmpdir ());
    };
  return concat (tempdir_basilys, "/", basnam, suffix, NULL);
}





/* the srcfile is a generated .c file or otherwise a dynamic library,
   the dlfile has no suffix, because the suffix is expected to be
   added by the basilys-gcc script */
static void
compile_to_dyl (const char *srcfile, const char *dlfile)
{
  struct pex_obj *pex = 0;
  int err = 0;
  int cstatus = 0;
  const char *errmsg = 0;
  /* possible improvement  : avoid recompiling
     when not necessary; using timestamps a la make is not enough,
     since the C source files are generated.  

     The melt-cc-script takes two arguments: the C source file path to
     compile as a basilys plugin, and the naked dynamic library file to be
     generated. Standard path are in Makefile.in $(melt_compile_script)

     The melt-cc-script should be generated in the building process.
     In addition of compiling the C source file, it should put into the
     generated dynamic library the following two constant strings;
     const char basilys_compiled_timestamp[];
     const char basilys_md5[];

     the basilys_compiled_timestamp should contain a human readable
     timestamp the basilys_md5 should contain the hexadecimal md5 digest,
     followed by the source file name (i.e. the single line output by the
     command: md5sum $Csourcefile; where $Csourcefile is replaced by the
     source file path)

   */
  const char *ourmeltcompilescript = NULL;
  struct pex_time ptime;
  const char *argv[4];
  char pwdbuf[500];
  memset(pwdbuf, 0, sizeof(pwdbuf));
  getcwd(pwdbuf, sizeof(pwdbuf)-1);
  memset (&ptime, 0, sizeof (ptime));
  /* compute the ourmeltcompilscript */
  debugeprintf ("compile_to_dyl basilys_compile_script_string %s", basilys_compile_script_string);
  ourmeltcompilescript = CONST_CAST (char *, basilys_compile_script_string);
  if (!ourmeltcompilescript || !ourmeltcompilescript[0])
    ourmeltcompilescript = melt_compile_script;
  debugeprintf ("compile_to_dyl melt ourmeltcompilescript=%s pwd %s",
		ourmeltcompilescript, pwdbuf);
  debugeprintf ("compile_to_dyl srcfile %s dlfile %s", srcfile, dlfile);
  fflush (stdout);
  fflush (stderr);
  pex = pex_init (PEX_RECORD_TIMES, ourmeltcompilescript, NULL);
  argv[0] = ourmeltcompilescript;
  argv[1] = srcfile;
  argv[2] = dlfile;
  argv[3] = NULL;
  errmsg =
    pex_run (pex, PEX_LAST | PEX_SEARCH, ourmeltcompilescript, 
	     CONST_CAST (char**, argv),
	     NULL, NULL, &err);
  if (errmsg)
    fatal_error
      ("failed to basilys compile to dyl: %s %s %s : %s",
       ourmeltcompilescript, srcfile, dlfile, errmsg);
  if (!pex_get_status (pex, 1, &cstatus))
    fatal_error
      ("failed to get status of basilys dynamic compilation to dyl:  %s %s %s - %m",
       ourmeltcompilescript, srcfile, dlfile);
  if (!pex_get_times (pex, 1, &ptime))
    fatal_error
      ("failed to get time of basilys dynamic compilation to dyl:  %s %s %s - %m",
       ourmeltcompilescript, srcfile, dlfile);
  pex_free (pex);
  debugeprintf ("compile_to_dyl done srcfile %s dlfile %s", srcfile, dlfile);
}



/* following code and comment is taken from the gcc/plugin.c file of
   the plugins branch */

/* We need a union to cast dlsym return value to a function pointer
   as ISO C forbids assignment between function pointer and 'void *'.
   Use explicit union instead of __extension__(<union_cast>) for
   portability.  */
#define PTR_UNION_TYPE(TOTYPE) union { void *_q; TOTYPE _nq; }
#define PTR_UNION_AS_VOID_PTR(NAME) (NAME._q)
#define PTR_UNION_AS_CAST_PTR(NAME) (NAME._nq)




/* load a dynamic library using the filepath DYPATH; if MD5SRC is
   given, check that the basilys_md5 inside is indeed MD5SRC, fill the
   info of the modulinfo stack and return the positive index,
   otherwise return 0 */
static int
load_checked_dynamic_module_index (const char *dypath, char *md5src)
{
  int ix = 0;
  char *dynmd5 = NULL;
  char *dynchecksum = NULL;
  void *dlh = NULL;
  char *dyncomptimstamp = NULL;
  typedef basilys_ptr_t startroutine_t (basilys_ptr_t);
  typedef void markroutine_t (void *);
  PTR_UNION_TYPE(startroutine_t*) startrout_uf = {0};
  PTR_UNION_TYPE(markroutine_t*) markrout_uf = {0};
  PTR_UNION_TYPE(void **) iniframe_up = {0};
  int i = 0, c = 0;
  char hbuf[4];
  debugeprintf ("load_check_dynamic_module_index dypath=%s md5src=%s", dypath, md5src);
  dlh = (void *) lt_dlopenext (dypath);
  debugeprintf ("load_check_dynamic_module_index dlh=%p dypath=%s", dlh, dypath);
  if (!dlh)
    return 0;
  /* we always check that a basilys_md5 exists within the dynamically
     loaded stuff; otherwise it was not generated from MELT/basilys */
  dynmd5 = (char *) lt_dlsym ((lt_dlhandle) dlh, "melt_md5");
  if (!dynmd5)
    dynmd5 = (char *) lt_dlsym ((lt_dlhandle) dlh, "basilys_md5");
  debugeprintf ("dynmd5=%s", dynmd5);
  if (!dynmd5) 
    {
      warning (0, "missing md5 signature in MELT module %s", dypath);
      goto bad;
  }
  dyncomptimstamp =
    (char *) lt_dlsym ((lt_dlhandle) dlh, "melt_compiled_timestamp");
  if (!dyncomptimstamp)
    dyncomptimstamp =
      (char *) lt_dlsym ((lt_dlhandle) dlh, "basilys_compiled_timestamp");
  debugeprintf ("dyncomptimstamp=%s", dyncomptimstamp);
  if (!dyncomptimstamp) 
    {
      warning (0, "missing timestamp in MELT module %s", dypath);
      goto bad;
    };
  /* check the checksum of the generating compiler with current */
  dynchecksum  =
    (char *) lt_dlsym ((lt_dlhandle) dlh, "genchecksum_melt");
  if (dynchecksum && memcmp(dynchecksum, executable_checksum, 16))
    warning(0, "loaded MELT plugin %s with a checksum mismatch!", dypath);
  PTR_UNION_AS_VOID_PTR(startrout_uf) =
    lt_dlsym ((lt_dlhandle) dlh, "start_module_melt");
  if (!PTR_UNION_AS_VOID_PTR(startrout_uf))
    PTR_UNION_AS_VOID_PTR(startrout_uf) 
      = lt_dlsym ((lt_dlhandle) dlh, "start_module_basilys");
  if (!PTR_UNION_AS_VOID_PTR(startrout_uf)) 
    {
      warning (0, "missing start_module routine in MELT module %s", dypath);
      goto bad;
    };
  PTR_UNION_AS_VOID_PTR(markrout_uf) =
    lt_dlsym ((lt_dlhandle) dlh, "mark_module_melt");
  if (!PTR_UNION_AS_VOID_PTR(markrout_uf))
   PTR_UNION_AS_VOID_PTR(markrout_uf) =
     lt_dlsym ((lt_dlhandle) dlh, "mark_module_basilys");
  if (!PTR_UNION_AS_VOID_PTR(markrout_uf))
    {
      warning (0, "missing mark_module routine in MELT module %s", dypath);
      goto bad;
    };
  PTR_UNION_AS_VOID_PTR(iniframe_up) 
    = lt_dlsym ((lt_dlhandle) dlh, "initial_frame_melt");
  if (!PTR_UNION_AS_VOID_PTR(iniframe_up) )
    PTR_UNION_AS_VOID_PTR(iniframe_up) =
      lt_dlsym ((lt_dlhandle) dlh, "initial_frame_basilys");
  if (!PTR_UNION_AS_VOID_PTR(iniframe_up))
    {
      warning (0, "missing initial_frame routine in MELT module %s", dypath);
      goto bad;
    }
  if (md5src && dynmd5)
    {
      for (i = 0; i < 16; i++)
	{
	  if (ISXDIGIT (dynmd5[2 * i]) && ISXDIGIT (dynmd5[2 * i + 1]))
	    {
	      hbuf[0] = dynmd5[2 * i];
	      hbuf[1] = dynmd5[2 * i + 1];
	      hbuf[2] = (char) 0;
	      c = (int) strtol (hbuf, (char **) 0, 16);
	      if (c != (int) (md5src[i] & 0xff))
		{
		  warning (0, "md5 source mismatch in MELT module %s", dypath);
		  goto bad;
		}
	    }
	  else
	    {
	      warning (0, "md5 source invalid in MELT module %s", dypath);
	      goto bad;
	    }
	}
    }
  {
    basilys_module_info_t minf = { 0, 0, 0, 0 };
    minf.dlh = dlh;
    minf.iniframp = PTR_UNION_AS_CAST_PTR (iniframe_up);
    minf.marker_rout = PTR_UNION_AS_CAST_PTR (markrout_uf);
    minf.start_rout = PTR_UNION_AS_CAST_PTR (startrout_uf);
    ix = VEC_length (basilys_module_info_t, modinfvec);
    VEC_safe_push (basilys_module_info_t, heap, modinfvec, &minf);
  }
  debugeprintf
    ("load_checked_dynamic_module_index %s dynmd5 %s dyncomptimstamp %s ix %d",
     dypath, dynmd5, dyncomptimstamp, ix);
  return ix;
bad:
  debugeprintf ("load_checked_dynamic_module_index failed dlerror:%s",
		lt_dlerror ());
  lt_dlclose ((lt_dlhandle) dlh);
  return 0;
}

void *
basilys_dlsym_all (const char *nam)
{
  int ix = 0;
  basilys_module_info_t *mi = 0;
  for (ix = 0; VEC_iterate (basilys_module_info_t, modinfvec, ix, mi); ix++)
    {
      void *p = (void *) lt_dlsym ((lt_dlhandle) mi->dlh, nam);
      if (p)
	return p;
    };
  return (void *) lt_dlsym (proghandle, nam);
}


/* compile (as a dynamically loadable module) some (usually generated)
   C code (or a dynamically loaded stuff) and dynamically load it; the
   C code should contain a function named start_module_basilys; that
   function is called with the given modata and returns the module;
   the modulnam should contain only letter, digits or one of +-_ */
basilys_ptr_t
basilysgc_load_melt_module (basilys_ptr_t modata_p, const char *modulnam)
{
  char *srcpath = NULL;
  char *dynpath = NULL;
  FILE *srcfi = NULL;
  int dlix = 0;
  int specialsuffix = 0; /* set for .d or .n suffix */
  int srcpathlen = 0;
  char md5srctab[16];
  char *md5src = NULL;
  char *tmpath = NULL;
  char *dupmodulnam = NULL;
  basilys_module_info_t *moduptr = 0;
  basilys_ptr_t (*startroutp) (basilys_ptr_t);	/* start routine */
  int modulnamlen = 0;
  BASILYS_ENTERFRAME (3, NULL);
#define modulv curfram__.varptr[0]
#define mdatav curfram__.varptr[1]
  mdatav = modata_p;
  if (!modulnam || !modulnam[0]) {
    error ("cannot load MELT module, no MELT module name given");
    goto end;
  }
  if (!ISALNUM(modulnam[0]) && modulnam[0] != '_')
    error ("bad MELT module name %s to load", modulnam);
  /* always check the module name */
  {
    const char* p = 0;
    for (p=modulnam; *p; p++)
      {
	/* special hack for names like warmelt-first-0.d or
	   warmelt-foo-1.n */
	if (p[0] == '.' && ISALNUM(p[1]) && !p[2])
	  {
	    specialsuffix = 1;
	    break;
	  };
	if (!ISALNUM(*p) && *p != '+' && *p != '-' && *p != '_')
	  {
	    error ("invalid MELT module name %s to load", modulnam);
	    goto end;
	  }
      }
  }
  /* duplicate the module name for safety, i.e. because it was in MELT
     heap or whatever ... */
  dupmodulnam = xstrdup(modulnam);
  if (specialsuffix) {
    modulnamlen = strlen(modulnam);
    gcc_assert (modulnamlen>2);
    gcc_assert (dupmodulnam[modulnamlen-2] == '.');
    dupmodulnam[modulnamlen-2] = '\0';
  }
  /***** first find the source path if possible ******/
  /* look first in the temporary directory */
  tmpath = basilys_tempdir_path (dupmodulnam, ".c");
  debugeprintf ("basilysgc_load_melt_module trying in tempdir %s", tmpath);
  if (tmpath && !access (tmpath, R_OK))
    {
      debugeprintf ("basilysgc_load_melt_module found source in tempdir %s", tmpath);
      srcpath = tmpath;
      goto foundsrcpath;
    }
  free(tmpath);
  tmpath = NULL;
  /* look in the generated source dir */
  if (basilys_gensrcdir_string && basilys_gensrcdir_string[0])
    tmpath = concat (basilys_gensrcdir_string, "/", dupmodulnam, ".c", NULL);
  debugeprintf ("basilysgc_load_melt_module trying in gensrcdir %s", tmpath);
  if (tmpath && !access (tmpath, R_OK))
    {
      debugeprintf ("basilysgc_load_melt_module found source in gensrcdir %s", tmpath);
      srcpath = tmpath;
      goto foundsrcpath;
    }
  free (tmpath);
  tmpath = NULL;
  /* look into the melt generated dir */
  tmpath = concat (melt_generated_dir, "/", dupmodulnam, ".c", NULL);
  debugeprintf ("basilysgc_load_melt_module trying in meltgenerateddir %s", tmpath);
  if (tmpath && !access (tmpath, R_OK))
    {
      debugeprintf ("basilysgc_load_melt_module found source in meltgendir %s", tmpath);
      srcpath = tmpath;
      goto foundsrcpath;
    }
  free (tmpath);
  tmpath = NULL;
  /* look into the melt source dir */
  tmpath = concat (melt_source_dir, "/", dupmodulnam, ".c", NULL);
  debugeprintf ("basilysgc_load_melt_module trying in meltsrcdir %s", tmpath);
  if (tmpath && !access (tmpath, R_OK))
    {
      debugeprintf ("basilysgc_load_melt_module found source in meltsrcdir %s", tmpath);
      srcpath = tmpath;
      goto foundsrcpath;
    }
  free (tmpath);
  tmpath = NULL;
  /* we didn't found the source */
  debugeprintf ("basilysgc_load_melt_module cannot find source for mudule %s", dupmodulnam);
  warning (0, "Didn't find MELT module %s 's C source code; perhaps need -fmelt-gensrcdir=...", dupmodulnam);
  inform (UNKNOWN_LOCATION, "MELT temporary source path tried %s for C source code", 
	  basilys_tempdir_path (dupmodulnam, ".c"));
  {
    /* explain only once the directories we searched the C source code in */
    static int nbexplain;
    if (nbexplain <= 0) 
      {
	if (basilys_gensrcdir_string && basilys_gensrcdir_string[0])
	  inform (UNKNOWN_LOCATION, "MELT generated source given directory is %s",
		  basilys_gensrcdir_string);
	inform (UNKNOWN_LOCATION, "MELT builtin generated source directory is %s", 
		melt_generated_dir);
	inform (UNKNOWN_LOCATION, "MELT builtin source directory is %s", 
		melt_source_dir);
      };
    nbexplain++;
  }
  if (srcpath) 
    {
    foundsrcpath:  /* we found the source file */
      srcpathlen = strlen (srcpath);
      debugeprintf ("basilysgc_load_melt_module found srcpathlen %d srcpath %s", srcpathlen, srcpath);
      /* compute the md5 hash of the source code */
      srcfi = fopen (srcpath, "r");
      if (!srcfi)
	/* this really should not happen, we checked with access before
	   that the source file existed! */
	fatal_error ("cannot open generated source file %s for MELT : %m",
		     srcpath);
      memset (md5srctab, 0, sizeof (md5srctab));
      if (md5_stream (srcfi, &md5srctab))
	fatal_error
	  ("failed to compute md5sum of generated source file %s for MELT",
	   srcpath);
      md5src = md5srctab;
      fclose (srcfi);
      srcfi = NULL;
    }
  /* if it had a special suffix, restore it */
  if (specialsuffix)
    {
      gcc_assert (modulnamlen>2);
      dupmodulnam[modulnamlen-2] = '.';
      debugeprintf ("basilysgc_load_melt_module restored dupmodulnam %s", 
		    dupmodulnam);
    }
  /**
     we have to scan several dynlib directories to find the module;
     when we find a module, we dynamically load it to check that it
     has the right md5 sum (i.e. that its basilys_md5 is correct); if
     no dynlib is found, we have to compile the generated C source.
  **/
  /* if a dynlib directory is given, check it */
  if (basilys_dynlibdir_string && basilys_dynlibdir_string[0])
    {
      tmpath = concat (basilys_dynlibdir_string, "/", dupmodulnam, NULL);
      BASILYS_LOCATION_HERE
	("basilysgc_load_melt_module before load_checked_dylib pathed");
      dlix = load_checked_dynamic_module_index (tmpath, md5src);
      debugeprintf ("basilysgc_load_melt_module dlix=%d dynlib tmpath=%s", dlix,
		    tmpath);
      if (dlix > 0)
	{
	  dynpath = tmpath;
	  goto dylibfound;
	};
      free (tmpath);
      tmpath = NULL;
    }
  /* check in the builtin melt dynamic lib directory */
  tmpath = concat (melt_dynlib_dir, "/", dupmodulnam, NULL);
  BASILYS_LOCATION_HERE
    ("basilysgc_load_melt_module before load_checked_dylib builtin");
  dlix = load_checked_dynamic_module_index (tmpath, md5src);
  debugeprintf ("basilysgc_load_melt_module dlix=%d meltdynlib tmpath=%s", dlix,
		tmpath);
  if (dlix > 0)
    {
      dynpath = tmpath;
      goto dylibfound;
    };
  free (tmpath);
  tmpath = NULL;
  /* check in the temporary directory */
  tmpath = basilys_tempdir_path (dupmodulnam, NULL);
  debugeprintf ("basilysgc_load_melt_module trying %s", tmpath);
  BASILYS_LOCATION_HERE
    ("basilysgc_load_melt_module before load_checked_dylib tmpath");
  dlix = tmpath ? load_checked_dynamic_module_index (tmpath, md5src) : 0;
  debugeprintf ("basilysgc_load_melt_module dlix=%d tempdir tmpath=%s", dlix,
		tmpath);
  if (dlix > 0)
    {
      dynpath = tmpath;
      goto dylibfound;
    };
  free (tmpath);
  tmpath = NULL;
  debugeprintf ("basilysgc_load_melt_module md5src %p", (void*)md5src);
  /* if we really have the source, we can afford to check in the current directory */
  if (md5src)
    {
      tmpath = concat ("./", dupmodulnam, NULL);
      debugeprintf ("basilysgc_load_melt_module tmpath %s", tmpath);
      BASILYS_LOCATION_HERE
	("basilysgc_load_melt_module before load_checked_dylib src");
      dlix = load_checked_dynamic_module_index (tmpath, md5src);
      debugeprintf ("basilysgc_load_melt_module dlix=%d curdir tmpath=%s", dlix,
		    tmpath);
      if (dlix > 0)
	{
	  dynpath = tmpath;
	  goto dylibfound;
	};
      free (tmpath);
    }
  debugeprintf ("basilysgc_load_melt_module srcpath %s dynpath %s", srcpath, dynpath);
  /* if we have the srcpath but did'nt found the stuff, try to compile it using the temporary directory */
  if (srcpath)
    {
      tmpath = basilys_tempdir_path (dupmodulnam, NULL);
      debugeprintf ("basilysgc_load_melt_module before compiling tmpath %s", tmpath);
      compile_to_dyl (srcpath, tmpath);
      debugeprintf ("basilysgc_compile srcpath=%s compiled to tmpath=%s",
		    srcpath, tmpath);
      BASILYS_LOCATION_HERE
	("basilysgc_load_melt_module before load_checked_dylib compiled tmpath");
      dlix = load_checked_dynamic_module_index (tmpath, md5src);
      debugeprintf ("basilysgc_load_melt_module dlix=%d tempdirpath tmpath=%s",
		    dlix, tmpath);
      if (dlix > 0)
	{
	  dynpath = tmpath;
	  goto dylibfound;
	};
    }
  debugeprintf ("failed here dlix=%d", dlix);
  /* catch all situation, failed to find the dynamic stuff */
  /* give info to user */
  error("failed to find dynamic stuff for MELT module %s (%s)",
	dupmodulnam, lt_dlerror ());
  inform (UNKNOWN_LOCATION, 
	  "not found dynamic stuff using tempdir %s", 
	  basilys_tempdir_path (dupmodulnam, NULL));
  if (srcpath)
    inform (UNKNOWN_LOCATION, 
	    "not found dynamic stuff using srcpath %s", 
	    srcpath);
  fatal_error ("unable to continue since failed to load MELT module %s", 
	       dupmodulnam);
 dylibfound:
  debugeprintf ("dylibfound dlix=%d", dlix);
  gcc_assert (dlix > 0
	      && dlix < (int) VEC_length (basilys_module_info_t, modinfvec));
  moduptr = VEC_index (basilys_module_info_t, modinfvec, dlix);
  debugeprintf ("dylibfound moduptr=%p", (void *) moduptr);
  gcc_assert (moduptr != 0);
  startroutp = moduptr->start_rout;
  gcc_assert (moduptr->iniframp != 0 && *moduptr->iniframp == (void *) 0);
#if ENABLE_CHECKING
  {
    static char locbuf[80];
    memset (locbuf, 0, sizeof (locbuf));
    snprintf (locbuf, sizeof (locbuf) - 1,
	      "%s:%d:basilysgc_load_melt_module before calling module %s",
	      basename (__FILE__), __LINE__, dupmodulnam);
    curfram__.flocs = locbuf;
  }
#endif
  modulv = (*startroutp) ((basilys_ptr_t) mdatav);
  gcc_assert (moduptr->iniframp != 0 && *moduptr->iniframp == (void *) 0);
  BASILYS_LOCATION_HERE ("basilysgc_load_melt_module after calling module");
 end:
  debugeprintf ("basilysgc_load_melt_module returns modulv %p", (void *) modulv);
  /* we never free dynpath -since it is stored in moduptr- and we
     never release the shared library with a dlclose or something! */
  BASILYS_EXITFRAME ();
  free(dupmodulnam);
  return (basilys_ptr_t) modulv;
#undef mdatav
#undef modulv
}


/* generate a loadable module from a MELT generated C source file; the
   out is the dynloaded module without any *.so suffix */
void
basilysgc_generate_melt_module (basilys_ptr_t src_p, basilys_ptr_t out_p)
{
  char*srcdup = NULL;
  char* outdup = NULL;
  char* outso = NULL;
  BASILYS_ENTERFRAME (2, NULL);
#define srcv   curfram__.varptr[0]
#define outv      curfram__.varptr[1]
  srcv = src_p;
  outv = out_p;
  if (basilys_magic_discr((basilys_ptr_t) srcv) != OBMAG_STRING 
      || basilys_magic_discr((basilys_ptr_t) outv) != OBMAG_STRING)
    goto end;
  srcdup = xstrdup(basilys_string_str((basilys_ptr_t) srcv));
  outdup = xstrdup(basilys_string_str((basilys_ptr_t) outv));
  outso = concat(outdup, ".so", NULL);
  (void) remove (outso);
  debugeprintf("basilysgc_generate_melt_module start srcdup %s outdup %s",
	       srcdup, outdup);
  if (access(srcdup, R_OK)) 
    {
      error("no MELT generated source file %s", srcdup);
      goto end;
    }
  compile_to_dyl (srcdup, outdup);
  debugeprintf ("basilysgc_generate_module did srcdup %s outdup %s",
	       srcdup, outdup);
  if (!access(outso, R_OK))
    inform (UNKNOWN_LOCATION, "MELT generated module %s", outso);
 end:
  free (srcdup);
  free (outdup);
  free (outso);
  BASILYS_EXITFRAME ();
#undef srcv
#undef outv
}


#define MODLIS_SUFFIX ".modlis"

basilys_ptr_t
basilysgc_load_modulelist (basilys_ptr_t modata_p, const char *modlistbase)
{
  char *modlistpath = 0;
  FILE *filmod = 0;
  /* @@@ ugly, we should have a getline function */
  char linbuf[1024];
  BASILYS_ENTERFRAME (1, NULL);
  memset (linbuf, 0, sizeof (linbuf));
#define mdatav curfram__.varptr[0]
  mdatav = modata_p;
  debugeprintf ("basilysgc_load_modulelist start modlistbase %s", modlistbase);
  /* first check directly for the file */
  modlistpath = concat (modlistbase, MODLIS_SUFFIX, NULL);
  if (IS_ABSOLUTE_PATH (modlistpath) || !access (modlistpath, R_OK))
    goto loadit;
  free (modlistpath);
  modlistpath = 0;
  /* check for module list in melt_source_dir */
  modlistpath = concat (melt_source_dir,
			"/", modlistbase, MODLIS_SUFFIX, NULL);
  if (!access (modlistpath, R_OK))
    goto loadit;
  free (modlistpath);
  modlistpath = 0;
  /* check for module list in dynamic library dir */
  if (basilys_dynlibdir_string && basilys_dynlibdir_string[0])
    {
      modlistpath = concat (basilys_dynlibdir_string,
			    "/", modlistbase, MODLIS_SUFFIX, NULL);
      if (!access (modlistpath, R_OK))
	goto loadit;
    }
  free (modlistpath);
  modlistpath = 0;
  /* check for module list in melt_dynlib_dir */
  modlistpath = concat (melt_dynlib_dir,
			"/", modlistbase, MODLIS_SUFFIX, NULL);
  if (!access (modlistpath, R_OK))
    goto loadit;
  free (modlistpath);
  modlistpath = 0;
  /* check for module list in gensrcdir */
  if (basilys_gensrcdir_string && basilys_gensrcdir_string[0])
    {
      /* check for modfile in the gensrcdir */
      modlistpath =
	concat (basilys_gensrcdir_string, "/", modlistbase, MODLIS_SUFFIX,
		NULL);
      if (!access (modlistpath, R_OK))
	goto loadit;
    }
  free (modlistpath);
  modlistpath = 0;
  /* check in the temporary directory */
  modlistpath = basilys_tempdir_path (modlistbase, MODLIS_SUFFIX);
  if (!access (modlistpath, R_OK))
    goto loadit;
  free (modlistpath);
  modlistpath = 0;
  fatal_error ("cannot load MELT module list %s - incorrect name?",
	       modlistbase);
  if (!modlistpath)
    goto end;
loadit:
  debugeprintf ("basilysgc_load_modulelist loadit modlistpath %s", modlistpath);
  filmod = fopen (modlistpath, "r");
  dbgprintf ("reading module list '%s'", modlistpath);
  if (!filmod)
    fatal_error ("failed to open basilys module list file %s - %m",
		 modlistpath);
#if ENABLE_CHECKING
  {
    static char locbuf[80];
    memset (locbuf, 0, sizeof (locbuf));
    snprintf (locbuf, sizeof (locbuf) - 1,
	      "%s:%d:basilysgc_load_modulelist before reading module list : %s",
	      basename (__FILE__), __LINE__, modlistpath);
    curfram__.flocs = locbuf;
  }
#endif
  while (!feof (filmod))
    {
      char *pc = 0;
      memset (linbuf, 0, sizeof (linbuf));
      fgets (linbuf, sizeof (linbuf) - 1, filmod);
      pc = strchr (linbuf, '\n');
      if (pc)
	*pc = (char) 0;
      /* maybe we should not skip spaces */
      for (pc = linbuf; *pc && ISSPACE (*pc); pc++);
      if (*pc == '#' || *pc == (char) 0)
	continue;
      dbgprintf ("in module list %s loading module '%s'", modlistbase, pc);
      mdatav = basilysgc_load_melt_module ((basilys_ptr_t) mdatav, pc);
    }
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) mdatav;
#undef mdatav
}

/*************** initial load machinery *******************/


struct reading_st
{
  FILE *rfil;
  const char *rpath;
  char *rcurlin;		/* current line mallocated buffer */
  int rlineno;			/* current line number */
  int rcol;			/* current column */
  source_location rsrcloc;	/* current source location */
  basilys_ptr_t *rpfilnam;	/* pointer to location of file name string */
  basilys_ptr_t *rpgenv;	/* pointer to location of environment */
};
/* Obstack used for reading strings */
static struct obstack bstring_obstack;
#define rdback() (rd->rcol--)
#define rdnext() (rd->rcol++)
#define rdcurc() rd->rcurlin[rd->rcol]
#define rdfollowc(Rk) rd->rcurlin[rd->rcol + (Rk)]
#define rdeof() ((rd->rfil?feof(rd->rfil):1) && rd->rcurlin[rd->rcol]==0)
#define READ_ERROR(Fmt,...)	do {		\
  error_at(rd->rsrcloc, Fmt, ##__VA_ARGS__);	\
  fatal_error("MELT read failure <%s:%d>",	\
	      basename(__FILE__), __LINE__);	\
} while(0)
/* readval returns the read value and sets *PGOT to true if something
   was read */
static basilys_ptr_t readval (struct reading_st *rd, bool * pgot);

enum commenthandling_en
{ COMMENT_SKIP = 0, COMMENT_NO };
static int
skipspace_getc (struct reading_st *rd, enum commenthandling_en comh)
{
  int c = 0;
  int incomm = 0;
readagain:
  if (rdeof ())
    return EOF;
  if (!rd->rcurlin)
    goto readline;
  c = rdcurc ();
  if ((c == '\n' && !rdfollowc (1)) || c == 0)
  readline:
    {
      /* we expect most lines to fit into linbuf, so we don't handle
         efficiently long lines */
      static char linbuf[400];
      char *mlin = 0;		/* partial mallocated line buffer when
				   not fitting into linbuf */
      char *eol = 0;
      if (!rd->rfil)		/* reading from a buffer */
	return EOF;
      if (rd->rcurlin)
	free ((void *) rd->rcurlin);
      rd->rcurlin = NULL;
      /* we really want getline here .... */
      for (;;)
	{
	  memset (linbuf, 0, sizeof (linbuf));
	  eol = NULL;
	  if (!fgets (linbuf, sizeof (linbuf) - 2, rd->rfil))
	    {
	      /* reached eof, so either give mlin or duplicate an empty
	         line */
	      if (mlin)
		rd->rcurlin = mlin;
	      else
		rd->rcurlin = xstrdup ("");
	      break;
	    }
	  else
	    eol = strchr (linbuf, '\n');
	  if (eol)
	    {
	      if (rd->rcurlin)
		free ((void *) rd->rcurlin);
	      if (!mlin)
		rd->rcurlin = xstrdup (linbuf);
	      else
		{
		  rd->rcurlin = concat (mlin, linbuf, NULL);
		  free (mlin);
		}
	      break;
	    }
	  else
	    {
	      /* read partly a long line without reaching the end of line */
	      if (mlin)
		{
		  char *newmlin = concat (mlin, linbuf, NULL);
		  free (mlin);
		  mlin = newmlin;
		}
	      else
		mlin = xstrdup (linbuf);
	    }
	};
      rd->rlineno++;
      rd->rsrcloc =
	linemap_line_start (line_table, rd->rlineno, strlen (linbuf) + 1);
      rd->rcol = 0;
      if (comh == COMMENT_NO) 
	return rdcurc();
      goto readagain;
    }
  else if (c == ';' && comh == COMMENT_SKIP)
    goto readline;
  else if (c == '#' && comh == COMMENT_SKIP && rdfollowc (1) == '|')
    {
      incomm = 1;
      rdnext ();
      c = rdcurc ();
      goto readagain;
    }
  else if (incomm && comh == COMMENT_SKIP && c == '|' && rdfollowc (1) == '#')
    {
      incomm = 0;
      rdnext ();
      rdnext ();
      c = rdcurc ();
      goto readagain;
    }
  else if (ISSPACE (c) || incomm)
    {
      rdnext ();
      c = rdcurc ();
      goto readagain;
    }
  else
    return c;
}


#define EXTRANAMECHARS "_+-*/<>=!?:%~&@$"
/* read a simple name on the bname_obstack */
static char *
readsimplename (struct reading_st *rd)
{
  int c = 0;
  while (!rdeof () && (c = rdcurc ()) > 0 &&
	 (ISALNUM (c) || strchr (EXTRANAMECHARS, c) != NULL))
    {
      obstack_1grow (&bname_obstack, (char) c);
      rdnext ();
    }
  obstack_1grow (&bname_obstack, (char) 0);
  return XOBFINISH (&bname_obstack, char *);
}


/* read an integer, like +123, which may also be +%numbername or +|fieldname */
static long
readsimplelong (struct reading_st *rd)
{
  int c = 0;
  long r = 0;
  char *endp = 0;
  char *nam = 0;
  bool neg = FALSE;
  /* we do not need any GC locals ie BASILYS_ENTERFRAME because no
     garbage collection occurs here */
  c = rdcurc ();
  if (((c == '+' || c == '-') && ISDIGIT (rdfollowc (1))) || ISDIGIT (c))
    {
      /* R5RS and R6RS require decimal notation -since the binary and
         hex numbers are hash-prefixed but for convenience we accept
         them thru strtol */
      r = strtol (&rdcurc (), &endp, 0);
      if (r == 0 && endp <= &rdcurc ())
	READ_ERROR ("MELT: failed to read number %.20s", &rdcurc ());
      rd->rcol += endp - &rdcurc ();
      return r;
    }
  else if ((c == '+' || c == '-') && rdfollowc (1) == '%')
    {
      neg = (c == '-');
      rdnext ();
      rdnext ();
      nam = readsimplename (rd);
      r = -1;
      /* the +%magicname notation is seldom used, we don't care to do
         many needless strcmp-s in that case, to be able to define the
         below simple macro */
      if (!nam)
	READ_ERROR
	  ("MELT: magic number name expected after +%% or -%% for magic %s",
	   nam);
#define NUMNAM(N) else if (!strcmp(nam,#N)) r = (N)
      NUMNAM (OBMAG_OBJECT);
      NUMNAM (OBMAG_MULTIPLE);
      NUMNAM (OBMAG_BOX);
      NUMNAM (OBMAG_CLOSURE);
      NUMNAM (OBMAG_ROUTINE);
      NUMNAM (OBMAG_LIST);
      NUMNAM (OBMAG_PAIR);
      NUMNAM (OBMAG_TRIPLE);
      NUMNAM (OBMAG_INT);
      NUMNAM (OBMAG_MIXINT);
      NUMNAM (OBMAG_MIXLOC);
      NUMNAM (OBMAG_REAL);
      NUMNAM (OBMAG_STRING);
      NUMNAM (OBMAG_STRBUF);
      NUMNAM (OBMAG_TREE);
      NUMNAM (OBMAG_GIMPLE);
      NUMNAM (OBMAG_GIMPLESEQ);
      NUMNAM (OBMAG_BASICBLOCK);
      NUMNAM (OBMAG_EDGE);
      NUMNAM (OBMAG_MAPOBJECTS);
      NUMNAM (OBMAG_MAPSTRINGS);
      NUMNAM (OBMAG_MAPTREES);
      NUMNAM (OBMAG_MAPGIMPLES);
      NUMNAM (OBMAG_MAPGIMPLESEQS);
      NUMNAM (OBMAG_MAPBASICBLOCKS);
      NUMNAM (OBMAG_MAPEDGES);
      NUMNAM (OBMAG_DECAY);
      NUMNAM (OBMAG_SPEC_FILE);
      NUMNAM (OBMAG_SPEC_MPFR);
      NUMNAM (OBMAG_SPECPPL_COEFFICIENT);
      NUMNAM (OBMAG_SPECPPL_LINEAR_EXPRESSION);
      NUMNAM (OBMAG_SPECPPL_CONSTRAINT);
      NUMNAM (OBMAG_SPECPPL_CONSTRAINT_SYSTEM);
      NUMNAM (OBMAG_SPECPPL_GENERATOR);
      NUMNAM (OBMAG_SPECPPL_GENERATOR_SYSTEM);
      NUMNAM (OBMAG_SPECPPL_POLYHEDRON);
      /** the fields' ranks of basilys.h have been removed in rev126278 */
#undef NUMNAM
      if (r < 0)
	READ_ERROR ("MELT: bad magic number name %s", nam);
      obstack_free (&bname_obstack, nam);
      return neg ? -r : r;
    }
  else
    READ_ERROR ("MELT: invalid number %.20s", &rdcurc ());
  return 0;
}


static basilys_ptr_t
readseqlist (struct reading_st *rd, int endc)
{
  int c = 0;
  int nbcomp = 0;
  int startlin = rd->rlineno;
  bool got = FALSE;
  BASILYS_ENTERFRAME (2, NULL);
#define seqv curfram__.varptr[0]
#define compv curfram__.varptr[1]
  seqv = basilysgc_new_list (BASILYSGOB (DISCR_LIST));
readagain:
  compv = NULL;
  c = skipspace_getc (rd, COMMENT_SKIP);
  if (c == endc)
    {
      rdnext ();
      goto end;
    }
  got = FALSE;
  compv = readval (rd, &got);
  if (!compv && !got)
    READ_ERROR ("MELT: unexpected stuff in seq %.20s ... started line %d",
		&rdcurc (), startlin);
  basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) compv);
  nbcomp++;
  goto readagain;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) seqv;
#undef compv
#undef seqv
}




static basilys_ptr_t
makesexpr (struct reading_st *rd, int lineno, basilys_ptr_t contents_p,
	   location_t loc)
{
  BASILYS_ENTERFRAME (4, NULL);
#define sexprv  curfram__.varptr[0]
#define contsv   curfram__.varptr[1]
#define locmixv curfram__.varptr[2]
  contsv = contents_p;
  gcc_assert (basilys_magic_discr ((basilys_ptr_t) contsv) == OBMAG_LIST);
  if (loc == 0)
    locmixv = basilysgc_new_mixint (BASILYSGOB (DISCR_MIXEDINT),
				    *rd->rpfilnam, (long) lineno);
  else
    locmixv = basilysgc_new_mixloc (BASILYSGOB (DISCR_MIXEDLOC),
				    *rd->rpfilnam, (long) lineno, loc);
  sexprv = basilysgc_new_raw_object (BASILYSGOB (CLASS_SEXPR), FSEXPR__LAST);
  ((basilysobject_ptr_t) (sexprv))->obj_vartab[FSEXPR_LOCATION] =
    (basilys_ptr_t) locmixv;
  ((basilysobject_ptr_t) (sexprv))->obj_vartab[FSEXPR_CONTENTS] =
    (basilys_ptr_t) contsv;
  basilysgc_touch (sexprv);
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) sexprv;
#undef sexprv
#undef contsv
#undef locmixv
}


basilys_ptr_t
basilysgc_named_symbol (const char *nam, int create)
{
  int namlen = 0, ix = 0;
  char *namdup = 0;
  char tinybuf[130];
  BASILYS_ENTERFRAME (4, NULL);
#define symbv    curfram__.varptr[0]
#define dictv    curfram__.varptr[1]
#define closv    curfram__.varptr[2]
#define nstrv    curfram__.varptr[3]
  symbv = NULL;
  dictv = NULL;
  closv = NULL;
  if (!nam)
    goto end;
  namlen = strlen (nam);
  memset (tinybuf, 0, sizeof (tinybuf));
  if (namlen < (int) sizeof (tinybuf) - 2)
    namdup = strcpy (tinybuf, nam);
  else
    namdup = strcpy ((char *) xcalloc (namlen + 1, 1), nam);
  gcc_assert (basilys_magic_discr (BASILYSG (CLASS_SYSTEM_DATA))
	      == OBMAG_OBJECT);
  gcc_assert (basilys_magic_discr (BASILYSG (INITIAL_SYSTEM_DATA)) ==
	      OBMAG_OBJECT);
  for (ix = 0; ix < namlen; ix++)
    if (ISALPHA (namdup[ix]))
      namdup[ix] = TOUPPER (namdup[ix]);
  if (BASILYSG (INITIAL_SYSTEM_DATA) != 0
      && basilys_is_instance_of
      (BASILYSG (INITIAL_SYSTEM_DATA), BASILYSG (CLASS_SYSTEM_DATA))
      && basilys_object_length ((basilys_ptr_t)
				BASILYSG (INITIAL_SYSTEM_DATA)) >=
      FSYSDAT__LAST)
    {
      dictv =
	BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_vartab[FSYSDAT_SYMBOLDICT];
      if (basilys_magic_discr ((basilys_ptr_t) dictv) == OBMAG_MAPSTRINGS)
	symbv =
	  basilys_get_mapstrings ((struct basilysmapstrings_st *) dictv,
				  namdup);
      if (symbv || !create)
	goto end;
      closv = BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_vartab[FSYSDAT_ADDSYMBOL];
      if (basilys_magic_discr ((basilys_ptr_t) closv) == OBMAG_CLOSURE)
	{
	  union basilysparam_un pararg[1];
	  memset (&pararg, 0, sizeof (pararg));
	  nstrv = basilysgc_new_string (BASILYSGOB (DISCR_STRING), namdup);
	  pararg[0].bp_aptr = (basilys_ptr_t *) & nstrv;
	  symbv =
	    basilys_apply ((basilysclosure_ptr_t) closv,
			   (basilys_ptr_t) BASILYSG (INITIAL_SYSTEM_DATA),
			   BPARSTR_PTR, pararg, "", NULL);
	  goto end;
	}
    }
end:;
  if (namdup && namdup != tinybuf)
    free (namdup);
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) symbv;
#undef symbv
#undef dictv
#undef closv
#undef nstrv
}

basilys_ptr_t
basilysgc_intern_symbol (basilys_ptr_t symb_p)
{
  BASILYS_ENTERFRAME (5, NULL);
#define symbv    curfram__.varptr[0]
#define dictv    curfram__.varptr[1]
#define closv    curfram__.varptr[2]
#define nstrv    curfram__.varptr[3]
#define resv     curfram__.varptr[4]
#define obj_symbv    ((basilysobject_ptr_t)(symbv))
  symbv = symb_p;
  if (basilys_magic_discr ((basilys_ptr_t) symbv) != OBMAG_OBJECT
      || obj_symbv->obj_len < FSYMB__LAST
      || !basilys_is_instance_of ((basilys_ptr_t) symbv,
				  (basilys_ptr_t) BASILYSG (CLASS_SYMBOL)))
    goto fail;
  nstrv = obj_symbv->obj_vartab[FNAMED_NAME];
  if (basilys_magic_discr ((basilys_ptr_t) nstrv) != OBMAG_STRING)
    goto fail;
  if (basilys_magic_discr (BASILYSG (INITIAL_SYSTEM_DATA)) !=
      OBMAG_OBJECT
      || basilys_object_length ((basilys_ptr_t)
				BASILYSGOB (INITIAL_SYSTEM_DATA)) <
      FSYSDAT__LAST
      || !basilys_is_instance_of (BASILYSG (INITIAL_SYSTEM_DATA),
				  BASILYSG (CLASS_SYSTEM_DATA)))
    goto fail;
  closv = BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_vartab[FSYSDAT_INTERNSYMBOL];
  if (basilys_magic_discr ((basilys_ptr_t) closv) != OBMAG_CLOSURE)
    goto fail;
  else
    {
      union basilysparam_un pararg[1];
      memset (&pararg, 0, sizeof (pararg));
      pararg[0].bp_aptr = (basilys_ptr_t *) & symbv;
      BASILYS_LOCATION_HERE ("intern symbol before apply");
      resv =
	basilys_apply ((basilysclosure_ptr_t) closv,
		       (basilys_ptr_t) BASILYSG (INITIAL_SYSTEM_DATA),
		       BPARSTR_PTR, pararg, "", NULL);
      goto end;
    }
fail:
  resv = NULL;
end:;
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) resv;
#undef symbv
#undef dictv
#undef closv
#undef nstrv
#undef resv
#undef obj_symbv
}


basilys_ptr_t
basilysgc_intern_keyword (basilys_ptr_t keyw_p)
{
  BASILYS_ENTERFRAME (5, NULL);
#define keywv    curfram__.varptr[0]
#define dictv    curfram__.varptr[1]
#define closv    curfram__.varptr[2]
#define nstrv    curfram__.varptr[3]
#define resv     curfram__.varptr[4]
#define obj_keywv    ((basilysobject_ptr_t)(keywv))
  keywv = keyw_p;
  if (basilys_magic_discr ((basilys_ptr_t) keywv) != OBMAG_OBJECT
      || basilys_object_length ((basilys_ptr_t) obj_keywv) < FSYMB__LAST
      || !basilys_is_instance_of ((basilys_ptr_t) keywv,
				  (basilys_ptr_t) BASILYSG (CLASS_KEYWORD)))
    goto fail;
  nstrv = obj_keywv->obj_vartab[FNAMED_NAME];
  if (basilys_magic_discr ((basilys_ptr_t) nstrv) != OBMAG_STRING)
    goto fail;
  if (basilys_magic_discr (BASILYSG (INITIAL_SYSTEM_DATA)) !=
      OBMAG_OBJECT
      || BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_len < FSYSDAT__LAST
      || !basilys_is_instance_of (BASILYSG (INITIAL_SYSTEM_DATA),
				  BASILYSG (CLASS_SYSTEM_DATA)))
    goto fail;
  closv = BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_vartab[FSYSDAT_INTERNKEYW];
  if (basilys_magic_discr ((basilys_ptr_t) closv) != OBMAG_CLOSURE)
    goto fail;
  else
    {
      union basilysparam_un pararg[1];
      memset (&pararg, 0, sizeof (pararg));
      pararg[0].bp_aptr = (basilys_ptr_t *) & keywv;
      BASILYS_LOCATION_HERE ("intern keyword before apply");
      resv =
	basilys_apply ((basilysclosure_ptr_t) closv,
		       (basilys_ptr_t) BASILYSG (INITIAL_SYSTEM_DATA),
		       BPARSTR_PTR, pararg, "", NULL);
      goto end;
    }
fail:
  resv = NULL;
end:;
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) resv;
#undef symbv
#undef dictv
#undef closv
#undef nstrv
#undef resv
#undef obj_symbv
}






basilys_ptr_t
basilysgc_named_keyword (const char *nam, int create)
{
  int namlen = 0, ix = 0;
  char *namdup = 0;
  char tinybuf[130];
  BASILYS_ENTERFRAME (4, NULL);
#define keywv    curfram__.varptr[0]
#define dictv    curfram__.varptr[1]
#define closv    curfram__.varptr[2]
#define nstrv    curfram__.varptr[3]
  keywv = NULL;
  dictv = NULL;
  closv = NULL;
  if (!nam)
    goto end;
  if (nam[0] == ':')
    nam++;
  namlen = strlen (nam);
  memset (tinybuf, 0, sizeof (tinybuf));
  if (namlen < (int) sizeof (tinybuf) - 2)
    namdup = strcpy (tinybuf, nam);
  else
    namdup = strcpy ((char *) xcalloc (namlen + 1, 1), nam);
  for (ix = 0; ix < namlen; ix++)
    if (ISALPHA (namdup[ix]))
      namdup[ix] = TOUPPER (namdup[ix]);
  gcc_assert (basilys_magic_discr (BASILYSG (CLASS_SYSTEM_DATA))
	      == OBMAG_OBJECT);
  gcc_assert (basilys_magic_discr (BASILYSG (INITIAL_SYSTEM_DATA)) ==
	      OBMAG_OBJECT);
  if (basilys_is_instance_of
      (BASILYSG (INITIAL_SYSTEM_DATA), BASILYSG (CLASS_SYSTEM_DATA))
      && BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_len >= FSYSDAT__LAST)
    {
      dictv = BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_vartab[FSYSDAT_KEYWDICT];
      if (basilys_magic_discr ((basilys_ptr_t) dictv) == OBMAG_MAPSTRINGS)
	keywv =
	  basilys_get_mapstrings ((struct basilysmapstrings_st *) dictv,
				  namdup);
      if (keywv || !create)
	goto end;
      closv = BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_vartab[FSYSDAT_ADDKEYW];
      if (basilys_magic_discr ((basilys_ptr_t) closv) == OBMAG_CLOSURE)
	{
	  union basilysparam_un pararg[1];
	  memset (&pararg, 0, sizeof (pararg));
	  nstrv = basilysgc_new_string (BASILYSGOB (DISCR_STRING), namdup);
	  pararg[0].bp_aptr = (basilys_ptr_t *) & nstrv;
	  keywv =
	    basilys_apply ((basilysclosure_ptr_t) closv,
			   (basilys_ptr_t) BASILYSG (INITIAL_SYSTEM_DATA),
			   BPARSTR_PTR, pararg, "", NULL);
	  goto end;
	}
    }
end:;
  if (namdup && namdup != tinybuf)
    free (namdup);
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) keywv;
#undef keywv
#undef dictv
#undef closv
#undef nstrv
}



static basilys_ptr_t
readsexpr (struct reading_st *rd, int endc)
{
  int c = 0, lineno = rd->rlineno;
  location_t loc = 0;
  BASILYS_ENTERFRAME (3, NULL);
#define sexprv  curfram__.varptr[0]
#define contv   curfram__.varptr[1]
#define locmixv curfram__.varptr[2]
  if (!endc || rdeof ())
    READ_ERROR ("MELT: eof in s-expr (lin%d)", lineno);
  c = skipspace_getc (rd, COMMENT_SKIP);
  LINEMAP_POSITION_FOR_COLUMN (loc, line_table, rd->rcol);
  contv = readseqlist (rd, endc);
  sexprv = makesexpr (rd, lineno, (basilys_ptr_t) contv, loc);
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) sexprv;
#undef sexprv
#undef contv
#undef locmixv
}


static basilys_ptr_t
readassoc (struct reading_st *rd)
{
  int sz = 0, c = 0, ln = 0, pos = 0;
  BASILYS_ENTERFRAME (3, NULL);
#define mapv  curfram__.varptr[0]
#define attrv curfram__.varptr[1]
#define valv   curfram__.varptr[2]
  /* maybe read the size */
  if (rdcurc () == '/')
    {
      sscanf (&rdcurc (), "/%d%n", &sz, &pos);
      if (pos > 0)
	rd->rcol += pos;
    };
  if (sz > BASILYS_MAXLEN)
    sz = BASILYS_MAXLEN;
  else if (sz < 0)
    sz = 2;
  mapv = basilysgc_new_mapobjects (BASILYSGOB (DISCR_MAPOBJECTS), sz);
  c = skipspace_getc (rd, COMMENT_SKIP);
  while (c != '}' && !rdeof ())
    {
      bool gotat = FALSE, gotva = FALSE;
      ln = rd->rlineno;
      attrv = readval (rd, &gotat);
      if (!gotat || !attrv
	  || basilys_magic_discr ((basilys_ptr_t) attrv) != OBMAG_OBJECT)
	READ_ERROR ("MELT: bad attribute in mapoobject line %d", ln);
      c = skipspace_getc (rd, COMMENT_SKIP);
      if (c != '=')
	READ_ERROR ("MELT: expected equal = after attribute but got %c",
		    ISPRINT (c) ? c : ' ');
      rdnext ();
      ln = rd->rlineno;
      valv = readval (rd, &gotva);
      if (!valv)
	READ_ERROR ("MELT: null or missing value in mapobject line %d", ln);
      c = skipspace_getc (rd, COMMENT_SKIP);
      if (c == '.')
	c = skipspace_getc (rd, COMMENT_SKIP);
    }
  if (c == '}')
    rdnext ();
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) mapv;
#undef mapv
#undef attrv
#undef valv
}


/* if the string ends with "_ call gettext on it to have it
   localized/internationlized -i18n- */
static basilys_ptr_t
readstring (struct reading_st *rd)
{
  int c = 0;
  int nbesc = 0;
  char *cstr = 0, *endc = 0;
  bool isintl = false;
  BASILYS_ENTERFRAME (1, NULL);
#define strv   curfram__.varptr[0]
#define str_strv  ((struct basilysstring_st*)(strv))
  while ((c = rdcurc ()) != '"' && !rdeof ())
    {
      if (c != '\\')
	{
	  obstack_1grow (&bstring_obstack, (char) c);
	  if (c == '\n')
	    c = skipspace_getc (rd, COMMENT_NO);
	  else
	    rdnext ();
	}
      else
	{
	  rdnext ();
	  c = rdcurc ();
	  nbesc++;
	  switch (c)
	    {
	    case 'a':
	      c = '\a';
	      rdnext ();
	      break;
	    case 'b':
	      c = '\b';
	      rdnext ();
	      break;
	    case 't':
	      c = '\t';
	      rdnext ();
	      break;
	    case 'n':
	      c = '\n';
	      rdnext ();
	      break;
	    case 'v':
	      c = '\v';
	      rdnext ();
	      break;
	    case 'f':
	      c = '\f';
	      rdnext ();
	      break;
	    case 'r':
	      c = '\r';
	      rdnext ();
	      break;
	    case '"':
	      c = '\"';
	      rdnext ();
	      break;
	    case '\\':
	      c = '\\';
	      rdnext ();
	      break;
	    case '\n':
	    case '\r':
	      skipspace_getc (rd, COMMENT_NO);
	      continue;
	    case ' ':
	      c = ' ';
	      rdnext ();
	      break;
	    case 'x':
	      rdnext ();
	      c = (char) strtol (&rdcurc (), &endc, 16);
	      if (c == 0 && endc <= &rdcurc ())
		READ_ERROR ("MELT: illegal hex \\x escape in string %.20s",
			    &rdcurc ());
	      if (*endc == ';')
		endc++;
	      rd->rcol += endc - &rdcurc ();
	      break;
	    case '{':
	      {
		int linbrac = rd->rlineno;
		/* the escaped left brace \{ read verbatim all the string till the right brace } */
		rdnext ();
		while (rdcurc () != '}')
		  {
		    int cc;
		    if (rdeof ())
		      READ_ERROR
			("MELT: reached end of file in braced block string starting line %d",
			 linbrac);
		    cc = rdcurc ();
		    if (cc == '\n')
		      cc = skipspace_getc (rd, COMMENT_NO);
		    else
		      obstack_1grow (&bstring_obstack, (char) cc);
		    rdnext ();
		  };
		rdnext ();
	      }
	      break;
	    default:
	      READ_ERROR
		("MELT: illegal escape sequence %.10s in string -- got \\%c (hex %x)",
		 &rdcurc () - 1, c, c);
	    }
	  obstack_1grow (&bstring_obstack, (char) c);
	}
    }
  if (c == '"')
    rdnext ();
  else
    READ_ERROR ("MELT: unterminated string %.20s", &rdcurc ());
  c = rdcurc ();
  if (c == '_' && !rdeof ())
    {
      isintl = true;
      rdnext ();
    }
  obstack_1grow (&bstring_obstack, (char) 0);
  cstr = XOBFINISH (&bstring_obstack, char *);
  if (isintl)
    cstr = gettext (cstr);
  strv = basilysgc_new_string (BASILYSGOB (DISCR_STRING), cstr);
  obstack_free (&bstring_obstack, cstr);
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) strv;
#undef strv
#undef str_strv
}

/**
   macrostring so #{if ($A>0) printf("%s", $B);}# is parsed as would
   be parsed the s-expr ("if (" A ">0) printf(\"%s\", " B ");")

   read a macrostring sequence starting with #{ and ending with }#
   perhaps spanning several lines in the source no escape characters
   are handled (in particular no backslash escapes) except the dollar
   sign $ and then ending }# 

   A $ followed by alphabetical caracters (or as in C by underscores
   or digits, provided the first is not a digit) is handled as a
   symbol. If it is immediately followed by an hash # the # is
   skipped

**/
static basilys_ptr_t
readmacrostringsequence (struct reading_st *rd) 
{
#if ENABLE_CHECKING
  static long _herecallcount;
  long callcount = ++_herecallcount;
#endif
  int lineno = rd->rlineno;
  location_t loc = 0;
  BASILYS_ENTERFRAME (6, NULL);
#define readv    curfram__.varptr[0]
#define strv     curfram__.varptr[1]
#define symbv    curfram__.varptr[2]
#define seqv     curfram__.varptr[3]
#define sbufv    curfram__.varptr[4]
  LINEMAP_POSITION_FOR_COLUMN (loc, line_table, rd->rcol);
  seqv = basilysgc_new_list (BASILYSGOB (DISCR_LIST));
  sbufv = basilysgc_new_strbuf(BASILYSGOB(DISCR_STRBUF), (char*)0);
  for(;;) {
    if (!rdcurc()) {
      /* reached end of line */
      skipspace_getc(rd, COMMENT_NO);
      continue;
    }
    if (rdeof()) 
      READ_ERROR("reached end of file in macrostring sequence started line %d", lineno);
    debugeprintf ("macrostring startloop rdcur:%s", &rdcurc());
    debugeprintf ("macrostring startloop sbuf@%pL%d:%s", sbufv, 
		 basilys_strbuf_usedlength((basilys_ptr_t)sbufv),
		 basilys_strbuf_str((basilys_ptr_t) sbufv));
    if (rdcurc()=='}' && rdfollowc(1)=='#') {
      rdnext(); 
      rdnext();
      if (sbufv && basilys_strbuf_usedlength((basilys_ptr_t)sbufv)>0) {
	strv = basilysgc_new_stringdup (BASILYSGOB(DISCR_STRING),
					basilys_strbuf_str((basilys_ptr_t) sbufv));
	debugeprintf ("macrostring end appending str:%s", basilys_string_str((basilys_ptr_t)strv));
	basilysgc_append_list((basilys_ptr_t) seqv, (basilys_ptr_t) strv);
	sbufv = NULL;
	strv = NULL;
      }
      break;
    }
    else if (rdcurc()=='$') {
      /* $ followed by letters or underscore makes a symbol */
      if (ISALPHA(rdfollowc(1)) || rdfollowc(1)=='_') {
	int lnam = 1;
	char tinybuf[64];
	/* if there is any sbuf, make a string of it and add the
	   string into the sequence */
	if (sbufv && basilys_strbuf_usedlength((basilys_ptr_t)sbufv)>0) {
	  strv = basilysgc_new_stringdup(BASILYSGOB(DISCR_STRING),
					 basilys_strbuf_str((basilys_ptr_t) sbufv));
	  basilysgc_append_list((basilys_ptr_t) seqv, (basilys_ptr_t) strv);
	  debugeprintf ("macrostring var appending str:%s", basilys_string_str((basilys_ptr_t)strv));
	  sbufv = NULL;
	  strv = NULL;
	}
	while (ISALNUM(rdfollowc(lnam)) || rdfollowc(lnam) == '_') 
	  lnam++;
	if (lnam< (int)sizeof(tinybuf)-2) {
	  memset(tinybuf, 0, sizeof(tinybuf));
	  memcpy(tinybuf, &rdfollowc(1), lnam-1);
	  tinybuf[lnam] = (char)0;
	  debugeprintf ("macrostring tinysymb lnam.%d <%s>", lnam, tinybuf);
	  symbv = basilysgc_named_symbol(tinybuf, BASILYS_CREATE);
	}
	else {
	  char *nambuf = (char*) xcalloc(lnam+2, 1);
	  memcpy(nambuf, &rdfollowc(1), lnam-1);
	  nambuf[lnam] = (char)0;
	  symbv = basilysgc_named_symbol(nambuf, BASILYS_CREATE);
	  debugeprintf ("macrostring bigsymb <%s>", tinybuf);
	  free(nambuf);
	}
	rd->rcol += lnam;
	/* skip the hash # if just after the symbol */
	if (rdcurc() == '#') 
	  rdnext();
	/* append the symbol */
	basilysgc_append_list((basilys_ptr_t) seqv, (basilys_ptr_t) symbv);
	symbv = NULL;
      }
      /* $. is silently skipped */
      else if (rdfollowc(1) == '.') {
	rdnext(); 
	rdnext();
      }
      /* $$ is handled as a single dollar $ */
      else if (rdfollowc(1) == '$') {
	if (!sbufv)
	  sbufv = basilysgc_new_strbuf(BASILYSGOB(DISCR_STRBUF), (char*)0);
	basilysgc_add_strbuf_raw_len((basilys_ptr_t)sbufv, "$", 1);
	rdnext();
	rdnext();
      }
      /* $# is handled as a single hash # */
      else if (rdfollowc(1) == '#') {
	if (!sbufv)
	  sbufv = basilysgc_new_strbuf(BASILYSGOB(DISCR_STRBUF), (char*)0);
	basilysgc_add_strbuf_raw_len((basilys_ptr_t)sbufv, "#", 1);
	rdnext();
	rdnext();
      }
      /* any other dollar something is an error */
      else READ_ERROR("unexpected dollar escape in macrostring %.4s started line %d",
		      &rdcurc(), lineno);
    }
    else if ( ISALNUM(rdcurc()) || ISSPACE(rdcurc()) ) { 
      /* handle efficiently the common case of alphanum and spaces */
      int nbc = 0;
      if (!sbufv)
	sbufv = basilysgc_new_strbuf(BASILYSGOB(DISCR_STRBUF), (char*)0);
      while (ISALNUM(rdfollowc(nbc)) || ISSPACE(rdfollowc(nbc))) 
	nbc++;
      basilysgc_add_strbuf_raw_len((basilys_ptr_t)sbufv, &rdcurc(), nbc);
      rd->rcol += nbc;
    }
    else { /* the current char is not a dollar $ */
      if (!sbufv)
	sbufv = basilysgc_new_strbuf(BASILYSGOB(DISCR_STRBUF), (char*)0);
      basilysgc_add_strbuf_raw_len((basilys_ptr_t)sbufv, &rdcurc(), 1);
      rdnext();
    }
  }
  debugmsgval("readmacrostringsequence seqv", seqv, callcount);
  readv = makesexpr (rd, lineno, (basilys_ptr_t) seqv, loc);
  debugmsgval("readmacrostringsequence readv", readv, callcount);
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) readv;
#undef readv
#undef strv
#undef symbv
#undef seqv
#undef sbufv
}


static basilys_ptr_t
readhashescape (struct reading_st *rd)
{
  int c = 0;
  BASILYS_ENTERFRAME (4, NULL);
#define readv  curfram__.varptr[0]
#define compv  curfram__.varptr[1]
#define listv  curfram__.varptr[2]
#define pairv  curfram__.varptr[3]
  readv = NULL;
  c = rdcurc ();
  if (!c || rdeof ())
    READ_ERROR ("MELT: eof in hashescape %.20s", &rdcurc ());
  if (c == '\\')
    {
      rdnext ();
      if (ISALPHA (rdcurc ()) && rdcurc () != 'x' && ISALPHA (rdfollowc (1)))
	{
	  char *nam = readsimplename (rd);
	  c = 0;
	  if (!strcmp (nam, "nul"))
	    c = 0;
	  else if (!strcmp (nam, "alarm"))
	    c = '\a';
	  else if (!strcmp (nam, "backspace"))
	    c = '\b';
	  else if (!strcmp (nam, "tab"))
	    c = '\t';
	  else if (!strcmp (nam, "linefeed"))
	    c = '\n';
	  else if (!strcmp (nam, "vtab"))
	    c = '\v';
	  else if (!strcmp (nam, "page"))
	    c = '\f';
	  else if (!strcmp (nam, "return"))
	    c = '\r';
	  else if (!strcmp (nam, "space"))
	    c = ' ';
	  /* won't work on non ASCII or ISO or Unicode host, but we don't care */
	  else if (!strcmp (nam, "delete"))
	    c = 0xff;
	  else if (!strcmp (nam, "esc"))
	    c = 0x1b;
	  else
	    READ_ERROR ("MELT: invalid char escape %s", nam);
	  obstack_free (&bname_obstack, nam);
	char_escape:
	  readv = basilysgc_new_int (BASILYSGOB (DISCR_CHARINTEGER), c);
	}
      else if (rdcurc () == 'x' && ISXDIGIT (rdfollowc (1)))
	{
	  char *endc = 0;
	  rdnext ();
	  c = strtol (&rdcurc (), &endc, 16);
	  if (c == 0 && endc <= &rdcurc ())
	    READ_ERROR ("MELT: illigal hex #\\x escape in char %.20s",
			&rdcurc ());
	  rd->rcol += endc - &rdcurc ();
	  goto char_escape;
	}
      else if (ISPRINT (rdcurc ()))
	{
	  c = rdcurc ();
	  rdnext ();
	  goto char_escape;
	}
      else
	READ_ERROR ("MELT: unrecognized char escape #\\%s", &rdcurc ());
    }
  else if (c == '(')
    {
      int ln = 0, ix = 0;
      listv = readseqlist (rd, ')');
      ln = basilys_list_length ((basilys_ptr_t) listv);
      gcc_assert (ln >= 0);
      readv = basilysgc_new_multiple (BASILYSGOB (DISCR_MULTIPLE), ln);
      for ((ix = 0), (pairv =
		      ((struct basilyslist_st *) (listv))->first);
	   ix < ln
	   && basilys_magic_discr ((basilys_ptr_t) pairv) == OBMAG_PAIR;
	   pairv = ((struct basilyspair_st *) (pairv))->tl)
	((struct basilysmultiple_st *) (readv))->tabval[ix++] =
	  ((struct basilyspair_st *) (pairv))->hd;
      basilysgc_touch (readv);
    }
  else if (c == '[')
    {
      /* a basilys extension #[ .... ] for lists */
      readv = readseqlist (rd, ']');
    }
  else if ((c == 'b' || c == 'B') && ISDIGIT (rdfollowc (1)))
    {
      /* binary number */
      char *endc = 0;
      long n = 0;
      rdnext ();
      n = strtol (&rdcurc (), &endc, 2);
      if (n == 0 && endc <= &rdcurc ())
	READ_ERROR ("MELT: bad binary number %s", endc);
      readv = basilysgc_new_int (BASILYSGOB (DISCR_INTEGER), n);
    }
  else if ((c == 'o' || c == 'O') && ISDIGIT (rdfollowc (1)))
    {
      /* octal number */
      char *endc = 0;
      long n = 0;
      rdnext ();
      n = strtol (&rdcurc (), &endc, 8);
      if (n == 0 && endc <= &rdcurc ())
	READ_ERROR ("MELT: bad octal number %s", endc);
      readv = basilysgc_new_int (BASILYSGOB (DISCR_INTEGER), n);
    }
  else if ((c == 'd' || c == 'D') && ISDIGIT (rdfollowc (1)))
    {
      /* decimal number */
      char *endc = 0;
      long n = 0;
      rdnext ();
      n = strtol (&rdcurc (), &endc, 10);
      if (n == 0 && endc <= &rdcurc ())
	READ_ERROR ("MELT: bad decimal number %s", endc);
      readv = basilysgc_new_int (BASILYSGOB (DISCR_INTEGER), n);
    }
  else if ((c == 'x' || c == 'x') && ISDIGIT (rdfollowc (1)))
    {
      /* hex number */
      char *endc = 0;
      long n = 0;
      rdnext ();
      n = strtol (&rdcurc (), &endc, 16);
      if (n == 0 && endc <= &rdcurc ())
	READ_ERROR ("MELT: bad octal number %s", endc);
      readv = basilysgc_new_int (BASILYSGOB (DISCR_INTEGER), n);
    }
  else if (c == '+' && ISALPHA (rdfollowc (1)))
    {
      bool gotcomp = FALSE;
      char *nam = 0;
      nam = readsimplename (rd);
      compv = readval (rd, &gotcomp);
      if (!strcmp (nam, "BASILYS"))
	readv = compv;
      else
	readv = readval (rd, &gotcomp);
    }
  /* #{ is a macrostringsequence; it is terminated by }# and each
      occurrence of $ followed by alphanum char is considered as a
      MELT symbol, the other caracters are considered as string
      chunks; the entire read is a sequence */
  else if (c == '{') {
      rdnext ();
      readv = readmacrostringsequence(rd);
  }
  else
    READ_ERROR ("MELT: invalid escape %.20s", &rdcurc ());
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) readv;
#undef readv
#undef listv
#undef compv
#undef pairv
}



static basilys_ptr_t
readval (struct reading_st *rd, bool * pgot)
{
  int c = 0;
  char *nam = 0;
  BASILYS_ENTERFRAME (4, NULL);
#define readv   curfram__.varptr[0]
#define compv   curfram__.varptr[1]
#define seqv    curfram__.varptr[2]
#define altv    curfram__.varptr[3]
  readv = NULL;
  c = skipspace_getc (rd, COMMENT_SKIP);
  /*   debugeprintf ("start readval line %d col %d char %c", rd->rlineno, rd->rcol,
     ISPRINT (c) ? c : ' '); */
  if (ISDIGIT (c)
      || ((c == '-' || c == '+')
	  && (ISDIGIT (rdfollowc (1)) || rdfollowc (1) == '%'
	      || rdfollowc (1) == '|')))
    {
      long num = 0;
      num = readsimplelong (rd);
      readv =
	basilysgc_new_int ((basilysobject_ptr_t) BASILYSGOB (DISCR_INTEGER),
			   num);
      *pgot = TRUE;
      goto end;
    }				/* end if ISDIGIT or '-' or '+' */
  else if (c == '"')
    {
      rdnext ();
      readv = readstring (rd);
      *pgot = TRUE;
      goto end;
    }				/* end if '"' */
  else if (c == '(')
    {
      rdnext ();
      if (rdcurc () == ')')
	{
	  rdnext ();
	  readv = NULL;
	  *pgot = TRUE;
	  goto end;
	}
      readv = readsexpr (rd, ')');
      *pgot = TRUE;
      goto end;
    }				/* end if '(' */
  else if (c == '[')
    {
      rdnext ();
      readv = readsexpr (rd, ']');
      *pgot = TRUE;
      goto end;
    }				/* end if '[' */
  else if (c == '{')
    {
      rdnext ();
      readv = readassoc (rd);
      *pgot = TRUE;
      goto end;
    }
  else if (c == '#')
    {
      rdnext ();
      c = rdcurc ();
      readv = readhashescape (rd);
      *pgot = TRUE;
      goto end;
    }
  else if (c == '\'')
    {
      int lineno = rd->rlineno;
      bool got = false;
      location_t loc = 0;
      rdnext ();
      compv = readval (rd, &got);
      if (!got)
	READ_ERROR ("MELT: expecting value after quote %.20s", &rdcurc ());
      seqv = basilysgc_new_list (BASILYSGOB (DISCR_LIST));
      altv = basilysgc_named_symbol ("quote", BASILYS_CREATE);
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) altv);
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) compv);
      LINEMAP_POSITION_FOR_COLUMN (loc, line_table, rd->rcol);
      readv = makesexpr (rd, lineno, (basilys_ptr_t) seqv, loc);
      *pgot = TRUE;
      goto end;
    }
  else if (c == '`')
    {
      int lineno = rd->rlineno;
      bool got = false;
      location_t loc = 0;
      rdnext ();
      LINEMAP_POSITION_FOR_COLUMN (loc, line_table, rd->rcol);
      compv = readval (rd, &got);
      if (!got)
	READ_ERROR ("MELT: expecting value after backquote %.20s",
		    &rdcurc ());
      seqv = basilysgc_new_list (BASILYSGOB (DISCR_LIST));
      altv = basilysgc_named_symbol ("backquote", BASILYS_CREATE);
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) altv);
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) compv);
      readv = makesexpr (rd, lineno, (basilys_ptr_t) seqv, loc);
      *pgot = TRUE;
      goto end;
    }
  else if (c == ',')
    {
      int lineno = rd->rlineno;
      bool got = false;
      location_t loc = 0;
      rdnext ();
      LINEMAP_POSITION_FOR_COLUMN (loc, line_table, rd->rcol);
      compv = readval (rd, &got);
      if (!got)
	READ_ERROR ("MELT: expecting value after comma %.20s", &rdcurc ());
      seqv = basilysgc_new_list (BASILYSGOB (DISCR_LIST));
      altv = basilysgc_named_symbol ("comma", BASILYS_CREATE);
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) altv);
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) compv);
      readv = makesexpr (rd, lineno, (basilys_ptr_t) seqv, loc);
      *pgot = TRUE;
      goto end;
    }
  else if (c == '?')
    {
      int lineno = rd->rlineno;
      bool got = false;
      location_t loc = 0;
      rdnext ();
      LINEMAP_POSITION_FOR_COLUMN (loc, line_table, rd->rcol);
      compv = readval (rd, &got);
      if (!got)
	READ_ERROR ("MELT: expecting value after question %.20s", &rdcurc ());
      seqv = basilysgc_new_list (BASILYSGOB (DISCR_LIST));
      altv = basilysgc_named_symbol ("question", BASILYS_CREATE);
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) altv);
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) compv);
      readv = makesexpr (rd, lineno, (basilys_ptr_t) seqv, loc);
      *pgot = TRUE;
      goto end;
    }
  else if (c == ':')
    {
      nam = readsimplename (rd);
      readv = basilysgc_named_keyword (nam, BASILYS_CREATE);
      if (!readv)
	READ_ERROR ("MELT: unknown named keyword %s", nam);
      *pgot = TRUE;
      goto end;
    }
  else if (ISALPHA (c) || strchr (EXTRANAMECHARS, c) != NULL)
    {
      nam = readsimplename (rd);
      readv = basilysgc_named_symbol (nam, BASILYS_CREATE);
      *pgot = TRUE;
      goto end;
    }
  else
    {
      if (c >= 0)
	rdback ();
      readv = NULL;
    }
end:
  BASILYS_EXITFRAME ();
  if (nam)
    {
      *nam = 0;
      obstack_free (&bname_obstack, nam);
    };
  return (basilys_ptr_t) readv;
#undef readv
#undef compv
#undef seqv
#undef altv
}


void
basilys_error_str (basilys_ptr_t mixloc_p, const char *msg,
		   basilys_ptr_t str_p)
{
  int mixmag = 0;
  int lineno = 0;
  location_t loc = 0;
  BASILYS_ENTERFRAME (3, NULL);
#define mixlocv    curfram__.varptr[0]
#define strv       curfram__.varptr[1]
#define finamv     curfram__.varptr[2]
  gcc_assert (msg && msg[0]);
  mixlocv = mixloc_p;
  strv = str_p;
  mixmag = basilys_magic_discr ((basilys_ptr_t) mixlocv);
  if (mixmag == OBMAG_MIXLOC)
    {
      loc = basilys_location_mixloc ((basilys_ptr_t) mixlocv);
      finamv = basilys_val_mixloc ((basilys_ptr_t) mixlocv);
      lineno = basilys_num_mixloc ((basilys_ptr_t) mixlocv);
    }
  else if (mixmag == OBMAG_MIXINT)
    {
      loc = 0;
      finamv = basilys_val_mixint ((basilys_ptr_t) mixlocv);
      lineno = basilys_num_mixint ((basilys_ptr_t) mixlocv);
    }
  else
    {
      loc = 0;
      finamv = NULL;
      lineno = 0;
    }
  if (loc)
    {
      const char *cstr = basilys_string_str ((basilys_ptr_t) strv);
      if (cstr)
	error_at (loc, "Basilys Error[#%ld]: %s - %s", basilys_dbgcounter,
		  msg, cstr);
      else
	error_at (loc, "Basilys Error[#%ld]: %s", basilys_dbgcounter, msg);
    }
  else
    {
      const char *cfilnam = basilys_string_str ((basilys_ptr_t) finamv);
      const char *cstr = basilys_string_str ((basilys_ptr_t) strv);
      if (cfilnam)
	{
	  if (cstr)
	    error ("Basilys Error[#%ld] @ %s:%d: %s - %s", basilys_dbgcounter,
		   cfilnam, lineno, msg, cstr);
	  else
	    error ("Basilys Error[#%ld] @ %s:%d: %s", basilys_dbgcounter,
		   cfilnam, lineno, msg);
	}
      else
	{
	  if (cstr)
	    error ("Basilys Error[#%ld]: %s - %s", basilys_dbgcounter, msg,
		   cstr);
	  else
	    error ("Basilys Error[#%ld]: %s", basilys_dbgcounter, msg);
	}
    }
  BASILYS_EXITFRAME ();
}

#undef mixlocv
#undef strv
#undef finamv


void
basilys_warning_str (int opt, basilys_ptr_t mixloc_p, const char *msg,
		     basilys_ptr_t str_p)
{
  int mixmag = 0;
  int lineno = 0;
  location_t loc = 0;
  BASILYS_ENTERFRAME (3, NULL);
#define mixlocv    curfram__.varptr[0]
#define strv       curfram__.varptr[1]
#define finamv     curfram__.varptr[2]
  gcc_assert (msg && msg[0]);
  mixlocv = mixloc_p;
  strv = str_p;
  mixmag = basilys_magic_discr ((basilys_ptr_t) mixlocv);
  if (mixmag == OBMAG_MIXLOC)
    {
      loc = basilys_location_mixloc ((basilys_ptr_t) mixlocv);
      finamv = basilys_val_mixloc ((basilys_ptr_t) mixlocv);
      lineno = basilys_num_mixloc ((basilys_ptr_t) mixlocv);
    }
  else if (mixmag == OBMAG_MIXINT)
    {
      loc = 0;
      finamv = basilys_val_mixint ((basilys_ptr_t) mixlocv);
      lineno = basilys_num_mixint ((basilys_ptr_t) mixlocv);
    }
  else
    {
      loc = 0;
      finamv = NULL;
      lineno = 0;
    }
  if (loc)
    {
      const char *cstr = basilys_string_str ((basilys_ptr_t) strv);
      if (cstr)
	warning_at (loc, opt, "Basilys Warning[#%ld]: %s - %s",
		    basilys_dbgcounter, msg, cstr);
      else
	warning_at (loc, opt, "Basilys Warning[#%ld]: %s", 
		    basilys_dbgcounter, msg);
    }
  else
    {
      const char *cfilnam = basilys_string_str ((basilys_ptr_t) finamv);
      const char *cstr = basilys_string_str ((basilys_ptr_t) strv);
      if (cfilnam)
	{
	  if (cstr)
	    warning (opt, "Basilys Warning[#%ld] @ %s:%d: %s - %s",
		     basilys_dbgcounter, cfilnam, lineno, msg, cstr);
	  else
	    warning (opt, "Basilys Warning[#%ld] @ %s:%d: %s",
		     basilys_dbgcounter, cfilnam, lineno, msg);
	}
      else
	{
	  if (cstr)
	    warning (opt, "Basilys Warning[#%ld]: %s - %s",
		     basilys_dbgcounter, msg, cstr);
	  else
	    warning (opt, "Basilys Warning[#%ld]: %s", basilys_dbgcounter,
		     msg);
	}
    }
  BASILYS_EXITFRAME ();
}

#undef mixlocv
#undef strv
#undef finamv



void
basilys_inform_str (basilys_ptr_t mixloc_p, const char *msg,
		    basilys_ptr_t str_p)
{
  int mixmag = 0;
  int lineno = 0;
  location_t loc = 0;
  BASILYS_ENTERFRAME (3, NULL);
#define mixlocv    curfram__.varptr[0]
#define strv       curfram__.varptr[1]
#define finamv     curfram__.varptr[2]
  gcc_assert (msg && msg[0]);
  mixlocv = mixloc_p;
  strv = str_p;
  mixmag = basilys_magic_discr ((basilys_ptr_t) mixlocv);
  if (mixmag == OBMAG_MIXLOC)
    {
      loc = basilys_location_mixloc ((basilys_ptr_t) mixlocv);
      finamv = basilys_val_mixloc ((basilys_ptr_t) mixlocv);
      lineno = basilys_num_mixloc ((basilys_ptr_t) mixlocv);
    }
  else if (mixmag == OBMAG_MIXINT)
    {
      loc = 0;
      finamv = basilys_val_mixint ((basilys_ptr_t) mixlocv);
      lineno = basilys_num_mixint ((basilys_ptr_t) mixlocv);
    }
  else
    {
      loc = 0;
      finamv = NULL;
      lineno = 0;
    }
  if (loc)
    {
      const char *cstr = basilys_string_str ((basilys_ptr_t) strv);
      if (cstr)
	inform (loc, "Basilys Inform[#%ld]: %s - %s", basilys_dbgcounter,
		msg, cstr);
      else
	inform (loc, "Basilys Inform[#%ld]: %s", basilys_dbgcounter, msg);
    }
  else
    {
      const char *cfilnam = basilys_string_str ((basilys_ptr_t) finamv);
      const char *cstr = basilys_string_str ((basilys_ptr_t) strv);
      if (cfilnam)
	{
	  if (cstr)
	    inform (UNKNOWN_LOCATION, "Basilys Inform[#%ld] @ %s:%d: %s - %s",
		    basilys_dbgcounter, cfilnam, lineno, msg, cstr);
	  else
	    inform (UNKNOWN_LOCATION, "Basilys Inform[#%ld] @ %s:%d: %s",
		    basilys_dbgcounter, cfilnam, lineno, msg);
	}
      else
	{
	  if (cstr)
	    inform (UNKNOWN_LOCATION, "Basilys Inform[#%ld]: %s - %s",
		    basilys_dbgcounter, msg, cstr);
	  else
	    inform (UNKNOWN_LOCATION, "Basilys Inform[#%ld]: %s",
		    basilys_dbgcounter, msg);
	}
    }
  BASILYS_EXITFRAME ();
}

#undef mixlocv
#undef strv
#undef finamv




basilys_ptr_t
basilysgc_read_file (const char *filnam, const char *locnam)
{
  struct reading_st rds;
  FILE *fil = 0;
  struct reading_st *rd = 0;
  char *filnamdup = 0;
  BASILYS_ENTERFRAME (4, NULL);
#define genv      curfram__.varptr[0]
#define valv      curfram__.varptr[1]
#define locnamv   curfram__.varptr[2]
#define seqv      curfram__.varptr[3]
  memset (&rds, 0, sizeof (rds));
  if (!filnam)
    goto end;
  if (!locnam || !locnam[0])
    locnam = basename (filnam);
  filnamdup = xstrdup (filnam);	/* filnamdup is never freed */
  debugeprintf ("basilysgc_read_file filnam %s locnam %s", filnam, locnam);
  fil = fopen (filnam, "rt");
  if (!fil)
    fatal_error ("cannot open basilys file %s - %m", filnam);
  /*  debugeprintf ("starting loading file %s", filnamdup); */
  rds.rfil = fil;
  rds.rpath = filnam;
  rds.rlineno = 0;
  linemap_add (line_table, LC_RENAME, false, filnamdup, 0);
  rd = &rds;
  locnamv = basilysgc_new_stringdup (BASILYSGOB (DISCR_STRING), locnam);
  rds.rpfilnam = (basilys_ptr_t *) & locnamv;
  rds.rpgenv = (basilys_ptr_t *) & genv;
  seqv = basilysgc_new_list (BASILYSGOB (DISCR_LIST));
  while (!rdeof ())
    {
      bool got = FALSE;
      skipspace_getc (rd, COMMENT_SKIP);
      if (rdeof ())
	break;
      valv = readval (rd, &got);
      if (!got)
	READ_ERROR ("MELT: no value read %.20s", &rdcurc ());
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) valv);
    };
  rd = 0;
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) seqv;
#undef vecshv
#undef genv
#undef locnamv
#undef seqv
}


basilys_ptr_t
basilysgc_read_from_rawstring (const char *rawstr, const char *locnam,
			       location_t loch)
{
  struct reading_st rds;
  char *rbuf = 0;
  struct reading_st *rd = 0;
  BASILYS_ENTERFRAME (4, NULL);
#define genv      curfram__.varptr[0]
#define valv      curfram__.varptr[1]
#define locnamv   curfram__.varptr[2]
#define seqv      curfram__.varptr[3]
  memset (&rds, 0, sizeof (rds));
  if (!rawstr)
    goto end;
  rbuf = xstrdup (rawstr);
  rds.rfil = 0;
  rds.rpath = 0;
  rds.rlineno = 0;
  rds.rcurlin = rbuf;
  rds.rsrcloc = loch;
  rd = &rds;
  if (locnam)
    locnamv = basilysgc_new_stringdup (BASILYSGOB (DISCR_STRING), locnam);
  seqv = basilysgc_new_list (BASILYSGOB (DISCR_LIST));
  rds.rpfilnam = (basilys_ptr_t *) & locnamv;
  rds.rpgenv = (basilys_ptr_t *) & genv;
  while (rdcurc ())
    {
      bool got = FALSE;
      skipspace_getc (rd, COMMENT_SKIP);
      if (!rdcurc () || rdeof ())
	break;
      valv = readval (rd, &got);
      if (!got)
	READ_ERROR ("MELT: no value read %.20s", &rdcurc ());
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) valv);
    };
  rd = 0;
  free (rbuf);
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) seqv;
#undef vecshv
#undef genv
#undef locnamv
#undef seqv
}


basilys_ptr_t
basilysgc_read_from_val (basilys_ptr_t strv_p, basilys_ptr_t locnam_p)
{
  struct reading_st rds;
  char *rbuf = 0;
  struct reading_st *rd = 0;
  int strmagic = 0;
  BASILYS_ENTERFRAME (5, NULL);
#define genv      curfram__.varptr[0]
#define valv      curfram__.varptr[1]
#define locnamv   curfram__.varptr[2]
#define seqv      curfram__.varptr[3]
#define strv      curfram__.varptr[4]
  memset (&rds, 0, sizeof (rds));
  strv = strv_p;
  locnamv = locnam_p;
  rbuf = 0;
  strmagic = basilys_magic_discr ((basilys_ptr_t) strv);
  switch (strmagic)
    {
    case OBMAG_STRING:
      rbuf = (char *) xstrdup (basilys_string_str ((basilys_ptr_t) strv));
      break;
    case OBMAG_STRBUF:
      rbuf = xstrdup (basilys_strbuf_str ((basilys_ptr_t) strv));
      break;
    case OBMAG_OBJECT:
      if (basilys_is_instance_of
	  ((basilys_ptr_t) strv, BASILYSG (CLASS_NAMED)))
	strv = basilys_object_nth_field ((basilys_ptr_t) strv, FNAMED_NAME);
      else
	strv = NULL;
      if (basilys_string_str ((basilys_ptr_t) strv))
	rbuf = xstrdup (basilys_string_str ((basilys_ptr_t) strv));
      break;
    default:
      break;
    }
  if (!rbuf)
    goto end;
  rds.rfil = 0;
  rds.rpath = 0;
  rds.rlineno = 0;
  rds.rcurlin = rbuf;
  rd = &rds;
  rds.rpfilnam = (basilys_ptr_t *) & locnamv;
  rds.rpgenv = (basilys_ptr_t *) & genv;
  while (rdcurc ())
    {
      bool got = FALSE;
      skipspace_getc (rd, COMMENT_SKIP);
      if (!rdcurc () || rdeof ())
	break;
      valv = readval (rd, &got);
      if (!got)
	READ_ERROR ("MELT: no value read %.20s", &rdcurc ());
      basilysgc_append_list ((basilys_ptr_t) seqv, (basilys_ptr_t) valv);
    };
  rd = 0;
  free (rbuf);
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) seqv;
#undef vecshv
#undef genv
#undef locnamv
#undef seqv
#undef strv
#undef valv
}

static void
do_initial_command (basilys_ptr_t modata_p)
{
  BASILYS_ENTERFRAME (8, NULL);
#define dictv     curfram__.varptr[0]
#define closv     curfram__.varptr[1]
#define cstrv     curfram__.varptr[2]
#define arglv     curfram__.varptr[3]
#define csecstrv  curfram__.varptr[4]
#define modatav   curfram__.varptr[5]
#define curargv   curfram__.varptr[6]
#define resv      curfram__.varptr[7]
  modatav = modata_p;
  debugeprintf ("do_initial_command mode_string %s modatav %p",
		basilys_mode_string, (void *) modatav);
  if (basilys_magic_discr
      ((BASILYSG (INITIAL_SYSTEM_DATA))) != OBMAG_OBJECT
      || BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_len <
      FSYSDAT_CMD_FUNDICT + 1
      || !BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_vartab
      || !basilys_mode_string || !basilys_mode_string[0])
    goto end;
  dictv = BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_vartab[FSYSDAT_CMD_FUNDICT];
  debugeprintf ("do_initial_command dictv=%p", dictv);
  debugeprintvalue ("do_initial_command dictv", dictv);
  if (basilys_magic_discr ((basilys_ptr_t) dictv) != OBMAG_MAPSTRINGS)
    goto end;
  closv =
    basilys_get_mapstrings ((struct basilysmapstrings_st *) dictv,
			    basilys_mode_string);
  debugeprintf ("do_initial_command closv=%p", closv);
  if (basilys_magic_discr ((basilys_ptr_t) closv) != OBMAG_CLOSURE)
    {
      error ("no closure for basilys command %s", basilys_mode_string);
      goto end;
    };
  debugeprintf ("do_initial_command argument_string %s",
		basilys_argument_string);
  debugeprintf ("do_initial_command arglist_string %s",
		basilys_arglist_string);
  debugeprintf ("do_initial_command secondargument_string %s",
		basilys_secondargument_string);
  if (basilys_argument_string && basilys_argument_string[0]
      && basilys_arglist_string && basilys_arglist_string[0])
    {
      error
	("cannot have both -fmelt-arg=%s & -fmelt-arglist=%s given as program arguments",
	 basilys_argument_string, basilys_arglist_string);
      goto end;
    }
  {
    union basilysparam_un pararg[3];
    memset (pararg, 0, sizeof (pararg));
    if (basilys_argument_string && basilys_argument_string[0])
      {
	cstrv =
	  basilysgc_new_string (BASILYSGOB (DISCR_STRING),
				basilys_argument_string);
	pararg[0].bp_aptr = (basilys_ptr_t *) & cstrv;
      }
    else if (basilys_arglist_string && basilys_arglist_string[0])
      {
	char *comma = 0;
	char *pc = 0;
	arglv = basilysgc_new_list (BASILYSGOB (DISCR_LIST));
	for (pc = CONST_CAST(char *, basilys_arglist_string); pc;
	     pc = comma ? (comma + 1) : 0)
	  {
	    comma = strchr (pc, ',');
	    if (comma)
	      *comma = (char) 0;
	    curargv = basilysgc_new_string (BASILYSGOB (DISCR_STRING), pc);
	    if (comma)
	      *comma = ',';
	    basilysgc_append_list ((basilys_ptr_t) arglv,
				   (basilys_ptr_t) curargv);
	  }
	pararg[0].bp_aptr = (basilys_ptr_t *) & arglv;
      };
    if (basilys_secondargument_string && basilys_secondargument_string[0])
      {
	csecstrv =
	  basilysgc_new_string (BASILYSGOB (DISCR_STRING),
				basilys_secondargument_string);
	pararg[1].bp_aptr = (basilys_ptr_t *) & csecstrv;
      }
    else
      {
	debugeprintf ("do_initial_command no second argument %p",
		      basilys_secondargument_string);
	csecstrv = NULL;
	pararg[1].bp_aptr = (basilys_ptr_t *) 0;
      }
    pararg[2].bp_aptr = (basilys_ptr_t *) & modatav;
    debugeprintf ("do_initial_command before apply closv %p", closv);
    BASILYS_LOCATION_HERE ("do_initial_command before apply");
    resv = basilys_apply ((basilysclosure_ptr_t) closv,
			  (basilys_ptr_t) BASILYSG
			  (INITIAL_SYSTEM_DATA),
			  BPARSTR_PTR BPARSTR_PTR BPARSTR_PTR, pararg, "",
			  NULL);
    debugeprintf ("do_initial_command after apply closv %p resv %p", closv,
		  resv);
    exit_after_options = (resv == NULL);
  }
end:
  debugeprintf ("do_initial_command end %s", basilys_argument_string);
  BASILYS_EXITFRAME ();
#undef dictv
#undef closv
#undef cstrv
#undef csecstrv
#undef modatav
#undef arglv
#undef curargv
#undef resv
}


/****
 * load the initial modules do the initial command if needed
 * the initstring is a semi-colon separated list of module names
 * and do the initial command
 ****/

static void
load_basilys_modules_and_do_command (void)
{
  char *dupmodpath = 0;
  char *curmod = 0;
  char *nextmod = 0;
  BASILYS_ENTERFRAME (2, NULL);
#define modatv curfram__.varptr[0]
  debugeprintf ("load_initial_basilys_modules start init=%s command=%s",
		basilys_init_string, basilys_mode_string);
  /* if there is no -fmelt-init use the default list of modules */
  if (!basilys_init_string || !basilys_init_string[0])
  {
    basilys_init_string = "@" MELT_DEFAULT_MODLIS;
    debugeprintf("basilys_init_string set to default %s", basilys_init_string);
  }
  dupmodpath = xstrdup (basilys_init_string);
  if (flag_basilys_debug)
    {
      fflush (stderr);
#define modatv curfram__.varptr[0]
      dump_file = stderr;
      fflush (stderr);
    }
#if ENABLE_CHECKING
  if (flag_basilys_debug)
    {
      char *tracenam = getenv ("BASILYSTRACE");
      if (tracenam)
	basilys_dbgtracefile = fopen (tracenam, "w");
      if (basilys_dbgtracefile)
	{
	  time_t now = 0;
	  time (&now);
	  debugeprintf ("load_basilys_modules_and_do_command dbgtracefile %s",
			tracenam);
	  fprintf (basilys_dbgtracefile, "**BASILYS TRACE %s pid %d at %s",
		   tracenam, (int) getpid (), ctime (&now));
	  fflush (basilys_dbgtracefile);
	}
    }
#endif
  curmod = dupmodpath;
  modatv = NULL;
  /**
   * first we load all the initial modules 
   **/
  while (curmod && curmod[0])
    {
      /* modules are separated by a semicolon ';' - this should be
         acceptable on Unixes and even Windows */
      nextmod = strchr (curmod, ';');
#if !HAVE_DOS_BASED_FILE_SYSTEM
      /* for convenience, on non DOS based systems like Unix-es and
         Linux, we also accept the colon ':' */
      if (!nextmod)
	nextmod = strchr (curmod, ':');
#endif
      if (nextmod)
	{
	  *nextmod = (char) 0;
	  nextmod++;
	}
      debugeprintf ("load_initial_basilys_modules curmod %s before", curmod);
      BASILYS_LOCATION_HERE
	("load_initial_basilys_modules before compile_dyn");
      if (curmod[0] == '@' && curmod[1])
	{
	  /* read the file which contains a list of modules, one per
	     non empty, non comment line */
	  modatv =
	    basilysgc_load_modulelist ((basilys_ptr_t) modatv, curmod + 1);
	  debugeprintf
	    ("load_initial_basilys_modules curmod %s loaded modulist %p",
	     curmod, (void *) modatv);
	}
      else
	{
	  modatv = basilysgc_load_melt_module ((basilys_ptr_t) modatv, curmod);
	  debugeprintf
	    ("load_initial_basilys_modules curmod %s loaded modatv %p",
	     curmod, (void *) modatv);
	}
      curmod = nextmod;
    }
  /**
   * then we do the command if needed 
   **/
  /* the command exit is builtin */
  if (basilys_mode_string && !strcmp (basilys_mode_string, "exit"))
    exit_after_options = true;
  /* other commands */
  else if (basilys_magic_discr
	   ((BASILYSG (INITIAL_SYSTEM_DATA))) == OBMAG_OBJECT
	   && BASILYSGOB (INITIAL_SYSTEM_DATA)->obj_len >=
	   FSYSDAT_CMD_FUNDICT && basilys_mode_string
	   && basilys_mode_string[0])
    {
      debugeprintf
	("load_basilys_modules_and_do_command sets exit_after_options for command %s",
	 basilys_mode_string);
      BASILYS_LOCATION_HERE
	("load_initial_basilys_modules before do_initial_command");
      do_initial_command ((basilys_ptr_t) modatv);
      debugeprintf
	("load_basilys_modules_and_do_command after do_initial_command  command_string %s",
	 basilys_mode_string);
      if (dump_file == stderr && flag_basilys_debug)
	{
	  debugeprintf
	    ("load_basilys_modules_and_do_command dump_file cleared was %p",
	     (void *) dump_file);
	  fflush (dump_file);
	  dump_file = 0;
	}
    }
  else if (basilys_mode_string)
    fatal_error ("basilys with command string %s without command dispatcher",
		 basilys_mode_string);
  debugeprintf
    ("load_basilys_modules_and_do_command ended with %ld GarbColl, %ld fullGc",
     basilys_nb_garbcoll, basilys_nb_full_garbcoll);
#if ENABLE_CHECKING
  if (basilys_dbgtracefile)
    {
      fprintf (basilys_dbgtracefile, "\n**END TRACE\n");
      fclose (basilys_dbgtracefile);
      basilys_dbgtracefile = NULL;
    }
#endif
  free (dupmodpath);
  debugeprintf
    ("load_basilys_modules_and_do_command done modules %s command %s",
     basilys_init_string, basilys_mode_string);
  BASILYS_EXITFRAME ();
#undef modatv
}


static void basilys_ppl_error_handler(enum ppl_enum_error_code err, const char* descr);



/* handle a "melt" attribute
 */
static tree
handle_melt_attribute(tree *node, tree name,
		      tree args,
		      int flag ATTRIBUTE_UNUSED,
		      bool *no_add_attrs ATTRIBUTE_UNUSED)
{
  tree decl = *node;
  tree id = 0;
  const char* attrstr = 0;
  id = TREE_VALUE (args);
  if (TREE_CODE (id) != STRING_CST)
    {
      error ("melt attribute argument not a string");
      return NULL_TREE;
    }
  attrstr = TREE_STRING_POINTER (id);
  basilys_handle_melt_attribute (decl, name, attrstr, input_location);
  return NULL_TREE;
}

static struct attribute_spec 
melt_attr_spec =
  { "melt",                   1, 1, true, false, false,
    handle_melt_attribute };


/* the plugin callback to register melt attributes */
static void 
melt_attribute_callback(void *gcc_data ATTRIBUTE_UNUSED,
			void* user_data ATTRIBUTE_UNUSED) 
{
  register_attribute(&melt_attr_spec);
}

/****
 * Initialize basilys.  Called from toplevel.c before pass management.
 * Should become the MELT plugin initializer.
 ****/
void
basilys_initialize (void)
{
  static int inited;
  long seed;
  const char *pc;
  const char *randomseed = 0;
  if (inited)
    return;
  modinfvec = VEC_alloc (basilys_module_info_t, heap, 32);
  /* don't use the index 0 so push a null */
  VEC_safe_push (basilys_module_info_t, heap, modinfvec,
		 (basilys_module_info_t *) 0);
  proghandle = lt_dlopen (NULL);
  if (!proghandle)
    fatal_error ("basilys failed to get whole program handle - %s",
		 lt_dlerror ());
  if (count_basilys_debugskip_string != (char *) 0)
    basilys_debugskipcount = atol (count_basilys_debugskip_string);
  seed = 0;
  randomseed = get_random_seed (false);
  gcc_assert (randomseed != (char *) 0);
  gcc_assert (BASILYS_ALIGN == sizeof (void *)
	      || BASILYS_ALIGN == 2 * sizeof (void *)
	      || BASILYS_ALIGN == 4 * sizeof (void *));
  inited = 1;
  ggc_collect ();
  obstack_init (&bstring_obstack);
  obstack_init (&bname_obstack);
  for (pc = randomseed; *pc; pc++)
    seed ^= (seed << 6) + (*pc);
  srand48 (seed);
  gcc_assert (!basilys_curalz);
  {
    size_t wantedwords = MINOR_SIZE_KILOWORD * 4096;
    if (wantedwords < (1 << 20))
      wantedwords = (1 << 20);
    gcc_assert (basilys_startalz == NULL && basilys_endalz == NULL);
    gcc_assert (wantedwords * sizeof (void *) >
		300 * BGLOB__LASTGLOB * sizeof (struct basilysobject_st));
    basilys_curalz = (char *) xcalloc (sizeof (void *), wantedwords);
    basilys_startalz = basilys_curalz;
    basilys_endalz = (char *) basilys_curalz + wantedwords * sizeof (void *);
    basilys_storalz = ((void **) basilys_endalz) - 2;
    basilys_newspeclist = NULL;
    basilys_oldspeclist = NULL;
    debugeprintf ("basilys_initialize alloczon %p - %p (%ld Kw)",
		  basilys_startalz, basilys_endalz, (long) wantedwords >> 10);
  }
  /* we are using register_callback here, even if MELT is not yet a
     plugin; we hope that MELT would become a plugin. */
  register_callback (MELT_PLUGIN_NAME, PLUGIN_GGC_MARKING, 
		     basilys_marking_callback,
		     NULL);
  register_callback (MELT_PLUGIN_NAME, PLUGIN_ATTRIBUTES,
		     melt_attribute_callback,
		     NULL);
  debugeprintf ("basilys_initialize cpp_PREFIX=%s", cpp_PREFIX);
  debugeprintf ("basilys_initialize cpp_EXEC_PREFIX=%s", cpp_EXEC_PREFIX);
  debugeprintf ("basilys_initialize gcc_exec_prefix=%s", gcc_exec_prefix);
  debugeprintf ("basilys_initialize melt_private_include_dir=%s",
		melt_private_include_dir);
  debugeprintf ("basilys_initialize melt_source_dir=%s", melt_source_dir);
  debugeprintf ("basilys_initialize melt_generated_dir=%s",
		melt_generated_dir);
  debugeprintf ("basilys_initialize melt_dynlib_dir=%s", melt_dynlib_dir);
  debugeprintf ("basilys_initialize basilys_init_string=%s", basilys_init_string);
  if (ppl_set_error_handler(basilys_ppl_error_handler))
    fatal_error ("failed to set PPL handler");
  load_basilys_modules_and_do_command ();
  debugeprintf ("basilys_initialize ended init=%s command=%s",
		basilys_init_string, basilys_mode_string);
}


static void
do_finalize_basilys (void)
{
  BASILYS_ENTERFRAME (1, NULL);
#define finclosv curfram__.varptr[0]
  finclosv =
    basilys_field_object ((basilys_ptr_t) BASILYSGOB (INITIAL_SYSTEM_DATA),
			  FSYSDAT_EXIT_FINALIZER);
  if (basilys_magic_discr ((basilys_ptr_t) finclosv) == OBMAG_CLOSURE)
    {
      BASILYS_LOCATION_HERE
	("do_finalize_basilys before applying final closure");
      (void) basilys_apply ((basilysclosure_ptr_t) finclosv,
			    (basilys_ptr_t) NULL, "", NULL, "", NULL);
      basilys_garbcoll (0, BASILYS_NEED_FULL);
    }
  BASILYS_EXITFRAME ();
#undef finclosv
}

int *
basilys_dynobjstruct_fieldoffset_at (const char *fldnam, const char *fil,
				     int lin)
{
  char *nam = 0;
  void *ptr = 0;
  nam = concat ("fieldoff__", fldnam, NULL);
  ptr = basilys_dlsym_all (nam);
  if (!ptr)
    fatal_error ("basilys failed to find field offset %s - %s (%s:%d)", nam,
		 lt_dlerror (), fil, lin);
  free (nam);
  return (int *) ptr;
}


int *
basilys_dynobjstruct_classlength_at (const char *clanam, const char *fil,
				     int lin)
{
  char *nam = 0;
  void *ptr = 0;
  nam = concat ("classlen__", clanam, NULL);
  ptr = basilys_dlsym_all (nam);
  if (!ptr)
    fatal_error ("basilys failed to find class length %s - %s (%s:%d)", nam,
		 lt_dlerror (), fil, lin);
  free (nam);
  return (int *) ptr;
}


typedef char *char_p;

DEF_VEC_P (char_p);
DEF_VEC_ALLOC_P (char_p, heap);

/****
 * finalize basilys. Called from toplevel.c after all is done
 ****/
void
basilys_finalize (void)
{
  do_finalize_basilys ();
  debugeprintf ("basilys_finalize with %ld GarbColl, %ld fullGc",
		basilys_nb_garbcoll, basilys_nb_full_garbcoll);
  if (gdbm_basilys)
    {
      gdbm_close (gdbm_basilys);
      gdbm_basilys = NULL;
    }
  if (tempdir_basilys[0])
    {
      DIR *tdir = opendir (tempdir_basilys);
      VEC (char_p, heap) * dirvec = 0;
      struct dirent *dent = 0;
      if (!tdir)
	fatal_error ("failed to open tempdir %s %m", tempdir_basilys);
      dirvec = VEC_alloc (char_p, heap, 30);
      while ((dent = readdir (tdir)) != NULL)
	{
	  if (dent->d_name[0] && dent->d_name[0] != '.')
	    /* this skips  '.' & '..' and we have no  .* file */
	    VEC_safe_push (char_p, heap, dirvec,
			   concat (tempdir_basilys, "/", dent->d_name, NULL));
	}
      closedir (tdir);
      while (!VEC_empty (char_p, dirvec))
	{
	  char *tfilnam = VEC_pop (char_p, dirvec);
	  debugeprintf ("basilys_finalize remove file %s", tfilnam);
	  remove (tfilnam);
	  free (tfilnam);
	};
      VEC_free (char_p, heap, dirvec);
    }
  if (made_tempdir_basilys && tempdir_basilys[0])
    {
      errno = 0;
      if (rmdir (tempdir_basilys))
	/* @@@ I don't know if it should be a warning or a fatal error -
	   we are finalizing! */
	warning (0, "failed to rmdir basilys tempdir %s (%s)",
		 tempdir_basilys, strerror (errno));
    }
}




static void
discr_out (struct debugprint_basilys_st *dp, basilysobject_ptr_t odiscr)
{
  int dmag = basilys_magic_discr ((basilys_ptr_t) odiscr);
  struct basilysstring_st *str = NULL;
  if (dmag != OBMAG_OBJECT)
    {
      fprintf (dp->dfil, "?discr@%p?", (void *) odiscr);
      return;
    }
  if (odiscr->obj_len >= FNAMED__LAST && odiscr->obj_vartab)
    {
      str = (struct basilysstring_st *) odiscr->obj_vartab[FNAMED_NAME];
      if (basilys_magic_discr ((basilys_ptr_t) str) != OBMAG_STRING)
	str = NULL;
    }
  if (!str)
    {
      fprintf (dp->dfil, "?odiscr/%d?", odiscr->obj_hash);
      return;
    }
  fprintf (dp->dfil, "#%s", str->val);
}


static void
nl_debug_out (struct debugprint_basilys_st *dp, int depth)
{
  int i;
  putc ('\n', dp->dfil);
  for (i = 0; i < depth; i++)
    putc (' ', dp->dfil);
}

static void
skip_debug_out (struct debugprint_basilys_st *dp, int depth)
{
  if (dp->dcount % 4 == 0)
    nl_debug_out (dp, depth);
  else
    putc (' ', dp->dfil);
}


static bool
is_named_obj (basilysobject_ptr_t ob)
{
  struct basilysstring_st *str = 0;
  if (basilys_magic_discr ((basilys_ptr_t) ob) != OBMAG_OBJECT)
    return FALSE;
  if (ob->obj_len < FNAMED__LAST || !ob->obj_vartab)
    return FALSE;
  str = (struct basilysstring_st *) ob->obj_vartab[FNAMED_NAME];
  if (basilys_magic_discr ((basilys_ptr_t) str) != OBMAG_STRING)
    return FALSE;
  if (basilys_is_instance_of ((basilys_ptr_t) ob, BASILYSG (CLASS_NAMED)))
    return TRUE;
  return FALSE;
}

static void
debug_outstr (struct debugprint_basilys_st *dp, const char *str)
{
  int nbclin = 0;
  const char *pc;
  for (pc = str; *pc; pc++)
    {
      nbclin++;
      if (nbclin > 60 && strlen (pc) > 5)
	{
	  if (ISSPACE (*pc) || ISPUNCT (*pc) || nbclin > 72)
	    {
	      fputs ("\\\n", dp->dfil);
	      nbclin = 0;
	    }
	}
      switch (*pc)
	{
	case '\n':
	  fputs ("\\n", dp->dfil);
	  break;
	case '\r':
	  fputs ("\\r", dp->dfil);
	  break;
	case '\t':
	  fputs ("\\t", dp->dfil);
	  break;
	case '\v':
	  fputs ("\\v", dp->dfil);
	  break;
	case '\f':
	  fputs ("\\f", dp->dfil);
	  break;
	case '\"':
	  fputs ("\\q", dp->dfil);
	  break;
	case '\'':
	  fputs ("\\a", dp->dfil);
	  break;
	default:
	  if (ISPRINT (*pc))
	    putc (*pc, dp->dfil);
	  else
	    fprintf (dp->dfil, "\\x%02x", (*pc) & 0xff);
	  break;
	}
    }
}


void
basilys_debug_out (struct debugprint_basilys_st *dp,
		   basilys_ptr_t ptr, int depth)
{
  int mag = basilys_magic_discr (ptr);
  int ix;
  if (!dp->dfil)
    return;
  dp->dcount++;
  switch (mag)
    {
    case 0:
      {
	if (ptr)
	  fprintf (dp->dfil, "??@%p??", (void *) ptr);
	else
	  fputs ("@@", dp->dfil);
	break;
      }
    case OBMAG_OBJECT:
      {
	struct basilysobject_st *p = (struct basilysobject_st *) ptr;
	bool named = is_named_obj (p);
	fputs ("%", dp->dfil);
	discr_out (dp, p->obj_class);
	fprintf (dp->dfil, "/L%dH%d", p->obj_len, p->obj_hash);
	if (p->obj_num)
	  fprintf (dp->dfil, "N%d", p->obj_num);
#if ENABLE_CHECKING
	if (p->obj_serial)
	  fprintf (dp->dfil, "S##%ld", p->obj_serial);
#endif
	if (named)
	  fprintf (dp->dfil, "<#%s>",
		   ((struct basilysstring_st *) (p->obj_vartab
						 [FNAMED_NAME]))->val);
	if ((!named || depth == 0) && depth < dp->dmaxdepth)
	  {
	    fputs ("[", dp->dfil);
	    if (p->obj_vartab)
	      for (ix = 0; ix < p->obj_len; ix++)
		{
		  if (ix > 0)
		    skip_debug_out (dp, depth);
		  basilys_debug_out (dp, p->obj_vartab[ix], depth + 1);
		}
	    fputs ("]", dp->dfil);
	  }
	else if (!named)
	  fputs ("..", dp->dfil);
	break;
      }
    case OBMAG_MULTIPLE:
      {
	struct basilysmultiple_st *p = (struct basilysmultiple_st *) ptr;
	fputs ("*", dp->dfil);
	discr_out (dp, p->discr);
	if (depth < dp->dmaxdepth)
	  {
	    fputs ("(", dp->dfil);
	    for (ix = 0; ix < (int) p->nbval; ix++)
	      {
		if (ix > 0)
		  skip_debug_out (dp, depth);
		basilys_debug_out (dp, p->tabval[ix], depth + 1);
	      }
	    fputs (")", dp->dfil);
	  }
	else
	  fputs ("..", dp->dfil);
	break;
      }
    case OBMAG_STRING:
      {
	struct basilysstring_st *p = (struct basilysstring_st *) ptr;
	fputs ("!", dp->dfil);
	discr_out (dp, p->discr);
	if (depth < dp->dmaxdepth)
	  {
	    fputs ("\"", dp->dfil);
	    debug_outstr (dp, p->val);
	    fputs ("\"", dp->dfil);
	  }
	else
	  fputs ("..", dp->dfil);
	break;
      }
    case OBMAG_INT:
      {
	struct basilysint_st *p = (struct basilysint_st *) ptr;
	fputs ("!", dp->dfil);
	discr_out (dp, p->discr);
	fprintf (dp->dfil, "#%ld", p->val);
	break;
      }
    case OBMAG_MIXINT:
      {
	struct basilysmixint_st *p = (struct basilysmixint_st *) ptr;
	fputs ("!", dp->dfil);
	discr_out (dp, p->discr);
	fprintf (dp->dfil, "[#%ld&", p->intval);
	basilys_debug_out (dp, p->ptrval, depth + 1);
	fputs ("]", dp->dfil);
	break;
      }
    case OBMAG_MIXLOC:
      {
	struct basilysmixloc_st *p = (struct basilysmixloc_st *) ptr;
	fputs ("!", dp->dfil);
	discr_out (dp, p->discr);
	fprintf (dp->dfil, "[#%ld&", p->intval);
	basilys_debug_out (dp, p->ptrval, depth + 1);
	fputs ("]", dp->dfil);
	break;
      }
    case OBMAG_LIST:
      {
	struct basilyslist_st *p = (struct basilyslist_st *) ptr;
	fputs ("!", dp->dfil);
	discr_out (dp, p->discr);
	if (depth < dp->dmaxdepth)
	  {
	    int ln = basilys_list_length ((basilys_ptr_t) p);
	    struct basilyspair_st *pr = 0;
	    if (ln > 2)
	      fprintf (dp->dfil, "[/%d ", ln);
	    else
	      fputs ("[", dp->dfil);
	    for (pr = p->first;
		 pr && basilys_magic_discr ((basilys_ptr_t) pr) == OBMAG_PAIR;
		 pr = pr->tl)
	      {
		basilys_debug_out (dp, pr->hd, depth + 1);
		if (pr->tl)
		  skip_debug_out (dp, depth);
	      }
	    fputs ("]", dp->dfil);
	  }
	else
	  fputs ("..", dp->dfil);
	break;
      }
    case OBMAG_MAPSTRINGS:
      {
	struct basilysmapstrings_st *p = (struct basilysmapstrings_st *) ptr;
	fputs ("|", dp->dfil);
	discr_out (dp, p->discr);
	if (depth < dp->dmaxdepth)
	  {
	    int ln = basilys_primtab[p->lenix];
	    fprintf (dp->dfil, "{~%d/", p->count);
	    if (p->entab)
	      for (ix = 0; ix < ln; ix++)
		{
		  const char *ats = p->entab[ix].e_at;
		  if (!ats || ats == HTAB_DELETED_ENTRY)
		    continue;
		  nl_debug_out (dp, depth);
		  fputs ("'", dp->dfil);
		  debug_outstr (dp, ats);
		  fputs ("' = ", dp->dfil);
		  basilys_debug_out (dp, p->entab[ix].e_va, depth + 1);
		  fputs (";", dp->dfil);
		}
	    fputs (" ~}", dp->dfil);
	  }
	else
	  fputs ("..", dp->dfil);
	break;
      }
    case OBMAG_MAPOBJECTS:
      {
	struct basilysmapobjects_st *p = (struct basilysmapobjects_st *) ptr;
	fputs ("|", dp->dfil);
	discr_out (dp, p->discr);
	if (depth < dp->dmaxdepth)
	  {
	    int ln = basilys_primtab[p->lenix];
	    fprintf (dp->dfil, "{%d/", p->count);
	    if (p->entab)
	      for (ix = 0; ix < ln; ix++)
		{
		  basilysobject_ptr_t atp = p->entab[ix].e_at;
		  if (!atp || atp == HTAB_DELETED_ENTRY)
		    continue;
		  nl_debug_out (dp, depth);
		  basilys_debug_out (dp, (basilys_ptr_t) atp, dp->dmaxdepth);
		  fputs ("' = ", dp->dfil);
		  basilys_debug_out (dp, p->entab[ix].e_va, depth + 1);
		  fputs (";", dp->dfil);
		}
	    fputs (" }", dp->dfil);
	  }
	else
	  fputs ("..", dp->dfil);
	break;
      }
    case OBMAG_CLOSURE:
      {
	struct basilysclosure_st *p = (struct basilysclosure_st *) ptr;
	fputs ("!.", dp->dfil);
	discr_out (dp, p->discr);
	if (depth < dp->dmaxdepth)
	  {
	    fprintf (dp->dfil, "[. rout=");
	    basilys_debug_out (dp, (basilys_ptr_t) p->rout, depth + 1);
	    skip_debug_out (dp, depth);
	    fprintf (dp->dfil, " /%d: ", p->nbval);
	    for (ix = 0; ix < (int) p->nbval; ix++)
	      {
		if (ix > 0)
		  skip_debug_out (dp, depth);
		basilys_debug_out (dp, p->tabval[ix], depth + 1);
	      }
	    fputs (".]", dp->dfil);
	  }
	else
	  fputs ("..", dp->dfil);
	break;
      }
    case OBMAG_ROUTINE:
      {
	struct basilysroutine_st *p = (struct basilysroutine_st *) ptr;
	fputs ("!:", dp->dfil);
	discr_out (dp, p->discr);
	if (depth < dp->dmaxdepth)
	  {
	    fprintf (dp->dfil, ".%s[:/%d ", p->routdescr, p->nbval);
	    for (ix = 0; ix < (int) p->nbval; ix++)
	      {
		if (ix > 0)
		  skip_debug_out (dp, depth);
		basilys_debug_out (dp, p->tabval[ix], depth + 1);
	      }
	    fputs (":]", dp->dfil);
	  }
	else
	  fputs ("..", dp->dfil);
	break;
      }
    case OBMAG_STRBUF:
      {
	struct basilysstrbuf_st *p = (struct basilysstrbuf_st *) ptr;
	fputs ("!`", dp->dfil);
	discr_out (dp, p->discr);
	if (depth < dp->dmaxdepth)
	  {
	    fprintf (dp->dfil, "[`buflen=%ld ", basilys_primtab[p->buflenix]);
	    gcc_assert (p->bufstart <= p->bufend
			&& p->bufend < basilys_primtab[p->buflenix]);
	    fprintf (dp->dfil, "bufstart=%u bufend=%u buf='",
		     p->bufstart, p->bufend);
	    if (p->bufzn)
	      debug_outstr (dp, p->bufzn + p->bufstart);
	    fputs ("' `]", dp->dfil);
	  }
	else
	  fputs ("..", dp->dfil);
	break;
      }
    case OBMAG_PAIR:
      {
	struct basilyspair_st *p = (struct basilyspair_st *) ptr;
	fputs ("[pair:", dp->dfil);
	discr_out (dp, p->discr);
	if (depth < dp->dmaxdepth)
	  {
	    fputs ("hd:", dp->dfil);
	    basilys_debug_out (dp, p->hd, depth + 1);
	    fputs ("; ti:", dp->dfil);
	    basilys_debug_out (dp, (basilys_ptr_t) p->tl, depth + 1);
	  }
	else
	  fputs ("..", dp->dfil);
	fputs ("]", dp->dfil);
	break;
      }
    case OBMAG_TRIPLE:
    case OBMAG_TREE:
    case OBMAG_GIMPLE:
    case OBMAG_GIMPLESEQ:
    case OBMAG_BASICBLOCK:
    case OBMAG_EDGE:
    case OBMAG_MAPTREES:
    case OBMAG_MAPGIMPLES:
    case OBMAG_MAPGIMPLESEQS:
    case OBMAG_MAPBASICBLOCKS:
    case OBMAG_MAPEDGES:
    case OBMAG_DECAY:
      fatal_error ("debug_out unimplemented magic %d", mag);
    default:
      fatal_error ("debug_out invalid magic %d", mag);
    }
}



void
basilys_dbgeprint (void *p)
{
  struct debugprint_basilys_st dps = {
    0, 4, 0
  };
  dps.dfil = stderr;
  basilys_debug_out (&dps, (basilys_ptr_t) p, 0);
  putc ('\n', stderr);
  fflush (stderr);
}


void basilysgc_debugmsgval(void* val_p, const char*msg, long count)
{ 
  BASILYS_ENTERFRAME(2,NULL);
#define valv   curfram__.varptr[0]
#define dbgfv  curfram__.varptr[1]
  valv = val_p;
  dbgfv = 
    basilys_object_nth_field ((basilys_ptr_t)
			      BASILYSGOB (INITIAL_SYSTEM_DATA),
			      FSYSDAT_DEBUGMSG);
  {
    union basilysparam_un argtab[2];
    memset(argtab, 0, sizeof(argtab));
    argtab[0].bp_cstring = msg;
    argtab[1].bp_long = count;
    (void) basilys_apply ((basilysclosure_ptr_t) dbgfv, (basilys_ptr_t)valv, 
			  BPARSTR_CSTRING BPARSTR_LONG, argtab, "", NULL);
  }
  BASILYS_EXITFRAME();
#undef valv
#undef dbgfv
}

void
basilys_dbgbacktrace (int depth)
{
  int curdepth = 1, totdepth = 0;
  struct callframe_basilys_st *fr = 0;
  fprintf (stderr, "    <{\n");
  for (fr = basilys_topframe; fr != NULL && curdepth < depth;
       (fr = fr->prev), (curdepth++))
    {
      fprintf (stderr, "frame#%d closure: ", curdepth);
#if ENABLE_CHECKING
      if (fr->flocs)
	fprintf (stderr, "{%s} ", fr->flocs);
      else
	fputs (" ", stderr);
#endif
      basilys_dbgeprint (fr->clos);
    }
  for (totdepth = curdepth; fr != NULL; fr = fr->prev);
  fprintf (stderr, "}> backtraced %d frames of %d\n", curdepth, totdepth);
  fflush (stderr);
}


void
basilys_dbgshortbacktrace (const char *msg, int maxdepth)
{
  int curdepth = 1;
  struct callframe_basilys_st *fr = 0;
  if (maxdepth < 2)
    maxdepth = 2;
  fprintf (stderr, "\nSHORT BACKTRACE[#%ld] %s;", basilys_dbgcounter,
	   msg ? msg : "/");
  for (fr = basilys_topframe; fr != NULL && curdepth < maxdepth;
       (fr = fr->prev), (curdepth++))
    {
      fputs ("\n", stderr);
      fprintf (stderr, "#%d:", curdepth);
      if (basilys_magic_discr ((basilys_ptr_t) fr->clos) == OBMAG_CLOSURE)
	{
	  basilysroutine_ptr_t curout = fr->clos->rout;
	  if (basilys_magic_discr ((basilys_ptr_t) curout) == OBMAG_ROUTINE)
	    fprintf (stderr, "<%s> ", curout->routdescr);
	  else
	    fputs ("?norout?", stderr);
	}
      else
	fprintf (stderr, "_ ");
#if ENABLE_CHECKING
      if (fr->flocs)
	fprintf (stderr, "{%s} ", fr->flocs);
      else
	fputs (" ", stderr);
#endif
    };
  if (fr)
    fprintf (stderr, "...&%d", maxdepth - curdepth);
  else
    fputs (".", stderr);
  putc ('\n', stderr);
  putc ('\n', stderr);
  fflush (stderr);
}


/*****************
 * state GDBM access
 ****************/

static void
fatal_gdbm (char *msg)
{
  fatal_error ("fatal basilys GDBM (%s) error : %s", basilys_gdbmstate_string,
	       msg);
}

static inline GDBM_FILE
get_basilys_gdbm (void)
{
  if (gdbm_basilys)
    return gdbm_basilys;
  if (!basilys_gdbmstate_string || !basilys_gdbmstate_string[0])
    return NULL;
  gdbm_basilys =
    gdbm_open (CONST_CAST(char*, basilys_gdbmstate_string), 8192, GDBM_WRCREAT, 0600,
	       fatal_gdbm);
  if (!gdbm_basilys)
    fatal_error ("failed to lazily open basilys GDBM (%s) - %m",
		 basilys_gdbmstate_string);
  return gdbm_basilys;
}

bool
basilys_has_gdbmstate (void)
{
  return get_basilys_gdbm () != NULL;
}

basilys_ptr_t
basilysgc_fetch_gdbmstate_constr (const char *key)
{
  datum keydata = { 0, 0 };
  datum valdata = { 0, 0 };
  GDBM_FILE dbf = get_basilys_gdbm ();
  BASILYS_ENTERFRAME (1, NULL);
#define restrv curfram__.varptr[0]
  if (!dbf || !key || !key[0])
    goto end;
  keydata.dptr = CONST_CAST (char *, key);
  keydata.dsize = strlen (key);
  valdata = gdbm_fetch (dbf, keydata);
  if (valdata.dptr != NULL && valdata.dsize >= 0)
    {
      gcc_assert ((int) valdata.dsize == (int) strlen (valdata.dptr));
      restrv = basilysgc_new_string (BASILYSGOB (DISCR_STRING), valdata.dptr);
      free (valdata.dptr);
      valdata.dptr = 0;
    }
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) restrv;
#undef restrv
}

basilys_ptr_t
basilysgc_fetch_gdbmstate (basilys_ptr_t key_p)
{
  datum keydata = { 0, 0 };
  datum valdata = { 0, 0 };
  int keymagic = 0;
  GDBM_FILE dbf = get_basilys_gdbm ();
  BASILYS_ENTERFRAME (3, NULL);
#define restrv curfram__.varptr[0]
#define keyv   curfram__.varptr[1]
#define kstrv  curfram__.varptr[2]
  keyv = key_p;
  if (!dbf || !keyv)
    goto end;
  keymagic = basilys_magic_discr ((basilys_ptr_t) keyv);
  if (keymagic == OBMAG_STRING)
    {
      keydata.dptr = CONST_CAST (char *, basilys_string_str ((basilys_ptr_t) keyv));
      keydata.dsize = strlen (keydata.dptr);
      valdata = gdbm_fetch (dbf, keydata);
    }
  else if (keymagic == OBMAG_STRBUF)
    {
      keydata.dptr = CONST_CAST(char*, basilys_strbuf_str ((basilys_ptr_t) keyv));
      keydata.dsize = strlen (keydata.dptr);
      valdata = gdbm_fetch (dbf, keydata);
    }
  else if (keymagic == OBMAG_OBJECT
	   && basilys_is_instance_of ((basilys_ptr_t) keyv,
				      (basilys_ptr_t)
				      BASILYSGOB (CLASS_NAMED)))
    {
      kstrv = basilys_field_object ((basilys_ptr_t) keyv, FNAMED_NAME);
      if (basilys_magic_discr ((basilys_ptr_t) kstrv) == OBMAG_STRING)
	{
	  keydata.dptr = CONST_CAST(char*, basilys_string_str ((basilys_ptr_t) kstrv));
	  keydata.dsize = strlen (keydata.dptr);
	  valdata = gdbm_fetch (dbf, keydata);
	}
      else
	goto end;
    }
  else
    goto end;
  if (valdata.dptr != NULL && valdata.dsize >= 0)
    {
      gcc_assert ((int) valdata.dsize == (int) strlen (valdata.dptr));
      restrv = basilysgc_new_string (BASILYSGOB (DISCR_STRING), valdata.dptr);
      free (valdata.dptr);
      valdata.dptr = 0;
    }
end:
  BASILYS_EXITFRAME ();
  return (basilys_ptr_t) restrv;
#undef keyv
#undef restrv
#undef kstrv
}



void
basilysgc_put_gdbmstate_constr (const char *key, basilys_ptr_t data_p)
{
  datum keydata = { 0, 0 };
  datum valdata = { 0, 0 };
  int datamagic = 0;
  GDBM_FILE dbf = get_basilys_gdbm ();
  BASILYS_ENTERFRAME (2, NULL);
#define datav  curfram__.varptr[0]
#define dstrv  curfram__.varptr[1]
  datav = data_p;
  if (!dbf || !key || !key[0])
    goto end;
  keydata.dptr = CONST_CAST(char *, key);
  keydata.dsize = strlen (key);
  if (!datav)
    {
      gdbm_delete (dbf, keydata);
      goto end;
    }
  datamagic = basilys_magic_discr ((basilys_ptr_t) datav);
  if (datamagic == OBMAG_STRING)
    {
      valdata.dptr = CONST_CAST(char *, basilys_string_str ((basilys_ptr_t) datav));
      valdata.dsize = strlen (keydata.dptr);
      gdbm_store (dbf, keydata, valdata, GDBM_REPLACE);
    }
  else if (datamagic == OBMAG_STRBUF)
    {
      valdata.dptr = CONST_CAST(char*, basilys_strbuf_str ((basilys_ptr_t) datav));
      valdata.dsize = strlen (keydata.dptr);
      gdbm_store (dbf, keydata, valdata, GDBM_REPLACE);
    }
  else if (datamagic == OBMAG_OBJECT
	   && basilys_is_instance_of ((basilys_ptr_t) datav,
				      (basilys_ptr_t)
				      BASILYSGOB (CLASS_NAMED)))
    {
      dstrv = basilys_field_object ((basilys_ptr_t) datav, FNAMED_NAME);
      if (basilys_magic_discr ((basilys_ptr_t) dstrv) == OBMAG_STRING)
	{
	  valdata.dptr = CONST_CAST (char *, basilys_string_str ((basilys_ptr_t) dstrv));
	  valdata.dsize = strlen (keydata.dptr);
	  gdbm_store (dbf, keydata, valdata, GDBM_REPLACE);
	}
      else
	goto end;
    }
  else
    goto end;
end:
  BASILYS_EXITFRAME ();
#undef datav
#undef dstrv
}


void
basilysgc_put_gdbmstate (basilys_ptr_t key_p, basilys_ptr_t data_p)
{
  datum keydata = { 0, 0 };
  datum valdata = { 0, 0 };
  int keymagic = 0;
  int datamagic = 0;
  GDBM_FILE dbf = get_basilys_gdbm ();
  BASILYS_ENTERFRAME (4, NULL);
#define keyv   curfram__.varptr[0]
#define kstrv  curfram__.varptr[1]
#define datav  curfram__.varptr[2]
#define dstrv  curfram__.varptr[3]
  keyv = key_p;
  datav = data_p;
  if (!dbf || !keyv)
    goto end;
  keymagic = basilys_magic_discr ((basilys_ptr_t) keyv);
  if (keymagic == OBMAG_STRING)
    {
      keydata.dptr = CONST_CAST(char*, basilys_string_str ((basilys_ptr_t) keyv));
      keydata.dsize = strlen (keydata.dptr);
    }
  else if (keymagic == OBMAG_STRBUF)
    {
      keydata.dptr = CONST_CAST(char*, basilys_strbuf_str ((basilys_ptr_t) keyv));
      keydata.dsize = strlen (keydata.dptr);
    }
  else if (keymagic == OBMAG_OBJECT
	   && basilys_is_instance_of ((basilys_ptr_t) keyv,
				      (basilys_ptr_t)
				      BASILYSGOB (CLASS_NAMED)))
    {
      kstrv = basilys_field_object ((basilys_ptr_t) keyv, FNAMED_NAME);
      if (basilys_magic_discr ((basilys_ptr_t) kstrv) == OBMAG_STRING)
	{
	  keydata.dptr = CONST_CAST(char*, basilys_string_str ((basilys_ptr_t) kstrv));
	  keydata.dsize = strlen (keydata.dptr);
	}
      else
	goto end;
    }
  else
    goto end;
  if (!datav)
    {
      gdbm_delete (dbf, keydata);
      goto end;
    }
  datamagic = basilys_magic_discr ((basilys_ptr_t) datav);
  if (datamagic == OBMAG_STRING)
    {
      valdata.dptr = CONST_CAST(char*, basilys_string_str ((basilys_ptr_t) datav));
      valdata.dsize = strlen (keydata.dptr);
      gdbm_store (dbf, keydata, valdata, GDBM_REPLACE);
    }
  else if (datamagic == OBMAG_STRBUF)
    {
      valdata.dptr = CONST_CAST(char*, basilys_strbuf_str ((basilys_ptr_t) datav));
      valdata.dsize = strlen (keydata.dptr);
      gdbm_store (dbf, keydata, valdata, GDBM_REPLACE);
    }
  else if (datamagic == OBMAG_OBJECT
	   && basilys_is_instance_of ((basilys_ptr_t) datav,
				      (basilys_ptr_t)
				      BASILYSGOB (CLASS_NAMED)))
    {
      dstrv = basilys_field_object ((basilys_ptr_t) datav, FNAMED_NAME);
      if (basilys_magic_discr ((basilys_ptr_t) dstrv) == OBMAG_STRING)
	{
	  valdata.dptr = CONST_CAST(char*, basilys_string_str ((basilys_ptr_t) dstrv));
	  valdata.dsize = strlen (keydata.dptr);
	  gdbm_store (dbf, keydata, valdata, GDBM_REPLACE);
	}
      else
	goto end;
    }
  else
    goto end;
end:
  BASILYS_EXITFRAME ();
#undef keyv
#undef kstrv
#undef datav
#undef dstrv
}

/* wrapping gimple & tree prettyprinting for MELT debug */

/* the  prettyprinter buflushdata */
#define PPBASILYS_MAGIC 0x094f2de3
struct ppbasilysflushdata_st
{
  int gf_magic;			/* always  PPBASILYS_MAGIC */
  basilys_ptr_t *gf_sbufad;	/* adress of pointer to sbuf */
  pretty_printer gf_pp;
  int gf_indent;		/* current indentation */
};

static void
ppbasilys_flushrout (const char *txt, void *data)
{
  const char *bl = 0, *nl = 0;
  struct ppbasilysflushdata_st *fldata =
    (struct ppbasilysflushdata_st *) data;
  gcc_assert (fldata->gf_magic == PPBASILYS_MAGIC);
  gcc_assert (txt != NULL);
  for (bl = txt; bl; bl = nl?(nl+1):0) {
    int linelen = 0;
    nl = strchr(bl, '\n');
    if (nl && nl[1]==0) 
      break;
    linelen = nl?((int)(nl-bl)):((int)(strlen(bl)));
    basilysgc_add_strbuf_raw_len ((basilys_ptr_t) (*fldata->gf_sbufad),
				  bl, linelen);
    if (nl) 
      basilysgc_strbuf_add_indent((basilys_ptr_t) (*fldata->gf_sbufad), 
				  fldata->gf_indent, 72);
  }
}

/* pretty print into an sbuf a gimple */
void
basilysgc_ppstrbuf_gimple (basilys_ptr_t sbuf_p, int indentsp, gimple gstmt)
{
  struct ppbasilysflushdata_st ppgdat;
#define sbufv curfram__.varptr[0]
  BASILYS_ENTERFRAME (2, NULL);
  sbufv = sbuf_p;
  if (!sbufv || basilys_magic_discr ((basilys_ptr_t) sbufv) != OBMAG_STRBUF)
    goto end;
  if (!gstmt)
    {
      basilysgc_add_strbuf ((basilys_ptr_t) sbufv,
			    "%nullgimple%");
      goto end;
    }
  memset (&ppgdat, 0, sizeof (ppgdat));
  ppgdat.gf_sbufad = (basilys_ptr_t *) & sbufv;
  ppgdat.gf_magic = PPBASILYS_MAGIC;
  ppgdat.gf_indent = indentsp;
  pp_construct_routdata (&ppgdat.gf_pp, NULL, 72, ppbasilys_flushrout,
			 (void *) &ppgdat);
  dump_gimple_stmt (&ppgdat.gf_pp, gstmt, indentsp,
		    TDF_LINENO | TDF_SLIM | TDF_VOPS);
  pp_flush (&ppgdat.gf_pp);
  pp_destruct (&ppgdat.gf_pp);
end:
  memset (&ppgdat, 0, sizeof (ppgdat));
  BASILYS_EXITFRAME ();
#undef sbufv
}

/* pretty print into an sbuf a gimple seq */
void
basilysgc_ppstrbuf_gimple_seq (basilys_ptr_t sbuf_p, int indentsp,
			       gimple_seq gseq)
{
  struct ppbasilysflushdata_st ppgdat;
#define sbufv curfram__.varptr[0]
  BASILYS_ENTERFRAME (2, NULL);
  sbufv = sbuf_p;
  if (!sbufv || basilys_magic_discr ((basilys_ptr_t) sbufv) != OBMAG_STRBUF)
    goto end;
  if (!gseq)
    {
      basilysgc_add_strbuf ((basilys_ptr_t) sbufv,
			    "%nullgimpleseq%");
      goto end;
    }
  memset (&ppgdat, 0, sizeof (ppgdat));
  ppgdat.gf_sbufad = (basilys_ptr_t *) & sbufv;
  ppgdat.gf_magic = PPBASILYS_MAGIC;
  ppgdat.gf_indent = indentsp;
  pp_construct_routdata (&ppgdat.gf_pp, NULL, 72, ppbasilys_flushrout,
			 (void *) &ppgdat);
  dump_gimple_seq (&ppgdat.gf_pp, gseq, indentsp,
		   TDF_LINENO | TDF_SLIM | TDF_VOPS);
  pp_flush (&ppgdat.gf_pp);
  pp_destruct (&ppgdat.gf_pp);
end:
  memset (&ppgdat, 0, sizeof (ppgdat));
  BASILYS_EXITFRAME ();
#undef sbufv
}

/* pretty print into an sbuf a tree */
void
basilysgc_ppstrbuf_tree (basilys_ptr_t sbuf_p, int indentsp, tree tr)
{
  struct ppbasilysflushdata_st ppgdat;
#define sbufv curfram__.varptr[0]
  BASILYS_ENTERFRAME (2, NULL);
  sbufv = sbuf_p;
  if (!sbufv || basilys_magic_discr ((basilys_ptr_t) sbufv) != OBMAG_STRBUF)
    goto end;
  if (!tr)
    {
      basilysgc_add_strbuf ((basilys_ptr_t) sbufv, "%nulltree%");
      goto end;
    }
  memset (&ppgdat, 0, sizeof (ppgdat));
  ppgdat.gf_sbufad = (basilys_ptr_t *) & sbufv;
  ppgdat.gf_magic = PPBASILYS_MAGIC;
  ppgdat.gf_indent = indentsp;
  pp_construct_routdata (&ppgdat.gf_pp, NULL, 72, ppbasilys_flushrout,
			 (void *) &ppgdat);
  dump_generic_node (&ppgdat.gf_pp, tr, indentsp,
		     TDF_LINENO | TDF_SLIM | TDF_VOPS, false);
  pp_flush (&ppgdat.gf_pp);
  pp_destruct (&ppgdat.gf_pp);
end:
  memset (&ppgdat, 0, sizeof (ppgdat));
  BASILYS_EXITFRAME ();
#undef sbufv
}


/* pretty print into an sbuf a basicblock */
void
basilysgc_ppstrbuf_basicblock (basilys_ptr_t sbuf_p, int indentsp,
			       basic_block bb)
{
  gimple_seq gsq = 0;
#define sbufv curfram__.varptr[0]
  BASILYS_ENTERFRAME (2, NULL);
  sbufv = sbuf_p;
  if (!sbufv || basilys_magic_discr ((basilys_ptr_t) sbufv) != OBMAG_STRBUF)
    goto end;
  if (!bb)
    {
      basilysgc_add_strbuf ((basilys_ptr_t) sbufv,
			    "%nullbasicblock%");
      goto end;
    }
  basilysgc_strbuf_printf ((basilys_ptr_t) sbufv,
			   "basicblock ix%d", bb->index);
  gsq = bb_seq (bb);
  if (gsq)
    {
      basilysgc_add_strbuf_raw ((basilys_ptr_t) sbufv, "{.");
      basilysgc_ppstrbuf_gimple_seq ((basilys_ptr_t) sbufv,
				     indentsp + 1, gsq);
      basilysgc_add_strbuf_raw ((basilys_ptr_t) sbufv, ".}");
    }
  else
    basilysgc_add_strbuf_raw ((basilys_ptr_t) sbufv, ";");
end:
  BASILYS_EXITFRAME ();
#undef sbufv
}

/* pretty print into an sbuf a mpz_t GMP multiprecision integer */
void
basilysgc_ppstrbuf_mpz (basilys_ptr_t sbuf_p, int indentsp,
			  mpz_t mp)
{
  int len = 0;
  char* cbuf = 0;
  char tinybuf [64];
#define sbufv curfram__.varptr[0]
  BASILYS_ENTERFRAME (2, NULL);
  sbufv = sbuf_p;
  memset(tinybuf, 0, sizeof (tinybuf));
  if (!sbufv || basilys_magic_discr ((basilys_ptr_t) sbufv) != OBMAG_STRBUF || indentsp<0)
    goto end;
  if (!mp)
    {
      basilysgc_add_strbuf_raw ((basilys_ptr_t) sbufv,
				"%nullmp%");
      goto end;
    }
  len = mpz_sizeinbase(mp, 10) + 2;
  if (len < (int)sizeof(tinybuf)-2) 
    {
      mpz_get_str (tinybuf, 10, mp);
      basilysgc_add_strbuf_raw ((basilys_ptr_t) sbufv, tinybuf);
    }
  else
    {
      cbuf = (char*) xcalloc(len+2, 1);
      mpz_get_str(cbuf, 10, mp);
      basilysgc_add_strbuf_raw ((basilys_ptr_t) sbufv, cbuf);
      free(cbuf);
    }
 end:
  BASILYS_EXITFRAME ();
#undef sbufv
}


/* pretty print into an sbuf the GMP multiprecision integer of a mixbigint */
void
basilysgc_ppstrbuf_mixbigint (basilys_ptr_t sbuf_p, int indentsp,
			      basilys_ptr_t big_p)
{
#define sbufv curfram__.varptr[0]
#define bigv  curfram__.varptr[1]
  BASILYS_ENTERFRAME (3, NULL);
  sbufv = sbuf_p;
  bigv = big_p;
  if (!sbufv || basilys_magic_discr ((basilys_ptr_t) sbufv) != OBMAG_STRBUF)
    goto end;
  if (!bigv || basilys_magic_discr ((basilys_ptr_t) bigv) != OBMAG_MIXBIGINT)
    goto end;
  {
    mpz_t mp;
    mpz_init (mp);
    if (basilys_fill_mpz_from_mixbigint((basilys_ptr_t) bigv, mp)) 
      basilysgc_ppstrbuf_mpz ((basilys_ptr_t) sbufv, indentsp, mp);
    mpz_clear (mp);
  }
 end:
  BASILYS_EXITFRAME ();
#undef sbufv
#undef bigv
}

/***********************************************************************
 *   P A R M A     P O L Y H E D R A     L I B R A R Y     S T U F F   *
 ***********************************************************************/

/* utility to make a ppl_Coefficient_t out of a constant tree */
ppl_Coefficient_t
basilys_make_ppl_coefficient_from_tree(tree tr)
{
  HOST_WIDE_INT lo=0, hi=0;
  ppl_Coefficient_t coef=NULL;
  mpz_t mp;
  if (!tr) return NULL;
  switch (TREE_CODE(tr)) {
  case INTEGER_CST:
    mpz_init(mp);
    lo = TREE_INT_CST_LOW(tr);
    hi = TREE_INT_CST_HIGH(tr);
    if (hi==0 && lo>=0) 
      mpz_set_ui(mp, lo);
    else if (hi== -1 && lo<0)
      mpz_set_si(mp, lo);
    else {
      mpz_t mp2;
      mpz_init_set_ui (mp2, lo);
      mpz_set_si(mp, hi);
      mpz_mul_2exp(mp, mp, HOST_BITS_PER_WIDE_INT);
      mpz_add(mp, mp, mp2);
      mpz_clear(mp2);
    };
    if (ppl_new_Coefficient_from_mpz_t (&coef, mp))
      fatal_error("ppl_new_Coefficient_from_mpz_t failed");
    mpz_clear(mp);
    return coef;
  default:
    break;
  }
  return NULL;
}

/* utility to make a ppl_Coefficient_t from a long number */
ppl_Coefficient_t
basilys_make_ppl_coefficient_from_long(long l)
{
  ppl_Coefficient_t coef=NULL;
  mpz_t mp;
  mpz_init_set_si (mp, l);
  if (ppl_new_Coefficient_from_mpz_t (&coef, mp))
    fatal_error("ppl_new_Coefficient_from_mpz_t failed");
  mpz_clear(mp);
  return coef;
}

/* make a new boxed PPL empty or unsatisfiable constraint system */
basilys_ptr_t
basilysgc_new_ppl_constraint_system(basilys_ptr_t discr_p, bool unsatisfiable)
{
  int err = 0;
  BASILYS_ENTERFRAME(2, NULL);
#define discrv curfram__.varptr[0]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define resv   curfram__.varptr[1]
#define spec_resv ((struct basilysspecial_st*)(resv))
  discrv = (void *) discr_p;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_SPECPPL_CONSTRAINT_SYSTEM)
    goto end;
  resv = basilysgc_allocate (sizeof (struct basilysspecial_st), 0);
  spec_resv->discr = (basilysobject_ptr_t) discrv;
  spec_resv->mark = 0;
  spec_resv->val.sp_pointer = NULL;
  if (!unsatisfiable)
    err = ppl_new_Constraint_System(&spec_resv->val.sp_constraint_system);
  else
    err = ppl_new_Constraint_System_zero_dim_empty(&spec_resv->val.sp_constraint_system);
  if (err) 
    fatal_error("PPL new Constraint System failed in Basilys"); 
 end:
  BASILYS_EXITFRAME();
  return (basilys_ptr_t)resv;
#undef discrv
#undef object_discrv
#undef resv
#undef spec_resv
}

/* box clone a PPL constraint system */
basilys_ptr_t
basilysgc_clone_ppl_constraint_system (basilys_ptr_t ppl_p)
{
  int err = 0;
  ppl_Constraint_System_t oldconsys = NULL, newconsys = NULL;
  BASILYS_ENTERFRAME(3, NULL);
#define pplv   curfram__.varptr[0]
#define spec_pplv ((struct basilysspecial_st*)(pplv))
#define discrv curfram__.varptr[1]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define resv   curfram__.varptr[2]
#define spec_resv ((struct basilysspecial_st*)(resv))
  pplv = ppl_p;
  resv = NULL;
  if (basilys_magic_discr ((basilys_ptr_t) (pplv)) != OBMAG_SPECPPL_CONSTRAINT_SYSTEM)
    goto end;
  oldconsys =  spec_pplv->val.sp_constraint_system;
  resv = basilysgc_allocate (sizeof (struct basilysspecial_st), 0);
  spec_resv->discr = spec_pplv->discr;
  spec_resv->mark = 0;
  spec_resv->val.sp_pointer = NULL;
  if (oldconsys)
    err = ppl_new_Constraint_System_from_Constraint_System(&newconsys, oldconsys);
  if (err) 
    fatal_error("PPL clone Constraint System failed in Basilys");
  spec_resv->val.sp_constraint_system = newconsys;
 end:
  BASILYS_EXITFRAME();
  return (basilys_ptr_t)resv;
#undef discrv
#undef object_discrv
#undef resv
#undef spec_resv
#undef pplv
#undef spec_pplv
}

/* insert a raw PPL constraint into a boxed constraint system */
void
basilys_insert_ppl_constraint_in_boxed_system(ppl_Constraint_t cons, basilys_ptr_t ppl_p) 
{
  BASILYS_ENTERFRAME(3, NULL);
#define pplv   curfram__.varptr[0]
#define spec_pplv ((struct basilysspecial_st*)(pplv))
  pplv = ppl_p;
  if (!pplv || !cons 
      || basilys_magic_discr((basilys_ptr_t)pplv) != OBMAG_SPECPPL_CONSTRAINT_SYSTEM)
    goto end;
  if (spec_pplv->val.sp_constraint_system
      && ppl_Constraint_System_insert_Constraint (spec_pplv->val.sp_constraint_system,
						  cons))
    fatal_error("failed to ppl_Constraint_System_insert_Constraint");
 end:
  BASILYS_EXITFRAME();
#undef pplv
#undef spec_pplv
}

/* utility to make a NNC [=not necessarily closed] ppl_Polyhedron_t
   out of a constraint system */
ppl_Polyhedron_t 
basilys_make_ppl_NNC_Polyhedron_from_Constraint_System(ppl_Constraint_System_t consys)
{
  ppl_Polyhedron_t poly = NULL;
  if (ppl_new_NNC_Polyhedron_from_Constraint_System(&poly, consys))
    fatal_error("basilys_make_ppl_NNC_Polyhedron_from_Constraint_System failed");
  return poly;
}

/* make a new boxed PPL polyhedron; if cloned is true, the poly is
   copied otherwise taken as is */
basilys_ptr_t
basilysgc_new_ppl_polyhedron(basilys_ptr_t discr_p, ppl_Polyhedron_t poly, bool cloned)
{
  BASILYS_ENTERFRAME(2, NULL);
#define discrv curfram__.varptr[0]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define resv   curfram__.varptr[1]
#define spec_resv ((struct basilysspecial_st*)(resv))
  discrv = (void *) discr_p;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_SPECPPL_POLYHEDRON)
    goto end;
  resv = basilysgc_allocate (sizeof (struct basilysspecial_st), 0);
  spec_resv->discr = (basilysobject_ptr_t) discrv;
  spec_resv->mark = 0;
  spec_resv->val.sp_pointer = NULL;
  if (cloned && poly)
    {
      if (ppl_new_NNC_Polyhedron_from_NNC_Polyhedron(&spec_resv->val.sp_polyhedron, poly))
	fatal_error("failed to ppl_new_NNC_Polyhedron_from_NNC_Polyhedron");
    }
  else
    spec_resv->val.sp_polyhedron = poly;
 end:
  BASILYS_EXITFRAME();
  return (basilys_ptr_t)resv;
#undef discrv
#undef object_discrv
#undef resv
#undef spec_resv
}

/* utility to make a ppl_Linear_Expression_t */
ppl_Linear_Expression_t 
basilys_make_ppl_linear_expression(void)
{
  ppl_Linear_Expression_t liex = NULL;
  if (ppl_new_Linear_Expression(&liex))
    fatal_error("basilys_make_ppl_linear_expression failed");
  return liex;
}

/* utility to make a ppl_Constraint ; the constraint type is a string
   "==" or "!=" ">" "<" ">=" "<=" because we don't want enums in
   MELT... */
ppl_Constraint_t 
basilys_make_ppl_constraint_cstrtype(ppl_Linear_Expression_t liex, const char*constyp) {
  ppl_Constraint_t cons = NULL;
  if (!liex || !constyp) return NULL;
  if (!strcmp(constyp, "==")
      && !ppl_new_Constraint(&cons, liex,
			     PPL_CONSTRAINT_TYPE_EQUAL)) 
    return cons;
  else if (!strcmp(constyp, ">")
	   && !ppl_new_Constraint(&cons, liex, 
				  PPL_CONSTRAINT_TYPE_GREATER_THAN))
    return cons;
  else if (!strcmp(constyp, "<")
	   && !ppl_new_Constraint(&cons, liex, 
				  PPL_CONSTRAINT_TYPE_LESS_THAN))
    return cons;
  else if (!strcmp(constyp, ">=")
	   && !ppl_new_Constraint(&cons, liex, 
				  PPL_CONSTRAINT_TYPE_GREATER_OR_EQUAL))
    return cons;
  else if (!strcmp(constyp, "<=")
	   && !ppl_new_Constraint(&cons, liex, 
				  PPL_CONSTRAINT_TYPE_LESS_OR_EQUAL))
    return cons;
  return NULL;
}

/* make a new boxed PPL linear expression  */
basilys_ptr_t
basilysgc_new_ppl_linear_expression(basilys_ptr_t discr_p)
{
  int err = 0;
  BASILYS_ENTERFRAME(2, NULL);
#define discrv curfram__.varptr[0]
#define object_discrv ((basilysobject_ptr_t)(discrv))
#define resv   curfram__.varptr[1]
#define spec_resv ((struct basilysspecial_st*)(resv))
  discrv = (void *) discr_p;
  if (basilys_magic_discr ((basilys_ptr_t) (discrv)) != OBMAG_OBJECT)
    goto end;
  if (object_discrv->object_magic != OBMAG_SPECPPL_LINEAR_EXPRESSION)
    goto end;
  resv = basilysgc_allocate (sizeof (struct basilysspecial_st), 0);
  spec_resv->discr = (basilysobject_ptr_t) discrv;
  spec_resv->mark = 0;
  spec_resv->val.sp_pointer = NULL;
  err = ppl_new_Linear_Expression(&spec_resv->val.sp_linear_expression);
  if (err) 
    fatal_error("PPL new Linear Expression failed in Basilys"); 
 end:
  BASILYS_EXITFRAME();
  return (basilys_ptr_t)resv;
#undef discrv
#undef object_discrv
#undef resv
#undef spec_resv
}


void basilys_clear_special(basilys_ptr_t val_p)
{
  BASILYS_ENTERFRAME(1, NULL);
#define valv curfram__.varptr[0]
#define spec_valv ((struct basilysspecial_st*)valv)
  valv = val_p;
  if (!valv) goto end;
  switch(basilys_magic_discr((basilys_ptr_t) valv)) {
  case ALL_OBMAG_SPECIAL_CASES:
      delete_special(spec_valv);
      break;
  default:
    break;
  }
 end:
  BASILYS_EXITFRAME();
#undef valv
#undef spec_valv
}


/***
  pretty print into an sbuf a PPL related value; 

recent PPL (ie 0.10.1) has a ppl_io_asprint_##Type (char** strp,
  ppl_const_##Type##_t x); which mallocs a string buffer, print x
  inside it, and return it in *STRP but this is supposed to
  change. Seee
  http://www.cs.unipr.it/pipermail/ppl-devel/2009-March/014162.html
***/

static basilys_ptr_t* basilys_pplcoefvectp;

static const char* 
ppl_basilys_variable_output_function(ppl_dimension_type var)
{
  static char buf[80];
  const char *s = 0;
  BASILYS_ENTERFRAME(2, NULL);
#define vectv  curfram__.varptr[0]
#define namv   curfram__.varptr[1]
  if (basilys_pplcoefvectp)
    vectv =  *basilys_pplcoefvectp;
  memset(buf, 0, sizeof(buf));
  if (vectv)
    namv = basilys_multiple_nth((basilys_ptr_t) vectv, (int)var);
  if (basilys_is_instance_of((basilys_ptr_t) namv, BASILYSG (CLASS_NAMED))) 
    namv = basilys_object_nth_field((basilys_ptr_t) namv, FNAMED_NAME);
  if (namv)
    s = basilys_string_str((basilys_ptr_t) namv);
  if (!s && basilys_magic_discr((basilys_ptr_t) namv) == OBMAG_TREE) {
    tree trnam = basilys_tree_content((basilys_ptr_t) namv);
    if (trnam) {
      switch (TREE_CODE(trnam)) {
      case IDENTIFIER_NODE:
	s = IDENTIFIER_POINTER(trnam);
	break;
      case VAR_DECL:
      case PARM_DECL:
      case TYPE_DECL:
      case FIELD_DECL:
      case LABEL_DECL:
      case CONST_DECL:
      case RESULT_DECL:
	if (DECL_NAME(trnam))
	  s = IDENTIFIER_POINTER(DECL_NAME(trnam));
	break;
      case SSA_NAME:
	snprintf (buf, sizeof(buf)-1, "%s.%d", 
		 get_name(trnam), SSA_NAME_VERSION(trnam));
	goto end;
      default:
	snprintf (buf, sizeof(buf)-1, "@%p!%s", 
		 (void*)trnam, tree_code_name[TREE_CODE(trnam)]);
	goto end;
      }
    }
  }
  if (s) 
    strncpy(buf, s, sizeof(buf)-1);
  else if (!buf[0])
    snprintf (buf, sizeof(buf)-1, "_$_%d", (int)var);
 end:
  BASILYS_EXITFRAME();
  return buf;
}


/* call the ppl_io_asprint_##Type (char** strp, ppl_const_##Type##_t
   x); these functions are now stable in PPL */
void
basilysgc_ppstrbuf_ppl_varnamvect (basilys_ptr_t sbuf_p, int indentsp, basilys_ptr_t ppl_p, basilys_ptr_t varnamvect_p)
{
  int mag = 0;
  char *ppstr = NULL;
  BASILYS_ENTERFRAME(4, NULL);
#define sbufv    curfram__.varptr[0]
#define pplv     curfram__.varptr[1]
#define varvectv curfram__.varptr[2]
#define spec_pplv ((struct basilysspecial_st*)(pplv))
  sbufv = sbuf_p;
  pplv = ppl_p;
  varvectv = varnamvect_p;
  if (!pplv) 
    goto end;
  ppl_io_set_variable_output_function (ppl_basilys_variable_output_function);
  mag = basilys_magic_discr((basilys_ptr_t) pplv);
  if (varvectv)
    basilys_pplcoefvectp = (basilys_ptr_t*)&varvectv;
  else
    basilys_pplcoefvectp = NULL;
  switch (mag) {
  case OBMAG_SPECPPL_COEFFICIENT:
    if (ppl_io_asprint_Coefficient(&ppstr, 
				   spec_pplv->val.sp_coefficient))
      fatal_error("failed to ppl_io_asprint_Coefficient");
    break;
  case OBMAG_SPECPPL_LINEAR_EXPRESSION:
    if (ppl_io_asprint_Linear_Expression(&ppstr,
					 spec_pplv->val.sp_linear_expression))
      fatal_error("failed to ppl_io_asprint_Linear_Expression");
    break;
  case OBMAG_SPECPPL_CONSTRAINT:
    if (ppl_io_asprint_Constraint(&ppstr, 
				  spec_pplv->val.sp_constraint))
      fatal_error("failed to ppl_io_asprint_Constraint");
    break;
  case OBMAG_SPECPPL_CONSTRAINT_SYSTEM:
    if (ppl_io_asprint_Constraint_System(&ppstr, 
					 spec_pplv->val.sp_constraint_system))
      fatal_error("failed to ppl_io_asprint_Constraint_System");
    break;
  case OBMAG_SPECPPL_GENERATOR:
    if (ppl_io_asprint_Generator(&ppstr, spec_pplv->val.sp_generator))
      fatal_error("failed to ppl_io_asprint_Generator");
    break;
  case OBMAG_SPECPPL_GENERATOR_SYSTEM:
    if (ppl_io_asprint_Generator_System(&ppstr, 
					spec_pplv->val.sp_generator_system))
      fatal_error("failed to ppl_io_asprint_Generator_System");
    break;
  case OBMAG_SPECPPL_POLYHEDRON:
    if (ppl_io_asprint_Polyhedron(&ppstr, 
					spec_pplv->val.sp_polyhedron))
      fatal_error("failed to ppl_io_asprint_Polyhedron");
    break;
  default:
    {
      char errmsg[64];
      memset(errmsg, 0, sizeof(errmsg));
      snprintf (errmsg, sizeof(errmsg)-1, "{{unknown PPL magic %d}}", mag); 
      ppstr = xstrdup(errmsg);
    }
    break;
  }
  if (!ppstr) 
    fatal_error("ppl_io_asprint_* gives a null string pointer");
  /* in the resulting ppstr, replace each newline with appropriate
     indentation */
  {
    char*bl = NULL;		/* current begin of line */
    char*nl = NULL;		/* current newline = end of line */
    for (bl = ppstr; (nl = bl?strchr(bl, '\n'):NULL), bl; bl = nl?(nl+1):NULL) {
      if (nl) 
	*nl = (char)0;
      basilysgc_add_strbuf_raw((basilys_ptr_t) sbufv, bl);
      if (nl) 
	basilysgc_strbuf_add_indent((basilys_ptr_t) sbufv, indentsp, 0);
    }
  }
  free(ppstr);
 end:
  basilys_pplcoefvectp = (basilys_ptr_t*)0;
  BASILYS_EXITFRAME();
#undef sbufv
#undef pplv
#undef varvectv
#undef spec_pplv
}



static void basilys_ppl_error_handler(enum ppl_enum_error_code err, const char* descr)
{
  switch(err) {
  case PPL_ERROR_OUT_OF_MEMORY: 
    error("Basilys PPL out of memory: %s", descr);
    return;
  case PPL_ERROR_INVALID_ARGUMENT:
    error("Basilys PPL invalid argument: %s", descr);
    return;
  case PPL_ERROR_DOMAIN_ERROR:
    error("Basilys PPL domain error: %s", descr);
    return;
  case PPL_ERROR_LENGTH_ERROR:
    error("Basilys PPL length error: %s", descr);
    return;
  case PPL_ARITHMETIC_OVERFLOW:
    error("Basilys PPL arithmetic overflow: %s", descr);
    return;
  case PPL_STDIO_ERROR:
    error("Basilys PPL stdio error: %s", descr);
    return;
  case PPL_ERROR_INTERNAL_ERROR:
    error("Basilys PPL internal error: %s", descr);
    return;
  case PPL_ERROR_UNKNOWN_STANDARD_EXCEPTION:
    error("Basilys PPL unknown exception: %s", descr);
    return;
  case PPL_ERROR_UNEXPECTED_ERROR:
    error("Basilys PPL unexpected error: %s", descr);
    return;
  default:
    fatal_error("Basilys unexpected PPL error #%d - %s", err, descr);
  }
}


/***********************************************************
 * generate C code for a basilys unit name 
 ***********************************************************/
void
basilys_output_cfile_decl_impl (basilys_ptr_t unitnam,
				basilys_ptr_t declbuf, basilys_ptr_t implbuf)
{
  int unamlen = 0;
  char *dotcnam = NULL;
  char *dotcdotnam = NULL;
  char *dotcpercentnam = NULL;
  FILE *cfil = NULL;
  gcc_assert (basilys_magic_discr (unitnam) == OBMAG_STRING);
  gcc_assert (basilys_magic_discr (declbuf) == OBMAG_STRBUF);
  gcc_assert (basilys_magic_discr (implbuf) == OBMAG_STRBUF);
  /** FIXME : should implement some policy about the location of the
      generated C file; currently using the pwd */
  unamlen = strlen (basilys_string_str (unitnam));
  dotcnam = (char *) xcalloc (unamlen + 3, 1);
  dotcpercentnam = (char *) xcalloc (unamlen + 4, 1);
  dotcdotnam = (char *) xcalloc (unamlen + 5, 1);
  strcpy (dotcnam, basilys_string_str (unitnam));
  if (unamlen > 4
      && (dotcnam[unamlen - 2] != '.' || dotcnam[unamlen - 1] != 'c'))
    strcat (dotcnam, ".c");
  strcpy (dotcpercentnam, dotcnam);
  strcat (dotcpercentnam, "%");
  strcpy (dotcdotnam, dotcnam);
  strcat (dotcdotnam, ".");
  cfil = fopen (dotcdotnam, "w");
  if (!cfil)
    fatal_error ("failed to open basilys generated file %s - %m", dotcnam);
  fprintf (cfil,
	   "/* GCC MELT GENERATED FILE %s - DO NOT EDIT */\n", dotcnam);
  {
    time_t now = 0;
    char nowtimstr[64];
    time (&now);
    memset (nowtimstr, 0, sizeof (nowtimstr));
    /* we only write the date (not the hour); the comment is for
       humans only.  This might make ccache happier, when the same
       file is generated several times in the same day */
    strftime(nowtimstr, sizeof(nowtimstr)-1, "%Y %b %d", gmtime(&now));
    if (strlen (nowtimstr) > 2)
      fprintf (cfil, "/* generated on %s */\n\n", nowtimstr);
  }
  /* we protect genchecksum_melt with MELTGCC_DYNAMIC_OBJSTRUCT since
     for sure when compiling the warmelt*0.c it would mismatch, and we
     want to avoid a useless warning */
  fprintf (cfil, "\n#ifndef MELTGCC_DYNAMIC_OBJSTRUCT\n"
	   "/* checksum of the gcc executable generating this file: */\n"
	   "const unsigned char genchecksum_melt[16]=\n {");
  {
    int i;
    for (i=0; i<16; i++) {
      if (i>0) 
	fputc(',', cfil);
      if (i==8) 
	fputs("\n   ", cfil);
      fprintf (cfil, " %#x", executable_checksum[i]);
    }
  };
  fprintf (cfil, "};\n" "#endif\n" "\n");
  fprintf (cfil, "#include \"run-basilys.h\"\n");
  fprintf (cfil, "\n/**** %s declarations ****/\n",
	   basilys_string_str (unitnam));
  basilys_putstrbuf (cfil, declbuf);
  putc ('\n', cfil);
  fflush (cfil);
  fprintf (cfil, "\n/**** %s implementations ****/\n",
	   basilys_string_str (unitnam));
  basilys_putstrbuf (cfil, implbuf);
  putc ('\n', cfil);
  fflush (cfil);
  fprintf (cfil, "\n/**** end of %s ****/\n", basilys_string_str (unitnam));
  fclose (cfil);
  debugeprintf ("output_cfile done dotcnam %s", dotcnam);
  /* if possible, rename the previous 'foo.c' file to 'foo.c%' as a backup */
  (void) rename (dotcnam, dotcpercentnam);
  /* always rename the just generated 'foo.c.' file to 'foo.c' to make
     the generation more atomic */
  if (rename (dotcdotnam, dotcnam))
    fatal_error ("failed to rename basilys generated file %s to %s - %m",
		 dotcdotnam, dotcnam);
  free (dotcnam);
  free (dotcdotnam);
  free (dotcpercentnam);
}

/* Added */
#undef basilys_assert_failed
#undef basilys_check_failed

void
basilys_assert_failed (const char *msg, const char *filnam,
		       int lineno, const char *fun)
{
  static char msgbuf[500];
  if (!msg)
    msg = "??no-msg??";
  if (!filnam)
    filnam = "??no-filnam??";
  if (!fun)
    fun = "??no-func??";
  if (basilys_dbgcounter > 0)
    snprintf (msgbuf, sizeof (msgbuf) - 1,
	      "%s:%d: BASILYS ASSERT #!%ld: %s {%s}", basename (filnam),
	      lineno, basilys_dbgcounter, fun, msg);
  else
    snprintf (msgbuf, sizeof (msgbuf) - 1, "%s:%d: BASILYS ASSERT: %s {%s}",
	      basename (filnam), lineno, fun, msg);
  basilys_dbgshortbacktrace (msgbuf, 100);
  fatal_error ("%s:%d: BASILYS ASSERT FAILED <%s> : %s\n",
	       basename (filnam), lineno, fun, msg);
}

void
basilys_check_failed (const char *msg, const char *filnam,
		      int lineno, const char *fun)
{
  static char msgbuf[500];
  if (!msg)
    msg = "??no-msg??";
  if (!filnam)
    filnam = "??no-filnam??";
  if (!fun)
    fun = "??no-func??";
  if (basilys_dbgcounter > 0)
    snprintf (msgbuf, sizeof (msgbuf) - 1,
	      "%s:%d: BASILYS CHECK #!%ld: %s {%s}", basename (filnam),
	      lineno, basilys_dbgcounter, fun, msg);
  else
    snprintf (msgbuf, sizeof (msgbuf) - 1, "%s:%d: BASILYS CHECK: %s {%s}",
	      basename (filnam), lineno, fun, msg);
  basilys_dbgshortbacktrace (msgbuf, 100);
  warning (0, "%s:%d: BASILYS CHECK FAILED <%s> : %s\n",
	   basename (filnam), lineno, fun, msg);
}


/* convert a MELT value to a plugin flag or option */
static unsigned long 
basilys_val2passflag(basilys_ptr_t val_p)
{
  unsigned long res = 0;
  int valmag = 0;
  BASILYS_ENTERFRAME (3, NULL);
#define valv    curfram__.varptr[0]
#define compv   curfram__.varptr[1]
#define pairv   curfram__.varptr[2]
  valv = val_p;
  if (!valv) goto end;
  valmag = basilys_magic_discr((basilys_ptr_t) valv);
  if (valmag == OBMAG_INT || valmag == OBMAG_MIXINT)
    { 
      res = basilys_get_int((basilys_ptr_t) valv);
      goto end;
    }
  else if (valmag == OBMAG_OBJECT 
	   && basilys_is_instance_of((basilys_ptr_t) valv, 
				     BASILYSG(CLASS_NAMED)))
    {
      compv = ((basilysobject_ptr_t)valv)->obj_vartab[FNAMED_NAME];
      res = basilys_val2passflag((basilys_ptr_t) compv);
      goto end;
    }
  else if (valmag == OBMAG_STRING) {
    const char *valstr = basilys_string_str((basilys_ptr_t) valv);
    /* should be kept in sync with the defines in tree-pass.h */
#define WHENFLAG(F) if (!strcasecmp(valstr, #F)) { res = F; goto end; } 
    WHENFLAG(PROP_gimple_any);
    WHENFLAG(PROP_gimple_lcf);		
    WHENFLAG(PROP_gimple_leh);		
    WHENFLAG(PROP_cfg);	
    WHENFLAG(PROP_referenced_vars);
    WHENFLAG(PROP_ssa);
    WHENFLAG(PROP_no_crit_edges);
    WHENFLAG(PROP_rtl);
    WHENFLAG(PROP_gimple_lomp);
    WHENFLAG(PROP_cfglayout);
    WHENFLAG(PROP_trees);
    /* likewise for TODO flags */
    WHENFLAG(TODO_dump_func);
    WHENFLAG(TODO_ggc_collect);
    WHENFLAG(TODO_verify_ssa);
    WHENFLAG(TODO_verify_flow);
    WHENFLAG(TODO_verify_stmts);
    WHENFLAG(TODO_cleanup_cfg);
    WHENFLAG(TODO_verify_loops);
    WHENFLAG(TODO_dump_cgraph);
    WHENFLAG(TODO_remove_functions);
    WHENFLAG(TODO_rebuild_frequencies);
    WHENFLAG(TODO_verify_rtl_sharing);
    WHENFLAG(TODO_update_ssa);
    WHENFLAG(TODO_update_ssa_no_phi);
    WHENFLAG(TODO_update_ssa_full_phi);
    WHENFLAG(TODO_update_ssa_only_virtuals);
    WHENFLAG(TODO_remove_unused_locals);
    WHENFLAG(TODO_df_finish);
    WHENFLAG(TODO_df_verify);
    WHENFLAG(TODO_mark_first_instance);
    WHENFLAG(TODO_rebuild_alias);
    WHENFLAG(TODO_update_address_taken);
    WHENFLAG(TODO_update_ssa_any);
    WHENFLAG(TODO_verify_all);
#undef WHENFLAG
    goto end;
  }
  else if (valmag == OBMAG_LIST) {
    for (pairv = ((struct basilyslist_st *) valv)->first;
	 basilys_magic_discr ((basilys_ptr_t) pairv) ==
	   OBMAG_PAIR; 
	 pairv = ((struct basilyspair_st *)pairv)->tl) {
      compv = ((struct basilyspair_st *)pairv)->hd;
      res |= basilys_val2passflag((basilys_ptr_t) compv);
    }
  }
  else if (valmag == OBMAG_MULTIPLE) {
    int i=0, l=0;
    l = basilys_multiple_length((basilys_ptr_t)valv);
    for (i=0; i<l; i++) {
      compv = basilys_multiple_nth((basilys_ptr_t) valv, i);
      res |= basilys_val2passflag((basilys_ptr_t) compv);
    }
  }
 end:
  BASILYS_EXITFRAME();
  return res;
#undef valv    
#undef compv   
#undef pairv   
}




/* the gate function of MELT gimple passes */
static bool 
basilysgc_gimple_gate(void)
{
  int ok = 0;
  BASILYS_ENTERFRAME(6, NULL);
#define passv        curfram__.varptr[0]
#define passdictv    curfram__.varptr[1]
#define closv        curfram__.varptr[2]
#define resv        curfram__.varptr[3]
  if (!basilys_mode_string || !basilys_mode_string[0]) 
    goto end;
  passdictv =
    basilys_object_nth_field ((basilys_ptr_t)
			      BASILYSGOB (INITIAL_SYSTEM_DATA),
			      FSYSDAT_PASS_DICT);
  if (basilys_magic_discr((basilys_ptr_t) passdictv) != OBMAG_MAPSTRINGS) 
    goto end;
  gcc_assert(current_pass != NULL);
  gcc_assert(current_pass->name != NULL);
  gcc_assert(current_pass->type == GIMPLE_PASS);
  passv = basilys_get_mapstrings((struct basilysmapstrings_st*) passdictv, current_pass->name);
  if (!passv 
      || !basilys_is_instance_of((basilys_ptr_t) passv, (basilys_ptr_t) BASILYSGOB(CLASS_GCC_GIMPLE_PASS)))
    goto end;
  closv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_GATE);
  if (basilys_magic_discr((basilys_ptr_t) closv) != OBMAG_CLOSURE) 
    goto end;
  resv = 
    basilys_apply ((struct basilysclosure_st *) closv,
		   (basilys_ptr_t) passv, "",
		   (union basilysparam_un *) 0, "",
		   (union basilysparam_un *) 0);
  ok = (resv != NULL);
  /* force a minor GC to be sure that nothing is in the young region */
  basilys_garbcoll (0, BASILYS_MINOR_OR_FULL);
 end:
  BASILYS_EXITFRAME();
  return ok;
}


/* the execute function of MELT gimple passes */
static unsigned int
basilysgc_gimple_execute(void)
{
  unsigned int res = 0;
  BASILYS_ENTERFRAME(6, NULL);
#define passv        curfram__.varptr[0]
#define passdictv    curfram__.varptr[1]
#define closv        curfram__.varptr[2]
#define resvalv        curfram__.varptr[3]
  if (!basilys_mode_string || !basilys_mode_string[0]) 
    goto end;
  passdictv =
    basilys_object_nth_field ((basilys_ptr_t)
			      BASILYSGOB (INITIAL_SYSTEM_DATA),
			      FSYSDAT_PASS_DICT);
  if (basilys_magic_discr((basilys_ptr_t) passdictv) != OBMAG_MAPSTRINGS) 
    goto end;
  gcc_assert (current_pass != NULL);
  gcc_assert (current_pass->name != NULL);
  gcc_assert (current_pass->type == GIMPLE_PASS);
  passv = basilys_get_mapstrings((struct basilysmapstrings_st *)passdictv, current_pass->name);
  if (!passv 
      || !basilys_is_instance_of((basilys_ptr_t) passv, (basilys_ptr_t) BASILYSGOB(CLASS_GCC_GIMPLE_PASS)))
    goto end;
  closv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_EXEC);
  if (basilys_magic_discr((basilys_ptr_t) closv) != OBMAG_CLOSURE) 
    goto end;
  {
    long passdbgcounter = basilys_dbgcounter;
    long todol = 0;
    union basilysparam_un restab[1];
    memset (&restab, 0, sizeof (restab));
    restab[0].bp_longptr = &todol;
    debugeprintf
      ("gimple_execute passname %s dbgcounter %ld cfun %p ",
       current_pass->name, basilys_dbgcounter, (void *) cfun);
    if (cfun && flag_basilys_debug)
      debug_tree (cfun->decl);
    debugeprintf ("gimple_execute passname %s before apply",
		  current_pass->name);
    /* apply with one extra long result */
    resvalv =
      basilys_apply ((struct basilysclosure_st *) closv,
		     (basilys_ptr_t) passv, "",
		     (union basilysparam_un *) 0, BPARSTR_LONG "",
		     restab);
    debugeprintf ("gimple_execute passname %s after apply dbgcounter %ld",
		  current_pass->name, passdbgcounter);
    if (resvalv)
      res = (unsigned int) todol;
    /* force a minor GC to be sure that nothing is in the young region */
    basilys_garbcoll (0, BASILYS_MINOR_OR_FULL);
  }
 end:
  BASILYS_EXITFRAME();
  return res;
}



/* the gate function of MELT rtl passes */
static bool 
basilysgc_rtl_gate(void)
{
  int ok = 0;
  BASILYS_ENTERFRAME(6, NULL);
#define passv        curfram__.varptr[0]
#define passdictv    curfram__.varptr[1]
#define closv        curfram__.varptr[2]
#define resv        curfram__.varptr[3]
  if (!basilys_mode_string || !basilys_mode_string[0]) 
    goto end;
  passdictv =
    basilys_object_nth_field ((basilys_ptr_t)
			      BASILYSGOB (INITIAL_SYSTEM_DATA),
			      FSYSDAT_PASS_DICT);
  if (basilys_magic_discr((basilys_ptr_t) passdictv) != OBMAG_MAPSTRINGS) 
    goto end;
  gcc_assert(current_pass != NULL);
  gcc_assert(current_pass->name != NULL);
  gcc_assert(current_pass->type == RTL_PASS);
  passv = basilys_get_mapstrings((struct basilysmapstrings_st*) passdictv, 
				  current_pass->name);
  if (!passv 
      || !basilys_is_instance_of((basilys_ptr_t) passv, 
				 (basilys_ptr_t) BASILYSGOB(CLASS_GCC_RTL_PASS)))
    goto end;
  closv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_GATE);
  if (basilys_magic_discr((basilys_ptr_t) closv) != OBMAG_CLOSURE) 
    goto end;
  resv = 
    basilys_apply ((struct basilysclosure_st *) closv,
		   (basilys_ptr_t) passv, "",
		   (union basilysparam_un *) 0, "",
		   (union basilysparam_un *) 0);
  ok = (resv != NULL);
  /* force a minor GC to be sure that nothing is in the young region */
  basilys_garbcoll (0, BASILYS_MINOR_OR_FULL);
 end:
  BASILYS_EXITFRAME();
  return ok;
}


/* the execute function of MELT rtl passes */
static unsigned int
basilysgc_rtl_execute(void)
{
  unsigned int res = 0;
  BASILYS_ENTERFRAME(6, NULL);
#define passv        curfram__.varptr[0]
#define passdictv    curfram__.varptr[1]
#define closv        curfram__.varptr[2]
#define namev        curfram__.varptr[3]
  if (!basilys_mode_string || !basilys_mode_string[0]) 
    goto end;
  passdictv =
    basilys_object_nth_field ((basilys_ptr_t)
			      BASILYSGOB (INITIAL_SYSTEM_DATA),
			      FSYSDAT_PASS_DICT);
  if (basilys_magic_discr((basilys_ptr_t) passdictv) != OBMAG_MAPSTRINGS) 
    goto end;
  gcc_assert (current_pass != NULL);
  gcc_assert (current_pass->name != NULL);
  gcc_assert (current_pass->type == RTL_PASS);
  passv = basilys_get_mapstrings((struct basilysmapstrings_st*) passdictv, 
				 current_pass->name);
  if (!passv 
      || !basilys_is_instance_of((basilys_ptr_t) passv, (basilys_ptr_t) BASILYSGOB(CLASS_GCC_RTL_PASS)))
    goto end;
  closv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_EXEC);
  if (basilys_magic_discr((basilys_ptr_t) closv) != OBMAG_CLOSURE) 
    goto end;
  {
    long passdbgcounter = basilys_dbgcounter;
    long todol = 0;
    union basilysparam_un restab[1];
    memset (&restab, 0, sizeof (restab));
    restab[0].bp_longptr = &todol;
    debugeprintf
      ("rtl_execute passname %s dbgcounter %ld",
       current_pass->name, basilys_dbgcounter);
    debugeprintf ("rtl_execute passname %s before apply",
		  current_pass->name);
    /* apply with one extra long result */
    resvalv =
      basilys_apply ((struct basilysclosure_st *) closv,
		     (basilys_ptr_t) passv, "",
		     (union basilysparam_un *) 0, BPARSTR_LONG "",
		     restab);
    debugeprintf ("rtl_execute passname %s after apply dbgcounter %ld",
		  current_pass->name, passdbgcounter);
    if (resvalv)
      res = (unsigned int) todol;
    /* force a minor GC to be sure that nothing is in the young region */
    basilys_garbcoll (0, BASILYS_MINOR_OR_FULL);
  }
 end:
  BASILYS_EXITFRAME();
  return res;
}



/* the gate function of MELT simple_ipa passes */
static bool 
basilysgc_simple_ipa_gate(void)
{
  int ok = 0;
  BASILYS_ENTERFRAME(6, NULL);
#define passv        curfram__.varptr[0]
#define passdictv    curfram__.varptr[1]
#define closv        curfram__.varptr[2]
#define resv        curfram__.varptr[3]
  if (!basilys_mode_string || !basilys_mode_string[0]) 
    goto end;
  passdictv =
    basilys_object_nth_field ((basilys_ptr_t)
			      BASILYSGOB (INITIAL_SYSTEM_DATA),
			      FSYSDAT_PASS_DICT);
  if (basilys_magic_discr((basilys_ptr_t) passdictv) != OBMAG_MAPSTRINGS) 
    goto end;
  gcc_assert(current_pass != NULL);
  gcc_assert(current_pass->name != NULL);
  gcc_assert(current_pass->type == SIMPLE_IPA_PASS);
  passv = basilys_get_mapstrings((struct basilysmapstrings_st*) passdictv, 
				 current_pass->name);
  if (!passv 
      || !basilys_is_instance_of((basilys_ptr_t) passv, 
				 (basilys_ptr_t) BASILYSGOB(CLASS_GCC_SIMPLE_IPA_PASS)))
    goto end;
  closv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_GATE);
  if (basilys_magic_discr((basilys_ptr_t) closv) != OBMAG_CLOSURE) 
    goto end;
  resv = 
    basilys_apply ((struct basilysclosure_st *) closv,
		   (basilys_ptr_t) passv, "",
		   (union basilysparam_un *) 0, "",
		   (union basilysparam_un *) 0);
  ok = (resv != NULL);
  /* force a minor GC to be sure that nothing is in the young region */
  basilys_garbcoll (0, BASILYS_MINOR_OR_FULL);
 end:
  BASILYS_EXITFRAME();
  return ok;
}


/* the execute function of MELT simple_ipa passes */
static unsigned int
basilysgc_simple_ipa_execute(void)
{
  unsigned int res = 0;
  BASILYS_ENTERFRAME(6, NULL);
#define passv        curfram__.varptr[0]
#define passdictv    curfram__.varptr[1]
#define closv        curfram__.varptr[2]
#define namev        curfram__.varptr[3]
  if (!basilys_mode_string || !basilys_mode_string[0]) 
    goto end;
  passdictv =
    basilys_object_nth_field ((basilys_ptr_t)
			      BASILYSGOB (INITIAL_SYSTEM_DATA),
			      FSYSDAT_PASS_DICT);
  if (basilys_magic_discr((basilys_ptr_t) passdictv) != OBMAG_MAPSTRINGS) 
    goto end;
  gcc_assert (current_pass != NULL);
  gcc_assert (current_pass->name != NULL);
  gcc_assert (current_pass->type == SIMPLE_IPA_PASS);
  passv = basilys_get_mapstrings((struct basilysmapstrings_st*)passdictv, 
				 current_pass->name);
  if (!passv 
      || !basilys_is_instance_of((basilys_ptr_t) passv, 
				 (basilys_ptr_t) BASILYSGOB(CLASS_GCC_SIMPLE_IPA_PASS)))
    goto end;
  closv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_EXEC);
  if (basilys_magic_discr((basilys_ptr_t) closv) != OBMAG_CLOSURE) 
    goto end;
  {
    long passdbgcounter = basilys_dbgcounter;
    long todol = 0;
    union basilysparam_un restab[1];
    memset (&restab, 0, sizeof (restab));
    restab[0].bp_longptr = &todol;
    debugeprintf
      ("simple_ipa_execute passname %s dbgcounter %ld",
       current_pass->name, basilys_dbgcounter);
    debugeprintf ("simple_ipa_execute passname %s before apply",
		  current_pass->name);
    /* apply with one extra long result */
    resvalv =
      basilys_apply ((struct basilysclosure_st *) closv,
		     (basilys_ptr_t) passv, "",
		     (union basilysparam_un *) 0, BPARSTR_LONG "",
		     restab);
    debugeprintf ("simple_ipa_execute passname %s after apply dbgcounter %ld",
		  current_pass->name, passdbgcounter);
    if (resvalv)
      res = (unsigned int) todol;
    /* force a minor GC to be sure that nothing is in the young region */
    basilys_garbcoll (0, BASILYS_MINOR_OR_FULL);
  }
 end:
  BASILYS_EXITFRAME();
  return res;
}





/* register a MELT pass; there is no way to unregister it, and the
   opt_pass and plugin_pass used internally are never deallocated.
   Non-simple IPA passes are not yet implemented! */
void
basilysgc_register_pass (basilys_ptr_t pass_p, 
			 const char* positioning, 
			 const char*refpassname,
			 int refpassnum)
{
  /* the plugin_pass can be local, since it is only locally used in
     plugin.c */
  struct plugin_pass plugpass = { NULL, NULL, 0, 0 };
  enum pass_positioning_ops posop = PASS_POS_INSERT_AFTER;
  unsigned long propreq=0, propprov=0, propdest=0, todostart=0, todofinish=0;
  BASILYS_ENTERFRAME (7, NULL);
#define passv        curfram__.varptr[0]
#define passdictv    curfram__.varptr[1]
#define compv        curfram__.varptr[2]
#define namev        curfram__.varptr[3]
  passv = pass_p;
  if (!basilys_mode_string || !basilys_mode_string[0]) 
    goto end;
  if (!refpassname || !refpassname[0]) 
    goto end;
  if (!positioning || !positioning[0])
    goto end;
  if (!strcasecmp(positioning,"after"))
    posop = PASS_POS_INSERT_AFTER;
  else if (!strcasecmp(positioning,"before"))
    posop = PASS_POS_INSERT_BEFORE;
  else if (!strcasecmp(positioning,"replace"))
    posop = PASS_POS_REPLACE;
  else fatal_error("invalid positioning string %s in MELT pass", positioning);
  if (!passv || basilys_object_length((basilys_ptr_t) passv) < FGCCPASS__LAST
      || !basilys_is_instance_of((basilys_ptr_t) passv, 
				 (basilys_ptr_t) BASILYSGOB(CLASS_GCC_PASS)))
    goto end;
  namev = basilys_object_nth_field((basilys_ptr_t) passv, FNAMED_NAME);
  if (basilys_magic_discr((basilys_ptr_t) namev) != OBMAG_STRING)
    goto end;
  passdictv =
    basilys_object_nth_field ((basilys_ptr_t)
			      BASILYSGOB (INITIAL_SYSTEM_DATA),
			      FSYSDAT_PASS_DICT);
  if (basilys_magic_discr((basilys_ptr_t)passdictv) != OBMAG_MAPSTRINGS) 
    goto end;
  if (basilys_get_mapstrings((struct basilysmapstrings_st*)passdictv, 
			     basilys_string_str((basilys_ptr_t) namev)))
    goto end;
  compv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_PROPERTIES_REQUIRED);
  propreq = basilys_val2passflag((basilys_ptr_t) compv);
  compv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_PROPERTIES_PROVIDED);
  propprov = basilys_val2passflag((basilys_ptr_t) compv);
  compv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_TODO_FLAGS_START);
  todostart = basilys_val2passflag((basilys_ptr_t) compv);
  compv = basilys_object_nth_field((basilys_ptr_t) passv, FGCCPASS_TODO_FLAGS_FINISH);
  todofinish = basilys_val2passflag((basilys_ptr_t) compv);
  /* allocate the opt pass and fill it; it is never deallocated (ie it
     is never free-d)! */
  if (basilys_is_instance_of((basilys_ptr_t) passv, (basilys_ptr_t) BASILYSGOB(CLASS_GCC_GIMPLE_PASS))) {
    struct gimple_opt_pass* gimpass = NULL;     
    gimpass = XNEW(struct gimple_opt_pass);
    memset(gimpass, 0, sizeof(struct gimple_opt_pass));
    gimpass->pass.type = GIMPLE_PASS;
    /* the name of the pass is also strduped and is never deallocated
       (so it it never free-d! */
    gimpass->pass.name = xstrdup(basilys_string_str((basilys_ptr_t) namev));
    gimpass->pass.gate = basilysgc_gimple_gate;
    gimpass->pass.execute = basilysgc_gimple_execute;
    gimpass->pass.tv_id = TV_PLUGIN_RUN;
    gimpass->pass.properties_required = propreq;
    gimpass->pass.properties_provided = propprov;
    gimpass->pass.properties_destroyed = propdest;
    gimpass->pass.todo_flags_start = todostart;
    gimpass->pass.todo_flags_finish = todofinish;
    plugpass.pass = (struct opt_pass*) gimpass;
    plugpass.reference_pass_name = refpassname;
    plugpass.ref_pass_instance_number = refpassnum;
    plugpass.pos_op = posop;
    register_callback(MELT_PLUGIN_NAME, PLUGIN_PASS_MANAGER_SETUP, 
		      NULL, &plugpass);
    /* add the pass into the pass dict */
    basilysgc_put_mapstrings((struct basilysmapstrings_st*) passdictv,
			     gimpass->pass.name, (basilys_ptr_t) passv);
  }
  else if (basilys_is_instance_of((basilys_ptr_t) passv, 
				  (basilys_ptr_t) BASILYSGOB(CLASS_GCC_RTL_PASS))) {
    struct rtl_opt_pass* rtlpass = NULL;     
    rtlpass = XNEW(struct rtl_opt_pass);
    memset(rtlpass, 0, sizeof(struct rtl_opt_pass));
    rtlpass->pass.type = RTL_PASS;
    /* the name of the pass is also strduped and is never deallocated
       (so it it never free-d! */
    rtlpass->pass.name = xstrdup(basilys_string_str((basilys_ptr_t) namev));
    rtlpass->pass.gate = basilysgc_rtl_gate;
    rtlpass->pass.execute = basilysgc_rtl_execute;
    rtlpass->pass.tv_id = TV_PLUGIN_RUN;
    rtlpass->pass.properties_required = propreq;
    rtlpass->pass.properties_provided = propprov;
    rtlpass->pass.properties_destroyed = propdest;
    rtlpass->pass.todo_flags_start = todostart;
    rtlpass->pass.todo_flags_finish = todofinish;
    plugpass.pass = (struct opt_pass*)rtlpass;
    plugpass.reference_pass_name = refpassname;
    plugpass.ref_pass_instance_number = refpassnum;
    plugpass.pos_op = posop;
    register_callback(MELT_PLUGIN_NAME, PLUGIN_PASS_MANAGER_SETUP, 
		      NULL, &plugpass);
    /* add the pass into the pass dict */
    basilysgc_put_mapstrings((struct basilysmapstrings_st*) passdictv,
			     rtlpass->pass.name, (basilys_ptr_t) passv);
  }
  else if (basilys_is_instance_of((basilys_ptr_t) passv, (basilys_ptr_t) BASILYSGOB(CLASS_GCC_SIMPLE_IPA_PASS))) {
    struct simple_ipa_opt_pass* sipapass = NULL;     
    sipapass = XNEW(struct simple_ipa_opt_pass);
    memset(sipapass, 0, sizeof(struct simple_ipa_opt_pass));
    sipapass->pass.type = SIMPLE_IPA_PASS;
    /* the name of the pass is also strduped and is never deallocated
       (so it it never free-d! */
    sipapass->pass.name = xstrdup(basilys_string_str((basilys_ptr_t) namev));
    sipapass->pass.gate = basilysgc_simple_ipa_gate;
    sipapass->pass.execute = basilysgc_simple_ipa_execute;
    sipapass->pass.tv_id = TV_PLUGIN_RUN;
    sipapass->pass.properties_required = propreq;
    sipapass->pass.properties_provided = propprov;
    sipapass->pass.properties_destroyed = propdest;
    sipapass->pass.todo_flags_start = todostart;
    sipapass->pass.todo_flags_finish = todofinish;
    plugpass.pass = (struct opt_pass*) sipapass;
    plugpass.reference_pass_name = refpassname;
    plugpass.ref_pass_instance_number = refpassnum;
    plugpass.pos_op = posop;
    register_callback(MELT_PLUGIN_NAME, PLUGIN_PASS_MANAGER_SETUP, 
		      NULL, &plugpass);
    /* add the pass into the pass dict */
    basilysgc_put_mapstrings((struct basilysmapstrings_st*) passdictv,
			     sipapass->pass.name, (basilys_ptr_t) passv);
  }
  /* non simple ipa passes are a different story - TODO! */
  else 
    fatal_error ("MELT cannot register pass %s of unexpected class %s",
		 basilys_string_str ((basilys_ptr_t) namev), 
		 basilys_string_str (basilys_object_nth_field 
				     ((basilys_ptr_t) basilys_discr((basilys_ptr_t) passv), 
				      FNAMED_NAME)));
 end:
  BASILYS_EXITFRAME();
#undef passv
#undef passdictv
#undef namev
}








/*****
 * called from handle_melt_attribute
 *****/

void
basilys_handle_melt_attribute (tree decl, tree name, const char *attrstr,
			       location_t loch)
{
  BASILYS_ENTERFRAME (4, NULL);
#define seqv       curfram__.varptr[0]
#define declv      curfram__.varptr[1]
#define namev      curfram__.varptr[2]
#define atclov	   curfram__.varptr[3]
  if (!attrstr || !attrstr[0])
    goto end;
  seqv = basilysgc_read_from_rawstring (attrstr, "*melt-attr*", loch);
  atclov =
    basilys_field_object ((basilys_ptr_t) BASILYSGOB (INITIAL_SYSTEM_DATA),
			  FSYSDAT_MELTATTR_DEFINER);
  if (basilys_magic_discr ((basilys_ptr_t) atclov) == OBMAG_CLOSURE)
    {
      union basilysparam_un argtab[2];
      BASILYS_LOCATION_HERE ("melt attribute definer");
      declv =
	basilysgc_new_tree ((basilysobject_ptr_t) BASILYSG (DISCR_TREE),
			    decl);
      namev =
	basilysgc_new_tree ((basilysobject_ptr_t) BASILYSG (DISCR_TREE),
			    name);
      memset (argtab, 0, sizeof (argtab));
      argtab[0].bp_aptr = (basilys_ptr_t *) & namev;
      argtab[1].bp_aptr = (basilys_ptr_t *) & seqv;
      (void) basilys_apply ((basilysclosure_ptr_t) atclov,
			    (basilys_ptr_t) declv,
			    BPARSTR_PTR BPARSTR_PTR, argtab, "", NULL);
    }
end:
  BASILYS_EXITFRAME ();
#undef seqv
#undef declv
#undef namev
#undef atclov
}




#include "gt-basilys.h"
/* eof basilys.c */
