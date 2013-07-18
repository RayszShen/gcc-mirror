/* GCC MELT GENERATED C++ FILE warmelt-base+02.cc - DO NOT EDIT - see http://gcc-melt.org/ */
/* secondary MELT generated C++ file of rank #2 */
#include "melt-run.h"


/* used hash from melt-run.h when compiling this file: */
MELT_EXTERN const char meltrun_used_md5_melt_f2[] = MELT_RUN_HASHMD5 /* from melt-run.h */;


/**** warmelt-base+02.cc declarations ****/
/**** MELT GENERATED DECLARATIONS for warmelt-base ** DO NOT EDIT ; see gcc-melt.org ****/

/****++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
***
    Copyright 2008 - 2013 Free Software Foundation, Inc.
    Contributed by Basile Starynkevitch <basile@starynkevitch.net>
               and Pierre Vittet  <piervit@pvittet.com>

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
    along with GCC; see the file COPYING3.  If not see
    <http://www.gnu.org/licenses/>.
***
----------------------------------------------------------------****/


/** ordinary MELT module meltbuild-sources/warmelt-base**/
#define MELT_HAS_INITIAL_ENVIRONMENT 1 /*usualmodule*/



class Melt_InitialClassyFrame_WARMELTmiBASE_h75707954; // forward declaration fromline 6518
typedef Melt_InitialClassyFrame_WARMELTmiBASE_h75707954 Melt_InitialFrame;
/*** 4 MELT module variables declarations ****/
MELT_EXTERN void melt_forwarding_module_data (void);
MELT_EXTERN void melt_marking_module_data (void);

MELT_EXTERN melt_ptr_t MELT_MODULE_VISIBILITY melt_WARMELTmiBASE_module_var_ptr_tab[16];
MELT_EXTERN bool MELT_MODULE_VISIBILITY melt_WARMELTmiBASE_module_var_flags[1];

static inline melt_ptr_t
melt_module_var_fetch (int ix)
{
    return (ix > 0  &&  ix <= 4)?melt_WARMELTmiBASE_module_var_ptr_tab[ix]:NULL;
}

static inline void
melt_module_var_put (int ix, melt_ptr_t val)
{
    if (ix > 0  &&  ix <= 4)
        {
            melt_WARMELTmiBASE_module_var_ptr_tab[ix]= val;
            melt_WARMELTmiBASE_module_var_flags[ix/16] = true;
        }
}

/* MELT module variables indexes */
enum
{
    meltmodatix_none,
    meltmodvarix_DIAGV_GIMPLE_symb = 1,
    meltmodvarix_DIAGV_GIMPLE_SEQ_symb = 2,
    meltmodvarix_DIAGV_TREE_symb = 3,
    meltmodvarix_DIAGV_BASIC_BLOCK_symb = 4,
    meltmodatix_last
}; /* end MELT module variables indexes */

/*** 3 MELT called hook declarations ***/

/*declare MELT called hook #0 HOOK_FRESH_ENVIRONMENT_REFERENCE_MAKER **/
MELT_EXTERN melt_ptr_t melthook_HOOK_FRESH_ENVIRONMENT_REFERENCE_MAKER (melt_ptr_t melthookdatap, melt_ptr_t meltinp0_PREVENV, const char* meltinp1_MODULNAME);

/*declare MELT called hook #1 HOOK_SYMBOL_IMPORTER **/
MELT_EXTERN melt_ptr_t melthook_HOOK_SYMBOL_IMPORTER (melt_ptr_t melthookdatap, const char* meltinp0_SYMNAMESTR, const char* meltinp1_MODULENAMESTR, melt_ptr_t meltinp2_PARENV);

/*declare MELT called hook #2 HOOK_VALUE_EXPORTER **/
MELT_EXTERN void melthook_HOOK_VALUE_EXPORTER (melt_ptr_t melthookdatap, melt_ptr_t meltinp0_SYM, melt_ptr_t meltinp1_VAL, melt_ptr_t meltinp2_CONTENV);

/*** end of 3 MELT called hook declarations ***/

/*** no extra MELT c-headers ***/




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_1_WARMELTmiBASE_plIV(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_2_WARMELTmiBASE_miIV(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_3_WARMELTmiBASE_stIV(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_4_WARMELTmiBASE_diIV(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_5_WARMELTmiBASE_pcIV(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_6_WARMELTmiBASE_LIST_NTH_ELEMENT(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_7_WARMELTmiBASE_REGISTER_DIAG_GIMPLE(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_8_WARMELTmiBASE_REGISTER_DIAG_GIMPLE_SEQ(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_9_WARMELTmiBASE_REGISTER_DIAG_TREE(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_10_WARMELTmiBASE_REGISTER_DIAG_BASIC_BLOCK(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_11_WARMELTmiBASE_COMPARE_NAMED_ALPHA(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_12_WARMELTmiBASE_ADD2OUT(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_13_WARMELTmiBASE_STRING4OUT(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_14_WARMELTmiBASE_ADD2OUT4NULL(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_15_WARMELTmiBASE_ADD2OUT4INTEGER(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_16_WARMELTmiBASE_ADD2OUT4STRING(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_17_WARMELTmiBASE_ADD2OUT4STRBUF(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_18_WARMELTmiBASE_ADD2OUT4CLONEDSYMB(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_19_WARMELTmiBASE_ADD2OUT4NAMED(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_20_WARMELTmiBASE_ADD2OUT4ROOTOBJECT(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_21_WARMELTmiBASE_ADD2OUT4ANY(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_22_WARMELTmiBASE_MAPSTRING_EVERY(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_23_WARMELTmiBASE_MAPSTRING_ITERATE_TEST(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_24_WARMELTmiBASE_MULTIPLE_EVERY(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_25_WARMELTmiBASE_MULTIPLE_BACKWARD_EVERY(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_26_WARMELTmiBASE_MULTIPLE_EVERY_BOTH(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_27_WARMELTmiBASE_REGISTER_PRAGMA_HANDLER(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_28_WARMELTmiBASE_SET_REFERENCE(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_35_WARMELTmiBASE_SYMBOL_CNAME(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);




melt_ptr_t MELT_MODULE_VISIBILITY meltrout_36_WARMELTmiBASE_MAKE_SEXPR(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_, const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_);


MELT_EXTERN void* melt_start_this_module (void*); /*mandatory start of module*/


/*declare opaque initial frame: */

typedef Melt_InitialClassyFrame_WARMELTmiBASE_h75707954 /*opaqueinitialclassy*/ meltinitial_frame_t;


/* define different names when debugging or not */
#if MELT_HAVE_DEBUG
MELT_EXTERN const char meltmodule_WARMELTmiBASE__melt_have_debug_enabled[];
#define melt_have_debug_string meltmodule_WARMELTmiBASE__melt_have_debug_enabled
#else /*!MELT_HAVE_DEBUG*/
MELT_EXTERN const char meltmodule_WARMELTmiBASE__melt_have_debug_disabled[];
#define melt_have_debug_string meltmodule_WARMELTmiBASE__melt_have_debug_disabled
#endif /*!MELT_HAVE_DEBUG*/



void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_0 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_1 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_2 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_3 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_4 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_5 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_6 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_7 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_8 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_9 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_10 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_11 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_12 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_13 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_14 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_15 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_16 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_17 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_18 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_19 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_20 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_21 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_22 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_23 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_24 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_25 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_26 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_27 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_28 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_29 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_30 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_31 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_32 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_33 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_34 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_35 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_36 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_37 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_38 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_39 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_40 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_41 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_42 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_43 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_44 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_45 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_46 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_47 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_48 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_49 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_50 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_51 (meltinitial_frame_t*, char*);
void MELT_MODULE_VISIBILITY meltmod__WARMELTmiBASE__initialmeltchunk_52 (meltinitial_frame_t*, char*);


/**** warmelt-base+02.cc implementations ****/




melt_ptr_t MELT_MODULE_VISIBILITY
meltrout_28_WARMELTmiBASE_SET_REFERENCE(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_,
                                        const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_)
{
    long current_blocklevel_signals_meltrout_28_WARMELTmiBASE_SET_REFERENCE_melt =  melt_blocklevel_signals;

#if MELT_HAVE_DEBUG
    static long melt_call_counter__;
    long melt_thiscallcounter__ ATTRIBUTE_UNUSED = ++ melt_call_counter__;
#undef meltcallcount
#define meltcallcount melt_thiscallcounter__
#else
#undef meltcallcount
#define meltcallcount 0L
#endif
//// emitted by output_curframe_declstruct_init using OUTPUT_CURFRAME_DECLSTRUCT @warmelt-outobj.melt:1501

    /* start of frame for routine meltrout_28_WARMELTmiBASE_SET_REFERENCE fromline 1716 */

    /** start of frame for meltrout_28_WARMELTmiBASE_SET_REFERENCE of CLASS_PROCROUTINEOBJ from 1518**/

    /*curframdeclclassy*/ class MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE// fromline 1531
        : public Melt_CallFrameWithValues<3>
    {
    public: /* fromline 1535*/
// no classy longs
        /*classy others*/
        long _meltsparel;
        void melt_mark_stuff (void)
        {
        } /*end melt_mark_stuff*/
        virtual void melt_mark_ggc_data (void)
        {
            melt_mark_values ();
            melt_mark_stuff ();
        }; /*end melt_mark_ggc_data*/
        MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE(meltclosure_ptr_t clos) //the closure constructor fromline 1594
            : Melt_CallFrameWithValues<3> (
#if ENABLE_CHECKING /*fromline 1598*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1602*/
                sizeof(MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE), clos) {};
        MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE() //the constructor fromline 1606
            : Melt_CallFrameWithValues<3> (
#if ENABLE_CHECKING /*fromline 1610*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1614*/
                sizeof(MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE)) {};
#if ENABLE_CHECKING /*fromline 1618*/
        MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE(const char*fil, int lin) //the constructor fromline 1620
            : Melt_CallFrameWithValues<3> (fil,lin, sizeof(MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE)) {};
        MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE(const char*fil, int lin, meltclosure_ptr_t clos) //the closure constructor fromline 1625
            : Melt_CallFrameWithValues<3> (fil,lin, sizeof(MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE), clos) {};
#endif /* ENABLE_CHECKING fromline 1630*/

    }; // end  class MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE


    /** end of frame for meltrout_28_WARMELTmiBASE_SET_REFERENCE fromline 1661**/

    /* end of frame for routine meltrout_28_WARMELTmiBASE_SET_REFERENCE fromline 1720 */

    /* classy proc frame meltrout_28_WARMELTmiBASE_SET_REFERENCE */
    MeltFrame_meltrout_28_WARMELTmiBASE_SET_REFERENCE
    meltfram__ /*declfrastruct fromline 1744*/
    /*classyprocarg meltrout_28_WARMELTmiBASE_SET_REFERENCE fromline 1749*/
#if ENABLE_CHECKING
    (__FILE__, __LINE__, meltclosp_) /* fromline 1753*/
#else /*ENABLE_CHECKING */
    (meltclosp_) /* fromline 1757*/
#endif /*ENABLE_CHECKING */

    ;
#define meltframe meltfram__

    melt_trace_start("SET_REFERENCE", meltcallcount);
    /*use arguments*/
    (void) meltclosp_;
    (void)meltfirstargp_;
    (void)meltxargdescr_;
    (void)meltxargtab_;
    (void)meltxresdescr_;
    (void)meltxrestab_;
    /*getargs*/

    /*getarg#0*/
    MELT_LOCATION("warmelt-base.melt:1892:/ getarg");
    /*_.R__V2*/
    meltfptr[1] = (melt_ptr_t) meltfirstargp_;

    /*getarg#1*/
    /*^getarg*/
    if (meltxargdescr_[0] != MELTBPAR_PTR) goto meltlab_endgetargs;
    /*_.V__V3*/
    meltfptr[2] = (meltxargtab_[0].meltbp_aptr) ? (*(meltxargtab_[0].meltbp_aptr)) : NULL;
    gcc_assert(melt_discr((melt_ptr_t)(/*_.V__V3*/ meltfptr[2])) != NULL);

    ;
    goto meltlab_endgetargs;
meltlab_endgetargs:
    ;
    /*body*/
    /*^block*/
    /*anyblock*/
    {


        MELT_CHECK_SIGNAL();
        ;
        MELT_LOCATION("warmelt-base.melt:1895:/ cond");
        /*cond*/
        if (
            /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.R__V2*/ meltfptr[1]),
                                          (melt_ptr_t)((/*!CLASS_REFERENCE*/ meltfrout->tabval[0])))
        ) /*then*/
            {
                /*^cond.then*/
                /*^block*/
                /*anyblock*/
                {

                    /*^putslot*/
                    /*putslot*/
                    melt_assertmsg("checkobj putslot  _ @REFERENCED_VALUE", melt_magic_discr((melt_ptr_t)(/*_.R__V2*/ meltfptr[1])) == MELTOBMAG_OBJECT);
                    melt_putfield_object((/*_.R__V2*/ meltfptr[1]), (0), (/*_.V__V3*/ meltfptr[2]), "REFERENCED_VALUE");
                    ;
                    /*^touch*/
                    meltgc_touch(/*_.R__V2*/ meltfptr[1]);
                    ;
                    /*^touchobj*/

                    melt_dbgtrace_written_object (/*_.R__V2*/ meltfptr[1], "put-fields");
                    ;
                    /*epilog*/
                }
                ;
            } /*noelse*/
        ;
        /*epilog*/
    }

    ;
    goto meltlabend_rout;
meltlabend_rout:
    melt_trace_end("SET_REFERENCE", meltcallcount);
    melt_blocklevel_signals = current_blocklevel_signals_meltrout_28_WARMELTmiBASE_SET_REFERENCE_melt;
    return (melt_ptr_t)(/*noretval*/ NULL);
#undef meltcallcount
#undef meltfram__
#undef MELTFRAM_NBVARNUM
#undef MELTFRAM_NBVARPTR
} /*end meltrout_28_WARMELTmiBASE_SET_REFERENCE*/





melt_ptr_t MELT_MODULE_VISIBILITY
meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_,
        const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_)
{
    long current_blocklevel_signals_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR_melt =  melt_blocklevel_signals;

#if MELT_HAVE_DEBUG
    static long melt_call_counter__;
    long melt_thiscallcounter__ ATTRIBUTE_UNUSED = ++ melt_call_counter__;
#undef meltcallcount
#define meltcallcount melt_thiscallcounter__
#else
#undef meltcallcount
#define meltcallcount 0L
#endif
//// emitted by output_curframe_declstruct_init using OUTPUT_CURFRAME_DECLSTRUCT @warmelt-outobj.melt:1501

    /* start of frame for routine meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR fromline 1716 */

    /** start of frame for meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR of CLASS_PROCROUTINEOBJ from 1518**/

    /*curframdeclclassy*/ class MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR// fromline 1531
        : public Melt_CallFrameWithValues<5>
    {
    public: /* fromline 1535*/
        long mcfr_varnum[1];
        /*classy others*/
        long _meltsparel;
        void melt_mark_stuff (void)
        {
        } /*end melt_mark_stuff*/
        virtual void melt_mark_ggc_data (void)
        {
            melt_mark_values ();
            melt_mark_stuff ();
        }; /*end melt_mark_ggc_data*/
        MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR(meltclosure_ptr_t clos) //the closure constructor fromline 1594
            : Melt_CallFrameWithValues<5> (
#if ENABLE_CHECKING /*fromline 1598*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1602*/
                sizeof(MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR), clos) {};
        MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR() //the constructor fromline 1606
            : Melt_CallFrameWithValues<5> (
#if ENABLE_CHECKING /*fromline 1610*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1614*/
                sizeof(MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR)) {};
#if ENABLE_CHECKING /*fromline 1618*/
        MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR(const char*fil, int lin) //the constructor fromline 1620
            : Melt_CallFrameWithValues<5> (fil,lin, sizeof(MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR)) {};
        MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR(const char*fil, int lin, meltclosure_ptr_t clos) //the closure constructor fromline 1625
            : Melt_CallFrameWithValues<5> (fil,lin, sizeof(MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR), clos) {};
#endif /* ENABLE_CHECKING fromline 1630*/

    }; // end  class MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR


    /** end of frame for meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR fromline 1661**/

    /* end of frame for routine meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR fromline 1720 */

    /* classy proc frame meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR */
    MeltFrame_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR
    meltfram__ /*declfrastruct fromline 1744*/
    /*classyprocarg meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR fromline 1749*/
#if ENABLE_CHECKING
    (__FILE__, __LINE__, meltclosp_) /* fromline 1753*/
#else /*ENABLE_CHECKING */
    (meltclosp_) /* fromline 1757*/
#endif /*ENABLE_CHECKING */

    ;
#define meltframe meltfram__

    melt_trace_start("INSTALL_VALUE_DESCRIPTOR", meltcallcount);
    /*use arguments*/
    (void) meltclosp_;
    (void)meltfirstargp_;
    (void)meltxargdescr_;
    (void)meltxargtab_;
    (void)meltxresdescr_;
    (void)meltxrestab_;
    /*getargs*/

    /*getarg#0*/
    MELT_LOCATION("warmelt-base.melt:1904:/ getarg");
    /*_.VD__V2*/
    meltfptr[1] = (melt_ptr_t) meltfirstargp_;
    ;
    goto meltlab_endgetargs;
meltlab_endgetargs:
    ;
    /*body*/
    /*^block*/
    /*anyblock*/
    {


#if MELT_HAVE_DEBUG
        MELT_LOCATION("warmelt-base.melt:1905:/ cppif.then");
        /*^block*/
        /*anyblock*/
        {


            MELT_CHECK_SIGNAL();
            ;
            /*_#IS_A__L1*/
            meltfnum[0] =
                melt_is_instance_of((melt_ptr_t)(/*_.VD__V2*/ meltfptr[1]), (melt_ptr_t)((/*!CLASS_VALUE_DESCRIPTOR*/ meltfrout->tabval[0])));;
            MELT_LOCATION("warmelt-base.melt:1905:/ cond");
            /*cond*/
            if (/*_#IS_A__L1*/ meltfnum[0]) /*then*/
                {
                    /*^cond.then*/
                    /*_._IFELSE___V4*/ meltfptr[3] = (/*nil*/NULL);;
                }
            else
                {
                    MELT_LOCATION("warmelt-base.melt:1905:/ cond.else");

                    /*^block*/
                    /*anyblock*/
                    {




                        {
                            /*^locexp*/
                            melt_assert_failed(( "check vd"),( "warmelt-base.melt")?( "warmelt-base.melt"):__FILE__,
                                               (1905)?(1905):__LINE__, __FUNCTION__);
                            ;
                        }
                        ;
                        /*clear*/ /*_._IFELSE___V4*/
                        meltfptr[3] = 0 ;
                        /*epilog*/
                    }
                    ;
                }
            ;
            /*^compute*/
            /*_.IFCPP___V3*/
            meltfptr[2] = /*_._IFELSE___V4*/ meltfptr[3];;
            /*epilog*/

            MELT_LOCATION("warmelt-base.melt:1905:/ clear");
            /*clear*/ /*_#IS_A__L1*/
            meltfnum[0] = 0 ;
            /*^clear*/
            /*clear*/ /*_._IFELSE___V4*/
            meltfptr[3] = 0 ;
        }

#else /*MELT_HAVE_DEBUG*/
        /*^cppif.else*/
        /*_.IFCPP___V3*/ meltfptr[2] = (/*nil*/NULL);
#endif /*MELT_HAVE_DEBUG*/
        ;
        MELT_LOCATION("warmelt-base.melt:1906:/ cond");
        /*cond*/
        if (
            /*ifisa*/ melt_is_instance_of((melt_ptr_t)((/*!VALUE_DESCRIPTOR_LIST_REFERENCE*/ meltfrout->tabval[1])),
                                          (melt_ptr_t)((/*!CLASS_REFERENCE*/ meltfrout->tabval[2])))
        ) /*then*/
            {
                /*^cond.then*/
                /*^getslot*/
                {
                    melt_ptr_t slot=NULL, obj=NULL;
                    obj = (melt_ptr_t)((/*!VALUE_DESCRIPTOR_LIST_REFERENCE*/ meltfrout->tabval[1])) /*=obj*/;
                    melt_object_get_field(slot,obj, 0, "REFERENCED_VALUE");
                    /*_.REFERENCED_VALUE__V5*/
                    meltfptr[3] = slot;
                };
                ;
            }
        else    /*^cond.else*/
            {

                /*_.REFERENCED_VALUE__V5*/ meltfptr[3] =  /*fromline 1341*/ NULL ;;
            }
        ;

        {
            MELT_LOCATION("warmelt-base.melt:1906:/ locexp");
            meltgc_append_list((melt_ptr_t)(/*_.REFERENCED_VALUE__V5*/ meltfptr[3]), (melt_ptr_t)(/*_.VD__V2*/ meltfptr[1]));
        }
        ;

        MELT_CHECK_SIGNAL();
        ;
        /*epilog*/

        MELT_LOCATION("warmelt-base.melt:1904:/ clear");
        /*clear*/ /*_.IFCPP___V3*/
        meltfptr[2] = 0 ;
        /*^clear*/
        /*clear*/ /*_.REFERENCED_VALUE__V5*/
        meltfptr[3] = 0 ;
    }

    ;
    goto meltlabend_rout;
meltlabend_rout:
    melt_trace_end("INSTALL_VALUE_DESCRIPTOR", meltcallcount);
    melt_blocklevel_signals = current_blocklevel_signals_meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR_melt;
    return (melt_ptr_t)(/*noretval*/ NULL);
#undef meltcallcount
#undef meltfram__
#undef MELTFRAM_NBVARNUM
#undef MELTFRAM_NBVARPTR
} /*end meltrout_29_WARMELTmiBASE_INSTALL_VALUE_DESCRIPTOR*/





melt_ptr_t MELT_MODULE_VISIBILITY
meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_,
        const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_)
{
    long current_blocklevel_signals_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST_melt =  melt_blocklevel_signals;

#if MELT_HAVE_DEBUG
    static long melt_call_counter__;
    long melt_thiscallcounter__ ATTRIBUTE_UNUSED = ++ melt_call_counter__;
#undef meltcallcount
#define meltcallcount melt_thiscallcounter__
#else
#undef meltcallcount
#define meltcallcount 0L
#endif
//// emitted by output_curframe_declstruct_init using OUTPUT_CURFRAME_DECLSTRUCT @warmelt-outobj.melt:1501

    /* start of frame for routine meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST fromline 1716 */

    /** start of frame for meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST of CLASS_PROCROUTINEOBJ from 1518**/

    /*curframdeclclassy*/ class MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST// fromline 1531
        : public Melt_CallFrameWithValues<2>
    {
    public: /* fromline 1535*/
// no classy longs
        /*classy others*/
        long _meltsparel;
        void melt_mark_stuff (void)
        {
        } /*end melt_mark_stuff*/
        virtual void melt_mark_ggc_data (void)
        {
            melt_mark_values ();
            melt_mark_stuff ();
        }; /*end melt_mark_ggc_data*/
        MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST(meltclosure_ptr_t clos) //the closure constructor fromline 1594
            : Melt_CallFrameWithValues<2> (
#if ENABLE_CHECKING /*fromline 1598*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1602*/
                sizeof(MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST), clos) {};
        MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST() //the constructor fromline 1606
            : Melt_CallFrameWithValues<2> (
#if ENABLE_CHECKING /*fromline 1610*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1614*/
                sizeof(MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST)) {};
#if ENABLE_CHECKING /*fromline 1618*/
        MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST(const char*fil, int lin) //the constructor fromline 1620
            : Melt_CallFrameWithValues<2> (fil,lin, sizeof(MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST)) {};
        MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST(const char*fil, int lin, meltclosure_ptr_t clos) //the closure constructor fromline 1625
            : Melt_CallFrameWithValues<2> (fil,lin, sizeof(MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST), clos) {};
#endif /* ENABLE_CHECKING fromline 1630*/

    }; // end  class MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST


    /** end of frame for meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST fromline 1661**/

    /* end of frame for routine meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST fromline 1720 */

    /* classy proc frame meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST */
    MeltFrame_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST
    meltfram__ /*declfrastruct fromline 1744*/
    /*classyprocarg meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST fromline 1749*/
#if ENABLE_CHECKING
    (__FILE__, __LINE__, meltclosp_) /* fromline 1753*/
#else /*ENABLE_CHECKING */
    (meltclosp_) /* fromline 1757*/
#endif /*ENABLE_CHECKING */

    ;
#define meltframe meltfram__

    melt_trace_start("RETRIEVE_VALUE_DESCRIPTOR_LIST", meltcallcount);
    /*use arguments*/
    (void) meltclosp_;
    (void)meltfirstargp_;
    (void)meltxargdescr_;
    (void)meltxargtab_;
    (void)meltxresdescr_;
    (void)meltxrestab_;
    /*getargs*/
    ;
    goto meltlab_endgetargs;
meltlab_endgetargs:
    ;
    /*body*/
    MELT_LOCATION("warmelt-base.melt:1909:/ block");
    /*anyblock*/
    {

        MELT_LOCATION("warmelt-base.melt:1910:/ cond");
        /*cond*/
        if (
            /*ifisa*/ melt_is_instance_of((melt_ptr_t)((/*!VALUE_DESCRIPTOR_LIST_REFERENCE*/ meltfrout->tabval[0])),
                                          (melt_ptr_t)((/*!CLASS_REFERENCE*/ meltfrout->tabval[1])))
        ) /*then*/
            {
                /*^cond.then*/
                /*^getslot*/
                {
                    melt_ptr_t slot=NULL, obj=NULL;
                    obj = (melt_ptr_t)((/*!VALUE_DESCRIPTOR_LIST_REFERENCE*/ meltfrout->tabval[0])) /*=obj*/;
                    melt_object_get_field(slot,obj, 0, "REFERENCED_VALUE");
                    /*_.REFERENCED_VALUE__V2*/
                    meltfptr[1] = slot;
                };
                ;
            }
        else    /*^cond.else*/
            {

                /*_.REFERENCED_VALUE__V2*/ meltfptr[1] =  /*fromline 1341*/ NULL ;;
            }
        ;
        MELT_LOCATION("warmelt-base.melt:1909:/ quasiblock");


        /*_._RETVAL___V1*/
        meltfptr[0] = /*_.REFERENCED_VALUE__V2*/ meltfptr[1];;

        {
            MELT_LOCATION("warmelt-base.melt:1909:/ locexp");
            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                melt_warn_for_no_expected_secondary_results();
            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
            ;
        }
        ;
        /*^finalreturn*/
        ;
        /*finalret*/
        goto meltlabend_rout ;
        /*epilog*/

        /*^clear*/
        /*clear*/ /*_.REFERENCED_VALUE__V2*/
        meltfptr[1] = 0 ;
    }

    ;
    goto meltlabend_rout;
meltlabend_rout:
    melt_trace_end("RETRIEVE_VALUE_DESCRIPTOR_LIST", meltcallcount);
    melt_blocklevel_signals = current_blocklevel_signals_meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST_melt;
    return (melt_ptr_t)(/*_._RETVAL___V1*/ meltfptr[0]);
#undef meltcallcount
#undef meltfram__
#undef MELTFRAM_NBVARNUM
#undef MELTFRAM_NBVARPTR
} /*end meltrout_30_WARMELTmiBASE_RETRIEVE_VALUE_DESCRIPTOR_LIST*/





melt_ptr_t MELT_MODULE_VISIBILITY
meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_,
        const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_)
{
    long current_blocklevel_signals_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT_melt =  melt_blocklevel_signals;

#if MELT_HAVE_DEBUG
    static long melt_call_counter__;
    long melt_thiscallcounter__ ATTRIBUTE_UNUSED = ++ melt_call_counter__;
#undef meltcallcount
#define meltcallcount melt_thiscallcounter__
#else
#undef meltcallcount
#define meltcallcount 0L
#endif
//// emitted by output_curframe_declstruct_init using OUTPUT_CURFRAME_DECLSTRUCT @warmelt-outobj.melt:1501

    /* start of frame for routine meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT fromline 1716 */

    /** start of frame for meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT of CLASS_PROCROUTINEOBJ from 1518**/

    /*curframdeclclassy*/ class MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT// fromline 1531
        : public Melt_CallFrameWithValues<18>
    {
    public: /* fromline 1535*/
        long mcfr_varnum[6];
        /*classy others*/
        long _meltsparel;
        void melt_mark_stuff (void)
        {
        } /*end melt_mark_stuff*/
        virtual void melt_mark_ggc_data (void)
        {
            melt_mark_values ();
            melt_mark_stuff ();
        }; /*end melt_mark_ggc_data*/
        MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT(meltclosure_ptr_t clos) //the closure constructor fromline 1594
            : Melt_CallFrameWithValues<18> (
#if ENABLE_CHECKING /*fromline 1598*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1602*/
                sizeof(MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT), clos) {};
        MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT() //the constructor fromline 1606
            : Melt_CallFrameWithValues<18> (
#if ENABLE_CHECKING /*fromline 1610*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1614*/
                sizeof(MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT)) {};
#if ENABLE_CHECKING /*fromline 1618*/
        MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT(const char*fil, int lin) //the constructor fromline 1620
            : Melt_CallFrameWithValues<18> (fil,lin, sizeof(MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT)) {};
        MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT(const char*fil, int lin, meltclosure_ptr_t clos) //the closure constructor fromline 1625
            : Melt_CallFrameWithValues<18> (fil,lin, sizeof(MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT), clos) {};
#endif /* ENABLE_CHECKING fromline 1630*/

    }; // end  class MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT


    /** end of frame for meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT fromline 1661**/

    /* end of frame for routine meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT fromline 1720 */

    /* classy proc frame meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT */
    MeltFrame_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT
    meltfram__ /*declfrastruct fromline 1744*/
    /*classyprocarg meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT fromline 1749*/
#if ENABLE_CHECKING
    (__FILE__, __LINE__, meltclosp_) /* fromline 1753*/
#else /*ENABLE_CHECKING */
    (meltclosp_) /* fromline 1757*/
#endif /*ENABLE_CHECKING */

    ;
#define meltframe meltfram__

    melt_trace_start("LIST_REMOVE_LAST_ELEMENT", meltcallcount);
    /*use arguments*/
    (void) meltclosp_;
    (void)meltfirstargp_;
    (void)meltxargdescr_;
    (void)meltxargtab_;
    (void)meltxresdescr_;
    (void)meltxrestab_;
    /*getargs*/

    /*getarg#0*/
    MELT_LOCATION("warmelt-base.melt:3003:/ getarg");
    /*_.LIS__V2*/
    meltfptr[1] = (melt_ptr_t) meltfirstargp_;
    ;
    goto meltlab_endgetargs;
meltlab_endgetargs:
    ;
    /*body*/
    /*^block*/
    /*anyblock*/
    {


        MELT_CHECK_SIGNAL();
        ;
        /*_#IS_LIST__L1*/
        meltfnum[0] =
            (melt_magic_discr((melt_ptr_t)(/*_.LIS__V2*/ meltfptr[1])) == MELTOBMAG_LIST);;
        /*^compute*/
        /*_#NOT__L2*/
        meltfnum[1] =
            (!(/*_#IS_LIST__L1*/ meltfnum[0]));;
        MELT_LOCATION("warmelt-base.melt:3007:/ cond");
        /*cond*/
        if (/*_#NOT__L2*/ meltfnum[1]) /*then*/
            {
                /*^cond.then*/
                /*^block*/
                /*anyblock*/
                {


                    MELT_CHECK_SIGNAL();
                    ;
                    MELT_LOCATION("warmelt-base.melt:3008:/ quasiblock");


                    /*_._RETVAL___V1*/
                    meltfptr[0] = (/*nil*/NULL);;

                    {
                        MELT_LOCATION("warmelt-base.melt:3008:/ locexp");
                        /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
                        if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                            melt_warn_for_no_expected_secondary_results();
                        /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
                        ;
                    }
                    ;
                    /*^finalreturn*/
                    ;
                    /*finalret*/
                    goto meltlabend_rout ;
                    /*_._IF___V3*/
                    meltfptr[2] = /*_.RETURN___V4*/ meltfptr[3];;
                    /*epilog*/

                    MELT_LOCATION("warmelt-base.melt:3007:/ clear");
                    /*clear*/ /*_.RETURN___V4*/
                    meltfptr[3] = 0 ;
                }
                ;
            }
        else    /*^cond.else*/
            {

                /*_._IF___V3*/ meltfptr[2] =  /*fromline 1341*/ NULL ;;
            }
        ;
        MELT_LOCATION("warmelt-base.melt:3009:/ quasiblock");


        /*_.LASTPAIR__V5*/
        meltfptr[3] =
            (melt_list_last((melt_ptr_t)(/*_.LIS__V2*/ meltfptr[1])));;
        /*^compute*/
        /*_.PREVPAIR__V6*/
        meltfptr[5] = (/*nil*/NULL);;

        MELT_CHECK_SIGNAL();
        ;
        MELT_LOCATION("warmelt-base.melt:3012:/ cond");
        /*cond*/
        if (/*_.LASTPAIR__V5*/ meltfptr[3]) /*then*/
            {
                /*^cond.then*/
                /*^block*/
                /*anyblock*/
                {


#if MELT_HAVE_DEBUG
                    /*^cppif.then*/
                    /*^block*/
                    /*anyblock*/
                    {


                        MELT_CHECK_SIGNAL();
                        ;
                        /*_#IS_PAIR__L3*/
                        meltfnum[2] =
                        (melt_magic_discr((melt_ptr_t)(/*_.LASTPAIR__V5*/ meltfptr[3])) == MELTOBMAG_PAIR);;
                        MELT_LOCATION("warmelt-base.melt:3012:/ cond");
                        /*cond*/
                        if (/*_#IS_PAIR__L3*/ meltfnum[2]) /*then*/
                            {
                                /*^cond.then*/
                                /*_._IFELSE___V9*/ meltfptr[8] = (/*nil*/NULL);;
                            }
                        else {
                                MELT_LOCATION("warmelt-base.melt:3012:/ cond.else");

                                /*^block*/
                                /*anyblock*/
                                {




                                    {
                                        /*^locexp*/
                                        melt_assert_failed(( "check lastpair"),( "warmelt-base.melt")?( "warmelt-base.melt"):__FILE__,
                                        (3012)?(3012):__LINE__, __FUNCTION__);
                                        ;
                                    }
                                    ;
                                    /*clear*/ /*_._IFELSE___V9*/
                                    meltfptr[8] = 0 ;
                                    /*epilog*/
                                }
                                ;
                            }
                        ;
                        /*^compute*/
                        /*_.IFCPP___V8*/
                        meltfptr[7] = /*_._IFELSE___V9*/ meltfptr[8];;
                        /*epilog*/

                        MELT_LOCATION("warmelt-base.melt:3012:/ clear");
                        /*clear*/ /*_#IS_PAIR__L3*/
                        meltfnum[2] = 0 ;
                        /*^clear*/
                        /*clear*/ /*_._IFELSE___V9*/
                        meltfptr[8] = 0 ;
                    }

#else /*MELT_HAVE_DEBUG*/
                    /*^cppif.else*/
                    /*_.IFCPP___V8*/ meltfptr[7] = (/*nil*/NULL);
#endif /*MELT_HAVE_DEBUG*/
                    ;
                    /*^compute*/
                    /*_._IF___V7*/
                    meltfptr[6] = /*_.IFCPP___V8*/ meltfptr[7];;
                    /*epilog*/

                    MELT_LOCATION("warmelt-base.melt:3012:/ clear");
                    /*clear*/ /*_.IFCPP___V8*/
                    meltfptr[7] = 0 ;
                }
                ;
            }
        else    /*^cond.else*/
            {

                /*_._IF___V7*/ meltfptr[6] =  /*fromline 1341*/ NULL ;;
            }
        ;
        /*citerblock FOREACH_IN_LIST*/
        {
            /* start foreach_in_list meltcit1__EACHLIST */
            for (/*_.CURPAIR__V10*/ meltfptr[8] = melt_list_first( (melt_ptr_t)/*_.LIS__V2*/ meltfptr[1]);
                                    melt_magic_discr((melt_ptr_t) /*_.CURPAIR__V10*/ meltfptr[8]) == MELTOBMAG_PAIR;
                                    /*_.CURPAIR__V10*/ meltfptr[8] = melt_pair_tail((melt_ptr_t) /*_.CURPAIR__V10*/ meltfptr[8]))
                {
                    /*_.CURELEM__V11*/ meltfptr[7] = melt_pair_head((melt_ptr_t) /*_.CURPAIR__V10*/ meltfptr[8]);



#if MELT_HAVE_DEBUG
                    MELT_LOCATION("warmelt-base.melt:3016:/ cppif.then");
                    /*^block*/
                    /*anyblock*/
                    {


                        MELT_CHECK_SIGNAL();
                        ;
                        /*_#IS_PAIR__L4*/
                        meltfnum[2] =
                            (melt_magic_discr((melt_ptr_t)(/*_.CURPAIR__V10*/ meltfptr[8])) == MELTOBMAG_PAIR);;
                        MELT_LOCATION("warmelt-base.melt:3016:/ cond");
                        /*cond*/
                        if (/*_#IS_PAIR__L4*/ meltfnum[2]) /*then*/
                            {
                                /*^cond.then*/
                                /*_._IFELSE___V13*/ meltfptr[12] = (/*nil*/NULL);;
                            }
                        else
                            {
                                MELT_LOCATION("warmelt-base.melt:3016:/ cond.else");

                                /*^block*/
                                /*anyblock*/
                                {




                                    {
                                        /*^locexp*/
                                        melt_assert_failed(( "check curpair"),( "warmelt-base.melt")?( "warmelt-base.melt"):__FILE__,
                                                           (3016)?(3016):__LINE__, __FUNCTION__);
                                        ;
                                    }
                                    ;
                                    /*clear*/ /*_._IFELSE___V13*/
                                    meltfptr[12] = 0 ;
                                    /*epilog*/
                                }
                                ;
                            }
                        ;
                        /*^compute*/
                        /*_.IFCPP___V12*/
                        meltfptr[11] = /*_._IFELSE___V13*/ meltfptr[12];;
                        /*epilog*/

                        MELT_LOCATION("warmelt-base.melt:3016:/ clear");
                        /*clear*/ /*_#IS_PAIR__L4*/
                        meltfnum[2] = 0 ;
                        /*^clear*/
                        /*clear*/ /*_._IFELSE___V13*/
                        meltfptr[12] = 0 ;
                    }

#else /*MELT_HAVE_DEBUG*/
                    /*^cppif.else*/
                    /*_.IFCPP___V12*/ meltfptr[11] = (/*nil*/NULL);
#endif /*MELT_HAVE_DEBUG*/
                    ;

                    MELT_CHECK_SIGNAL();
                    ;
                    /*^compute*/
                    /*_#eqeq__L5*/
                    meltfnum[2] =
                        ((/*_.CURPAIR__V10*/ meltfptr[8]) == (/*_.LASTPAIR__V5*/ meltfptr[3]));;
                    MELT_LOCATION("warmelt-base.melt:3017:/ cond");
                    /*cond*/
                    if (/*_#eqeq__L5*/ meltfnum[2]) /*then*/
                        {
                            /*^cond.then*/
                            /*^block*/
                            /*anyblock*/
                            {

                                MELT_LOCATION("warmelt-base.melt:3018:/ compute");
                                /*_.CURPAIR__V10*/
                                meltfptr[8] = /*_.SETQ___V15*/ meltfptr[14] = (/*nil*/NULL);;

                                MELT_CHECK_SIGNAL();
                                ;
                                /*_#IS_PAIR__L6*/
                                meltfnum[5] =
                                    (melt_magic_discr((melt_ptr_t)(/*_.PREVPAIR__V6*/ meltfptr[5])) == MELTOBMAG_PAIR);;
                                MELT_LOCATION("warmelt-base.melt:3019:/ cond");
                                /*cond*/
                                if (/*_#IS_PAIR__L6*/ meltfnum[5]) /*then*/
                                    {
                                        /*^cond.then*/
                                        /*^block*/
                                        /*anyblock*/
                                        {


                                            {
                                                MELT_LOCATION("warmelt-base.melt:3020:/ locexp");
                                                /* list_remove_last_element UPDATEPAIR_CHK__1 */
                                                ((struct meltpair_st*)/*_.PREVPAIR__V6*/ meltfptr[5])->tl = NULL;
                                                ((struct meltlist_st*)/*_.LIS__V2*/ meltfptr[1])->last
                                                    = (struct meltpair_st*) /*_.PREVPAIR__V6*/ meltfptr[5];
                                                meltgc_touch_dest (/*_.LIS__V2*/ meltfptr[1], /*_.PREVPAIR__V6*/ meltfptr[5]);
                                                ;
                                            }
                                            ;
                                            /*epilog*/
                                        }
                                        ;
                                    }
                                else
                                    {
                                        MELT_LOCATION("warmelt-base.melt:3019:/ cond.else");

                                        /*^block*/
                                        /*anyblock*/
                                        {


                                            {
                                                MELT_LOCATION("warmelt-base.melt:3027:/ locexp");
                                                /* list_remove_last_element EMPTYLIST_CHK__1 */
                                                ((struct meltlist_st*)/*_.LIS__V2*/ meltfptr[1])->first = NULL;
                                                ((struct meltlist_st*)/*_.LIS__V2*/ meltfptr[1])->last = NULL;
                                                meltgc_touch (/*_.LIS__V2*/ meltfptr[1]);
                                                ;
                                            }
                                            ;
                                            /*epilog*/
                                        }
                                        ;
                                    }
                                ;

                                MELT_CHECK_SIGNAL();
                                ;
                                MELT_LOCATION("warmelt-base.melt:3033:/ quasiblock");


                                /*_._RETVAL___V1*/
                                meltfptr[0] = /*_.CURELEM__V11*/ meltfptr[7];;

                                {
                                    MELT_LOCATION("warmelt-base.melt:3033:/ locexp");
                                    /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
                                    if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                                        melt_warn_for_no_expected_secondary_results();
                                    /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
                                    ;
                                }
                                ;
                                /*^finalreturn*/
                                ;
                                /*finalret*/
                                goto meltlabend_rout ;
                                MELT_LOCATION("warmelt-base.melt:3017:/ quasiblock");


                                /*_.PROGN___V17*/
                                meltfptr[16] = /*_.RETURN___V16*/ meltfptr[15];;
                                /*^compute*/
                                /*_._IF___V14*/
                                meltfptr[12] = /*_.PROGN___V17*/ meltfptr[16];;
                                /*epilog*/

                                MELT_LOCATION("warmelt-base.melt:3017:/ clear");
                                /*clear*/ /*_.SETQ___V15*/
                                meltfptr[14] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_#IS_PAIR__L6*/
                                meltfnum[5] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_.RETURN___V16*/
                                meltfptr[15] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_.PROGN___V17*/
                                meltfptr[16] = 0 ;
                            }
                            ;
                        }
                    else    /*^cond.else*/
                        {

                            /*_._IF___V14*/ meltfptr[12] =  /*fromline 1341*/ NULL ;;
                        }
                    ;
                    MELT_LOCATION("warmelt-base.melt:3034:/ compute");
                    /*_.PREVPAIR__V6*/
                    meltfptr[5] = /*_.SETQ___V18*/ meltfptr[14] = /*_.CURPAIR__V10*/ meltfptr[8];;
                } /* end foreach_in_list meltcit1__EACHLIST */
            /*_.CURPAIR__V10*/ meltfptr[8] = NULL;
            /*_.CURELEM__V11*/
            meltfptr[7] = NULL;


            /*citerepilog*/

            MELT_LOCATION("warmelt-base.melt:3013:/ clear");
            /*clear*/ /*_.CURPAIR__V10*/
            meltfptr[8] = 0 ;
            /*^clear*/
            /*clear*/ /*_.CURELEM__V11*/
            meltfptr[7] = 0 ;
            /*^clear*/
            /*clear*/ /*_.IFCPP___V12*/
            meltfptr[11] = 0 ;
            /*^clear*/
            /*clear*/ /*_#eqeq__L5*/
            meltfnum[2] = 0 ;
            /*^clear*/
            /*clear*/ /*_._IF___V14*/
            meltfptr[12] = 0 ;
            /*^clear*/
            /*clear*/ /*_.SETQ___V18*/
            meltfptr[14] = 0 ;
        } /*endciterblock FOREACH_IN_LIST*/
        ;

        MELT_LOCATION("warmelt-base.melt:3009:/ clear");
        /*clear*/ /*_.LASTPAIR__V5*/
        meltfptr[3] = 0 ;
        /*^clear*/
        /*clear*/ /*_.PREVPAIR__V6*/
        meltfptr[5] = 0 ;
        /*^clear*/
        /*clear*/ /*_._IF___V7*/
        meltfptr[6] = 0 ;

        MELT_CHECK_SIGNAL();
        ;
        /*epilog*/

        MELT_LOCATION("warmelt-base.melt:3003:/ clear");
        /*clear*/ /*_#IS_LIST__L1*/
        meltfnum[0] = 0 ;
        /*^clear*/
        /*clear*/ /*_#NOT__L2*/
        meltfnum[1] = 0 ;
        /*^clear*/
        /*clear*/ /*_._IF___V3*/
        meltfptr[2] = 0 ;
    }

    ;
    goto meltlabend_rout;
meltlabend_rout:
    melt_trace_end("LIST_REMOVE_LAST_ELEMENT", meltcallcount);
    melt_blocklevel_signals = current_blocklevel_signals_meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT_melt;
    return (melt_ptr_t)(/*_._RETVAL___V1*/ meltfptr[0]);
#undef meltcallcount
#undef meltfram__
#undef MELTFRAM_NBVARNUM
#undef MELTFRAM_NBVARPTR
} /*end meltrout_31_WARMELTmiBASE_LIST_REMOVE_LAST_ELEMENT*/





melt_ptr_t MELT_MODULE_VISIBILITY
meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_,
        const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_)
{
    /*variadic*/ int variad_COMPLETE_SEQUENCE_AS_TUPLE_ix = 0, variad_COMPLETE_SEQUENCE_AS_TUPLE_len = melt_argdescr_length (meltxargdescr_);
#define melt_variadic_length  (0+variad_COMPLETE_SEQUENCE_AS_TUPLE_len)
#define melt_variadic_index variad_COMPLETE_SEQUENCE_AS_TUPLE_ix

    long current_blocklevel_signals_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE_melt =  melt_blocklevel_signals;

#if MELT_HAVE_DEBUG
    static long melt_call_counter__;
    long melt_thiscallcounter__ ATTRIBUTE_UNUSED = ++ melt_call_counter__;
#undef meltcallcount
#define meltcallcount melt_thiscallcounter__
#else
#undef meltcallcount
#define meltcallcount 0L
#endif
//// emitted by output_curframe_declstruct_init using OUTPUT_CURFRAME_DECLSTRUCT @warmelt-outobj.melt:1501

    /* start of frame for routine meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE fromline 1716 */

    /** start of frame for meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE of CLASS_PROCROUTINEOBJ from 1518**/

    /*curframdeclclassy*/ class MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE// fromline 1531
        : public Melt_CallFrameWithValues<18>
    {
    public: /* fromline 1535*/
        long mcfr_varnum[3];
        /*classy others*/
        long _meltsparel;
        void melt_mark_stuff (void)
        {
        } /*end melt_mark_stuff*/
        virtual void melt_mark_ggc_data (void)
        {
            melt_mark_values ();
            melt_mark_stuff ();
        }; /*end melt_mark_ggc_data*/
        MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE(meltclosure_ptr_t clos) //the closure constructor fromline 1594
            : Melt_CallFrameWithValues<18> (
#if ENABLE_CHECKING /*fromline 1598*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1602*/
                sizeof(MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE), clos) {};
        MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE() //the constructor fromline 1606
            : Melt_CallFrameWithValues<18> (
#if ENABLE_CHECKING /*fromline 1610*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1614*/
                sizeof(MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE)) {};
#if ENABLE_CHECKING /*fromline 1618*/
        MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE(const char*fil, int lin) //the constructor fromline 1620
            : Melt_CallFrameWithValues<18> (fil,lin, sizeof(MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE)) {};
        MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE(const char*fil, int lin, meltclosure_ptr_t clos) //the closure constructor fromline 1625
            : Melt_CallFrameWithValues<18> (fil,lin, sizeof(MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE), clos) {};
#endif /* ENABLE_CHECKING fromline 1630*/

    }; // end  class MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE


    /** end of frame for meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE fromline 1661**/

    /* end of frame for routine meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE fromline 1720 */

    /* classy proc frame meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE */
    MeltFrame_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE
    meltfram__ /*declfrastruct fromline 1744*/
    /*classyprocarg meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE fromline 1749*/
#if ENABLE_CHECKING
    (__FILE__, __LINE__, meltclosp_) /* fromline 1753*/
#else /*ENABLE_CHECKING */
    (meltclosp_) /* fromline 1757*/
#endif /*ENABLE_CHECKING */

    ;
#define meltframe meltfram__

    melt_trace_start("COMPLETE_SEQUENCE_AS_TUPLE", meltcallcount);
    /*use arguments*/
    (void) meltclosp_;
    (void)meltfirstargp_;
    (void)meltxargdescr_;
    (void)meltxargtab_;
    (void)meltxresdescr_;
    (void)meltxrestab_;
    /*getargs*/

    /*getarg#0*/
    MELT_LOCATION("warmelt-base.melt:3089:/ getarg");
    /*_.SRC__V2*/
    meltfptr[1] = (melt_ptr_t) meltfirstargp_;
    ;
    goto meltlab_endgetargs;
meltlab_endgetargs:
    ;
    /*body*/
    /*^block*/
    /*anyblock*/
    {

        MELT_LOCATION("warmelt-base.melt:3093:/ quasiblock");


        /*_.NEWLIST__V4*/
        meltfptr[3] =
            (meltgc_new_list((meltobject_ptr_t)((/*!DISCR_LIST*/ meltfrout->tabval[0]))));;

        MELT_CHECK_SIGNAL();
        ;
        /*^compute*/
        /*_#IS_MULTIPLE__L1*/
        meltfnum[0] =
            (melt_magic_discr((melt_ptr_t)(/*_.SRC__V2*/ meltfptr[1])) == MELTOBMAG_MULTIPLE);;
        MELT_LOCATION("warmelt-base.melt:3095:/ cond");
        /*cond*/
        if (/*_#IS_MULTIPLE__L1*/ meltfnum[0]) /*then*/
            {
                /*^cond.then*/
                /*^block*/
                /*anyblock*/
                {

                    /*citerblock FOREACH_IN_MULTIPLE*/
                    {
                        /* start foreach_in_multiple meltcit1__EACHTUP */
                        long  meltcit1__EACHTUP_ln = melt_multiple_length((melt_ptr_t)/*_.SRC__V2*/ meltfptr[1]);
                        for (/*_#IX__L2*/ meltfnum[1] = 0;
                                          (/*_#IX__L2*/ meltfnum[1] >= 0) && (/*_#IX__L2*/ meltfnum[1] <  meltcit1__EACHTUP_ln);
                                          /*_#IX__L2*/ meltfnum[1]++)
                            {
                                /*_.COMP__V5*/ meltfptr[4] = melt_multiple_nth((melt_ptr_t)(/*_.SRC__V2*/ meltfptr[1]),  /*_#IX__L2*/ meltfnum[1]);




                                {
                                    MELT_LOCATION("warmelt-base.melt:3099:/ locexp");
                                    meltgc_append_list((melt_ptr_t)(/*_.NEWLIST__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.COMP__V5*/ meltfptr[4]));
                                }
                                ;
                                if (/*_#IX__L2*/ meltfnum[1]<0) break;
                            } /* end  foreach_in_multiple meltcit1__EACHTUP */

                        /*citerepilog*/

                        MELT_LOCATION("warmelt-base.melt:3096:/ clear");
                        /*clear*/ /*_.COMP__V5*/
                        meltfptr[4] = 0 ;
                        /*^clear*/
                        /*clear*/ /*_#IX__L2*/
                        meltfnum[1] = 0 ;
                    } /*endciterblock FOREACH_IN_MULTIPLE*/
                    ;
                    /*epilog*/
                }
                ;
            }
        else
            {
                MELT_LOCATION("warmelt-base.melt:3095:/ cond.else");

                /*^block*/
                /*anyblock*/
                {


                    MELT_CHECK_SIGNAL();
                    ;
                    /*_#IS_LIST__L3*/
                    meltfnum[2] =
                        (melt_magic_discr((melt_ptr_t)(/*_.SRC__V2*/ meltfptr[1])) == MELTOBMAG_LIST);;
                    MELT_LOCATION("warmelt-base.melt:3101:/ cond");
                    /*cond*/
                    if (/*_#IS_LIST__L3*/ meltfnum[2]) /*then*/
                        {
                            /*^cond.then*/
                            /*^block*/
                            /*anyblock*/
                            {

                                /*citerblock FOREACH_IN_LIST*/
                                {
                                    /* start foreach_in_list meltcit2__EACHLIST */
                                    for (/*_.CURPAIR__V6*/ meltfptr[5] = melt_list_first( (melt_ptr_t)/*_.SRC__V2*/ meltfptr[1]);
                                                           melt_magic_discr((melt_ptr_t) /*_.CURPAIR__V6*/ meltfptr[5]) == MELTOBMAG_PAIR;
                                                           /*_.CURPAIR__V6*/ meltfptr[5] = melt_pair_tail((melt_ptr_t) /*_.CURPAIR__V6*/ meltfptr[5]))
                                        {
                                            /*_.CURCOMP__V7*/ meltfptr[6] = melt_pair_head((melt_ptr_t) /*_.CURPAIR__V6*/ meltfptr[5]);



                                            {
                                                MELT_LOCATION("warmelt-base.melt:3105:/ locexp");
                                                meltgc_append_list((melt_ptr_t)(/*_.NEWLIST__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.CURCOMP__V7*/ meltfptr[6]));
                                            }
                                            ;
                                        } /* end foreach_in_list meltcit2__EACHLIST */
                                    /*_.CURPAIR__V6*/ meltfptr[5] = NULL;
                                    /*_.CURCOMP__V7*/
                                    meltfptr[6] = NULL;


                                    /*citerepilog*/

                                    MELT_LOCATION("warmelt-base.melt:3102:/ clear");
                                    /*clear*/ /*_.CURPAIR__V6*/
                                    meltfptr[5] = 0 ;
                                    /*^clear*/
                                    /*clear*/ /*_.CURCOMP__V7*/
                                    meltfptr[6] = 0 ;
                                } /*endciterblock FOREACH_IN_LIST*/
                                ;
                                /*epilog*/
                            }
                            ;
                        }
                    else
                        {
                            MELT_LOCATION("warmelt-base.melt:3101:/ cond.else");

                            /*^block*/
                            /*anyblock*/
                            {


                                {
                                    MELT_LOCATION("warmelt-base.melt:3108:/ locexp");
                                    meltgc_append_list((melt_ptr_t)(/*_.NEWLIST__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.SRC__V2*/ meltfptr[1]));
                                }
                                ;
                                MELT_LOCATION("warmelt-base.melt:3107:/ quasiblock");


                                /*epilog*/
                            }
                            ;
                        }
                    ;
                    /*epilog*/

                    MELT_LOCATION("warmelt-base.melt:3095:/ clear");
                    /*clear*/ /*_#IS_LIST__L3*/
                    meltfnum[2] = 0 ;
                }
                ;
            }
        ;
        MELT_LOCATION("warmelt-base.melt:3109:/ loop");
        /*loop*/
        {
meltlabloop_ARGLOOP_3:;/*^loopbody*/

            /*^block*/
            /*anyblock*/
            {


                MELT_CHECK_SIGNAL();
                ;
                MELT_LOCATION("warmelt-base.melt:3111:/ cond");
                /*cond*/
                if (/*ifvariadic nomore*/ variad_COMPLETE_SEQUENCE_AS_TUPLE_ix == variad_COMPLETE_SEQUENCE_AS_TUPLE_len) /*then*/
                    {
                        /*^cond.then*/
                        /*^block*/
                        /*anyblock*/
                        {

                            /*^compute*/

                            /*consume variadic  !*/ variad_COMPLETE_SEQUENCE_AS_TUPLE_ix += 0;;
                            MELT_LOCATION("warmelt-base.melt:3113:/ quasiblock");


                            /*^compute*/
                            /*_.ARGLOOP__V9*/
                            meltfptr[8] =  /*fromline 1341*/ NULL ;;

                            /*^exit*/
                            /*exit*/
                            {
                                goto meltlabexit_ARGLOOP_3;
                            }
                            ;
                            /*epilog*/
                        }
                        ;
                    }
                else
                    {
                        MELT_LOCATION("warmelt-base.melt:3111:/ cond.else");

                        /*^block*/
                        /*anyblock*/
                        {

                            /*^cond*/
                            /*cond*/ if (/*ifvariadic arg#1*/ variad_COMPLETE_SEQUENCE_AS_TUPLE_ix>=0 && variad_COMPLETE_SEQUENCE_AS_TUPLE_ix + 1 <= variad_COMPLETE_SEQUENCE_AS_TUPLE_len && meltxargdescr_[variad_COMPLETE_SEQUENCE_AS_TUPLE_ix]== MELTBPAR_PTR) /*then*/
                                {
                                    /*^cond.then*/
                                    /*^block*/
                                    /*anyblock*/
                                    {

                                        /*^compute*/
                                        /*_.V__V10*/ meltfptr[9] =
                                            /*variadic argument value*/ ((meltxargtab_[variad_COMPLETE_SEQUENCE_AS_TUPLE_ix + 0].meltbp_aptr) ? (*(meltxargtab_[variad_COMPLETE_SEQUENCE_AS_TUPLE_ix + 0].meltbp_aptr)) : NULL);;
                                        /*^compute*/

                                        /*consume variadic Value !*/
                                        variad_COMPLETE_SEQUENCE_AS_TUPLE_ix += 1;;

                                        {
                                            MELT_LOCATION("warmelt-base.melt:3115:/ locexp");
                                            meltgc_append_list((melt_ptr_t)(/*_.NEWLIST__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.V__V10*/ meltfptr[9]));
                                        }
                                        ;
                                        /*epilog*/

                                        MELT_LOCATION("warmelt-base.melt:3111:/ clear");
                                        /*clear*/ /*_.V__V10*/
                                        meltfptr[9] = 0 ;
                                    }
                                    ;
                                }
                            else    /*^cond.else*/
                                {

                                    /*^block*/
                                    /*anyblock*/
                                    {

                                        MELT_LOCATION("warmelt-base.melt:3117:/ quasiblock");


                                        /*_.VCTY__V12*/
                                        meltfptr[11] =
                                            /*variadic_type_code*/
#ifdef melt_variadic_index
                                            (((melt_variadic_index + 0) >= 0
                                              && (melt_variadic_index + 0) < melt_variadic_length)
                                             ?  melt_code_to_ctype (meltxargdescr_[melt_variadic_index + 0]
                                                                    & MELT_ARGDESCR_MAX)
                                             : NULL)
#else
                                            NULL /* no variadic_ctype outside of variadic functions */
#endif /*melt_variadic_index*/
                                            ;;
                                        MELT_LOCATION("warmelt-base.melt:3120:/ cond");
                                        /*cond*/
                                        if (
                                            /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.VCTY__V12*/ meltfptr[11]),
                                                                          (melt_ptr_t)((/*!CLASS_NAMED*/ meltfrout->tabval[1])))
                                        ) /*then*/
                                            {
                                                /*^cond.then*/
                                                /*^getslot*/
                                                {
                                                    melt_ptr_t slot=NULL, obj=NULL;
                                                    obj = (melt_ptr_t)(/*_.VCTY__V12*/ meltfptr[11]) /*=obj*/;
                                                    melt_object_get_field(slot,obj, 1, "NAMED_NAME");
                                                    /*_.NAMED_NAME__V13*/
                                                    meltfptr[12] = slot;
                                                };
                                                ;
                                            }
                                        else    /*^cond.else*/
                                            {

                                                /*_.NAMED_NAME__V13*/ meltfptr[12] =  /*fromline 1341*/ NULL ;;
                                            }
                                        ;

                                        {
                                            MELT_LOCATION("warmelt-base.melt:3119:/ locexp");
                                            error ("MELT ERROR MSG [#%ld]::: %s - %s", melt_dbgcounter, ( "COMPLETE_SEQUENCE_AS_TUPLE with unsupported ctype"),
                                                   melt_string_str((melt_ptr_t)(/*_.NAMED_NAME__V13*/ meltfptr[12])));
                                        }
                                        ;

#if MELT_HAVE_DEBUG
                                        MELT_LOCATION("warmelt-base.melt:3121:/ cppif.then");
                                        /*^block*/
                                        /*anyblock*/
                                        {


                                            MELT_CHECK_SIGNAL();
                                            ;
                                            /*^cond*/
                                            /*cond*/
                                            if ((/*nil*/NULL)) /*then*/
                                                {
                                                    /*^cond.then*/
                                                    /*_._IFELSE___V15*/ meltfptr[14] = (/*nil*/NULL);;
                                                }
                                            else
                                                {
                                                    MELT_LOCATION("warmelt-base.melt:3121:/ cond.else");

                                                    /*^block*/
                                                    /*anyblock*/
                                                    {




                                                        {
                                                            /*^locexp*/
                                                            melt_assert_failed(( "invalid variadic argument to COMPLETE_SEQUENCE_AS_TUPLE"),( "warmelt-base.melt")?( "warmelt-base.melt"):__FILE__,
                                                                               (3121)?(3121):__LINE__, __FUNCTION__);
                                                            ;
                                                        }
                                                        ;
                                                        /*clear*/ /*_._IFELSE___V15*/
                                                        meltfptr[14] = 0 ;
                                                        /*epilog*/
                                                    }
                                                    ;
                                                }
                                            ;
                                            /*^compute*/
                                            /*_.IFCPP___V14*/
                                            meltfptr[13] = /*_._IFELSE___V15*/ meltfptr[14];;
                                            /*epilog*/

                                            MELT_LOCATION("warmelt-base.melt:3121:/ clear");
                                            /*clear*/ /*_._IFELSE___V15*/
                                            meltfptr[14] = 0 ;
                                        }

#else /*MELT_HAVE_DEBUG*/
                                        /*^cppif.else*/
                                        /*_.IFCPP___V14*/ meltfptr[13] = (/*nil*/NULL);
#endif /*MELT_HAVE_DEBUG*/
                                        ;
                                        /*^compute*/
                                        /*_.LET___V11*/
                                        meltfptr[9] = /*_.IFCPP___V14*/ meltfptr[13];;

                                        MELT_LOCATION("warmelt-base.melt:3117:/ clear");
                                        /*clear*/ /*_.VCTY__V12*/
                                        meltfptr[11] = 0 ;
                                        /*^clear*/
                                        /*clear*/ /*_.NAMED_NAME__V13*/
                                        meltfptr[12] = 0 ;
                                        /*^clear*/
                                        /*clear*/ /*_.IFCPP___V14*/
                                        meltfptr[13] = 0 ;
                                        /*epilog*/

                                        MELT_LOCATION("warmelt-base.melt:3111:/ clear");
                                        /*clear*/ /*_.LET___V11*/
                                        meltfptr[9] = 0 ;
                                    }
                                    ;
                                }
                            ;
                            /*epilog*/
                        }
                        ;
                    }
                ;
                /*epilog*/
            }
            ;
            ;
            goto meltlabloop_ARGLOOP_3;
meltlabexit_ARGLOOP_3:
            ;
            MELT_LOCATION("warmelt-base.melt:3109:/ loopepilog");
            /*loopepilog*/
            /*_.FOREVER___V8*/
            meltfptr[7] = /*_.ARGLOOP__V9*/ meltfptr[8];;
        }
        ;
        MELT_LOCATION("warmelt-base.melt:3123:/ quasiblock");



        MELT_CHECK_SIGNAL();
        ;
        /*^apply*/
        /*apply*/
        {
            /*_.RES__V17*/ meltfptr[11] =  melt_apply ((meltclosure_ptr_t)((/*!LIST_TO_MULTIPLE*/ meltfrout->tabval[2])), (melt_ptr_t)(/*_.NEWLIST__V4*/ meltfptr[3]), (""), (union meltparam_un*)0, "", (union meltparam_un*)0);
        }
        ;

        MELT_CHECK_SIGNAL();
        ;
        MELT_LOCATION("warmelt-base.melt:3125:/ quasiblock");


        /*_._RETVAL___V1*/
        meltfptr[0] = /*_.RES__V17*/ meltfptr[11];;

        {
            MELT_LOCATION("warmelt-base.melt:3125:/ locexp");
            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                melt_warn_for_no_expected_secondary_results();
            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
            ;
        }
        ;
        /*^finalreturn*/
        ;
        /*finalret*/
        goto meltlabend_rout ;
        /*_.LET___V16*/
        meltfptr[14] = /*_.RETURN___V18*/ meltfptr[12];;

        MELT_LOCATION("warmelt-base.melt:3123:/ clear");
        /*clear*/ /*_.RES__V17*/
        meltfptr[11] = 0 ;
        /*^clear*/
        /*clear*/ /*_.RETURN___V18*/
        meltfptr[12] = 0 ;
        /*_.LET___V3*/
        meltfptr[2] = /*_.LET___V16*/ meltfptr[14];;

        MELT_LOCATION("warmelt-base.melt:3093:/ clear");
        /*clear*/ /*_.NEWLIST__V4*/
        meltfptr[3] = 0 ;
        /*^clear*/
        /*clear*/ /*_#IS_MULTIPLE__L1*/
        meltfnum[0] = 0 ;
        /*^clear*/
        /*clear*/ /*_.FOREVER___V8*/
        meltfptr[7] = 0 ;
        /*^clear*/
        /*clear*/ /*_.LET___V16*/
        meltfptr[14] = 0 ;
        MELT_LOCATION("warmelt-base.melt:3089:/ quasiblock");


        /*_._RETVAL___V1*/
        meltfptr[0] = /*_.LET___V3*/ meltfptr[2];;

        {
            MELT_LOCATION("warmelt-base.melt:3089:/ locexp");
            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                melt_warn_for_no_expected_secondary_results();
            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
            ;
        }
        ;
        /*^finalreturn*/
        ;
        /*finalret*/
        goto meltlabend_rout ;
        /*epilog*/

        /*^clear*/
        /*clear*/ /*_.LET___V3*/
        meltfptr[2] = 0 ;
    }

    ;
    goto meltlabend_rout;
meltlabend_rout:
    melt_trace_end("COMPLETE_SEQUENCE_AS_TUPLE", meltcallcount);
    melt_blocklevel_signals = current_blocklevel_signals_meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE_melt;
    return (melt_ptr_t)(/*_._RETVAL___V1*/ meltfptr[0]);
#undef melt_variadic_length
#undef melt_variadic_index

#undef meltcallcount
#undef meltfram__
#undef MELTFRAM_NBVARNUM
#undef MELTFRAM_NBVARPTR
} /*end meltrout_32_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_TUPLE*/





melt_ptr_t MELT_MODULE_VISIBILITY
meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_,
        const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_)
{
    /*variadic*/ int variad_COMPLETE_SEQUENCE_AS_LIST_ix = 0, variad_COMPLETE_SEQUENCE_AS_LIST_len = melt_argdescr_length (meltxargdescr_);
#define melt_variadic_length  (0+variad_COMPLETE_SEQUENCE_AS_LIST_len)
#define melt_variadic_index variad_COMPLETE_SEQUENCE_AS_LIST_ix

    long current_blocklevel_signals_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST_melt =  melt_blocklevel_signals;

#if MELT_HAVE_DEBUG
    static long melt_call_counter__;
    long melt_thiscallcounter__ ATTRIBUTE_UNUSED = ++ melt_call_counter__;
#undef meltcallcount
#define meltcallcount melt_thiscallcounter__
#else
#undef meltcallcount
#define meltcallcount 0L
#endif
//// emitted by output_curframe_declstruct_init using OUTPUT_CURFRAME_DECLSTRUCT @warmelt-outobj.melt:1501

    /* start of frame for routine meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST fromline 1716 */

    /** start of frame for meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST of CLASS_PROCROUTINEOBJ from 1518**/

    /*curframdeclclassy*/ class MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST// fromline 1531
        : public Melt_CallFrameWithValues<16>
    {
    public: /* fromline 1535*/
        long mcfr_varnum[3];
        /*classy others*/
        long _meltsparel;
        void melt_mark_stuff (void)
        {
        } /*end melt_mark_stuff*/
        virtual void melt_mark_ggc_data (void)
        {
            melt_mark_values ();
            melt_mark_stuff ();
        }; /*end melt_mark_ggc_data*/
        MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST(meltclosure_ptr_t clos) //the closure constructor fromline 1594
            : Melt_CallFrameWithValues<16> (
#if ENABLE_CHECKING /*fromline 1598*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1602*/
                sizeof(MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST), clos) {};
        MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST() //the constructor fromline 1606
            : Melt_CallFrameWithValues<16> (
#if ENABLE_CHECKING /*fromline 1610*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1614*/
                sizeof(MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST)) {};
#if ENABLE_CHECKING /*fromline 1618*/
        MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST(const char*fil, int lin) //the constructor fromline 1620
            : Melt_CallFrameWithValues<16> (fil,lin, sizeof(MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST)) {};
        MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST(const char*fil, int lin, meltclosure_ptr_t clos) //the closure constructor fromline 1625
            : Melt_CallFrameWithValues<16> (fil,lin, sizeof(MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST), clos) {};
#endif /* ENABLE_CHECKING fromline 1630*/

    }; // end  class MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST


    /** end of frame for meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST fromline 1661**/

    /* end of frame for routine meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST fromline 1720 */

    /* classy proc frame meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST */
    MeltFrame_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST
    meltfram__ /*declfrastruct fromline 1744*/
    /*classyprocarg meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST fromline 1749*/
#if ENABLE_CHECKING
    (__FILE__, __LINE__, meltclosp_) /* fromline 1753*/
#else /*ENABLE_CHECKING */
    (meltclosp_) /* fromline 1757*/
#endif /*ENABLE_CHECKING */

    ;
#define meltframe meltfram__

    melt_trace_start("COMPLETE_SEQUENCE_AS_LIST", meltcallcount);
    /*use arguments*/
    (void) meltclosp_;
    (void)meltfirstargp_;
    (void)meltxargdescr_;
    (void)meltxargtab_;
    (void)meltxresdescr_;
    (void)meltxrestab_;
    /*getargs*/

    /*getarg#0*/
    MELT_LOCATION("warmelt-base.melt:3129:/ getarg");
    /*_.SRC__V2*/
    meltfptr[1] = (melt_ptr_t) meltfirstargp_;
    ;
    goto meltlab_endgetargs;
meltlab_endgetargs:
    ;
    /*body*/
    /*^block*/
    /*anyblock*/
    {

        MELT_LOCATION("warmelt-base.melt:3133:/ quasiblock");


        /*_.NEWLIST__V4*/
        meltfptr[3] =
            (meltgc_new_list((meltobject_ptr_t)((/*!DISCR_LIST*/ meltfrout->tabval[0]))));;

        MELT_CHECK_SIGNAL();
        ;
        /*^compute*/
        /*_#IS_MULTIPLE__L1*/
        meltfnum[0] =
            (melt_magic_discr((melt_ptr_t)(/*_.SRC__V2*/ meltfptr[1])) == MELTOBMAG_MULTIPLE);;
        MELT_LOCATION("warmelt-base.melt:3135:/ cond");
        /*cond*/
        if (/*_#IS_MULTIPLE__L1*/ meltfnum[0]) /*then*/
            {
                /*^cond.then*/
                /*^block*/
                /*anyblock*/
                {

                    /*citerblock FOREACH_IN_MULTIPLE*/
                    {
                        /* start foreach_in_multiple meltcit1__EACHTUP */
                        long  meltcit1__EACHTUP_ln = melt_multiple_length((melt_ptr_t)/*_.SRC__V2*/ meltfptr[1]);
                        for (/*_#IX__L2*/ meltfnum[1] = 0;
                                          (/*_#IX__L2*/ meltfnum[1] >= 0) && (/*_#IX__L2*/ meltfnum[1] <  meltcit1__EACHTUP_ln);
                                          /*_#IX__L2*/ meltfnum[1]++)
                            {
                                /*_.COMP__V5*/ meltfptr[4] = melt_multiple_nth((melt_ptr_t)(/*_.SRC__V2*/ meltfptr[1]),  /*_#IX__L2*/ meltfnum[1]);




                                {
                                    MELT_LOCATION("warmelt-base.melt:3139:/ locexp");
                                    meltgc_append_list((melt_ptr_t)(/*_.NEWLIST__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.COMP__V5*/ meltfptr[4]));
                                }
                                ;
                                if (/*_#IX__L2*/ meltfnum[1]<0) break;
                            } /* end  foreach_in_multiple meltcit1__EACHTUP */

                        /*citerepilog*/

                        MELT_LOCATION("warmelt-base.melt:3136:/ clear");
                        /*clear*/ /*_.COMP__V5*/
                        meltfptr[4] = 0 ;
                        /*^clear*/
                        /*clear*/ /*_#IX__L2*/
                        meltfnum[1] = 0 ;
                    } /*endciterblock FOREACH_IN_MULTIPLE*/
                    ;
                    /*epilog*/
                }
                ;
            }
        else
            {
                MELT_LOCATION("warmelt-base.melt:3135:/ cond.else");

                /*^block*/
                /*anyblock*/
                {


                    MELT_CHECK_SIGNAL();
                    ;
                    /*_#IS_LIST__L3*/
                    meltfnum[2] =
                        (melt_magic_discr((melt_ptr_t)(/*_.SRC__V2*/ meltfptr[1])) == MELTOBMAG_LIST);;
                    MELT_LOCATION("warmelt-base.melt:3141:/ cond");
                    /*cond*/
                    if (/*_#IS_LIST__L3*/ meltfnum[2]) /*then*/
                        {
                            /*^cond.then*/
                            /*^block*/
                            /*anyblock*/
                            {

                                /*citerblock FOREACH_IN_LIST*/
                                {
                                    /* start foreach_in_list meltcit2__EACHLIST */
                                    for (/*_.CURPAIR__V6*/ meltfptr[5] = melt_list_first( (melt_ptr_t)/*_.SRC__V2*/ meltfptr[1]);
                                                           melt_magic_discr((melt_ptr_t) /*_.CURPAIR__V6*/ meltfptr[5]) == MELTOBMAG_PAIR;
                                                           /*_.CURPAIR__V6*/ meltfptr[5] = melt_pair_tail((melt_ptr_t) /*_.CURPAIR__V6*/ meltfptr[5]))
                                        {
                                            /*_.CURCOMP__V7*/ meltfptr[6] = melt_pair_head((melt_ptr_t) /*_.CURPAIR__V6*/ meltfptr[5]);



                                            {
                                                MELT_LOCATION("warmelt-base.melt:3145:/ locexp");
                                                meltgc_append_list((melt_ptr_t)(/*_.NEWLIST__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.CURCOMP__V7*/ meltfptr[6]));
                                            }
                                            ;
                                        } /* end foreach_in_list meltcit2__EACHLIST */
                                    /*_.CURPAIR__V6*/ meltfptr[5] = NULL;
                                    /*_.CURCOMP__V7*/
                                    meltfptr[6] = NULL;


                                    /*citerepilog*/

                                    MELT_LOCATION("warmelt-base.melt:3142:/ clear");
                                    /*clear*/ /*_.CURPAIR__V6*/
                                    meltfptr[5] = 0 ;
                                    /*^clear*/
                                    /*clear*/ /*_.CURCOMP__V7*/
                                    meltfptr[6] = 0 ;
                                } /*endciterblock FOREACH_IN_LIST*/
                                ;
                                /*epilog*/
                            }
                            ;
                        }
                    else
                        {
                            MELT_LOCATION("warmelt-base.melt:3141:/ cond.else");

                            /*^block*/
                            /*anyblock*/
                            {


                                {
                                    MELT_LOCATION("warmelt-base.melt:3148:/ locexp");
                                    meltgc_append_list((melt_ptr_t)(/*_.NEWLIST__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.SRC__V2*/ meltfptr[1]));
                                }
                                ;
                                MELT_LOCATION("warmelt-base.melt:3147:/ quasiblock");


                                /*epilog*/
                            }
                            ;
                        }
                    ;
                    /*epilog*/

                    MELT_LOCATION("warmelt-base.melt:3135:/ clear");
                    /*clear*/ /*_#IS_LIST__L3*/
                    meltfnum[2] = 0 ;
                }
                ;
            }
        ;
        MELT_LOCATION("warmelt-base.melt:3149:/ loop");
        /*loop*/
        {
meltlabloop_ARGLOOP_4:;/*^loopbody*/

            /*^block*/
            /*anyblock*/
            {


                MELT_CHECK_SIGNAL();
                ;
                MELT_LOCATION("warmelt-base.melt:3151:/ cond");
                /*cond*/
                if (/*ifvariadic nomore*/ variad_COMPLETE_SEQUENCE_AS_LIST_ix == variad_COMPLETE_SEQUENCE_AS_LIST_len) /*then*/
                    {
                        /*^cond.then*/
                        /*^block*/
                        /*anyblock*/
                        {

                            /*^compute*/

                            /*consume variadic  !*/ variad_COMPLETE_SEQUENCE_AS_LIST_ix += 0;;
                            MELT_LOCATION("warmelt-base.melt:3153:/ quasiblock");


                            /*^compute*/
                            /*_.ARGLOOP__V9*/
                            meltfptr[8] =  /*fromline 1341*/ NULL ;;

                            /*^exit*/
                            /*exit*/
                            {
                                goto meltlabexit_ARGLOOP_4;
                            }
                            ;
                            /*epilog*/
                        }
                        ;
                    }
                else
                    {
                        MELT_LOCATION("warmelt-base.melt:3151:/ cond.else");

                        /*^block*/
                        /*anyblock*/
                        {

                            /*^cond*/
                            /*cond*/ if (/*ifvariadic arg#1*/ variad_COMPLETE_SEQUENCE_AS_LIST_ix>=0 && variad_COMPLETE_SEQUENCE_AS_LIST_ix + 1 <= variad_COMPLETE_SEQUENCE_AS_LIST_len && meltxargdescr_[variad_COMPLETE_SEQUENCE_AS_LIST_ix]== MELTBPAR_PTR) /*then*/
                                {
                                    /*^cond.then*/
                                    /*^block*/
                                    /*anyblock*/
                                    {

                                        /*^compute*/
                                        /*_.V__V10*/ meltfptr[9] =
                                            /*variadic argument value*/ ((meltxargtab_[variad_COMPLETE_SEQUENCE_AS_LIST_ix + 0].meltbp_aptr) ? (*(meltxargtab_[variad_COMPLETE_SEQUENCE_AS_LIST_ix + 0].meltbp_aptr)) : NULL);;
                                        /*^compute*/

                                        /*consume variadic Value !*/
                                        variad_COMPLETE_SEQUENCE_AS_LIST_ix += 1;;

                                        {
                                            MELT_LOCATION("warmelt-base.melt:3155:/ locexp");
                                            meltgc_append_list((melt_ptr_t)(/*_.NEWLIST__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.V__V10*/ meltfptr[9]));
                                        }
                                        ;
                                        /*epilog*/

                                        MELT_LOCATION("warmelt-base.melt:3151:/ clear");
                                        /*clear*/ /*_.V__V10*/
                                        meltfptr[9] = 0 ;
                                    }
                                    ;
                                }
                            else    /*^cond.else*/
                                {

                                    /*^block*/
                                    /*anyblock*/
                                    {

                                        MELT_LOCATION("warmelt-base.melt:3157:/ quasiblock");


                                        /*_.VCTY__V12*/
                                        meltfptr[11] =
                                            /*variadic_type_code*/
#ifdef melt_variadic_index
                                            (((melt_variadic_index + 0) >= 0
                                              && (melt_variadic_index + 0) < melt_variadic_length)
                                             ?  melt_code_to_ctype (meltxargdescr_[melt_variadic_index + 0]
                                                                    & MELT_ARGDESCR_MAX)
                                             : NULL)
#else
                                            NULL /* no variadic_ctype outside of variadic functions */
#endif /*melt_variadic_index*/
                                            ;;
                                        MELT_LOCATION("warmelt-base.melt:3160:/ cond");
                                        /*cond*/
                                        if (
                                            /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.VCTY__V12*/ meltfptr[11]),
                                                                          (melt_ptr_t)((/*!CLASS_NAMED*/ meltfrout->tabval[1])))
                                        ) /*then*/
                                            {
                                                /*^cond.then*/
                                                /*^getslot*/
                                                {
                                                    melt_ptr_t slot=NULL, obj=NULL;
                                                    obj = (melt_ptr_t)(/*_.VCTY__V12*/ meltfptr[11]) /*=obj*/;
                                                    melt_object_get_field(slot,obj, 1, "NAMED_NAME");
                                                    /*_.NAMED_NAME__V13*/
                                                    meltfptr[12] = slot;
                                                };
                                                ;
                                            }
                                        else    /*^cond.else*/
                                            {

                                                /*_.NAMED_NAME__V13*/ meltfptr[12] =  /*fromline 1341*/ NULL ;;
                                            }
                                        ;

                                        {
                                            MELT_LOCATION("warmelt-base.melt:3159:/ locexp");
                                            error ("MELT ERROR MSG [#%ld]::: %s - %s", melt_dbgcounter, ( "COMPLETE_SEQUENCE_AS_LIST with unsupported ctype"),
                                                   melt_string_str((melt_ptr_t)(/*_.NAMED_NAME__V13*/ meltfptr[12])));
                                        }
                                        ;

#if MELT_HAVE_DEBUG
                                        MELT_LOCATION("warmelt-base.melt:3161:/ cppif.then");
                                        /*^block*/
                                        /*anyblock*/
                                        {


                                            MELT_CHECK_SIGNAL();
                                            ;
                                            /*^cond*/
                                            /*cond*/
                                            if ((/*nil*/NULL)) /*then*/
                                                {
                                                    /*^cond.then*/
                                                    /*_._IFELSE___V15*/ meltfptr[14] = (/*nil*/NULL);;
                                                }
                                            else
                                                {
                                                    MELT_LOCATION("warmelt-base.melt:3161:/ cond.else");

                                                    /*^block*/
                                                    /*anyblock*/
                                                    {




                                                        {
                                                            /*^locexp*/
                                                            melt_assert_failed(( "invalid variadic argument to COMPLETE_SEQUENCE_AS_LIST"),( "warmelt-base.melt")?( "warmelt-base.melt"):__FILE__,
                                                                               (3161)?(3161):__LINE__, __FUNCTION__);
                                                            ;
                                                        }
                                                        ;
                                                        /*clear*/ /*_._IFELSE___V15*/
                                                        meltfptr[14] = 0 ;
                                                        /*epilog*/
                                                    }
                                                    ;
                                                }
                                            ;
                                            /*^compute*/
                                            /*_.IFCPP___V14*/
                                            meltfptr[13] = /*_._IFELSE___V15*/ meltfptr[14];;
                                            /*epilog*/

                                            MELT_LOCATION("warmelt-base.melt:3161:/ clear");
                                            /*clear*/ /*_._IFELSE___V15*/
                                            meltfptr[14] = 0 ;
                                        }

#else /*MELT_HAVE_DEBUG*/
                                        /*^cppif.else*/
                                        /*_.IFCPP___V14*/ meltfptr[13] = (/*nil*/NULL);
#endif /*MELT_HAVE_DEBUG*/
                                        ;
                                        /*^compute*/
                                        /*_.LET___V11*/
                                        meltfptr[9] = /*_.IFCPP___V14*/ meltfptr[13];;

                                        MELT_LOCATION("warmelt-base.melt:3157:/ clear");
                                        /*clear*/ /*_.VCTY__V12*/
                                        meltfptr[11] = 0 ;
                                        /*^clear*/
                                        /*clear*/ /*_.NAMED_NAME__V13*/
                                        meltfptr[12] = 0 ;
                                        /*^clear*/
                                        /*clear*/ /*_.IFCPP___V14*/
                                        meltfptr[13] = 0 ;
                                        /*epilog*/

                                        MELT_LOCATION("warmelt-base.melt:3151:/ clear");
                                        /*clear*/ /*_.LET___V11*/
                                        meltfptr[9] = 0 ;
                                    }
                                    ;
                                }
                            ;
                            /*epilog*/
                        }
                        ;
                    }
                ;
                /*epilog*/
            }
            ;
            ;
            goto meltlabloop_ARGLOOP_4;
meltlabexit_ARGLOOP_4:
            ;
            MELT_LOCATION("warmelt-base.melt:3149:/ loopepilog");
            /*loopepilog*/
            /*_.FOREVER___V8*/
            meltfptr[7] = /*_.ARGLOOP__V9*/ meltfptr[8];;
        }
        ;

        MELT_CHECK_SIGNAL();
        ;
        MELT_LOCATION("warmelt-base.melt:3163:/ quasiblock");


        /*_._RETVAL___V1*/
        meltfptr[0] = /*_.NEWLIST__V4*/ meltfptr[3];;

        {
            MELT_LOCATION("warmelt-base.melt:3163:/ locexp");
            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                melt_warn_for_no_expected_secondary_results();
            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
            ;
        }
        ;
        /*^finalreturn*/
        ;
        /*finalret*/
        goto meltlabend_rout ;
        /*_.LET___V3*/
        meltfptr[2] = /*_.RETURN___V16*/ meltfptr[14];;

        MELT_LOCATION("warmelt-base.melt:3133:/ clear");
        /*clear*/ /*_.NEWLIST__V4*/
        meltfptr[3] = 0 ;
        /*^clear*/
        /*clear*/ /*_#IS_MULTIPLE__L1*/
        meltfnum[0] = 0 ;
        /*^clear*/
        /*clear*/ /*_.FOREVER___V8*/
        meltfptr[7] = 0 ;
        /*^clear*/
        /*clear*/ /*_.RETURN___V16*/
        meltfptr[14] = 0 ;
        MELT_LOCATION("warmelt-base.melt:3129:/ quasiblock");


        /*_._RETVAL___V1*/
        meltfptr[0] = /*_.LET___V3*/ meltfptr[2];;

        {
            MELT_LOCATION("warmelt-base.melt:3129:/ locexp");
            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                melt_warn_for_no_expected_secondary_results();
            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
            ;
        }
        ;
        /*^finalreturn*/
        ;
        /*finalret*/
        goto meltlabend_rout ;
        /*epilog*/

        /*^clear*/
        /*clear*/ /*_.LET___V3*/
        meltfptr[2] = 0 ;
    }

    ;
    goto meltlabend_rout;
meltlabend_rout:
    melt_trace_end("COMPLETE_SEQUENCE_AS_LIST", meltcallcount);
    melt_blocklevel_signals = current_blocklevel_signals_meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST_melt;
    return (melt_ptr_t)(/*_._RETVAL___V1*/ meltfptr[0]);
#undef melt_variadic_length
#undef melt_variadic_index

#undef meltcallcount
#undef meltfram__
#undef MELTFRAM_NBVARNUM
#undef MELTFRAM_NBVARPTR
} /*end meltrout_33_WARMELTmiBASE_COMPLETE_SEQUENCE_AS_LIST*/





melt_ptr_t MELT_MODULE_VISIBILITY
meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_,
        const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_)
{
    long current_blocklevel_signals_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET_melt =  melt_blocklevel_signals;

#if MELT_HAVE_DEBUG
    static long melt_call_counter__;
    long melt_thiscallcounter__ ATTRIBUTE_UNUSED = ++ melt_call_counter__;
#undef meltcallcount
#define meltcallcount melt_thiscallcounter__
#else
#undef meltcallcount
#define meltcallcount 0L
#endif
//// emitted by output_curframe_declstruct_init using OUTPUT_CURFRAME_DECLSTRUCT @warmelt-outobj.melt:1501

    /* start of frame for routine meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET fromline 1716 */

    /** start of frame for meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET of CLASS_PROCROUTINEOBJ from 1518**/

    /*curframdeclclassy*/ class MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET// fromline 1531
        : public Melt_CallFrameWithValues<12>
    {
    public: /* fromline 1535*/
        long mcfr_varnum[4];
        /*classy others*/
        long _meltsparel;
        void melt_mark_stuff (void)
        {
        } /*end melt_mark_stuff*/
        virtual void melt_mark_ggc_data (void)
        {
            melt_mark_values ();
            melt_mark_stuff ();
        }; /*end melt_mark_ggc_data*/
        MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET(meltclosure_ptr_t clos) //the closure constructor fromline 1594
            : Melt_CallFrameWithValues<12> (
#if ENABLE_CHECKING /*fromline 1598*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1602*/
                sizeof(MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET), clos) {};
        MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET() //the constructor fromline 1606
            : Melt_CallFrameWithValues<12> (
#if ENABLE_CHECKING /*fromline 1610*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1614*/
                sizeof(MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET)) {};
#if ENABLE_CHECKING /*fromline 1618*/
        MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET(const char*fil, int lin) //the constructor fromline 1620
            : Melt_CallFrameWithValues<12> (fil,lin, sizeof(MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET)) {};
        MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET(const char*fil, int lin, meltclosure_ptr_t clos) //the closure constructor fromline 1625
            : Melt_CallFrameWithValues<12> (fil,lin, sizeof(MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET), clos) {};
#endif /* ENABLE_CHECKING fromline 1630*/

    }; // end  class MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET


    /** end of frame for meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET fromline 1661**/

    /* end of frame for routine meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET fromline 1720 */

    /* classy proc frame meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET */
    MeltFrame_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET
    meltfram__ /*declfrastruct fromline 1744*/
    /*classyprocarg meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET fromline 1749*/
#if ENABLE_CHECKING
    (__FILE__, __LINE__, meltclosp_) /* fromline 1753*/
#else /*ENABLE_CHECKING */
    (meltclosp_) /* fromline 1757*/
#endif /*ENABLE_CHECKING */

    ;
#define meltframe meltfram__

    melt_trace_start("MELT_PREDEFINED_GET", meltcallcount);
    /*use arguments*/
    (void) meltclosp_;
    (void)meltfirstargp_;
    (void)meltxargdescr_;
    (void)meltxargtab_;
    (void)meltxresdescr_;
    (void)meltxrestab_;
    /*getargs*/

    /*getarg#0*/
    MELT_LOCATION("warmelt-base.melt:3221:/ getarg");
    /*_.NAMV__V2*/
    meltfptr[1] = (melt_ptr_t) meltfirstargp_;
    ;
    goto meltlab_endgetargs;
meltlab_endgetargs:
    ;
    /*body*/
    /*^block*/
    /*anyblock*/
    {


        MELT_CHECK_SIGNAL();
        ;
        /*_#IS_STRING__L1*/
        meltfnum[0] =
            (melt_magic_discr((melt_ptr_t)(/*_.NAMV__V2*/ meltfptr[1])) == MELTOBMAG_STRING);;
        MELT_LOCATION("warmelt-base.melt:3223:/ cond");
        /*cond*/
        if (/*_#IS_STRING__L1*/ meltfnum[0]) /*then*/
            {
                /*^cond.then*/
                /*^block*/
                /*anyblock*/
                {

                    /*_.PREDEFSTR_CHK__V4*/ meltfptr[3] =
                        /*melt_predefined_get PREDEFSTR_CHK__1 */
                        melt_fetch_predefined (melt_predefined_index_by_name (melt_string_str ((melt_ptr_t)(/*_.NAMV__V2*/ meltfptr[1])))) ;;
                    /*^compute*/
                    /*_._IFELSE___V3*/
                    meltfptr[2] = /*_.PREDEFSTR_CHK__V4*/ meltfptr[3];;
                    /*epilog*/

                    MELT_LOCATION("warmelt-base.melt:3223:/ clear");
                    /*clear*/ /*_.PREDEFSTR_CHK__V4*/
                    meltfptr[3] = 0 ;
                }
                ;
            }
        else    /*^cond.else*/
            {

                /*^block*/
                /*anyblock*/
                {


                    MELT_CHECK_SIGNAL();
                    ;
                    /*_#IS_A__L2*/
                    meltfnum[1] =
                        melt_is_instance_of((melt_ptr_t)(/*_.NAMV__V2*/ meltfptr[1]), (melt_ptr_t)((/*!CLASS_NAMED*/ meltfrout->tabval[0])));;
                    MELT_LOCATION("warmelt-base.melt:3226:/ cond");
                    /*cond*/
                    if (/*_#IS_A__L2*/ meltfnum[1]) /*then*/
                        {
                            /*^cond.then*/
                            /*^block*/
                            /*anyblock*/
                            {

                                MELT_LOCATION("warmelt-base.melt:3227:/ quasiblock");


                                /*^cond*/
                                /*cond*/
                                if (
                                    /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.NAMV__V2*/ meltfptr[1]),
                                                                  (melt_ptr_t)((/*!CLASS_NAMED*/ meltfrout->tabval[0])))
                                ) /*then*/
                                    {
                                        /*^cond.then*/
                                        /*^getslot*/
                                        {
                                            melt_ptr_t slot=NULL, obj=NULL;
                                            obj = (melt_ptr_t)(/*_.NAMV__V2*/ meltfptr[1]) /*=obj*/;
                                            melt_object_get_field(slot,obj, 1, "NAMED_NAME");
                                            /*_.NAMSTR__V7*/
                                            meltfptr[6] = slot;
                                        };
                                        ;
                                    }
                                else    /*^cond.else*/
                                    {

                                        /*_.NAMSTR__V7*/ meltfptr[6] =  /*fromline 1341*/ NULL ;;
                                    }
                                ;
                                /*^compute*/
                                /*_.PREDEFNAM_CHK__V8*/
                                meltfptr[7] =
                                    /*melt_predefined_get PREDEFNAM_CHK__1 */
                                    melt_fetch_predefined (melt_predefined_index_by_name (melt_string_str ((melt_ptr_t)(/*_.NAMSTR__V7*/ meltfptr[6])))) ;;
                                /*^compute*/
                                /*_.LET___V6*/
                                meltfptr[5] = /*_.PREDEFNAM_CHK__V8*/ meltfptr[7];;

                                MELT_LOCATION("warmelt-base.melt:3227:/ clear");
                                /*clear*/ /*_.NAMSTR__V7*/
                                meltfptr[6] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_.PREDEFNAM_CHK__V8*/
                                meltfptr[7] = 0 ;
                                /*_._IFELSE___V5*/
                                meltfptr[3] = /*_.LET___V6*/ meltfptr[5];;
                                /*epilog*/

                                MELT_LOCATION("warmelt-base.melt:3226:/ clear");
                                /*clear*/ /*_.LET___V6*/
                                meltfptr[5] = 0 ;
                            }
                            ;
                        }
                    else    /*^cond.else*/
                        {

                            /*^block*/
                            /*anyblock*/
                            {


                                MELT_CHECK_SIGNAL();
                                ;
                                /*_#IS_INTEGERBOX__L3*/
                                meltfnum[2] =
                                    (melt_magic_discr((melt_ptr_t)(/*_.NAMV__V2*/ meltfptr[1])) == MELTOBMAG_INT);;
                                MELT_LOCATION("warmelt-base.melt:3232:/ cond");
                                /*cond*/
                                if (/*_#IS_INTEGERBOX__L3*/ meltfnum[2]) /*then*/
                                    {
                                        /*^cond.then*/
                                        /*^block*/
                                        /*anyblock*/
                                        {

                                            MELT_LOCATION("warmelt-base.melt:3233:/ quasiblock");


                                            /*_#NUM__L4*/
                                            meltfnum[3] =
                                                (melt_get_int((melt_ptr_t)(/*_.NAMV__V2*/ meltfptr[1])));;
                                            /*^compute*/
                                            /*_.PREDEFNAM_CHK__V11*/
                                            meltfptr[5] =
                                                /*melt_predefined_get PREDEFNAM_CHK__2 */
                                                melt_fetch_predefined (/*_#NUM__L4*/ meltfnum[3]) ;;
                                            /*^compute*/
                                            /*_.LET___V10*/
                                            meltfptr[7] = /*_.PREDEFNAM_CHK__V11*/ meltfptr[5];;

                                            MELT_LOCATION("warmelt-base.melt:3233:/ clear");
                                            /*clear*/ /*_#NUM__L4*/
                                            meltfnum[3] = 0 ;
                                            /*^clear*/
                                            /*clear*/ /*_.PREDEFNAM_CHK__V11*/
                                            meltfptr[5] = 0 ;
                                            /*_._IFELSE___V9*/
                                            meltfptr[6] = /*_.LET___V10*/ meltfptr[7];;
                                            /*epilog*/

                                            MELT_LOCATION("warmelt-base.melt:3232:/ clear");
                                            /*clear*/ /*_.LET___V10*/
                                            meltfptr[7] = 0 ;
                                        }
                                        ;
                                    }
                                else    /*^cond.else*/
                                    {

                                        /*^block*/
                                        /*anyblock*/
                                        {

                                            MELT_LOCATION("warmelt-base.melt:3237:/ quasiblock");


                                            /*_.PROGN___V12*/
                                            meltfptr[5] = (/*nil*/NULL);;
                                            /*^compute*/
                                            /*_._IFELSE___V9*/
                                            meltfptr[6] = /*_.PROGN___V12*/ meltfptr[5];;
                                            /*epilog*/

                                            MELT_LOCATION("warmelt-base.melt:3232:/ clear");
                                            /*clear*/ /*_.PROGN___V12*/
                                            meltfptr[5] = 0 ;
                                        }
                                        ;
                                    }
                                ;
                                /*_._IFELSE___V5*/
                                meltfptr[3] = /*_._IFELSE___V9*/ meltfptr[6];;
                                /*epilog*/

                                MELT_LOCATION("warmelt-base.melt:3226:/ clear");
                                /*clear*/ /*_#IS_INTEGERBOX__L3*/
                                meltfnum[2] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_._IFELSE___V9*/
                                meltfptr[6] = 0 ;
                            }
                            ;
                        }
                    ;
                    /*_._IFELSE___V3*/
                    meltfptr[2] = /*_._IFELSE___V5*/ meltfptr[3];;
                    /*epilog*/

                    MELT_LOCATION("warmelt-base.melt:3223:/ clear");
                    /*clear*/ /*_#IS_A__L2*/
                    meltfnum[1] = 0 ;
                    /*^clear*/
                    /*clear*/ /*_._IFELSE___V5*/
                    meltfptr[3] = 0 ;
                }
                ;
            }
        ;
        MELT_LOCATION("warmelt-base.melt:3221:/ quasiblock");


        /*_._RETVAL___V1*/
        meltfptr[0] = /*_._IFELSE___V3*/ meltfptr[2];;

        {
            MELT_LOCATION("warmelt-base.melt:3221:/ locexp");
            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                melt_warn_for_no_expected_secondary_results();
            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
            ;
        }
        ;
        /*^finalreturn*/
        ;
        /*finalret*/
        goto meltlabend_rout ;
        /*epilog*/

        /*^clear*/
        /*clear*/ /*_#IS_STRING__L1*/
        meltfnum[0] = 0 ;
        /*^clear*/
        /*clear*/ /*_._IFELSE___V3*/
        meltfptr[2] = 0 ;
    }

    ;
    goto meltlabend_rout;
meltlabend_rout:
    melt_trace_end("MELT_PREDEFINED_GET", meltcallcount);
    melt_blocklevel_signals = current_blocklevel_signals_meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET_melt;
    return (melt_ptr_t)(/*_._RETVAL___V1*/ meltfptr[0]);
#undef meltcallcount
#undef meltfram__
#undef MELTFRAM_NBVARNUM
#undef MELTFRAM_NBVARPTR
} /*end meltrout_34_WARMELTmiBASE_MELT_PREDEFINED_GET*/





melt_ptr_t MELT_MODULE_VISIBILITY
meltrout_35_WARMELTmiBASE_SYMBOL_CNAME(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_,
                                       const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_)
{
    long current_blocklevel_signals_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME_melt =  melt_blocklevel_signals;

#if MELT_HAVE_DEBUG
    static long melt_call_counter__;
    long melt_thiscallcounter__ ATTRIBUTE_UNUSED = ++ melt_call_counter__;
#undef meltcallcount
#define meltcallcount melt_thiscallcounter__
#else
#undef meltcallcount
#define meltcallcount 0L
#endif
//// emitted by output_curframe_declstruct_init using OUTPUT_CURFRAME_DECLSTRUCT @warmelt-outobj.melt:1501

    /* start of frame for routine meltrout_35_WARMELTmiBASE_SYMBOL_CNAME fromline 1716 */

    /** start of frame for meltrout_35_WARMELTmiBASE_SYMBOL_CNAME of CLASS_PROCROUTINEOBJ from 1518**/

    /*curframdeclclassy*/ class MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME// fromline 1531
        : public Melt_CallFrameWithValues<20>
    {
    public: /* fromline 1535*/
        long mcfr_varnum[3];
        /*classy others*/
        long _meltsparel;
        void melt_mark_stuff (void)
        {
        } /*end melt_mark_stuff*/
        virtual void melt_mark_ggc_data (void)
        {
            melt_mark_values ();
            melt_mark_stuff ();
        }; /*end melt_mark_ggc_data*/
        MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME(meltclosure_ptr_t clos) //the closure constructor fromline 1594
            : Melt_CallFrameWithValues<20> (
#if ENABLE_CHECKING /*fromline 1598*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1602*/
                sizeof(MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME), clos) {};
        MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME() //the constructor fromline 1606
            : Melt_CallFrameWithValues<20> (
#if ENABLE_CHECKING /*fromline 1610*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1614*/
                sizeof(MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME)) {};
#if ENABLE_CHECKING /*fromline 1618*/
        MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME(const char*fil, int lin) //the constructor fromline 1620
            : Melt_CallFrameWithValues<20> (fil,lin, sizeof(MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME)) {};
        MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME(const char*fil, int lin, meltclosure_ptr_t clos) //the closure constructor fromline 1625
            : Melt_CallFrameWithValues<20> (fil,lin, sizeof(MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME), clos) {};
#endif /* ENABLE_CHECKING fromline 1630*/

    }; // end  class MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME


    /** end of frame for meltrout_35_WARMELTmiBASE_SYMBOL_CNAME fromline 1661**/

    /* end of frame for routine meltrout_35_WARMELTmiBASE_SYMBOL_CNAME fromline 1720 */

    /* classy proc frame meltrout_35_WARMELTmiBASE_SYMBOL_CNAME */
    MeltFrame_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME
    meltfram__ /*declfrastruct fromline 1744*/
    /*classyprocarg meltrout_35_WARMELTmiBASE_SYMBOL_CNAME fromline 1749*/
#if ENABLE_CHECKING
    (__FILE__, __LINE__, meltclosp_) /* fromline 1753*/
#else /*ENABLE_CHECKING */
    (meltclosp_) /* fromline 1757*/
#endif /*ENABLE_CHECKING */

    ;
#define meltframe meltfram__

    melt_trace_start("SYMBOL_CNAME", meltcallcount);
    /*use arguments*/
    (void) meltclosp_;
    (void)meltfirstargp_;
    (void)meltxargdescr_;
    (void)meltxargtab_;
    (void)meltxresdescr_;
    (void)meltxrestab_;
    /*getargs*/

    /*getarg#0*/
    MELT_LOCATION("warmelt-base.melt:3241:/ getarg");
    /*_.SY__V2*/
    meltfptr[1] = (melt_ptr_t) meltfirstargp_;
    ;
    goto meltlab_endgetargs;
meltlab_endgetargs:
    ;
    /*body*/
    /*^block*/
    /*anyblock*/
    {


        MELT_CHECK_SIGNAL();
        ;
        /*_#IS_A__L1*/
        meltfnum[0] =
            melt_is_instance_of((melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]), (melt_ptr_t)((/*!CLASS_CLONED_SYMBOL*/ meltfrout->tabval[0])));;
        MELT_LOCATION("warmelt-base.melt:3243:/ cond");
        /*cond*/
        if (/*_#IS_A__L1*/ meltfnum[0]) /*then*/
            {
                /*^cond.then*/
                /*^block*/
                /*anyblock*/
                {

                    MELT_LOCATION("warmelt-base.melt:3244:/ quasiblock");


                    /*_.SBUF__V5*/
                    meltfptr[4] =
                        (melt_ptr_t) meltgc_new_strbuf((meltobject_ptr_t)((/*!DISCR_STRBUF*/ meltfrout->tabval[1])), (const char*)0);;
                    MELT_LOCATION("warmelt-base.melt:3245:/ cond");
                    /*cond*/
                    if (
                        /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]),
                                                      (melt_ptr_t)((/*!CLASS_NAMED*/ meltfrout->tabval[2])))
                    ) /*then*/
                        {
                            /*^cond.then*/
                            /*^getslot*/
                            {
                                melt_ptr_t slot=NULL, obj=NULL;
                                obj = (melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]) /*=obj*/;
                                melt_object_get_field(slot,obj, 1, "NAMED_NAME");
                                /*_.NAMED_NAME__V6*/
                                meltfptr[5] = slot;
                            };
                            ;
                        }
                    else    /*^cond.else*/
                        {

                            /*_.NAMED_NAME__V6*/ meltfptr[5] =  /*fromline 1341*/ NULL ;;
                        }
                    ;

                    {
                        MELT_LOCATION("warmelt-base.melt:3245:/ locexp");
                        meltgc_add_strbuf_cident((melt_ptr_t)(/*_.SBUF__V5*/ meltfptr[4]),
                                                 melt_string_str((melt_ptr_t)(/*_.NAMED_NAME__V6*/ meltfptr[5])));
                    }
                    ;
                    MELT_LOCATION("warmelt-base.melt:3246:/ cond");
                    /*cond*/
                    if (
                        /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]),
                                                      (melt_ptr_t)((/*!CLASS_CLONED_SYMBOL*/ meltfrout->tabval[0])))
                    ) /*then*/
                        {
                            /*^cond.then*/
                            /*^getslot*/
                            {
                                melt_ptr_t slot=NULL, obj=NULL;
                                obj = (melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]) /*=obj*/;
                                melt_object_get_field(slot,obj, 3, "CSYM_URANK");
                                /*_.CSYM_URANK__V7*/
                                meltfptr[6] = slot;
                            };
                            ;
                        }
                    else    /*^cond.else*/
                        {

                            /*_.CSYM_URANK__V7*/ meltfptr[6] =  /*fromline 1341*/ NULL ;;
                        }
                    ;

                    MELT_CHECK_SIGNAL();
                    ;
                    MELT_LOCATION("warmelt-base.melt:3246:/ apply");
                    /*apply*/
                    {
                        union meltparam_un argtab[2];
                        memset(&argtab, 0, sizeof(argtab));
                        /*^apply.arg*/
                        argtab[0].meltbp_cstring =  "cl";
                        /*^apply.arg*/
                        argtab[1].meltbp_aptr = (melt_ptr_t*) &/*_.CSYM_URANK__V7*/ meltfptr[6];
                        /*_.ADD2OUT__V8*/
                        meltfptr[7] =  melt_apply ((meltclosure_ptr_t)((/*!ADD2OUT*/ meltfrout->tabval[3])), (melt_ptr_t)(/*_.SBUF__V5*/ meltfptr[4]), (MELTBPARSTR_CSTRING MELTBPARSTR_PTR ""), argtab, "", (union meltparam_un*)0);
                    }
                    ;
                    /*_.STRBUF2STRING__V9*/
                    meltfptr[8] =
                        (meltgc_new_stringdup((meltobject_ptr_t)((/*!DISCR_VERBATIM_STRING*/ meltfrout->tabval[4])), melt_strbuf_str((melt_ptr_t)(/*_.SBUF__V5*/ meltfptr[4]))));;
                    /*^compute*/
                    /*_.LET___V4*/
                    meltfptr[3] = /*_.STRBUF2STRING__V9*/ meltfptr[8];;

                    MELT_LOCATION("warmelt-base.melt:3244:/ clear");
                    /*clear*/ /*_.SBUF__V5*/
                    meltfptr[4] = 0 ;
                    /*^clear*/
                    /*clear*/ /*_.NAMED_NAME__V6*/
                    meltfptr[5] = 0 ;
                    /*^clear*/
                    /*clear*/ /*_.CSYM_URANK__V7*/
                    meltfptr[6] = 0 ;
                    /*^clear*/
                    /*clear*/ /*_.ADD2OUT__V8*/
                    meltfptr[7] = 0 ;
                    /*^clear*/
                    /*clear*/ /*_.STRBUF2STRING__V9*/
                    meltfptr[8] = 0 ;
                    /*_._IFELSE___V3*/
                    meltfptr[2] = /*_.LET___V4*/ meltfptr[3];;
                    /*epilog*/

                    MELT_LOCATION("warmelt-base.melt:3243:/ clear");
                    /*clear*/ /*_.LET___V4*/
                    meltfptr[3] = 0 ;
                }
                ;
            }
        else    /*^cond.else*/
            {

                /*^block*/
                /*anyblock*/
                {


                    MELT_CHECK_SIGNAL();
                    ;
                    /*_#IS_A__L2*/
                    meltfnum[1] =
                        melt_is_instance_of((melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]), (melt_ptr_t)((/*!CLASS_KEYWORD*/ meltfrout->tabval[5])));;
                    MELT_LOCATION("warmelt-base.melt:3249:/ cond");
                    /*cond*/
                    if (/*_#IS_A__L2*/ meltfnum[1]) /*then*/
                        {
                            /*^cond.then*/
                            /*^block*/
                            /*anyblock*/
                            {

                                MELT_LOCATION("warmelt-base.melt:3250:/ quasiblock");


                                /*_.SBUF__V12*/
                                meltfptr[6] =
                                    (melt_ptr_t) meltgc_new_strbuf((meltobject_ptr_t)((/*!DISCR_STRBUF*/ meltfrout->tabval[1])), (const char*)0);;
                                MELT_LOCATION("warmelt-base.melt:3251:/ cond");
                                /*cond*/
                                if (
                                    /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]),
                                                                  (melt_ptr_t)((/*!CLASS_NAMED*/ meltfrout->tabval[2])))
                                ) /*then*/
                                    {
                                        /*^cond.then*/
                                        /*^getslot*/
                                        {
                                            melt_ptr_t slot=NULL, obj=NULL;
                                            obj = (melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]) /*=obj*/;
                                            melt_object_get_field(slot,obj, 1, "NAMED_NAME");
                                            /*_.NAMED_NAME__V13*/
                                            meltfptr[7] = slot;
                                        };
                                        ;
                                    }
                                else    /*^cond.else*/
                                    {

                                        /*_.NAMED_NAME__V13*/ meltfptr[7] =  /*fromline 1341*/ NULL ;;
                                    }
                                ;

                                {
                                    MELT_LOCATION("warmelt-base.melt:3251:/ locexp");
                                    meltgc_add_strbuf_cident((melt_ptr_t)(/*_.SBUF__V12*/ meltfptr[6]),
                                                             melt_string_str((melt_ptr_t)(/*_.NAMED_NAME__V13*/ meltfptr[7])));
                                }
                                ;

                                {
                                    MELT_LOCATION("warmelt-base.melt:3252:/ locexp");
                                    /*add2sbuf_strconst*/
                                    meltgc_add_strbuf((melt_ptr_t)(/*_.SBUF__V12*/ meltfptr[6]), ( "kw"));
                                }
                                ;
                                /*_.STRBUF2STRING__V14*/
                                meltfptr[8] =
                                    (meltgc_new_stringdup((meltobject_ptr_t)((/*!DISCR_VERBATIM_STRING*/ meltfrout->tabval[4])), melt_strbuf_str((melt_ptr_t)(/*_.SBUF__V12*/ meltfptr[6]))));;
                                /*^compute*/
                                /*_.LET___V11*/
                                meltfptr[5] = /*_.STRBUF2STRING__V14*/ meltfptr[8];;

                                MELT_LOCATION("warmelt-base.melt:3250:/ clear");
                                /*clear*/ /*_.SBUF__V12*/
                                meltfptr[6] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_.NAMED_NAME__V13*/
                                meltfptr[7] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_.STRBUF2STRING__V14*/
                                meltfptr[8] = 0 ;
                                /*_._IFELSE___V10*/
                                meltfptr[4] = /*_.LET___V11*/ meltfptr[5];;
                                /*epilog*/

                                MELT_LOCATION("warmelt-base.melt:3249:/ clear");
                                /*clear*/ /*_.LET___V11*/
                                meltfptr[5] = 0 ;
                            }
                            ;
                        }
                    else    /*^cond.else*/
                        {

                            /*^block*/
                            /*anyblock*/
                            {


                                MELT_CHECK_SIGNAL();
                                ;
                                /*_#IS_A__L3*/
                                meltfnum[2] =
                                    melt_is_instance_of((melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]), (melt_ptr_t)((/*!CLASS_SYMBOL*/ meltfrout->tabval[6])));;
                                MELT_LOCATION("warmelt-base.melt:3255:/ cond");
                                /*cond*/
                                if (/*_#IS_A__L3*/ meltfnum[2]) /*then*/
                                    {
                                        /*^cond.then*/
                                        /*^block*/
                                        /*anyblock*/
                                        {

                                            MELT_LOCATION("warmelt-base.melt:3256:/ quasiblock");


                                            /*_.SBUF__V17*/
                                            meltfptr[7] =
                                                (melt_ptr_t) meltgc_new_strbuf((meltobject_ptr_t)((/*!DISCR_STRBUF*/ meltfrout->tabval[1])), (const char*)0);;
                                            MELT_LOCATION("warmelt-base.melt:3257:/ cond");
                                            /*cond*/
                                            if (
                                                /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]),
                                                                              (melt_ptr_t)((/*!CLASS_NAMED*/ meltfrout->tabval[2])))
                                            ) /*then*/
                                                {
                                                    /*^cond.then*/
                                                    /*^getslot*/
                                                    {
                                                        melt_ptr_t slot=NULL, obj=NULL;
                                                        obj = (melt_ptr_t)(/*_.SY__V2*/ meltfptr[1]) /*=obj*/;
                                                        melt_object_get_field(slot,obj, 1, "NAMED_NAME");
                                                        /*_.NAMED_NAME__V18*/
                                                        meltfptr[8] = slot;
                                                    };
                                                    ;
                                                }
                                            else    /*^cond.else*/
                                                {

                                                    /*_.NAMED_NAME__V18*/ meltfptr[8] =  /*fromline 1341*/ NULL ;;
                                                }
                                            ;

                                            {
                                                MELT_LOCATION("warmelt-base.melt:3257:/ locexp");
                                                meltgc_add_strbuf_cident((melt_ptr_t)(/*_.SBUF__V17*/ meltfptr[7]),
                                                                         melt_string_str((melt_ptr_t)(/*_.NAMED_NAME__V18*/ meltfptr[8])));
                                            }
                                            ;
                                            /*_.STRBUF2STRING__V19*/
                                            meltfptr[5] =
                                                (meltgc_new_stringdup((meltobject_ptr_t)((/*!DISCR_VERBATIM_STRING*/ meltfrout->tabval[4])), melt_strbuf_str((melt_ptr_t)(/*_.SBUF__V17*/ meltfptr[7]))));;
                                            /*^compute*/
                                            /*_.LET___V16*/
                                            meltfptr[6] = /*_.STRBUF2STRING__V19*/ meltfptr[5];;

                                            MELT_LOCATION("warmelt-base.melt:3256:/ clear");
                                            /*clear*/ /*_.SBUF__V17*/
                                            meltfptr[7] = 0 ;
                                            /*^clear*/
                                            /*clear*/ /*_.NAMED_NAME__V18*/
                                            meltfptr[8] = 0 ;
                                            /*^clear*/
                                            /*clear*/ /*_.STRBUF2STRING__V19*/
                                            meltfptr[5] = 0 ;
                                            /*_._IFELSE___V15*/
                                            meltfptr[3] = /*_.LET___V16*/ meltfptr[6];;
                                            /*epilog*/

                                            MELT_LOCATION("warmelt-base.melt:3255:/ clear");
                                            /*clear*/ /*_.LET___V16*/
                                            meltfptr[6] = 0 ;
                                        }
                                        ;
                                    }
                                else    /*^cond.else*/
                                    {

                                        /*^block*/
                                        /*anyblock*/
                                        {

                                            MELT_LOCATION("warmelt-base.melt:3260:/ quasiblock");


                                            /*_.PROGN___V20*/
                                            meltfptr[7] = (/*nil*/NULL);;
                                            /*^compute*/
                                            /*_._IFELSE___V15*/
                                            meltfptr[3] = /*_.PROGN___V20*/ meltfptr[7];;
                                            /*epilog*/

                                            MELT_LOCATION("warmelt-base.melt:3255:/ clear");
                                            /*clear*/ /*_.PROGN___V20*/
                                            meltfptr[7] = 0 ;
                                        }
                                        ;
                                    }
                                ;
                                /*_._IFELSE___V10*/
                                meltfptr[4] = /*_._IFELSE___V15*/ meltfptr[3];;
                                /*epilog*/

                                MELT_LOCATION("warmelt-base.melt:3249:/ clear");
                                /*clear*/ /*_#IS_A__L3*/
                                meltfnum[2] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_._IFELSE___V15*/
                                meltfptr[3] = 0 ;
                            }
                            ;
                        }
                    ;
                    /*_._IFELSE___V3*/
                    meltfptr[2] = /*_._IFELSE___V10*/ meltfptr[4];;
                    /*epilog*/

                    MELT_LOCATION("warmelt-base.melt:3243:/ clear");
                    /*clear*/ /*_#IS_A__L2*/
                    meltfnum[1] = 0 ;
                    /*^clear*/
                    /*clear*/ /*_._IFELSE___V10*/
                    meltfptr[4] = 0 ;
                }
                ;
            }
        ;
        MELT_LOCATION("warmelt-base.melt:3241:/ quasiblock");


        /*_._RETVAL___V1*/
        meltfptr[0] = /*_._IFELSE___V3*/ meltfptr[2];;

        {
            MELT_LOCATION("warmelt-base.melt:3241:/ locexp");
            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                melt_warn_for_no_expected_secondary_results();
            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
            ;
        }
        ;
        /*^finalreturn*/
        ;
        /*finalret*/
        goto meltlabend_rout ;
        /*epilog*/

        /*^clear*/
        /*clear*/ /*_#IS_A__L1*/
        meltfnum[0] = 0 ;
        /*^clear*/
        /*clear*/ /*_._IFELSE___V3*/
        meltfptr[2] = 0 ;
    }

    ;
    goto meltlabend_rout;
meltlabend_rout:
    melt_trace_end("SYMBOL_CNAME", meltcallcount);
    melt_blocklevel_signals = current_blocklevel_signals_meltrout_35_WARMELTmiBASE_SYMBOL_CNAME_melt;
    return (melt_ptr_t)(/*_._RETVAL___V1*/ meltfptr[0]);
#undef meltcallcount
#undef meltfram__
#undef MELTFRAM_NBVARNUM
#undef MELTFRAM_NBVARPTR
} /*end meltrout_35_WARMELTmiBASE_SYMBOL_CNAME*/





melt_ptr_t MELT_MODULE_VISIBILITY
meltrout_36_WARMELTmiBASE_MAKE_SEXPR(meltclosure_ptr_t meltclosp_, melt_ptr_t meltfirstargp_, const melt_argdescr_cell_t meltxargdescr_[], union meltparam_un *meltxargtab_,
                                     const melt_argdescr_cell_t meltxresdescr_[], union meltparam_un *meltxrestab_)
{
    /*variadic*/ int variad_MAKE_SEXPR_ix = 0, variad_MAKE_SEXPR_len = melt_argdescr_length (meltxargdescr_);
#define melt_variadic_length  (0+variad_MAKE_SEXPR_len)
#define melt_variadic_index variad_MAKE_SEXPR_ix

    long current_blocklevel_signals_meltrout_36_WARMELTmiBASE_MAKE_SEXPR_melt =  melt_blocklevel_signals;

#if MELT_HAVE_DEBUG
    static long melt_call_counter__;
    long melt_thiscallcounter__ ATTRIBUTE_UNUSED = ++ melt_call_counter__;
#undef meltcallcount
#define meltcallcount melt_thiscallcounter__
#else
#undef meltcallcount
#define meltcallcount 0L
#endif
//// emitted by output_curframe_declstruct_init using OUTPUT_CURFRAME_DECLSTRUCT @warmelt-outobj.melt:1501

    /* start of frame for routine meltrout_36_WARMELTmiBASE_MAKE_SEXPR fromline 1716 */

    /** start of frame for meltrout_36_WARMELTmiBASE_MAKE_SEXPR of CLASS_PROCROUTINEOBJ from 1518**/

    /*curframdeclclassy*/ class MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR// fromline 1531
        : public Melt_CallFrameWithValues<49>
    {
    public: /* fromline 1535*/
        long mcfr_varnum[27];
        /*classy others*/
        const char* loc_CSTRING__o0;
        long _meltsparel;
        void melt_mark_stuff (void)
        {
        } /*end melt_mark_stuff*/
        virtual void melt_mark_ggc_data (void)
        {
            melt_mark_values ();
            melt_mark_stuff ();
        }; /*end melt_mark_ggc_data*/
        MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR(meltclosure_ptr_t clos) //the closure constructor fromline 1594
            : Melt_CallFrameWithValues<49> (
#if ENABLE_CHECKING /*fromline 1598*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1602*/
                sizeof(MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR), clos) {};
        MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR() //the constructor fromline 1606
            : Melt_CallFrameWithValues<49> (
#if ENABLE_CHECKING /*fromline 1610*/
                __FILE__, __LINE__,
#endif /* ENABLE_CHECKING fromline 1614*/
                sizeof(MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR)) {};
#if ENABLE_CHECKING /*fromline 1618*/
        MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR(const char*fil, int lin) //the constructor fromline 1620
            : Melt_CallFrameWithValues<49> (fil,lin, sizeof(MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR)) {};
        MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR(const char*fil, int lin, meltclosure_ptr_t clos) //the closure constructor fromline 1625
            : Melt_CallFrameWithValues<49> (fil,lin, sizeof(MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR), clos) {};
#endif /* ENABLE_CHECKING fromline 1630*/

    }; // end  class MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR


    /** end of frame for meltrout_36_WARMELTmiBASE_MAKE_SEXPR fromline 1661**/

    /* end of frame for routine meltrout_36_WARMELTmiBASE_MAKE_SEXPR fromline 1720 */

    /* classy proc frame meltrout_36_WARMELTmiBASE_MAKE_SEXPR */
    MeltFrame_meltrout_36_WARMELTmiBASE_MAKE_SEXPR
    meltfram__ /*declfrastruct fromline 1744*/
    /*classyprocarg meltrout_36_WARMELTmiBASE_MAKE_SEXPR fromline 1749*/
#if ENABLE_CHECKING
    (__FILE__, __LINE__, meltclosp_) /* fromline 1753*/
#else /*ENABLE_CHECKING */
    (meltclosp_) /* fromline 1757*/
#endif /*ENABLE_CHECKING */

    ;
#define meltframe meltfram__

    melt_trace_start("MAKE_SEXPR", meltcallcount);
    /*use arguments*/
    (void) meltclosp_;
    (void)meltfirstargp_;
    (void)meltxargdescr_;
    (void)meltxargtab_;
    (void)meltxresdescr_;
    (void)meltxrestab_;
    /*getargs*/

    /*getarg#0*/
    MELT_LOCATION("warmelt-base.melt:3267:/ getarg");
    /*_.LOC__V2*/
    meltfptr[1] = (melt_ptr_t) meltfirstargp_;
    ;
    goto meltlab_endgetargs;
meltlab_endgetargs:
    ;
    /*body*/
    /*^block*/
    /*anyblock*/
    {

        MELT_LOCATION("warmelt-base.melt:3269:/ quasiblock");


        /*_.CONT__V4*/
        meltfptr[3] =
            (meltgc_new_list((meltobject_ptr_t)((/*!DISCR_LIST*/ meltfrout->tabval[0]))));;

        MELT_CHECK_SIGNAL();
        ;
        /*^compute*/
        /*_#IS_MIXINT__L1*/
        meltfnum[0] =
            (melt_magic_discr((melt_ptr_t)(/*_.LOC__V2*/ meltfptr[1])) == MELTOBMAG_MIXINT);;
        MELT_LOCATION("warmelt-base.melt:3270:/ cond");
        /*cond*/
        if (/*_#IS_MIXINT__L1*/ meltfnum[0]) /*then*/
            {
                /*^cond.then*/
                /*_.VLOC__V5*/ meltfptr[4] = /*_.LOC__V2*/ meltfptr[1];;
            }
        else
            {
                MELT_LOCATION("warmelt-base.melt:3270:/ cond.else");

                /*^block*/
                /*anyblock*/
                {


                    MELT_CHECK_SIGNAL();
                    ;
                    /*_#IS_MIXLOC__L2*/
                    meltfnum[1] =
                        (melt_magic_discr((melt_ptr_t)(/*_.LOC__V2*/ meltfptr[1])) == MELTOBMAG_MIXLOC);;
                    MELT_LOCATION("warmelt-base.melt:3273:/ cond");
                    /*cond*/
                    if (/*_#IS_MIXLOC__L2*/ meltfnum[1]) /*then*/
                        {
                            /*^cond.then*/
                            /*_._IFELSE___V6*/ meltfptr[5] = /*_.LOC__V2*/ meltfptr[1];;
                        }
                    else
                        {
                            MELT_LOCATION("warmelt-base.melt:3273:/ cond.else");

                            /*^block*/
                            /*anyblock*/
                            {


                                MELT_CHECK_SIGNAL();
                                ;
                                /*_#IS_A__L3*/
                                meltfnum[2] =
                                    melt_is_instance_of((melt_ptr_t)(/*_.LOC__V2*/ meltfptr[1]), (melt_ptr_t)((/*!CLASS_LOCATED*/ meltfrout->tabval[1])));;
                                MELT_LOCATION("warmelt-base.melt:3275:/ cond");
                                /*cond*/
                                if (/*_#IS_A__L3*/ meltfnum[2]) /*then*/
                                    {
                                        /*^cond.then*/
                                        /*^block*/
                                        /*anyblock*/
                                        {

                                            MELT_LOCATION("warmelt-base.melt:3276:/ getslot");
                                            {
                                                melt_ptr_t slot=NULL, obj=NULL;
                                                obj = (melt_ptr_t)(/*_.LOC__V2*/ meltfptr[1]) /*=obj*/;
                                                melt_object_get_field(slot,obj, 1, "LOCA_LOCATION");
                                                /*_.LOCA_LOCATION__V8*/
                                                meltfptr[7] = slot;
                                            };
                                            ;
                                            /*_._IFELSE___V7*/
                                            meltfptr[6] = /*_.LOCA_LOCATION__V8*/ meltfptr[7];;
                                            /*epilog*/

                                            MELT_LOCATION("warmelt-base.melt:3275:/ clear");
                                            /*clear*/ /*_.LOCA_LOCATION__V8*/
                                            meltfptr[7] = 0 ;
                                        }
                                        ;
                                    }
                                else    /*^cond.else*/
                                    {

                                        /*^block*/
                                        /*anyblock*/
                                        {

                                            MELT_LOCATION("warmelt-base.melt:3277:/ quasiblock");


                                            /*_.PROGN___V9*/
                                            meltfptr[7] = (/*nil*/NULL);;
                                            /*^compute*/
                                            /*_._IFELSE___V7*/
                                            meltfptr[6] = /*_.PROGN___V9*/ meltfptr[7];;
                                            /*epilog*/

                                            MELT_LOCATION("warmelt-base.melt:3275:/ clear");
                                            /*clear*/ /*_.PROGN___V9*/
                                            meltfptr[7] = 0 ;
                                        }
                                        ;
                                    }
                                ;
                                /*_._IFELSE___V6*/
                                meltfptr[5] = /*_._IFELSE___V7*/ meltfptr[6];;
                                /*epilog*/

                                MELT_LOCATION("warmelt-base.melt:3273:/ clear");
                                /*clear*/ /*_#IS_A__L3*/
                                meltfnum[2] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_._IFELSE___V7*/
                                meltfptr[6] = 0 ;
                            }
                            ;
                        }
                    ;
                    /*_.VLOC__V5*/
                    meltfptr[4] = /*_._IFELSE___V6*/ meltfptr[5];;
                    /*epilog*/

                    MELT_LOCATION("warmelt-base.melt:3270:/ clear");
                    /*clear*/ /*_#IS_MIXLOC__L2*/
                    meltfnum[1] = 0 ;
                    /*^clear*/
                    /*clear*/ /*_._IFELSE___V6*/
                    meltfptr[5] = 0 ;
                }
                ;
            }
        ;

        MELT_CHECK_SIGNAL();
        ;
        MELT_LOCATION("warmelt-base.melt:3278:/ quasiblock");


        /*^rawallocobj*/
        /*rawallocobj*/
        {
            melt_ptr_t newobj = 0;
            melt_raw_object_create(newobj,(melt_ptr_t)((/*!CLASS_SEXPR*/ meltfrout->tabval[2])), (3), "CLASS_SEXPR");
            /*_.INST__V11*/
            meltfptr[6] =
                newobj;
        };
        ;
        /*^putslot*/
        /*putslot*/
        melt_assertmsg("checkobj putslot  _ @LOCA_LOCATION", melt_magic_discr((melt_ptr_t)(/*_.INST__V11*/ meltfptr[6])) == MELTOBMAG_OBJECT);
        melt_putfield_object((/*_.INST__V11*/ meltfptr[6]), (1), (/*_.VLOC__V5*/ meltfptr[4]), "LOCA_LOCATION");
        ;
        /*^putslot*/
        /*putslot*/
        melt_assertmsg("checkobj putslot  _ @SEXP_CONTENTS", melt_magic_discr((melt_ptr_t)(/*_.INST__V11*/ meltfptr[6])) == MELTOBMAG_OBJECT);
        melt_putfield_object((/*_.INST__V11*/ meltfptr[6]), (2), (/*_.CONT__V4*/ meltfptr[3]), "SEXP_CONTENTS");
        ;
        /*^touchobj*/

        melt_dbgtrace_written_object (/*_.INST__V11*/ meltfptr[6], "newly made instance");
        ;
        /*_.SEXPR__V10*/
        meltfptr[7] = /*_.INST__V11*/ meltfptr[6];;

#if MELT_HAVE_DEBUG
        MELT_LOCATION("warmelt-base.melt:3282:/ cppif.then");
        /*^block*/
        /*anyblock*/
        {


            {
                /*^locexp*/
                /*melt_increment_dbgcounter*/
#if MELT_HAVE_DEBUG
                melt_dbgcounter++;
#endif
                ;
            }
            ;

            MELT_CHECK_SIGNAL();
            ;
            /*_#MELT_NEED_DBG__L4*/
            meltfnum[2] =
                /*MELT_NEED_DBG*/
#if MELT_HAVE_DEBUG
                (/*melt_need_dbg*/ melt_need_debug ((int) 0))
#else
                0 /* no melt_need_dbg */
#endif /*MELT_HAVE_DEBUG*/
                ;;
            MELT_LOCATION("warmelt-base.melt:3282:/ cond");
            /*cond*/
            if (/*_#MELT_NEED_DBG__L4*/ meltfnum[2]) /*then*/
                {
                    /*^cond.then*/
                    /*^block*/
                    /*anyblock*/
                    {

                        /*_#MELT_CALLCOUNT__L5*/ meltfnum[1] =
                            /* for melt_callcount */
#if MELT_HAVE_DEBUG && defined (meltcallcount)
                            meltcallcount  /* melt_callcount debugging */
#else
                            0L  /* melt_callcount without debug */
#endif /* MELT_HAVE_DEBUG melt_callcount */
                            ;;

                        MELT_CHECK_SIGNAL();
                        ;
                        MELT_LOCATION("warmelt-base.melt:3282:/ apply");
                        /*apply*/
                        {
                            union meltparam_un argtab[5];
                            memset(&argtab, 0, sizeof(argtab));
                            /*^apply.arg*/
                            argtab[0].meltbp_long = /*_#MELT_CALLCOUNT__L5*/ meltfnum[1];
                            /*^apply.arg*/
                            argtab[1].meltbp_cstring =  "warmelt-base.melt";
                            /*^apply.arg*/
                            argtab[2].meltbp_long = 3282;
                            /*^apply.arg*/
                            argtab[3].meltbp_cstring =  "make_sexpr start loc=";
                            /*^apply.arg*/
                            argtab[4].meltbp_aptr = (melt_ptr_t*) &/*_.LOC__V2*/ meltfptr[1];
                            /*_.MELT_DEBUG_FUN__V13*/
                            meltfptr[12] =  melt_apply ((meltclosure_ptr_t)((/*!MELT_DEBUG_FUN*/ meltfrout->tabval[3])), (melt_ptr_t)((/*nil*/NULL)), (MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_PTR ""), argtab, "", (union meltparam_un*)0);
                        }
                        ;
                        /*_._IF___V12*/
                        meltfptr[5] = /*_.MELT_DEBUG_FUN__V13*/ meltfptr[12];;
                        /*epilog*/

                        MELT_LOCATION("warmelt-base.melt:3282:/ clear");
                        /*clear*/ /*_#MELT_CALLCOUNT__L5*/
                        meltfnum[1] = 0 ;
                        /*^clear*/
                        /*clear*/ /*_.MELT_DEBUG_FUN__V13*/
                        meltfptr[12] = 0 ;
                    }
                    ;
                }
            else    /*^cond.else*/
                {

                    /*_._IF___V12*/ meltfptr[5] =  /*fromline 1341*/ NULL ;;
                }
            ;

            {
                MELT_LOCATION("warmelt-base.melt:3282:/ locexp");
                /*void*/
                (void)0;
            }
            ;
            /*^quasiblock*/


            /*epilog*/

            /*^clear*/
            /*clear*/ /*_#MELT_NEED_DBG__L4*/
            meltfnum[2] = 0 ;
            /*^clear*/
            /*clear*/ /*_._IF___V12*/
            meltfptr[5] = 0 ;
        }

#else /*MELT_HAVE_DEBUG*/
        /*^cppif.else*/
        /*^block*/
        /*anyblock*/
        {


            {
                /*^locexp*/
                /*void*/(void)0;
            }
            ;
            /*epilog*/
        }

#endif /*MELT_HAVE_DEBUG*/
        ;
        MELT_LOCATION("warmelt-base.melt:3283:/ loop");
        /*loop*/
        {
meltlabloop_ARGLOOP_5:;/*^loopbody*/

            /*^block*/
            /*anyblock*/
            {


                MELT_CHECK_SIGNAL();
                ;

#if MELT_HAVE_DEBUG
                MELT_LOCATION("warmelt-base.melt:3285:/ cppif.then");
                /*^block*/
                /*anyblock*/
                {


                    {
                        /*^locexp*/
                        /*melt_increment_dbgcounter*/
#if MELT_HAVE_DEBUG
                        melt_dbgcounter++;
#endif
                        ;
                    }
                    ;

                    MELT_CHECK_SIGNAL();
                    ;
                    /*_#MELT_NEED_DBG__L6*/
                    meltfnum[1] =
                        /*MELT_NEED_DBG*/
#if MELT_HAVE_DEBUG
                        (/*melt_need_dbg*/ melt_need_debug ((int) 0))
#else
                        0 /* no melt_need_dbg */
#endif /*MELT_HAVE_DEBUG*/
                        ;;
                    MELT_LOCATION("warmelt-base.melt:3285:/ cond");
                    /*cond*/
                    if (/*_#MELT_NEED_DBG__L6*/ meltfnum[1]) /*then*/
                        {
                            /*^cond.then*/
                            /*^block*/
                            /*anyblock*/
                            {

                                /*_#MELT_CALLCOUNT__L7*/ meltfnum[2] =
                                    /* for melt_callcount */
#if MELT_HAVE_DEBUG && defined (meltcallcount)
                                    meltcallcount  /* melt_callcount debugging */
#else
                                    0L  /* melt_callcount without debug */
#endif /* MELT_HAVE_DEBUG melt_callcount */
                                    ;;
                                /*^compute*/
                                /*_#VARIADIC_INDEX__L8*/
                                meltfnum[7] =
                                    /*variadic_index*/
#ifdef melt_variadic_index
                                    (melt_variadic_index)
#else
                                    -1
#endif /*melt_variadic_index*/
                                    ;;

                                MELT_CHECK_SIGNAL();
                                ;
                                MELT_LOCATION("warmelt-base.melt:3285:/ apply");
                                /*apply*/
                                {
                                    union meltparam_un argtab[5];
                                    memset(&argtab, 0, sizeof(argtab));
                                    /*^apply.arg*/
                                    argtab[0].meltbp_long = /*_#MELT_CALLCOUNT__L7*/ meltfnum[2];
                                    /*^apply.arg*/
                                    argtab[1].meltbp_cstring =  "warmelt-base.melt";
                                    /*^apply.arg*/
                                    argtab[2].meltbp_long = 3285;
                                    /*^apply.arg*/
                                    argtab[3].meltbp_cstring =  "make_sexpr variadic_index:";
                                    /*^apply.arg*/
                                    argtab[4].meltbp_long = /*_#VARIADIC_INDEX__L8*/ meltfnum[7];
                                    /*_.MELT_DEBUG_FUN__V17*/
                                    meltfptr[16] =  melt_apply ((meltclosure_ptr_t)((/*!MELT_DEBUG_FUN*/ meltfrout->tabval[3])), (melt_ptr_t)((/*nil*/NULL)), (MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_LONG ""), argtab, "", (union meltparam_un*)0);
                                }
                                ;
                                /*_._IF___V16*/
                                meltfptr[15] = /*_.MELT_DEBUG_FUN__V17*/ meltfptr[16];;
                                /*epilog*/

                                MELT_LOCATION("warmelt-base.melt:3285:/ clear");
                                /*clear*/ /*_#MELT_CALLCOUNT__L7*/
                                meltfnum[2] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_#VARIADIC_INDEX__L8*/
                                meltfnum[7] = 0 ;
                                /*^clear*/
                                /*clear*/ /*_.MELT_DEBUG_FUN__V17*/
                                meltfptr[16] = 0 ;
                            }
                            ;
                        }
                    else    /*^cond.else*/
                        {

                            /*_._IF___V16*/ meltfptr[15] =  /*fromline 1341*/ NULL ;;
                        }
                    ;

                    {
                        MELT_LOCATION("warmelt-base.melt:3285:/ locexp");
                        /*void*/
                        (void)0;
                    }
                    ;
                    /*^quasiblock*/


                    /*epilog*/

                    /*^clear*/
                    /*clear*/ /*_#MELT_NEED_DBG__L6*/
                    meltfnum[1] = 0 ;
                    /*^clear*/
                    /*clear*/ /*_._IF___V16*/
                    meltfptr[15] = 0 ;
                }

#else /*MELT_HAVE_DEBUG*/
                /*^cppif.else*/
                /*^block*/
                /*anyblock*/
                {


                    {
                        /*^locexp*/
                        /*void*/(void)0;
                    }
                    ;
                    /*epilog*/
                }

#endif /*MELT_HAVE_DEBUG*/
                ;
                MELT_LOCATION("warmelt-base.melt:3286:/ cond");
                /*cond*/
                if (/*ifvariadic nomore*/ variad_MAKE_SEXPR_ix == variad_MAKE_SEXPR_len) /*then*/
                    {
                        /*^cond.then*/
                        /*^block*/
                        /*anyblock*/
                        {

                            /*^compute*/

                            /*consume variadic  !*/ variad_MAKE_SEXPR_ix += 0;;
                            MELT_LOCATION("warmelt-base.melt:3288:/ quasiblock");


                            /*^compute*/
                            /*_.ARGLOOP__V15*/
                            meltfptr[5] =  /*fromline 1341*/ NULL ;;

                            /*^exit*/
                            /*exit*/
                            {
                                goto meltlabexit_ARGLOOP_5;
                            }
                            ;
                            /*epilog*/
                        }
                        ;
                    }
                else
                    {
                        MELT_LOCATION("warmelt-base.melt:3286:/ cond.else");

                        /*^block*/
                        /*anyblock*/
                        {

                            /*^cond*/
                            /*cond*/ if (/*ifvariadic arg#1*/ variad_MAKE_SEXPR_ix>=0 && variad_MAKE_SEXPR_ix + 1 <= variad_MAKE_SEXPR_len && meltxargdescr_[variad_MAKE_SEXPR_ix]== MELTBPAR_CSTRING) /*then*/
                                {
                                    /*^cond.then*/
                                    /*^block*/
                                    /*anyblock*/
                                    {

                                        /*^compute*/
                                        /*_?*/ meltfram__.loc_CSTRING__o0 =
                                            /*variadic argument stuff*/ meltxargtab_[variad_MAKE_SEXPR_ix + 0].meltbp_cstring;;
                                        /*^compute*/

                                        /*consume variadic CSTRING !*/
                                        variad_MAKE_SEXPR_ix += 1;;

#if MELT_HAVE_DEBUG
                                        MELT_LOCATION("warmelt-base.melt:3290:/ cppif.then");
                                        /*^block*/
                                        /*anyblock*/
                                        {


                                            {
                                                /*^locexp*/
                                                /*melt_increment_dbgcounter*/
#if MELT_HAVE_DEBUG
                                                melt_dbgcounter++;
#endif
                                                ;
                                            }
                                            ;

                                            MELT_CHECK_SIGNAL();
                                            ;
                                            /*_#MELT_NEED_DBG__L9*/
                                            meltfnum[2] =
                                                /*MELT_NEED_DBG*/
#if MELT_HAVE_DEBUG
                                                (/*melt_need_dbg*/ melt_need_debug ((int) 0))
#else
                                                0 /* no melt_need_dbg */
#endif /*MELT_HAVE_DEBUG*/
                                                ;;
                                            MELT_LOCATION("warmelt-base.melt:3290:/ cond");
                                            /*cond*/
                                            if (/*_#MELT_NEED_DBG__L9*/ meltfnum[2]) /*then*/
                                                {
                                                    /*^cond.then*/
                                                    /*^block*/
                                                    /*anyblock*/
                                                    {

                                                        /*_#MELT_CALLCOUNT__L10*/ meltfnum[7] =
                                                            /* for melt_callcount */
#if MELT_HAVE_DEBUG && defined (meltcallcount)
                                                            meltcallcount  /* melt_callcount debugging */
#else
                                                            0L  /* melt_callcount without debug */
#endif /* MELT_HAVE_DEBUG melt_callcount */
                                                            ;;

                                                        MELT_CHECK_SIGNAL();
                                                        ;
                                                        MELT_LOCATION("warmelt-base.melt:3290:/ apply");
                                                        /*apply*/
                                                        {
                                                            union meltparam_un argtab[5];
                                                            memset(&argtab, 0, sizeof(argtab));
                                                            /*^apply.arg*/
                                                            argtab[0].meltbp_long = /*_#MELT_CALLCOUNT__L10*/ meltfnum[7];
                                                            /*^apply.arg*/
                                                            argtab[1].meltbp_cstring =  "warmelt-base.melt";
                                                            /*^apply.arg*/
                                                            argtab[2].meltbp_long = 3290;
                                                            /*^apply.arg*/
                                                            argtab[3].meltbp_cstring =  "make_sexpr str=";
                                                            /*^apply.arg*/
                                                            argtab[4].meltbp_cstring = /*_?*/ meltfram__.loc_CSTRING__o0;
                                                            /*_.MELT_DEBUG_FUN__V19*/
                                                            meltfptr[15] =  melt_apply ((meltclosure_ptr_t)((/*!MELT_DEBUG_FUN*/ meltfrout->tabval[3])), (melt_ptr_t)((/*nil*/NULL)), (MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_CSTRING ""), argtab, "", (union meltparam_un*)0);
                                                        }
                                                        ;
                                                        /*_._IF___V18*/
                                                        meltfptr[16] = /*_.MELT_DEBUG_FUN__V19*/ meltfptr[15];;
                                                        /*epilog*/

                                                        MELT_LOCATION("warmelt-base.melt:3290:/ clear");
                                                        /*clear*/ /*_#MELT_CALLCOUNT__L10*/
                                                        meltfnum[7] = 0 ;
                                                        /*^clear*/
                                                        /*clear*/ /*_.MELT_DEBUG_FUN__V19*/
                                                        meltfptr[15] = 0 ;
                                                    }
                                                    ;
                                                }
                                            else    /*^cond.else*/
                                                {

                                                    /*_._IF___V18*/ meltfptr[16] =  /*fromline 1341*/ NULL ;;
                                                }
                                            ;

                                            {
                                                MELT_LOCATION("warmelt-base.melt:3290:/ locexp");
                                                /*void*/
                                                (void)0;
                                            }
                                            ;
                                            /*^quasiblock*/


                                            /*epilog*/

                                            /*^clear*/
                                            /*clear*/ /*_#MELT_NEED_DBG__L9*/
                                            meltfnum[2] = 0 ;
                                            /*^clear*/
                                            /*clear*/ /*_._IF___V18*/
                                            meltfptr[16] = 0 ;
                                        }

#else /*MELT_HAVE_DEBUG*/
                                        /*^cppif.else*/
                                        /*^block*/
                                        /*anyblock*/
                                        {


                                            {
                                                /*^locexp*/
                                                /*void*/(void)0;
                                            }
                                            ;
                                            /*epilog*/
                                        }

#endif /*MELT_HAVE_DEBUG*/
                                        ;
                                        MELT_LOCATION("warmelt-base.melt:3291:/ quasiblock");


                                        /*_.BS__V20*/
                                        meltfptr[15] =
                                            /*full constboxing*/ /*ctype_cstring boxing*/ meltgc_new_stringdup((meltobject_ptr_t) (((melt_ptr_t)(MELT_PREDEF(DISCR_STRING)))), (/*_?*/ meltfram__.loc_CSTRING__o0));;

                                        {
                                            MELT_LOCATION("warmelt-base.melt:3293:/ locexp");
                                            meltgc_append_list((melt_ptr_t)(/*_.CONT__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.BS__V20*/ meltfptr[15]));
                                        }
                                        ;

                                        MELT_LOCATION("warmelt-base.melt:3291:/ clear");
                                        /*clear*/ /*_.BS__V20*/
                                        meltfptr[15] = 0 ;

                                        MELT_CHECK_SIGNAL();
                                        ;
                                        /*epilog*/

                                        MELT_LOCATION("warmelt-base.melt:3286:/ clear");
                                        /*clear*/ /*_?*/
                                        meltfram__.loc_CSTRING__o0 = 0 ;
                                    }
                                    ;
                                }
                            else    /*^cond.else*/
                                {

                                    /*^block*/
                                    /*anyblock*/
                                    {

                                        /*^cond*/
                                        /*cond*/ if (/*ifvariadic arg#1*/ variad_MAKE_SEXPR_ix>=0 && variad_MAKE_SEXPR_ix + 1 <= variad_MAKE_SEXPR_len && meltxargdescr_[variad_MAKE_SEXPR_ix]== MELTBPAR_LONG) /*then*/
                                            {
                                                /*^cond.then*/
                                                /*^block*/
                                                /*anyblock*/
                                                {

                                                    /*^compute*/
                                                    /*_#NUM__L11*/ meltfnum[1] =
                                                        /*variadic argument stuff*/ meltxargtab_[variad_MAKE_SEXPR_ix + 0].meltbp_long;;
                                                    /*^compute*/

                                                    /*consume variadic LONG !*/
                                                    variad_MAKE_SEXPR_ix += 1;;

#if MELT_HAVE_DEBUG
                                                    MELT_LOCATION("warmelt-base.melt:3295:/ cppif.then");
                                                    /*^block*/
                                                    /*anyblock*/
                                                    {


                                                        {
                                                            /*^locexp*/
                                                            /*melt_increment_dbgcounter*/
#if MELT_HAVE_DEBUG
                                                            melt_dbgcounter++;
#endif
                                                            ;
                                                        }
                                                        ;

                                                        MELT_CHECK_SIGNAL();
                                                        ;
                                                        /*_#MELT_NEED_DBG__L12*/
                                                        meltfnum[7] =
                                                            /*MELT_NEED_DBG*/
#if MELT_HAVE_DEBUG
                                                            (/*melt_need_dbg*/ melt_need_debug ((int) 0))
#else
                                                            0 /* no melt_need_dbg */
#endif /*MELT_HAVE_DEBUG*/
                                                            ;;
                                                        MELT_LOCATION("warmelt-base.melt:3295:/ cond");
                                                        /*cond*/
                                                        if (/*_#MELT_NEED_DBG__L12*/ meltfnum[7]) /*then*/
                                                            {
                                                                /*^cond.then*/
                                                                /*^block*/
                                                                /*anyblock*/
                                                                {

                                                                    /*_#MELT_CALLCOUNT__L13*/ meltfnum[2] =
                                                                        /* for melt_callcount */
#if MELT_HAVE_DEBUG && defined (meltcallcount)
                                                                        meltcallcount  /* melt_callcount debugging */
#else
                                                                        0L  /* melt_callcount without debug */
#endif /* MELT_HAVE_DEBUG melt_callcount */
                                                                        ;;

                                                                    MELT_CHECK_SIGNAL();
                                                                    ;
                                                                    MELT_LOCATION("warmelt-base.melt:3295:/ apply");
                                                                    /*apply*/
                                                                    {
                                                                        union meltparam_un argtab[5];
                                                                        memset(&argtab, 0, sizeof(argtab));
                                                                        /*^apply.arg*/
                                                                        argtab[0].meltbp_long = /*_#MELT_CALLCOUNT__L13*/ meltfnum[2];
                                                                        /*^apply.arg*/
                                                                        argtab[1].meltbp_cstring =  "warmelt-base.melt";
                                                                        /*^apply.arg*/
                                                                        argtab[2].meltbp_long = 3295;
                                                                        /*^apply.arg*/
                                                                        argtab[3].meltbp_cstring =  "make_sexpr num=";
                                                                        /*^apply.arg*/
                                                                        argtab[4].meltbp_long = /*_#NUM__L11*/ meltfnum[1];
                                                                        /*_.MELT_DEBUG_FUN__V22*/
                                                                        meltfptr[15] =  melt_apply ((meltclosure_ptr_t)((/*!MELT_DEBUG_FUN*/ meltfrout->tabval[3])), (melt_ptr_t)((/*nil*/NULL)), (MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_LONG ""), argtab, "", (union meltparam_un*)0);
                                                                    }
                                                                    ;
                                                                    /*_._IF___V21*/
                                                                    meltfptr[16] = /*_.MELT_DEBUG_FUN__V22*/ meltfptr[15];;
                                                                    /*epilog*/

                                                                    MELT_LOCATION("warmelt-base.melt:3295:/ clear");
                                                                    /*clear*/ /*_#MELT_CALLCOUNT__L13*/
                                                                    meltfnum[2] = 0 ;
                                                                    /*^clear*/
                                                                    /*clear*/ /*_.MELT_DEBUG_FUN__V22*/
                                                                    meltfptr[15] = 0 ;
                                                                }
                                                                ;
                                                            }
                                                        else    /*^cond.else*/
                                                            {

                                                                /*_._IF___V21*/ meltfptr[16] =  /*fromline 1341*/ NULL ;;
                                                            }
                                                        ;

                                                        {
                                                            MELT_LOCATION("warmelt-base.melt:3295:/ locexp");
                                                            /*void*/
                                                            (void)0;
                                                        }
                                                        ;
                                                        /*^quasiblock*/


                                                        /*epilog*/

                                                        /*^clear*/
                                                        /*clear*/ /*_#MELT_NEED_DBG__L12*/
                                                        meltfnum[7] = 0 ;
                                                        /*^clear*/
                                                        /*clear*/ /*_._IF___V21*/
                                                        meltfptr[16] = 0 ;
                                                    }

#else /*MELT_HAVE_DEBUG*/
                                                    /*^cppif.else*/
                                                    /*^block*/
                                                    /*anyblock*/
                                                    {


                                                        {
                                                            /*^locexp*/
                                                            /*void*/(void)0;
                                                        }
                                                        ;
                                                        /*epilog*/
                                                    }

#endif /*MELT_HAVE_DEBUG*/
                                                    ;
                                                    MELT_LOCATION("warmelt-base.melt:3296:/ quasiblock");


                                                    /*_.BS__V23*/
                                                    meltfptr[15] =
                                                        /*full constboxing*/ /*boxing ctype_long*/ meltgc_new_int((meltobject_ptr_t) (((melt_ptr_t)(MELT_PREDEF(DISCR_CONSTANT_INTEGER)))), (/*_#NUM__L11*/ meltfnum[1]));;

                                                    {
                                                        MELT_LOCATION("warmelt-base.melt:3298:/ locexp");
                                                        meltgc_append_list((melt_ptr_t)(/*_.CONT__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.BS__V23*/ meltfptr[15]));
                                                    }
                                                    ;

                                                    MELT_LOCATION("warmelt-base.melt:3296:/ clear");
                                                    /*clear*/ /*_.BS__V23*/
                                                    meltfptr[15] = 0 ;

                                                    MELT_CHECK_SIGNAL();
                                                    ;
                                                    /*epilog*/

                                                    MELT_LOCATION("warmelt-base.melt:3286:/ clear");
                                                    /*clear*/ /*_#NUM__L11*/
                                                    meltfnum[1] = 0 ;
                                                }
                                                ;
                                            }
                                        else    /*^cond.else*/
                                            {

                                                /*^block*/
                                                /*anyblock*/
                                                {

                                                    /*^cond*/
                                                    /*cond*/ if (/*ifvariadic arg#1*/ variad_MAKE_SEXPR_ix>=0 && variad_MAKE_SEXPR_ix + 1 <= variad_MAKE_SEXPR_len && meltxargdescr_[variad_MAKE_SEXPR_ix]== MELTBPAR_PTR) /*then*/
                                                        {
                                                            /*^cond.then*/
                                                            /*^block*/
                                                            /*anyblock*/
                                                            {

                                                                /*^compute*/
                                                                /*_.V__V24*/ meltfptr[16] =
                                                                    /*variadic argument value*/ ((meltxargtab_[variad_MAKE_SEXPR_ix + 0].meltbp_aptr) ? (*(meltxargtab_[variad_MAKE_SEXPR_ix + 0].meltbp_aptr)) : NULL);;
                                                                /*^compute*/

                                                                /*consume variadic Value !*/
                                                                variad_MAKE_SEXPR_ix += 1;;

#if MELT_HAVE_DEBUG
                                                                MELT_LOCATION("warmelt-base.melt:3300:/ cppif.then");
                                                                /*^block*/
                                                                /*anyblock*/
                                                                {


                                                                    {
                                                                        /*^locexp*/
                                                                        /*melt_increment_dbgcounter*/
#if MELT_HAVE_DEBUG
                                                                        melt_dbgcounter++;
#endif
                                                                        ;
                                                                    }
                                                                    ;

                                                                    MELT_CHECK_SIGNAL();
                                                                    ;
                                                                    /*_#MELT_NEED_DBG__L14*/
                                                                    meltfnum[2] =
                                                                        /*MELT_NEED_DBG*/
#if MELT_HAVE_DEBUG
                                                                        (/*melt_need_dbg*/ melt_need_debug ((int) 0))
#else
                                                                        0 /* no melt_need_dbg */
#endif /*MELT_HAVE_DEBUG*/
                                                                        ;;
                                                                    MELT_LOCATION("warmelt-base.melt:3300:/ cond");
                                                                    /*cond*/
                                                                    if (/*_#MELT_NEED_DBG__L14*/ meltfnum[2]) /*then*/
                                                                        {
                                                                            /*^cond.then*/
                                                                            /*^block*/
                                                                            /*anyblock*/
                                                                            {

                                                                                /*_#MELT_CALLCOUNT__L15*/ meltfnum[7] =
                                                                                    /* for melt_callcount */
#if MELT_HAVE_DEBUG && defined (meltcallcount)
                                                                                    meltcallcount  /* melt_callcount debugging */
#else
                                                                                    0L  /* melt_callcount without debug */
#endif /* MELT_HAVE_DEBUG melt_callcount */
                                                                                    ;;

                                                                                MELT_CHECK_SIGNAL();
                                                                                ;
                                                                                MELT_LOCATION("warmelt-base.melt:3300:/ apply");
                                                                                /*apply*/
                                                                                {
                                                                                    union meltparam_un argtab[5];
                                                                                    memset(&argtab, 0, sizeof(argtab));
                                                                                    /*^apply.arg*/
                                                                                    argtab[0].meltbp_long = /*_#MELT_CALLCOUNT__L15*/ meltfnum[7];
                                                                                    /*^apply.arg*/
                                                                                    argtab[1].meltbp_cstring =  "warmelt-base.melt";
                                                                                    /*^apply.arg*/
                                                                                    argtab[2].meltbp_long = 3300;
                                                                                    /*^apply.arg*/
                                                                                    argtab[3].meltbp_cstring =  "make_sexpr v=";
                                                                                    /*^apply.arg*/
                                                                                    argtab[4].meltbp_aptr = (melt_ptr_t*) &/*_.V__V24*/ meltfptr[16];
                                                                                    /*_.MELT_DEBUG_FUN__V26*/
                                                                                    meltfptr[25] =  melt_apply ((meltclosure_ptr_t)((/*!MELT_DEBUG_FUN*/ meltfrout->tabval[3])), (melt_ptr_t)((/*nil*/NULL)), (MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_PTR ""), argtab, "", (union meltparam_un*)0);
                                                                                }
                                                                                ;
                                                                                /*_._IF___V25*/
                                                                                meltfptr[15] = /*_.MELT_DEBUG_FUN__V26*/ meltfptr[25];;
                                                                                /*epilog*/

                                                                                MELT_LOCATION("warmelt-base.melt:3300:/ clear");
                                                                                /*clear*/ /*_#MELT_CALLCOUNT__L15*/
                                                                                meltfnum[7] = 0 ;
                                                                                /*^clear*/
                                                                                /*clear*/ /*_.MELT_DEBUG_FUN__V26*/
                                                                                meltfptr[25] = 0 ;
                                                                            }
                                                                            ;
                                                                        }
                                                                    else    /*^cond.else*/
                                                                        {

                                                                            /*_._IF___V25*/ meltfptr[15] =  /*fromline 1341*/ NULL ;;
                                                                        }
                                                                    ;

                                                                    {
                                                                        MELT_LOCATION("warmelt-base.melt:3300:/ locexp");
                                                                        /*void*/
                                                                        (void)0;
                                                                    }
                                                                    ;
                                                                    /*^quasiblock*/


                                                                    /*epilog*/

                                                                    /*^clear*/
                                                                    /*clear*/ /*_#MELT_NEED_DBG__L14*/
                                                                    meltfnum[2] = 0 ;
                                                                    /*^clear*/
                                                                    /*clear*/ /*_._IF___V25*/
                                                                    meltfptr[15] = 0 ;
                                                                }

#else /*MELT_HAVE_DEBUG*/
                                                                /*^cppif.else*/
                                                                /*^block*/
                                                                /*anyblock*/
                                                                {


                                                                    {
                                                                        /*^locexp*/
                                                                        /*void*/(void)0;
                                                                    }
                                                                    ;
                                                                    /*epilog*/
                                                                }

#endif /*MELT_HAVE_DEBUG*/
                                                                ;

                                                                MELT_CHECK_SIGNAL();
                                                                ;
                                                                /*_#IS_OBJECT__L16*/
                                                                meltfnum[1] =
                                                                    (melt_magic_discr((melt_ptr_t)(/*_.V__V24*/ meltfptr[16])) == MELTOBMAG_OBJECT);;
                                                                MELT_LOCATION("warmelt-base.melt:3301:/ cond");
                                                                /*cond*/
                                                                if (/*_#IS_OBJECT__L16*/ meltfnum[1]) /*then*/
                                                                    {
                                                                        /*^cond.then*/
                                                                        /*^block*/
                                                                        /*anyblock*/
                                                                        {




                                                                            {
                                                                                MELT_LOCATION("warmelt-base.melt:3302:/ locexp");
                                                                                meltgc_append_list((melt_ptr_t)(/*_.CONT__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.V__V24*/ meltfptr[16]));
                                                                            }
                                                                            ;
                                                                            /*clear*/ /*_._IFELSE___V27*/
                                                                            meltfptr[25] = 0 ;
                                                                            /*epilog*/
                                                                        }
                                                                        ;
                                                                    }
                                                                else
                                                                    {
                                                                        MELT_LOCATION("warmelt-base.melt:3301:/ cond.else");

                                                                        /*^block*/
                                                                        /*anyblock*/
                                                                        {


                                                                            MELT_CHECK_SIGNAL();
                                                                            ;
                                                                            /*_#IS_INTEGERBOX__L17*/
                                                                            meltfnum[7] =
                                                                                (melt_magic_discr((melt_ptr_t)(/*_.V__V24*/ meltfptr[16])) == MELTOBMAG_INT);;
                                                                            MELT_LOCATION("warmelt-base.melt:3303:/ cond");
                                                                            /*cond*/
                                                                            if (/*_#IS_INTEGERBOX__L17*/ meltfnum[7]) /*then*/
                                                                                {
                                                                                    /*^cond.then*/
                                                                                    /*^block*/
                                                                                    /*anyblock*/
                                                                                    {




                                                                                        {
                                                                                            MELT_LOCATION("warmelt-base.melt:3304:/ locexp");
                                                                                            meltgc_append_list((melt_ptr_t)(/*_.CONT__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.V__V24*/ meltfptr[16]));
                                                                                        }
                                                                                        ;
                                                                                        /*clear*/ /*_._IFELSE___V28*/
                                                                                        meltfptr[15] = 0 ;
                                                                                        /*epilog*/
                                                                                    }
                                                                                    ;
                                                                                }
                                                                            else
                                                                                {
                                                                                    MELT_LOCATION("warmelt-base.melt:3303:/ cond.else");

                                                                                    /*^block*/
                                                                                    /*anyblock*/
                                                                                    {


                                                                                        MELT_CHECK_SIGNAL();
                                                                                        ;
                                                                                        /*_#IS_STRING__L18*/
                                                                                        meltfnum[2] =
                                                                                            (melt_magic_discr((melt_ptr_t)(/*_.V__V24*/ meltfptr[16])) == MELTOBMAG_STRING);;
                                                                                        MELT_LOCATION("warmelt-base.melt:3305:/ cond");
                                                                                        /*cond*/
                                                                                        if (/*_#IS_STRING__L18*/ meltfnum[2]) /*then*/
                                                                                            {
                                                                                                /*^cond.then*/
                                                                                                /*^block*/
                                                                                                /*anyblock*/
                                                                                                {




                                                                                                    {
                                                                                                        MELT_LOCATION("warmelt-base.melt:3306:/ locexp");
                                                                                                        meltgc_append_list((melt_ptr_t)(/*_.CONT__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.V__V24*/ meltfptr[16]));
                                                                                                    }
                                                                                                    ;
                                                                                                    /*clear*/ /*_._IFELSE___V29*/
                                                                                                    meltfptr[28] = 0 ;
                                                                                                    /*epilog*/
                                                                                                }
                                                                                                ;
                                                                                            }
                                                                                        else
                                                                                            {
                                                                                                MELT_LOCATION("warmelt-base.melt:3305:/ cond.else");

                                                                                                /*^block*/
                                                                                                /*anyblock*/
                                                                                                {


                                                                                                    MELT_CHECK_SIGNAL();
                                                                                                    ;
                                                                                                    /*_#IS_MULTIPLE__L19*/
                                                                                                    meltfnum[18] =
                                                                                                        (melt_magic_discr((melt_ptr_t)(/*_.V__V24*/ meltfptr[16])) == MELTOBMAG_MULTIPLE);;
                                                                                                    MELT_LOCATION("warmelt-base.melt:3307:/ cond");
                                                                                                    /*cond*/
                                                                                                    if (/*_#IS_MULTIPLE__L19*/ meltfnum[18]) /*then*/
                                                                                                        {
                                                                                                            /*^cond.then*/
                                                                                                            /*^block*/
                                                                                                            /*anyblock*/
                                                                                                            {

                                                                                                                /*citerblock FOREACH_IN_MULTIPLE*/
                                                                                                                {
                                                                                                                    /* start foreach_in_multiple meltcit1__EACHTUP */
                                                                                                                    long  meltcit1__EACHTUP_ln = melt_multiple_length((melt_ptr_t)/*_.V__V24*/ meltfptr[16]);
                                                                                                                    for (/*_#IX__L20*/ meltfnum[19] = 0;
                                                                                                                                       (/*_#IX__L20*/ meltfnum[19] >= 0) && (/*_#IX__L20*/ meltfnum[19] <  meltcit1__EACHTUP_ln);
                                                                                                                                       /*_#IX__L20*/ meltfnum[19]++)
                                                                                                                        {
                                                                                                                            /*_.COMP__V31*/ meltfptr[30] = melt_multiple_nth((melt_ptr_t)(/*_.V__V24*/ meltfptr[16]),  /*_#IX__L20*/ meltfnum[19]);




                                                                                                                            {
                                                                                                                                MELT_LOCATION("warmelt-base.melt:3311:/ locexp");
                                                                                                                                meltgc_append_list((melt_ptr_t)(/*_.CONT__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.COMP__V31*/ meltfptr[30]));
                                                                                                                            }
                                                                                                                            ;
                                                                                                                            if (/*_#IX__L20*/ meltfnum[19]<0) break;
                                                                                                                        } /* end  foreach_in_multiple meltcit1__EACHTUP */

                                                                                                                    /*citerepilog*/

                                                                                                                    MELT_LOCATION("warmelt-base.melt:3308:/ clear");
                                                                                                                    /*clear*/ /*_.COMP__V31*/
                                                                                                                    meltfptr[30] = 0 ;
                                                                                                                    /*^clear*/
                                                                                                                    /*clear*/ /*_#IX__L20*/
                                                                                                                    meltfnum[19] = 0 ;
                                                                                                                } /*endciterblock FOREACH_IN_MULTIPLE*/
                                                                                                                ;
                                                                                                                /*epilog*/
                                                                                                            }
                                                                                                            ;
                                                                                                        }
                                                                                                    else
                                                                                                        {
                                                                                                            MELT_LOCATION("warmelt-base.melt:3307:/ cond.else");

                                                                                                            /*^block*/
                                                                                                            /*anyblock*/
                                                                                                            {


                                                                                                                MELT_CHECK_SIGNAL();
                                                                                                                ;
                                                                                                                /*_#IS_LIST__L21*/
                                                                                                                meltfnum[20] =
                                                                                                                    (melt_magic_discr((melt_ptr_t)(/*_.V__V24*/ meltfptr[16])) == MELTOBMAG_LIST);;
                                                                                                                MELT_LOCATION("warmelt-base.melt:3312:/ cond");
                                                                                                                /*cond*/
                                                                                                                if (/*_#IS_LIST__L21*/ meltfnum[20]) /*then*/
                                                                                                                    {
                                                                                                                        /*^cond.then*/
                                                                                                                        /*^block*/
                                                                                                                        /*anyblock*/
                                                                                                                        {

                                                                                                                            /*citerblock FOREACH_IN_LIST*/
                                                                                                                            {
                                                                                                                                /* start foreach_in_list meltcit2__EACHLIST */
                                                                                                                                for (/*_.CURPAIR__V33*/ meltfptr[32] = melt_list_first( (melt_ptr_t)/*_.V__V24*/ meltfptr[16]);
                                                                                                                                                        melt_magic_discr((melt_ptr_t) /*_.CURPAIR__V33*/ meltfptr[32]) == MELTOBMAG_PAIR;
                                                                                                                                                        /*_.CURPAIR__V33*/ meltfptr[32] = melt_pair_tail((melt_ptr_t) /*_.CURPAIR__V33*/ meltfptr[32]))
                                                                                                                                    {
                                                                                                                                        /*_.CURCOMP__V34*/ meltfptr[33] = melt_pair_head((melt_ptr_t) /*_.CURPAIR__V33*/ meltfptr[32]);



                                                                                                                                        {
                                                                                                                                            MELT_LOCATION("warmelt-base.melt:3316:/ locexp");
                                                                                                                                            meltgc_append_list((melt_ptr_t)(/*_.CONT__V4*/ meltfptr[3]), (melt_ptr_t)(/*_.CURCOMP__V34*/ meltfptr[33]));
                                                                                                                                        }
                                                                                                                                        ;
                                                                                                                                    } /* end foreach_in_list meltcit2__EACHLIST */
                                                                                                                                /*_.CURPAIR__V33*/ meltfptr[32] = NULL;
                                                                                                                                /*_.CURCOMP__V34*/
                                                                                                                                meltfptr[33] = NULL;


                                                                                                                                /*citerepilog*/

                                                                                                                                MELT_LOCATION("warmelt-base.melt:3313:/ clear");
                                                                                                                                /*clear*/ /*_.CURPAIR__V33*/
                                                                                                                                meltfptr[32] = 0 ;
                                                                                                                                /*^clear*/
                                                                                                                                /*clear*/ /*_.CURCOMP__V34*/
                                                                                                                                meltfptr[33] = 0 ;
                                                                                                                            } /*endciterblock FOREACH_IN_LIST*/
                                                                                                                            ;
                                                                                                                            /*epilog*/
                                                                                                                        }
                                                                                                                        ;
                                                                                                                    }
                                                                                                                else
                                                                                                                    {
                                                                                                                        MELT_LOCATION("warmelt-base.melt:3312:/ cond.else");

                                                                                                                        /*^block*/
                                                                                                                        /*anyblock*/
                                                                                                                        {


                                                                                                                            MELT_CHECK_SIGNAL();
                                                                                                                            ;
                                                                                                                            /*_#IS_CLOSURE__L22*/
                                                                                                                            meltfnum[21] =
                                                                                                                                (melt_magic_discr((melt_ptr_t)(/*_.V__V24*/ meltfptr[16])) == MELTOBMAG_CLOSURE);;
                                                                                                                            MELT_LOCATION("warmelt-base.melt:3317:/ cond");
                                                                                                                            /*cond*/
                                                                                                                            if (/*_#IS_CLOSURE__L22*/ meltfnum[21]) /*then*/
                                                                                                                                {
                                                                                                                                    /*^cond.then*/
                                                                                                                                    /*^block*/
                                                                                                                                    /*anyblock*/
                                                                                                                                    {


                                                                                                                                        MELT_CHECK_SIGNAL();
                                                                                                                                        ;
                                                                                                                                        MELT_LOCATION("warmelt-base.melt:3318:/ apply");
                                                                                                                                        /*apply*/
                                                                                                                                        {
                                                                                                                                            /*_.V__V36*/ meltfptr[35] =  melt_apply ((meltclosure_ptr_t)(/*_.V__V24*/ meltfptr[16]), (melt_ptr_t)(/*_.CONT__V4*/ meltfptr[3]), (""), (union meltparam_un*)0, "", (union meltparam_un*)0);
                                                                                                                                        }
                                                                                                                                        ;
                                                                                                                                        /*_._IFELSE___V35*/
                                                                                                                                        meltfptr[34] = /*_.V__V36*/ meltfptr[35];;
                                                                                                                                        /*epilog*/

                                                                                                                                        MELT_LOCATION("warmelt-base.melt:3317:/ clear");
                                                                                                                                        /*clear*/ /*_.V__V36*/
                                                                                                                                        meltfptr[35] = 0 ;
                                                                                                                                    }
                                                                                                                                    ;
                                                                                                                                }
                                                                                                                            else    /*^cond.else*/
                                                                                                                                {

                                                                                                                                    /*^block*/
                                                                                                                                    /*anyblock*/
                                                                                                                                    {


                                                                                                                                        {
                                                                                                                                            MELT_LOCATION("warmelt-base.melt:3320:/ locexp");
                                                                                                                                            /* error_plain */
                                                                                                                                            melt_error_str((melt_ptr_t)(/*_.VLOC__V5*/ meltfptr[4]), ( "unexpected value for MAKE_SEXPR"), (melt_ptr_t)0);
                                                                                                                                        }
                                                                                                                                        ;
                                                                                                                                        /*_.DISCRIM__V37*/
                                                                                                                                        meltfptr[35] =
                                                                                                                                            ((melt_ptr_t) (melt_discr((melt_ptr_t)(/*_.V__V24*/ meltfptr[16]))));;
                                                                                                                                        MELT_LOCATION("warmelt-base.melt:3321:/ cond");
                                                                                                                                        /*cond*/
                                                                                                                                        if (
                                                                                                                                            /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.DISCRIM__V37*/ meltfptr[35]),
                                                                                                                                                                          (melt_ptr_t)((/*!CLASS_NAMED*/ meltfrout->tabval[4])))
                                                                                                                                        ) /*then*/
                                                                                                                                            {
                                                                                                                                                /*^cond.then*/
                                                                                                                                                /*^getslot*/
                                                                                                                                                {
                                                                                                                                                    melt_ptr_t slot=NULL, obj=NULL;
                                                                                                                                                    obj = (melt_ptr_t)(/*_.DISCRIM__V37*/ meltfptr[35]) /*=obj*/;
                                                                                                                                                    melt_object_get_field(slot,obj, 1, "NAMED_NAME");
                                                                                                                                                    /*_.NAMED_NAME__V38*/
                                                                                                                                                    meltfptr[37] = slot;
                                                                                                                                                };
                                                                                                                                                ;
                                                                                                                                            }
                                                                                                                                        else    /*^cond.else*/
                                                                                                                                            {

                                                                                                                                                /*_.NAMED_NAME__V38*/ meltfptr[37] =  /*fromline 1341*/ NULL ;;
                                                                                                                                            }
                                                                                                                                        ;

                                                                                                                                        {
                                                                                                                                            MELT_LOCATION("warmelt-base.melt:3321:/ locexp");
                                                                                                                                            melt_inform_str((melt_ptr_t)(/*_.VLOC__V5*/ meltfptr[4]), ( "discriminant of bad value for MAKE_SEXPR is"), (melt_ptr_t)(/*_.NAMED_NAME__V38*/ meltfptr[37]));
                                                                                                                                        }
                                                                                                                                        ;

                                                                                                                                        MELT_CHECK_SIGNAL();
                                                                                                                                        ;
                                                                                                                                        MELT_LOCATION("warmelt-base.melt:3322:/ quasiblock");


                                                                                                                                        /*_._RETVAL___V1*/
                                                                                                                                        meltfptr[0] = (/*nil*/NULL);;

                                                                                                                                        {
                                                                                                                                            MELT_LOCATION("warmelt-base.melt:3322:/ locexp");
                                                                                                                                            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
                                                                                                                                            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                                                                                                                                                melt_warn_for_no_expected_secondary_results();
                                                                                                                                            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
                                                                                                                                            ;
                                                                                                                                        }
                                                                                                                                        ;
                                                                                                                                        /*^finalreturn*/
                                                                                                                                        ;
                                                                                                                                        /*finalret*/
                                                                                                                                        goto meltlabend_rout ;
                                                                                                                                        MELT_LOCATION("warmelt-base.melt:3319:/ quasiblock");


                                                                                                                                        /*_.PROGN___V40*/
                                                                                                                                        meltfptr[39] = /*_.RETURN___V39*/ meltfptr[38];;
                                                                                                                                        /*^compute*/
                                                                                                                                        /*_._IFELSE___V35*/
                                                                                                                                        meltfptr[34] = /*_.PROGN___V40*/ meltfptr[39];;
                                                                                                                                        /*epilog*/

                                                                                                                                        MELT_LOCATION("warmelt-base.melt:3317:/ clear");
                                                                                                                                        /*clear*/ /*_.DISCRIM__V37*/
                                                                                                                                        meltfptr[35] = 0 ;
                                                                                                                                        /*^clear*/
                                                                                                                                        /*clear*/ /*_.NAMED_NAME__V38*/
                                                                                                                                        meltfptr[37] = 0 ;
                                                                                                                                        /*^clear*/
                                                                                                                                        /*clear*/ /*_.RETURN___V39*/
                                                                                                                                        meltfptr[38] = 0 ;
                                                                                                                                        /*^clear*/
                                                                                                                                        /*clear*/ /*_.PROGN___V40*/
                                                                                                                                        meltfptr[39] = 0 ;
                                                                                                                                    }
                                                                                                                                    ;
                                                                                                                                }
                                                                                                                            ;
                                                                                                                            /*_._IFELSE___V32*/
                                                                                                                            meltfptr[31] = /*_._IFELSE___V35*/ meltfptr[34];;
                                                                                                                            /*epilog*/

                                                                                                                            MELT_LOCATION("warmelt-base.melt:3312:/ clear");
                                                                                                                            /*clear*/ /*_#IS_CLOSURE__L22*/
                                                                                                                            meltfnum[21] = 0 ;
                                                                                                                            /*^clear*/
                                                                                                                            /*clear*/ /*_._IFELSE___V35*/
                                                                                                                            meltfptr[34] = 0 ;
                                                                                                                        }
                                                                                                                        ;
                                                                                                                    }
                                                                                                                ;
                                                                                                                /*_._IFELSE___V30*/
                                                                                                                meltfptr[29] = /*_._IFELSE___V32*/ meltfptr[31];;
                                                                                                                /*epilog*/

                                                                                                                MELT_LOCATION("warmelt-base.melt:3307:/ clear");
                                                                                                                /*clear*/ /*_#IS_LIST__L21*/
                                                                                                                meltfnum[20] = 0 ;
                                                                                                                /*^clear*/
                                                                                                                /*clear*/ /*_._IFELSE___V32*/
                                                                                                                meltfptr[31] = 0 ;
                                                                                                            }
                                                                                                            ;
                                                                                                        }
                                                                                                    ;
                                                                                                    /*_._IFELSE___V29*/
                                                                                                    meltfptr[28] = /*_._IFELSE___V30*/ meltfptr[29];;
                                                                                                    /*epilog*/

                                                                                                    MELT_LOCATION("warmelt-base.melt:3305:/ clear");
                                                                                                    /*clear*/ /*_#IS_MULTIPLE__L19*/
                                                                                                    meltfnum[18] = 0 ;
                                                                                                    /*^clear*/
                                                                                                    /*clear*/ /*_._IFELSE___V30*/
                                                                                                    meltfptr[29] = 0 ;
                                                                                                }
                                                                                                ;
                                                                                            }
                                                                                        ;
                                                                                        /*_._IFELSE___V28*/
                                                                                        meltfptr[15] = /*_._IFELSE___V29*/ meltfptr[28];;
                                                                                        /*epilog*/

                                                                                        MELT_LOCATION("warmelt-base.melt:3303:/ clear");
                                                                                        /*clear*/ /*_#IS_STRING__L18*/
                                                                                        meltfnum[2] = 0 ;
                                                                                        /*^clear*/
                                                                                        /*clear*/ /*_._IFELSE___V29*/
                                                                                        meltfptr[28] = 0 ;
                                                                                    }
                                                                                    ;
                                                                                }
                                                                            ;
                                                                            /*_._IFELSE___V27*/
                                                                            meltfptr[25] = /*_._IFELSE___V28*/ meltfptr[15];;
                                                                            /*epilog*/

                                                                            MELT_LOCATION("warmelt-base.melt:3301:/ clear");
                                                                            /*clear*/ /*_#IS_INTEGERBOX__L17*/
                                                                            meltfnum[7] = 0 ;
                                                                            /*^clear*/
                                                                            /*clear*/ /*_._IFELSE___V28*/
                                                                            meltfptr[15] = 0 ;
                                                                        }
                                                                        ;
                                                                    }
                                                                ;

                                                                MELT_CHECK_SIGNAL();
                                                                ;
                                                                /*epilog*/

                                                                MELT_LOCATION("warmelt-base.melt:3286:/ clear");
                                                                /*clear*/ /*_.V__V24*/
                                                                meltfptr[16] = 0 ;
                                                                /*^clear*/
                                                                /*clear*/ /*_#IS_OBJECT__L16*/
                                                                meltfnum[1] = 0 ;
                                                                /*^clear*/
                                                                /*clear*/ /*_._IFELSE___V27*/
                                                                meltfptr[25] = 0 ;
                                                            }
                                                            ;
                                                        }
                                                    else    /*^cond.else*/
                                                        {

                                                            /*^block*/
                                                            /*anyblock*/
                                                            {


#if MELT_HAVE_DEBUG
                                                                MELT_LOCATION("warmelt-base.melt:3325:/ cppif.then");
                                                                /*^block*/
                                                                /*anyblock*/
                                                                {


                                                                    {
                                                                        /*^locexp*/
                                                                        /*melt_increment_dbgcounter*/
#if MELT_HAVE_DEBUG
                                                                        melt_dbgcounter++;
#endif
                                                                        ;
                                                                    }
                                                                    ;

                                                                    MELT_CHECK_SIGNAL();
                                                                    ;
                                                                    /*_#MELT_NEED_DBG__L23*/
                                                                    meltfnum[21] =
                                                                        /*MELT_NEED_DBG*/
#if MELT_HAVE_DEBUG
                                                                        (/*melt_need_dbg*/ melt_need_debug ((int) 0))
#else
                                                                        0 /* no melt_need_dbg */
#endif /*MELT_HAVE_DEBUG*/
                                                                        ;;
                                                                    MELT_LOCATION("warmelt-base.melt:3325:/ cond");
                                                                    /*cond*/
                                                                    if (/*_#MELT_NEED_DBG__L23*/ meltfnum[21]) /*then*/
                                                                        {
                                                                            /*^cond.then*/
                                                                            /*^block*/
                                                                            /*anyblock*/
                                                                            {

                                                                                /*_#MELT_CALLCOUNT__L24*/ meltfnum[20] =
                                                                                    /* for melt_callcount */
#if MELT_HAVE_DEBUG && defined (meltcallcount)
                                                                                    meltcallcount  /* melt_callcount debugging */
#else
                                                                                    0L  /* melt_callcount without debug */
#endif /* MELT_HAVE_DEBUG melt_callcount */
                                                                                    ;;
                                                                                /*^compute*/
                                                                                /*_.VARIADIC_CTYPE__V42*/
                                                                                meltfptr[37] =
                                                                                    /*variadic_type_code*/
#ifdef melt_variadic_index
                                                                                    (((melt_variadic_index + 0) >= 0
                                                                                      && (melt_variadic_index + 0) < melt_variadic_length)
                                                                                     ?  melt_code_to_ctype (meltxargdescr_[melt_variadic_index + 0]
                                                                                                            & MELT_ARGDESCR_MAX)
                                                                                     : NULL)
#else
                                                                                    NULL /* no variadic_ctype outside of variadic functions */
#endif /*melt_variadic_index*/
                                                                                    ;;
                                                                                /*^compute*/
                                                                                /*_#VARIADIC_INDEX__L25*/
                                                                                meltfnum[18] =
                                                                                    /*variadic_index*/
#ifdef melt_variadic_index
                                                                                    (melt_variadic_index)
#else
                                                                                    -1
#endif /*melt_variadic_index*/
                                                                                    ;;

                                                                                MELT_CHECK_SIGNAL();
                                                                                ;
                                                                                MELT_LOCATION("warmelt-base.melt:3325:/ apply");
                                                                                /*apply*/
                                                                                {
                                                                                    union meltparam_un argtab[7];
                                                                                    memset(&argtab, 0, sizeof(argtab));
                                                                                    /*^apply.arg*/
                                                                                    argtab[0].meltbp_long = /*_#MELT_CALLCOUNT__L24*/ meltfnum[20];
                                                                                    /*^apply.arg*/
                                                                                    argtab[1].meltbp_cstring =  "warmelt-base.melt";
                                                                                    /*^apply.arg*/
                                                                                    argtab[2].meltbp_long = 3325;
                                                                                    /*^apply.arg*/
                                                                                    argtab[3].meltbp_cstring =  "make_sexpr variadic_ctype:";
                                                                                    /*^apply.arg*/
                                                                                    argtab[4].meltbp_aptr = (melt_ptr_t*) &/*_.VARIADIC_CTYPE__V42*/ meltfptr[37];
                                                                                    /*^apply.arg*/
                                                                                    argtab[5].meltbp_cstring =  " at variadic_index:";
                                                                                    /*^apply.arg*/
                                                                                    argtab[6].meltbp_long = /*_#VARIADIC_INDEX__L25*/ meltfnum[18];
                                                                                    /*_.MELT_DEBUG_FUN__V43*/
                                                                                    meltfptr[38] =  melt_apply ((meltclosure_ptr_t)((/*!MELT_DEBUG_FUN*/ meltfrout->tabval[3])), (melt_ptr_t)((/*nil*/NULL)), (MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_PTR MELTBPARSTR_CSTRING MELTBPARSTR_LONG ""), argtab, "", (union meltparam_un*)0);
                                                                                }
                                                                                ;
                                                                                /*_._IF___V41*/
                                                                                meltfptr[35] = /*_.MELT_DEBUG_FUN__V43*/ meltfptr[38];;
                                                                                /*epilog*/

                                                                                MELT_LOCATION("warmelt-base.melt:3325:/ clear");
                                                                                /*clear*/ /*_#MELT_CALLCOUNT__L24*/
                                                                                meltfnum[20] = 0 ;
                                                                                /*^clear*/
                                                                                /*clear*/ /*_.VARIADIC_CTYPE__V42*/
                                                                                meltfptr[37] = 0 ;
                                                                                /*^clear*/
                                                                                /*clear*/ /*_#VARIADIC_INDEX__L25*/
                                                                                meltfnum[18] = 0 ;
                                                                                /*^clear*/
                                                                                /*clear*/ /*_.MELT_DEBUG_FUN__V43*/
                                                                                meltfptr[38] = 0 ;
                                                                            }
                                                                            ;
                                                                        }
                                                                    else    /*^cond.else*/
                                                                        {

                                                                            /*_._IF___V41*/ meltfptr[35] =  /*fromline 1341*/ NULL ;;
                                                                        }
                                                                    ;

                                                                    {
                                                                        MELT_LOCATION("warmelt-base.melt:3325:/ locexp");
                                                                        /*void*/
                                                                        (void)0;
                                                                    }
                                                                    ;
                                                                    /*^quasiblock*/


                                                                    /*epilog*/

                                                                    /*^clear*/
                                                                    /*clear*/ /*_#MELT_NEED_DBG__L23*/
                                                                    meltfnum[21] = 0 ;
                                                                    /*^clear*/
                                                                    /*clear*/ /*_._IF___V41*/
                                                                    meltfptr[35] = 0 ;
                                                                }

#else /*MELT_HAVE_DEBUG*/
                                                                /*^cppif.else*/
                                                                /*^block*/
                                                                /*anyblock*/
                                                                {


                                                                    {
                                                                        /*^locexp*/
                                                                        /*void*/(void)0;
                                                                    }
                                                                    ;
                                                                    /*epilog*/
                                                                }

#endif /*MELT_HAVE_DEBUG*/
                                                                ;
                                                                /*_.VARIADIC_CTYPE__V44*/
                                                                meltfptr[39] =
                                                                    /*variadic_type_code*/
#ifdef melt_variadic_index
                                                                    (((melt_variadic_index + 0) >= 0
                                                                      && (melt_variadic_index + 0) < melt_variadic_length)
                                                                     ?  melt_code_to_ctype (meltxargdescr_[melt_variadic_index + 0]
                                                                                            & MELT_ARGDESCR_MAX)
                                                                     : NULL)
#else
                                                                    NULL /* no variadic_ctype outside of variadic functions */
#endif /*melt_variadic_index*/
                                                                    ;;
                                                                MELT_LOCATION("warmelt-base.melt:3326:/ cond");
                                                                /*cond*/
                                                                if (
                                                                    /*ifisa*/ melt_is_instance_of((melt_ptr_t)(/*_.VARIADIC_CTYPE__V44*/ meltfptr[39]),
                                                                                                  (melt_ptr_t)((/*!CLASS_NAMED*/ meltfrout->tabval[4])))
                                                                ) /*then*/
                                                                    {
                                                                        /*^cond.then*/
                                                                        /*^getslot*/
                                                                        {
                                                                            melt_ptr_t slot=NULL, obj=NULL;
                                                                            obj = (melt_ptr_t)(/*_.VARIADIC_CTYPE__V44*/ meltfptr[39]) /*=obj*/;
                                                                            melt_object_get_field(slot,obj, 1, "NAMED_NAME");
                                                                            /*_.NAMED_NAME__V45*/
                                                                            meltfptr[34] = slot;
                                                                        };
                                                                        ;
                                                                    }
                                                                else    /*^cond.else*/
                                                                    {

                                                                        /*_.NAMED_NAME__V45*/ meltfptr[34] =  /*fromline 1341*/ NULL ;;
                                                                    }
                                                                ;

                                                                {
                                                                    MELT_LOCATION("warmelt-base.melt:3326:/ locexp");
                                                                    melt_error_str((melt_ptr_t)(/*_.VLOC__V5*/ meltfptr[4]), ( "unexpected stuff for MAKE_SEXPR"), (melt_ptr_t)(/*_.NAMED_NAME__V45*/ meltfptr[34]));
                                                                }
                                                                ;

                                                                MELT_CHECK_SIGNAL();
                                                                ;
                                                                MELT_LOCATION("warmelt-base.melt:3327:/ quasiblock");


                                                                /*_._RETVAL___V1*/
                                                                meltfptr[0] = (/*nil*/NULL);;

                                                                {
                                                                    MELT_LOCATION("warmelt-base.melt:3327:/ locexp");
                                                                    /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
                                                                    if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                                                                        melt_warn_for_no_expected_secondary_results();
                                                                    /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
                                                                    ;
                                                                }
                                                                ;
                                                                /*^finalreturn*/
                                                                ;
                                                                /*finalret*/
                                                                goto meltlabend_rout ;

                                                                MELT_CHECK_SIGNAL();
                                                                ;
                                                                /*epilog*/

                                                                MELT_LOCATION("warmelt-base.melt:3286:/ clear");
                                                                /*clear*/ /*_.VARIADIC_CTYPE__V44*/
                                                                meltfptr[39] = 0 ;
                                                                /*^clear*/
                                                                /*clear*/ /*_.NAMED_NAME__V45*/
                                                                meltfptr[34] = 0 ;
                                                                /*^clear*/
                                                                /*clear*/ /*_.RETURN___V46*/
                                                                meltfptr[31] = 0 ;
                                                            }
                                                            ;
                                                        }
                                                    ;
                                                    /*epilog*/
                                                }
                                                ;
                                            }
                                        ;
                                        /*epilog*/
                                    }
                                    ;
                                }
                            ;
                            /*epilog*/
                        }
                        ;
                    }
                ;

                MELT_CHECK_SIGNAL();
                ;
                /*epilog*/
            }
            ;
            ;
            goto meltlabloop_ARGLOOP_5;
meltlabexit_ARGLOOP_5:
            ;
            MELT_LOCATION("warmelt-base.melt:3283:/ loopepilog");
            /*loopepilog*/
            /*_.FOREVER___V14*/
            meltfptr[12] = /*_.ARGLOOP__V15*/ meltfptr[5];;
        }
        ;

