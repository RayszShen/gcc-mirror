2008-06-19  Ian Lance Taylor  <iant@google.com>

	* c-lex.c (narrowest_unsigned_type): Change itk to int.
	(narrowest_signed_type): Likewise.
	* c-typeck.c (c_common_type): Change local variable mclass to enum
	mode_class, twice.
	(parser_build_binary_op): Compare the TREE_CODE_CLASS with
	tcc_comparison, not the tree code itself.
	* c-common.c (def_fn_type): Pass int, not an enum, to va_arg.
	(c_expand_expr): Cast modifier to enum expand_modifier.
	* c-common.h (C_RID_CODE): Add casts.
	(C_SET_RID_CODE): Define.
	* c-parser.c (c_parse_init): Use C_SET_RID_CODE.
	(c_lex_one_token): Add cast to avoid warning.
	(c_parser_objc_type_name): Rename local typename to type_name.
	(check_no_duplicate_clause): Change code parameter to enum
	omp_clause_code.
	(c_parser_omp_var_list_parens): Change kind parameter to enum
	omp_clause_code.
	(c_parser_omp_flush): Pass OMP_CLAUSE_ERROR, not 0, to
	c_parser_omp_list_var_parens.
	(c_parser_omp_threadprivate): Likewise.
	* c-format.c (NO_FMT): Define.
	(printf_length_specs): Use NO_FMT.
	(asm_fprintf_length_specs): Likewise.
	(gcc_diag_length_specs): Likewise.
	(scanf_length_specs): Likewise.
	(strfmon_length_specs): Likewise.
	(gcc_gfc_length_specs): Likewise.
	(printf_flag_specs): Change 0 to STD_C89.
	(asm_fprintf_flag_specs): Likewise.
	(gcc_diag_flag_specs): Likewise.
	(gcc_cxxdiag_flag_specs): Likewise.
	(scanf_flag_specs): Likewise.
	(strftime_flag_specs): Likewise.
	(strfmon_flag_specs): Likewise.
	(print_char_table): Likewise.
	(asm_fprintf_char_table): Likewise.
	(gcc_diag_char_table): Likewise.
	(gcc_tdiag_char_table): Likewise.
	(gcc_cdiag_char_table): Likewise.
	(gcc_cxxdiag_char_table): Likewise.
	(gcc_gfc_char_table): Likewise.
	(scan_char_table): Likewise.
	(time_char_table): Likewis.
	(monetary_char_table): Likewise.
	* c-format.h (BADLEN): Likewise.

	* toplev.h (progname): Declare as extern "C" when compiling with
	C++.

2008-06-18  Ian Lance Taylor  <iant@google.com>

	* configure.ac: Split c_loose_warn out from loose_warn, and
	c_strict_warn from strict_warn.  Set and substitute
	warn_cxxflags.
	* Makefile.in (C_LOOSE_WARN, C_STRICT_WARN): New variables.
	(GCC_WARN_CFLAGS): Add $(C_LOOSE_WARN) and $(C_STRICT_WARN).
	(GCC_WARN_CXXFLAGS, WARN_CXXFLAGS): New variables.
	(GCC_CFLAGS): Add $(C_LOOSE_WARN).
	(ALL_CXXFLAGS): New variable.
	(.c.o): Compile with $(CXX) rather than $(CC).
	* configure: Rebuild.

Local Variables:
mode: change-log
change-log-default-name: "ChangeLog.cxx"
End:
