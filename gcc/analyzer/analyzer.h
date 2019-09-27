/* Utility functions for the analyzer.
   Copyright (C) 2019 Free Software Foundation, Inc.
   Contributed by David Malcolm <dmalcolm@redhat.com>.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_ANALYZER_ANALYZER_H
#define GCC_ANALYZER_ANALYZER_H

/* Forward decls of common types, with indentation to show inheritance.  */

class graphviz_out;
class supergraph;
class supernode;
class superedge;
  class cfg_superedge;
    class switch_cfg_superedge;
  class callgraph_superedge;
    class call_superedge;
    class return_superedge;
class svalue;
  class region_svalue;
  class constant_svalue;
  class poisoned_svalue;
  class unknown_svalue;
  class setjmp_svalue;
class region;
  class map_region;
  class symbolic_region;
class region_model;
class region_model_context;
  class impl_region_model_context;
class constraint_manager;
class equiv_class;
struct model_merger;
struct svalue_id_merger_mapping;
struct canonicalization;
class pending_diagnostic;
class checker_path;
class extrinsic_state;
class sm_state_map;
class stmt_finder;
class program_point;
class program_state;
class exploded_graph;
class exploded_node;
class exploded_edge;
class exploded_cluster;
class exploded_path;
class analysis_plan;
class state_purge_map;
class state_purge_per_ssa_name;
class state_change;

////////////////////////////////////////////////////////////////////////////

extern bool is_named_call_p (const gcall *call, const char *funcname);
extern bool is_named_call_p (const gcall *call, const char *funcname,
			     unsigned int num_args);
extern bool is_setjmp_call_p (const gimple *stmt);
extern bool is_longjmp_call_p (const gcall *call);

extern void register_analyzer_pass ();

extern label_text make_label_text (bool can_colorize, const char *fmt, ...);

////////////////////////////////////////////////////////////////////////////

/* An RAII-style class for pushing/popping cfun within a scope.
   Doing so ensures we get "In function " announcements
   from the diagnostics subsystem.  */

class auto_cfun
{
public:
  auto_cfun (function *fun) { push_cfun (fun); }
  ~auto_cfun () { pop_cfun (); }
};

////////////////////////////////////////////////////////////////////////////

/* Begin suppressing -Wformat and -Wformat-extra-args.  */

#define PUSH_IGNORE_WFORMAT \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wformat\"") \
  _Pragma("GCC diagnostic ignored \"-Wformat-extra-args\"")

/* Finish suppressing -Wformat and -Wformat-extra-args.  */

#define POP_IGNORE_WFORMAT \
  _Pragma("GCC diagnostic pop")

////////////////////////////////////////////////////////////////////////////

/* A template for creating hash traits for a POD type.  */

template <typename Type>
struct pod_hash_traits : typed_noop_remove<Type>
{
  typedef Type value_type;
  typedef Type compare_type;
  static inline hashval_t hash (value_type);
  static inline bool equal (const value_type &existing,
			    const value_type &candidate);
  static inline void mark_deleted (Type &);
  static inline void mark_empty (Type &);
  static inline bool is_deleted (Type);
  static inline bool is_empty (Type);
};

#endif /* GCC_ANALYZER_ANALYZER_H */
