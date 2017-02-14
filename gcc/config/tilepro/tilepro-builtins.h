/* Enum for builtin intrinsics for TILEPro.
   Copyright (C) 2011-2017 Free Software Foundation, Inc.
   Contributed by Walter Lee (walt@tilera.com)

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef GCC_TILEPRO_BUILTINS_H
#define GCC_TILEPRO_BUILTINS_H

enum tilepro_builtin
{
  TILEPRO_INSN_ADD,
  TILEPRO_INSN_ADDB,
  TILEPRO_INSN_ADDBS_U,
  TILEPRO_INSN_ADDH,
  TILEPRO_INSN_ADDHS,
  TILEPRO_INSN_ADDIB,
  TILEPRO_INSN_ADDIH,
  TILEPRO_INSN_ADDLIS,
  TILEPRO_INSN_ADDS,
  TILEPRO_INSN_ADIFFB_U,
  TILEPRO_INSN_ADIFFH,
  TILEPRO_INSN_AND,
  TILEPRO_INSN_AULI,
  TILEPRO_INSN_AVGB_U,
  TILEPRO_INSN_AVGH,
  TILEPRO_INSN_BITX,
  TILEPRO_INSN_BYTEX,
  TILEPRO_INSN_CLZ,
  TILEPRO_INSN_CRC32_32,
  TILEPRO_INSN_CRC32_8,
  TILEPRO_INSN_CTZ,
  TILEPRO_INSN_DRAIN,
  TILEPRO_INSN_DTLBPR,
  TILEPRO_INSN_DWORD_ALIGN,
  TILEPRO_INSN_FINV,
  TILEPRO_INSN_FLUSH,
  TILEPRO_INSN_FNOP,
  TILEPRO_INSN_ICOH,
  TILEPRO_INSN_ILL,
  TILEPRO_INSN_INFO,
  TILEPRO_INSN_INFOL,
  TILEPRO_INSN_INTHB,
  TILEPRO_INSN_INTHH,
  TILEPRO_INSN_INTLB,
  TILEPRO_INSN_INTLH,
  TILEPRO_INSN_INV,
  TILEPRO_INSN_LB,
  TILEPRO_INSN_LB_U,
  TILEPRO_INSN_LH,
  TILEPRO_INSN_LH_U,
  TILEPRO_INSN_LNK,
  TILEPRO_INSN_LW,
  TILEPRO_INSN_LW_NA,
  TILEPRO_INSN_LB_L2,
  TILEPRO_INSN_LB_U_L2,
  TILEPRO_INSN_LH_L2,
  TILEPRO_INSN_LH_U_L2,
  TILEPRO_INSN_LW_L2,
  TILEPRO_INSN_LW_NA_L2,
  TILEPRO_INSN_LB_MISS,
  TILEPRO_INSN_LB_U_MISS,
  TILEPRO_INSN_LH_MISS,
  TILEPRO_INSN_LH_U_MISS,
  TILEPRO_INSN_LW_MISS,
  TILEPRO_INSN_LW_NA_MISS,
  TILEPRO_INSN_MAXB_U,
  TILEPRO_INSN_MAXH,
  TILEPRO_INSN_MAXIB_U,
  TILEPRO_INSN_MAXIH,
  TILEPRO_INSN_MF,
  TILEPRO_INSN_MFSPR,
  TILEPRO_INSN_MINB_U,
  TILEPRO_INSN_MINH,
  TILEPRO_INSN_MINIB_U,
  TILEPRO_INSN_MINIH,
  TILEPRO_INSN_MM,
  TILEPRO_INSN_MNZ,
  TILEPRO_INSN_MNZB,
  TILEPRO_INSN_MNZH,
  TILEPRO_INSN_MOVE,
  TILEPRO_INSN_MOVELIS,
  TILEPRO_INSN_MTSPR,
  TILEPRO_INSN_MULHH_SS,
  TILEPRO_INSN_MULHH_SU,
  TILEPRO_INSN_MULHH_UU,
  TILEPRO_INSN_MULHHA_SS,
  TILEPRO_INSN_MULHHA_SU,
  TILEPRO_INSN_MULHHA_UU,
  TILEPRO_INSN_MULHHSA_UU,
  TILEPRO_INSN_MULHL_SS,
  TILEPRO_INSN_MULHL_SU,
  TILEPRO_INSN_MULHL_US,
  TILEPRO_INSN_MULHL_UU,
  TILEPRO_INSN_MULHLA_SS,
  TILEPRO_INSN_MULHLA_SU,
  TILEPRO_INSN_MULHLA_US,
  TILEPRO_INSN_MULHLA_UU,
  TILEPRO_INSN_MULHLSA_UU,
  TILEPRO_INSN_MULLL_SS,
  TILEPRO_INSN_MULLL_SU,
  TILEPRO_INSN_MULLL_UU,
  TILEPRO_INSN_MULLLA_SS,
  TILEPRO_INSN_MULLLA_SU,
  TILEPRO_INSN_MULLLA_UU,
  TILEPRO_INSN_MULLLSA_UU,
  TILEPRO_INSN_MVNZ,
  TILEPRO_INSN_MVZ,
  TILEPRO_INSN_MZ,
  TILEPRO_INSN_MZB,
  TILEPRO_INSN_MZH,
  TILEPRO_INSN_NAP,
  TILEPRO_INSN_NOP,
  TILEPRO_INSN_NOR,
  TILEPRO_INSN_OR,
  TILEPRO_INSN_PACKBS_U,
  TILEPRO_INSN_PACKHB,
  TILEPRO_INSN_PACKHS,
  TILEPRO_INSN_PACKLB,
  TILEPRO_INSN_PCNT,
  TILEPRO_INSN_PREFETCH,
  TILEPRO_INSN_PREFETCH_L1,
  TILEPRO_INSN_RL,
  TILEPRO_INSN_S1A,
  TILEPRO_INSN_S2A,
  TILEPRO_INSN_S3A,
  TILEPRO_INSN_SADAB_U,
  TILEPRO_INSN_SADAH,
  TILEPRO_INSN_SADAH_U,
  TILEPRO_INSN_SADB_U,
  TILEPRO_INSN_SADH,
  TILEPRO_INSN_SADH_U,
  TILEPRO_INSN_SB,
  TILEPRO_INSN_SEQ,
  TILEPRO_INSN_SEQB,
  TILEPRO_INSN_SEQH,
  TILEPRO_INSN_SEQIB,
  TILEPRO_INSN_SEQIH,
  TILEPRO_INSN_SH,
  TILEPRO_INSN_SHL,
  TILEPRO_INSN_SHLB,
  TILEPRO_INSN_SHLH,
  TILEPRO_INSN_SHLIB,
  TILEPRO_INSN_SHLIH,
  TILEPRO_INSN_SHR,
  TILEPRO_INSN_SHRB,
  TILEPRO_INSN_SHRH,
  TILEPRO_INSN_SHRIB,
  TILEPRO_INSN_SHRIH,
  TILEPRO_INSN_SLT,
  TILEPRO_INSN_SLT_U,
  TILEPRO_INSN_SLTB,
  TILEPRO_INSN_SLTB_U,
  TILEPRO_INSN_SLTE,
  TILEPRO_INSN_SLTE_U,
  TILEPRO_INSN_SLTEB,
  TILEPRO_INSN_SLTEB_U,
  TILEPRO_INSN_SLTEH,
  TILEPRO_INSN_SLTEH_U,
  TILEPRO_INSN_SLTH,
  TILEPRO_INSN_SLTH_U,
  TILEPRO_INSN_SLTIB,
  TILEPRO_INSN_SLTIB_U,
  TILEPRO_INSN_SLTIH,
  TILEPRO_INSN_SLTIH_U,
  TILEPRO_INSN_SNE,
  TILEPRO_INSN_SNEB,
  TILEPRO_INSN_SNEH,
  TILEPRO_INSN_SRA,
  TILEPRO_INSN_SRAB,
  TILEPRO_INSN_SRAH,
  TILEPRO_INSN_SRAIB,
  TILEPRO_INSN_SRAIH,
  TILEPRO_INSN_SUB,
  TILEPRO_INSN_SUBB,
  TILEPRO_INSN_SUBBS_U,
  TILEPRO_INSN_SUBH,
  TILEPRO_INSN_SUBHS,
  TILEPRO_INSN_SUBS,
  TILEPRO_INSN_SW,
  TILEPRO_INSN_TBLIDXB0,
  TILEPRO_INSN_TBLIDXB1,
  TILEPRO_INSN_TBLIDXB2,
  TILEPRO_INSN_TBLIDXB3,
  TILEPRO_INSN_TNS,
  TILEPRO_INSN_WH64,
  TILEPRO_INSN_XOR,
  TILEPRO_NETWORK_BARRIER,
  TILEPRO_IDN0_RECEIVE,
  TILEPRO_IDN1_RECEIVE,
  TILEPRO_IDN_SEND,
  TILEPRO_SN_RECEIVE,
  TILEPRO_SN_SEND,
  TILEPRO_UDN0_RECEIVE,
  TILEPRO_UDN1_RECEIVE,
  TILEPRO_UDN2_RECEIVE,
  TILEPRO_UDN3_RECEIVE,
  TILEPRO_UDN_SEND,
  TILEPRO_BUILTIN_max
};

#endif /* !GCC_TILEPRO_BUILTINS_H */
