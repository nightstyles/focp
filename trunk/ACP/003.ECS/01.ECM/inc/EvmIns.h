
#ifndef _EVM_INS_H_
#define _EVM_INS_H_

#ifdef __cplusplus
extern "C"{
#endif

/*
	ehc_void, ehc_char,ehc_uchar,..., ehc_double需要外部定义
*/

typedef enum EHC_INS_ARGOPT
{
	EHC_UN=0,	//未定义
	EHC_IS,		//常量段寻址
	EHC_DS,		//数据段寻址
	EHC_CS,		//代码段寻址
	EHC_LS,		//局部段寻址
	EHC_AS,		//参数段寻址
	EHC_TS,		//临时段寻址
	EHC_SS,		//符号表寻址
	EHC_HS,		//主机段寻址
}EHC_INS_ARGOPT;

typedef union CEhcVal
{
	ehc_char c;
	ehc_uchar uc;
	ehc_short s;
	ehc_ushort us;
	ehc_int i;
	ehc_uint ui;
	ehc_long l;
	ehc_ulong ul;
	ehc_float f;
	ehc_double d;
}CEhcVal;

typedef struct CEhcIns
{
	ehc_ushort nOp;
	ehc_uchar nOpt[2];
	ehc_uint nArg[2];
}CEhcIns;

typedef enum EVM_INS_CODE // 194 instructions
{
	ehc_non, 
	ehc_jmp,

	ehc_jmptc,
	ehc_jmpts,
	ehc_jmpti,
	ehc_jmptl,
	ehc_jmptf,
	ehc_jmptd,

	ehc_jmpfc,
	ehc_jmpfs,
	ehc_jmpfi,
	ehc_jmpfl,
	ehc_jmpff,
	ehc_jmpfd,

	ehc_jtabc,
	ehc_jtabs,
	ehc_jtabi,
	ehc_jtabl,

	ehc_newarg,
	ehc_delarg,
	ehc_newstk,
	ehc_delstk,
	ehc_call,
	ehc_ret,

	ehc_lea,

	ehc_rep,

	ehc_movc,
	ehc_movs,
	ehc_movi,
	ehc_movl,

	ehc_loadc,
	ehc_loads,
	ehc_loadi,
	ehc_loadl,

	ehc_savec,
	ehc_saves,
	ehc_savei,
	ehc_savel,

	ehc_xchgc,
	ehc_xchgs,
	ehc_xchgi,
	ehc_xchgl,

	ehc_addc,
	ehc_adds,
	ehc_addi,
	ehc_addl,
	ehc_addf,
	ehc_addd,

	ehc_incc,
	ehc_incs,
	ehc_inci,
	ehc_incl,
	ehc_incf,
	ehc_incd,

	ehc_subc,
	ehc_subs,
	ehc_subi,
	ehc_subl,
	ehc_subf,
	ehc_subd,

	ehc_decc,
	ehc_decs,
	ehc_deci,
	ehc_decl,
	ehc_decf,
	ehc_decd,

	ehc_negc,
	ehc_negs,
	ehc_negi,
	ehc_negl,
	ehc_negf,
	ehc_negd,

	ehc_mulc,
	ehc_muls,
	ehc_muli,
	ehc_mull,
	ehc_mulf,
	ehc_muld,
	ehc_muluc,
	ehc_mulus,
	ehc_mului,
	ehc_mulul,

	ehc_divc,
	ehc_divs,
	ehc_divi,
	ehc_divl,
	ehc_divf,
	ehc_divd,
	ehc_divuc,
	ehc_divus,
	ehc_divui,
	ehc_divul,

	ehc_modc,
	ehc_mods,
	ehc_modi,
	ehc_modl,
	ehc_moduc,
	ehc_modus,
	ehc_modui,
	ehc_modul,

	ehc_ltc,
	ehc_lts,
	ehc_lti,
	ehc_ltl,
	ehc_ltf,
	ehc_ltd,
	ehc_ltuc,
	ehc_ltus,
	ehc_ltui,
	ehc_ltul,

	ehc_lec,
	ehc_les,
	ehc_lei,
	ehc_lel,
	ehc_lef,
	ehc_led,
	ehc_leuc,
	ehc_leus,
	ehc_leui,
	ehc_leul,

	ehc_eqc,
	ehc_eqs,
	ehc_eqi,
	ehc_eql,
	ehc_eqf,
	ehc_eqd,

	ehc_nec,
	ehc_nes,
	ehc_nei,
	ehc_nel,
	ehc_nef,
	ehc_ned,

	ehc_gec,
	ehc_ges,
	ehc_gei,
	ehc_gel,
	ehc_gef,
	ehc_ged,
	ehc_geuc,
	ehc_geus,
	ehc_geui,
	ehc_geul,

	ehc_gtc,
	ehc_gts,
	ehc_gti,
	ehc_gtl,
	ehc_gtf,
	ehc_gtd,
	ehc_gtuc,
	ehc_gtus,
	ehc_gtui,
	ehc_gtul,

	ehc_c2s,
	ehc_s2i,
	ehc_i2l,
	ehc_l2i,
	ehc_i2s,
	ehc_s2c,
	ehc_uc2us,
	ehc_us2ui,
	ehc_ui2ul,
	ehc_ul2ui,
	ehc_ui2us,
	ehc_us2uc,
	ehc_f2d,
	ehc_d2f,
	ehc_f2i,
	ehc_i2f,
	ehc_ui2f,
	ehc_f2ui,
	ehc_d2l,
	ehc_l2d,
	ehc_ul2d,
	ehc_d2ul,

	ehc_lshuc,
	ehc_lshus,
	ehc_lshui,
	ehc_lshul,
	ehc_rshc,
	ehc_rshs,
	ehc_rshi,
	ehc_rshl,
	ehc_rshuc,
	ehc_rshus,
	ehc_rshui,
	ehc_rshul,

	ehc_anduc,
	ehc_andus,
	ehc_andui,
	ehc_andul,

	ehc_oruc,
	ehc_orus,
	ehc_orui,
	ehc_orul,

	ehc_notuc,
	ehc_notus,
	ehc_notui,
	ehc_notul,

	ehc_xoruc,
	ehc_xorus,
	ehc_xorui,
	ehc_xorul
}EVM_INS_CODE;

#ifdef __cplusplus
}
#endif

#endif