#if MELT_HAVE_DEBUG
        MELT_LOCATION("warmelt-base.melt:3329:/ cppif.then");
        /*^block*/
        /*anyblock*/
        {


            {
                /*^locexp*/
                /*melt_increment_dbgcounter*/
#if MELT_HAVE_DEBUG
                melt_dbgcounter++;
#endif
                ;
            }
            ;

            MELT_CHECK_SIGNAL();
            ;
            /*_#MELT_NEED_DBG__L26*/
            meltfnum[2] =
                /*MELT_NEED_DBG*/
#if MELT_HAVE_DEBUG
                (/*melt_need_dbg*/ melt_need_debug ((int) 0))
#else
                0 /* no melt_need_dbg */
#endif /*MELT_HAVE_DEBUG*/
                ;;
            MELT_LOCATION("warmelt-base.melt:3329:/ cond");
            /*cond*/
            if (/*_#MELT_NEED_DBG__L26*/ meltfnum[2]) /*then*/
                {
                    /*^cond.then*/
                    /*^block*/
                    /*anyblock*/
                    {

                        /*_#MELT_CALLCOUNT__L27*/ meltfnum[7] =
                            /* for melt_callcount */
#if MELT_HAVE_DEBUG && defined (meltcallcount)
                            meltcallcount  /* melt_callcount debugging */
#else
                            0L  /* melt_callcount without debug */
#endif /* MELT_HAVE_DEBUG melt_callcount */
                            ;;

                        MELT_CHECK_SIGNAL();
                        ;
                        MELT_LOCATION("warmelt-base.melt:3329:/ apply");
                        /*apply*/
                        {
                            union meltparam_un argtab[5];
                            memset(&argtab, 0, sizeof(argtab));
                            /*^apply.arg*/
                            argtab[0].meltbp_long = /*_#MELT_CALLCOUNT__L27*/ meltfnum[7];
                            /*^apply.arg*/
                            argtab[1].meltbp_cstring =  "warmelt-base.melt";
                            /*^apply.arg*/
                            argtab[2].meltbp_long = 3329;
                            /*^apply.arg*/
                            argtab[3].meltbp_cstring =  "make_sexpr returns sexpr=";
                            /*^apply.arg*/
                            argtab[4].meltbp_aptr = (melt_ptr_t*) &/*_.SEXPR__V10*/ meltfptr[7];
                            /*_.MELT_DEBUG_FUN__V48*/
                            meltfptr[28] =  melt_apply ((meltclosure_ptr_t)((/*!MELT_DEBUG_FUN*/ meltfrout->tabval[3])), (melt_ptr_t)((/*nil*/NULL)), (MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_LONG MELTBPARSTR_CSTRING MELTBPARSTR_PTR ""), argtab, "", (union meltparam_un*)0);
                        }
                        ;
                        /*_._IF___V47*/
                        meltfptr[29] = /*_.MELT_DEBUG_FUN__V48*/ meltfptr[28];;
                        /*epilog*/

                        MELT_LOCATION("warmelt-base.melt:3329:/ clear");
                        /*clear*/ /*_#MELT_CALLCOUNT__L27*/
                        meltfnum[7] = 0 ;
                        /*^clear*/
                        /*clear*/ /*_.MELT_DEBUG_FUN__V48*/
                        meltfptr[28] = 0 ;
                    }
                    ;
                }
            else    /*^cond.else*/
                {

                    /*_._IF___V47*/ meltfptr[29] =  /*fromline 1341*/ NULL ;;
                }
            ;

            {
                MELT_LOCATION("warmelt-base.melt:3329:/ locexp");
                /*void*/
                (void)0;
            }
            ;
            /*^quasiblock*/


            /*epilog*/

            /*^clear*/
            /*clear*/ /*_#MELT_NEED_DBG__L26*/
            meltfnum[2] = 0 ;
            /*^clear*/
            /*clear*/ /*_._IF___V47*/
            meltfptr[29] = 0 ;
        }

#else /*MELT_HAVE_DEBUG*/
        /*^cppif.else*/
        /*^block*/
        /*anyblock*/
        {


            {
                /*^locexp*/
                /*void*/(void)0;
            }
            ;
            /*epilog*/
        }

#endif /*MELT_HAVE_DEBUG*/
        ;

        MELT_CHECK_SIGNAL();
        ;
        MELT_LOCATION("warmelt-base.melt:3330:/ quasiblock");


        /*_._RETVAL___V1*/
        meltfptr[0] = /*_.SEXPR__V10*/ meltfptr[7];;

        {
            MELT_LOCATION("warmelt-base.melt:3330:/ locexp");
            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                melt_warn_for_no_expected_secondary_results();
            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
            ;
        }
        ;
        /*^finalreturn*/
        ;
        /*finalret*/
        goto meltlabend_rout ;
        /*_.LET___V3*/
        meltfptr[2] = /*_.RETURN___V49*/ meltfptr[15];;

        MELT_LOCATION("warmelt-base.melt:3269:/ clear");
        /*clear*/ /*_.CONT__V4*/
        meltfptr[3] = 0 ;
        /*^clear*/
        /*clear*/ /*_#IS_MIXINT__L1*/
        meltfnum[0] = 0 ;
        /*^clear*/
        /*clear*/ /*_.VLOC__V5*/
        meltfptr[4] = 0 ;
        /*^clear*/
        /*clear*/ /*_.SEXPR__V10*/
        meltfptr[7] = 0 ;
        /*^clear*/
        /*clear*/ /*_.FOREVER___V14*/
        meltfptr[12] = 0 ;
        /*^clear*/
        /*clear*/ /*_.RETURN___V49*/
        meltfptr[15] = 0 ;
        MELT_LOCATION("warmelt-base.melt:3267:/ quasiblock");


        /*_._RETVAL___V1*/
        meltfptr[0] = /*_.LET___V3*/ meltfptr[2];;

        {
            MELT_LOCATION("warmelt-base.melt:3267:/ locexp");
            /*ochecknores compilobj_nrep_return*/
#if MELT_HAVE_DEBUG
            if (meltxresdescr_ && meltxresdescr_[0] && meltxrestab_)
                melt_warn_for_no_expected_secondary_results();
            /* we warned when secondary results are expected but not returned. */
#endif /*MELT_HAVE_DEBUG*/
            ;
        }
        ;
        /*^finalreturn*/
        ;
        /*finalret*/
        goto meltlabend_rout ;
        /*epilog*/

        /*^clear*/
        /*clear*/ /*_.LET___V3*/
        meltfptr[2] = 0 ;
    }

    ;
    goto meltlabend_rout;
meltlabend_rout:
    melt_trace_end("MAKE_SEXPR", meltcallcount);
    melt_blocklevel_signals = current_blocklevel_signals_meltrout_36_WARMELTmiBASE_MAKE_SEXPR_melt;
    return (melt_ptr_t)(/*_._RETVAL___V1*/ meltfptr[0]);
#undef melt_variadic_length
#undef melt_variadic_index

#undef meltcallcount
#undef meltfram__
#undef MELTFRAM_NBVARNUM
#undef MELTFRAM_NBVARPTR
} /*end meltrout_36_WARMELTmiBASE_MAKE_SEXPR*/



/**** end of warmelt-base+02.cc ****/
