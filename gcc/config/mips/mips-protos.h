/* Prototypes of target machine for GNU compiler.  MIPS version.
   Copyright (C) 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2001, 2002 Free Software Foundation, Inc.
   Contributed by A. Lichnewsky (lich@inria.inria.fr).
   Changed by Michael Meissner	(meissner@osf.org).
   64 bit r4000 support by Ian Lance Taylor (ian@cygnus.com) and
   Brendan Eich (brendan@microunity.com).

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

#ifndef GCC_MIPS_PROTOS_H
#define GCC_MIPS_PROTOS_H

extern HOST_WIDE_INT	compute_frame_size PARAMS ((HOST_WIDE_INT));
extern int		mips_initial_elimination_offset PARAMS ((int, int));
extern void		iris6_asm_output_align PARAMS ((FILE *, unsigned));
extern const char *	current_section_name PARAMS ((void));
extern unsigned int	current_section_flags PARAMS ((void));
extern int		mips_can_use_return_insn PARAMS ((void));
extern void		mips_declare_object PARAMS ((FILE *, const char *,
						     const char *,
						     const char *, int));
extern void		mips_declare_object_name PARAMS ((FILE *, const char *,
							  tree));
extern void		mips_finish_declare_object PARAMS ((FILE *, tree,
							    int, int));
extern void		mips_output_aligned_bss
				PARAMS ((FILE *, tree, const char *,
					 unsigned HOST_WIDE_INT, int));
extern void		mips_expand_epilogue PARAMS ((int));
extern void		mips_expand_prologue PARAMS ((void));
extern void		mips_output_filename PARAMS ((FILE *, const char *));
extern void		mips_output_lineno PARAMS ((FILE *, int));
extern void		mips_output_ascii PARAMS ((FILE *, const char *,
						   size_t));
extern void		mips_order_regs_for_local_alloc PARAMS ((void));
extern struct rtx_def * embedded_pic_fnaddr_reg PARAMS ((void));
extern struct rtx_def *	mips16_gp_pseudo_reg PARAMS ((void));
#ifdef ASM_OUTPUT_UNDEF_FUNCTION
extern int		mips_output_external_libcall PARAMS ((FILE *, const char *));
#endif /* ASM_OUTPUT_UNDEF_FUNCTION */
extern struct rtx_def  *mips_function_value PARAMS ((tree, tree,
						     enum machine_mode));

extern unsigned int	mips_hard_regno_nregs PARAMS ((int,
						       enum machine_mode));
extern int              mips_return_in_memory PARAMS ((tree));

extern struct rtx_def  *function_arg PARAMS ((const CUMULATIVE_ARGS *,
					      enum machine_mode, tree, int));
extern void		function_arg_advance PARAMS ((CUMULATIVE_ARGS *,
						      enum machine_mode,
						      tree, int));
extern int		function_arg_partial_nregs
				PARAMS ((const CUMULATIVE_ARGS *,
					 enum machine_mode,
					 tree, int));
extern int		mips_setup_incoming_varargs
				PARAMS ((const CUMULATIVE_ARGS *,
					 enum machine_mode,
					 tree, int));
extern int		function_arg_pass_by_reference
				PARAMS ((const CUMULATIVE_ARGS *,
					 enum machine_mode, tree, int));
extern int		mips_output_external PARAMS ((FILE *, tree,
						      const char *));
extern tree		mips_build_va_list PARAMS ((void));
extern void		mips_va_start PARAMS ((tree, rtx));
extern struct rtx_def  *mips_va_arg PARAMS ((tree, tree));

extern bool		mips_expand_block_move PARAMS ((rtx, rtx, rtx));
extern bool		mips_expand_unaligned_load PARAMS ((rtx, rtx,
							    unsigned int,
							    int));
extern bool		mips_expand_unaligned_store PARAMS ((rtx, rtx,
							     unsigned int,
							     int));
extern void		init_cumulative_args PARAMS ((CUMULATIVE_ARGS *,
						      tree, rtx));
