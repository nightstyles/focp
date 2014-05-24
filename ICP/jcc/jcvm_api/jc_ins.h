
#ifndef _jc_ins_h_
#define _jc_ins_h_

#include "jcvm_api.h"

typedef enum JC_INS_ARGOPT
{
	JC_UN=0,	//Î´¶¨Òå
	JC_IS,		//³£Á¿¶ÎÑ°Ö·
	JC_DS,		//Êý¾Ý¶ÎÑ°Ö·
	JC_CS,		//´úÂë¶ÎÑ°Ö·
	JC_LS,		//¾Ö²¿¶ÎÑ°Ö·
	JC_AS,		//²ÎÊý¶ÎÑ°Ö·
	JC_TS,		//ÁÙÊ±¶ÎÑ°Ö·
	JC_SS,		//·ûºÅ±íÑ°Ö·
	JC_HS,		//Ö÷»ú¶ÎÑ°Ö·
}JC_INS_ARGOPT;

typedef union CJcVal
{
	jc_char c;
	jc_uchar uc;
	jc_short s;
	jc_ushort us;
	jc_int i;
	jc_uint ui;
	jc_long l;
	jc_ulong ul;
	jc_float f;
	jc_double d;
}CJcVal;

typedef struct CJcIns
{
	jc_ushort nOp;
	jc_uchar nOpt[2];
	jc_uint nArg[2];
}CJcIns;

typedef enum JCVM_INS_CODE // 194 instructions
{
	jc_non, 

	jc_jmp,

	jc_jmptc,
	jc_jmpts,
	jc_jmpti,
	jc_jmptl,
	jc_jmptf,
	jc_jmptd,

	jc_jmpfc,
	jc_jmpfs,
	jc_jmpfi,
	jc_jmpfl,
	jc_jmpff,
	jc_jmpfd,

	jc_jtabc,
	jc_jtabs,
	jc_jtabi,
	jc_jtabl,

	jc_newarg,
	jc_delarg,
	jc_newstk,
	jc_delstk,
	jc_call,
	jc_ret,

	jc_lea,

	jc_rep,

	jc_movc,
	jc_movs,
	jc_movi,
	jc_movl,

	jc_loadc,
	jc_loads,
	jc_loadi,
	jc_loadl,

	jc_savec,
	jc_saves,
	jc_savei,
	jc_savel,

	jc_xchgc,
	jc_xchgs,
	jc_xchgi,
	jc_xchgl,

	jc_addc,
	jc_adds,
	jc_addi,
	jc_addl,
	jc_addf,
	jc_addd,

	jc_incc,
	jc_incs,
	jc_inci,
	jc_incl,
	jc_incf,
	jc_incd,

	jc_subc,
	jc_subs,
	jc_subi,
	jc_subl,
	jc_subf,
	jc_subd,

	jc_decc,
	jc_decs,
	jc_deci,
	jc_decl,
	jc_decf,
	jc_decd,

	jc_negc,
	jc_negs,
	jc_negi,
	jc_negl,
	jc_negf,
	jc_negd,

	jc_mulc,
	jc_muls,
	jc_muli,
	jc_mull,
	jc_mulf,
	jc_muld,
	jc_muluc,
	jc_mulus,
	jc_mului,
	jc_mulul,

	jc_divc,
	jc_divs,
	jc_divi,
	jc_divl,
	jc_divf,
	jc_divd,
	jc_divuc,
	jc_divus,
	jc_divui,
	jc_divul,

	jc_modc,
	jc_mods,
	jc_modi,
	jc_modl,
	jc_moduc,
	jc_modus,
	jc_modui,
	jc_modul,

	jc_ltc,
	jc_lts,
	jc_lti,
	jc_ltl,
	jc_ltf,
	jc_ltd,
	jc_ltuc,
	jc_ltus,
	jc_ltui,
	jc_ltul,

	jc_lec,
	jc_les,
	jc_lei,
	jc_lel,
	jc_lef,
	jc_led,
	jc_leuc,
	jc_leus,
	jc_leui,
	jc_leul,

	jc_eqc,
	jc_eqs,
	jc_eqi,
	jc_eql,
	jc_eqf,
	jc_eqd,

	jc_nec,
	jc_nes,
	jc_nei,
	jc_nel,
	jc_nef,
	jc_ned,

	jc_gec,
	jc_ges,
	jc_gei,
	jc_gel,
	jc_gef,
	jc_ged,
	jc_geuc,
	jc_geus,
	jc_geui,
	jc_geul,

	jc_gtc,
	jc_gts,
	jc_gti,
	jc_gtl,
	jc_gtf,
	jc_gtd,
	jc_gtuc,
	jc_gtus,
	jc_gtui,
	jc_gtul,

	jc_c2s,
	jc_s2i,
	jc_i2l,
	jc_l2i,
	jc_i2s,
	jc_s2c,
	jc_uc2us,
	jc_us2ui,
	jc_ui2ul,
	jc_ul2ui,
	jc_ui2us,
	jc_us2uc,
	jc_f2d,
	jc_d2f,
	jc_f2i,
	jc_i2f,
	jc_d2l,
	jc_l2d,

	jc_lshuc,
	jc_lshus,
	jc_lshui,
	jc_lshul,
	jc_rshc,
	jc_rshs,
	jc_rshi,
	jc_rshl,
	jc_rshuc,
	jc_rshus,
	jc_rshui,
	jc_rshul,

	jc_anduc,
	jc_andus,
	jc_andui,
	jc_andul,

	jc_oruc,
	jc_orus,
	jc_orui,
	jc_orul,

	jc_notuc,
	jc_notus,
	jc_notui,
	jc_notul,

	jc_xoruc,
	jc_xorus,
	jc_xorui,
	jc_xorul
}JCVM_INS_CODE;

#endif