extern void		gen_conditional_move PARAMS ((rtx *));
extern void		mips_gen_conditional_trap PARAMS ((rtx *));
extern void		mips_emit_fcc_reload PARAMS ((rtx, rtx, rtx));
extern void		mips_set_return_address PARAMS ((rtx, rtx));
extern HOST_WIDE_INT	mips_debugger_offset PARAMS ((rtx, HOST_WIDE_INT));
extern rtx		mips_subword PARAMS ((rtx, int));
extern bool		mips_split_64bit_move_p PARAMS ((rtx, rtx));
extern void		mips_split_64bit_move PARAMS ((rtx, rtx));
extern const char      *mips_output_move PARAMS ((rtx, rtx));
extern const char      *mips_emit_prefetch PARAMS ((rtx *));
extern const char      *mips_restore_gp PARAMS ((rtx *));
extern void		override_options PARAMS ((void));
extern void		mips_conditional_register_usage PARAMS ((void));
extern void		print_operand_address PARAMS ((FILE *, rtx));
extern void		print_operand PARAMS ((FILE *, rtx, int));
extern struct rtx_def *	embedded_pic_offset PARAMS ((rtx));
extern int		build_mips16_call_stub PARAMS ((rtx, rtx, rtx, int));
extern const char	*mips_output_load_label PARAMS ((void));
extern const char       *mips_output_conditional_branch PARAMS ((rtx, rtx *,
								 int, int, int,
								 int));
extern const char	*mips_output_division PARAMS ((const char *, rtx *));
extern int              mips_adjust_insn_length PARAMS ((rtx, int));
extern enum reg_class	mips_secondary_reload_class PARAMS ((enum reg_class,
							     enum machine_mode,
							     rtx, int));
extern bool     	mips_cannot_change_mode_class
			  PARAMS ((enum machine_mode, enum machine_mode,
				   enum reg_class));
extern int              mips_class_max_nregs PARAMS ((enum reg_class,
						      enum machine_mode));
extern int              mips_register_move_cost PARAMS ((enum machine_mode,
							 enum reg_class,
							 enum reg_class));

extern int		se_arith_operand PARAMS ((rtx, enum machine_mode));
extern int		mips_address_insns PARAMS ((rtx, enum machine_mode));
extern int		mips_fetch_insns PARAMS ((rtx));
extern int		mips_const_insns PARAMS ((rtx));
extern bool		mips_global_pic_constant_p PARAMS ((rtx));
extern bool		mips_legitimate_address_p PARAMS ((enum machine_mode,
							   rtx, int));
extern bool		mips_legitimize_address PARAMS ((rtx *,
							 enum machine_mode));
extern bool		mips_legitimize_move PARAMS ((enum machine_mode,
						      rtx, rtx));
extern rtx		mips_delegitimize_address PARAMS ((rtx));
extern void		mips_expand_call PARAMS ((rtx, rtx, rtx, rtx, int));
extern int              mips_reg_mode_ok_for_base_p PARAMS ((rtx,
							     enum machine_mode,
							     int));
extern bool             mips_valid_pointer_mode PARAMS ((enum machine_mode));

extern int		m16_uimm3_b PARAMS ((rtx, enum machine_mode));
extern int		m16_simm4_1 PARAMS ((rtx, enum machine_mode));
extern int		m16_nsimm4_1 PARAMS ((rtx, enum machine_mode));
extern int		m16_simm5_1 PARAMS ((rtx, enum machine_mode));
extern int		m16_nsimm5_1 PARAMS ((rtx, enum machine_mode));
extern int		m16_uimm5_4 PARAMS ((rtx, enum machine_mode));
extern int		m16_nuimm5_4 PARAMS ((rtx, enum machine_mode));
extern int		m16_simm8_1 PARAMS ((rtx, enum machine_mode));
extern int		m16_nsimm8_1 PARAMS ((rtx, enum machine_mode));
extern int		m16_uimm8_1 PARAMS ((rtx, enum machine_mode));
extern int		m16_nuimm8_1 PARAMS ((rtx, enum machine_mode));
extern int		m16_uimm8_m1_1 PARAMS ((rtx, enum machine_mode));
extern int		m16_uimm8_4 PARAMS ((rtx, enum machine_mode));
extern int		m16_nuimm8_4 PARAMS ((rtx, enum machine_mode));
extern int		m16_simm8_8 PARAMS ((rtx, enum machine_mode));
extern int		m16_nsimm8_8 PARAMS ((rtx, enum machine_mode));
extern int		m16_usym8_4 PARAMS ((rtx, enum machine_mode));
extern int		m16_usym5_4 PARAMS ((rtx, enum machine_mode));

#ifdef RTX_CODE
extern rtx		gen_int_relational PARAMS ((enum rtx_code, rtx, rtx,
						    rtx,int *));
extern void		gen_conditional_branch PARAMS ((rtx *, enum rtx_code));
#endif
extern rtx              mips_return_addr PARAMS ((int, rtx));

#endif /* ! GCC_MIPS_PROTOS_H */
