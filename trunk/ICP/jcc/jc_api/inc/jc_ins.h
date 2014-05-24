
#ifndef _jc_ins_h_
#define _jc_ins_h_

#include "jc_type.h"

///////////////////////////////////////////////////////////////////////////////
// 指令操作数寻址方式定义:
//		JC系统，没有传统的栈，而是把栈分解成了固定长度的局部变量段和参数段。
//	参数段，在构造参数时叫临时段。JC系统支持外部C动态库提供的符号，这种寻址方式
//  称为主机段寻址(JC_HS)。JC_UN表示未定义寻址方式，和JC_HS一样仅用于符号表中符
//  号的属性描述，以说明该符号的寻址方式。未定义说明在连接时才能确定。
///////////////////////////////////////////////////////////////////////////////
typedef enum JC_INS_ARGOPT
{
	JC_UN, //未定义寻址方式，不会出现在指令中，只会出现在符号表的符号属性中。
	JC_IS, //常量段寻址【立即数】
	JC_DS, //数据段寻址
	JC_CS, //代码段寻址
	JC_LS, //局部段寻址
	JC_AS, //参数段寻址
	JC_TS, //临时段寻址，用于构造函数参数的段
	JC_SS, //符号段寻址
	JC_HS, //主机段寻址，不会出现在指令中，只会出现在符号表的符号属性中。
}JC_INS_ARGOPT;

///////////////////////////////////////////////////////////////////////////////
// 统一的基础数据：
//	注意，jc_long是64位的。和主机C语言不一定一直。
///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
// 统一的指令结构
//	注意，最多支持双操作数，未用的操作数的寻址方式被设置为JC_UN，即未定义形式。
//	nArg，操作数是可选的字段，根据具体指令而确定。
///////////////////////////////////////////////////////////////////////////////
typedef struct CJcIns
{
	jc_ushort nOp;
	jc_uchar nOpt[2];
	jc_uint nArg[2];
}CJcIns;

///////////////////////////////////////////////////////////////////////////////
// 指令操作码定义
///////////////////////////////////////////////////////////////////////////////
typedef enum JCVM_INS_CODE
{
//空操作指令【无操作数】
	jc_non, 

//直接跳转指令【单操作数】，直接跳转到第一个操作数的地址
	jc_jmp, // pc = cs + arg[0]

//真条件跳转指令【双操作数】，如果第一个操作数的值为真，则跳转到第二个操作数的
//地址【应该在代码段】。否则前转下一个指令。双操作数
	jc_jmptc, 
	jc_jmpts,
	jc_jmpti,
	jc_jmptl,
	jc_jmptf,
	jc_jmptd,

//假条件跳转指令【双操作数】，如果第一个操作数的值为假，则跳转到第二个操作数的
//地址【应该在代码段】。否则前转下一个指令。双操作数
	jc_jmpfc,
	jc_jmpfs,
	jc_jmpfi,
	jc_jmpfl,
	jc_jmpff,
	jc_jmpfd,

//表跳转指令【双操作数】，第一个操作数指定值；第二个操作数指向跳转表。根据值的
//匹配情况跳转到相应表项所指定的地址。跳转表的结构：
//  <表项个数n> <第一个匹配项偏移> {跳转地址} ... {匹配项}
//	其中第一个<匹配项>需要按自然边界对齐，所以，所以需要在表头指明偏移。
//	<第一个匹配项>不做匹配，是冗余的，对应第一个跳转地址【缺省跳转地址】。
//  <匹配项>按从小到大排列，匹配时采用二分法。
	jc_jtabc,
	jc_jtabs,
	jc_jtabi,
	jc_jtabl,

//函数调用指令集
	jc_newarg, //新建参数段指令【单操作数】，根据第一个常量操作数，分配临时段
	jc_delarg, //删除参数段指令【无操作数】，释放临时段内存。
	jc_newstk, //新建局部段指令【单操作数】，根据第一个常量操作数，分配局部段
	jc_delstk, //删除局部段指令【无操作数】，释放局部段内存。
	jc_call,   //函数调用指令【单操作数】，根据第一个操作数，获取函数地址，进行调用。
			   //这里的函数可以是主机函数，也可以是JC函数。
	jc_ret,	   //函数返回指令【无操作数】，

// 取地址指令【双操作数】
	jc_lea,	//取出第二个操作数的地址给第一个操作数的值。

//循环指令【单操作数】，用于mov、load、save三类内存存取操作。
	jc_rep, //取出第一个常量操作数，作为循环次数。

//内存传送指令【双操作数】，memcpy(arg[0], arg[1], rep?rep:1);
	jc_movc,
	jc_movs,
	jc_movi,
	jc_movl,

//内存传送指令【双操作数】，memcpy(arg[0], *arg[1], rep?rep:1);
	jc_loadc,
	jc_loads,
	jc_loadi,
	jc_loadl,

//内存传送指令【双操作数】，memcpy(*arg[0], arg[1], rep?rep:1);
	jc_savec,
	jc_saves,
	jc_savei,
	jc_savel,

//内存交换指令【双操作数】，memswap(arg[0], arg[1], rep?rep:1);在JC编译中并不使用该指令，
	jc_xchgc,
	jc_xchgs,
	jc_xchgi,
	jc_xchgl,

//加法指令【双操作数】，*arg[0] += *arg[1]
	jc_addc,
	jc_adds,
	jc_addi,
	jc_addl,
	jc_addf,
	jc_addd,

//递增指令【单操作数】，*arg[0] += *arg[0]
	jc_incc,
	jc_incs,
	jc_inci,
	jc_incl,
	jc_incf,
	jc_incd,

//减法指令【双操作数】，*arg[0] -= *arg[1]
	jc_subc,
	jc_subs,
	jc_subi,
	jc_subl,
	jc_subf,
	jc_subd,

//递减指令【单操作数】，*arg[0] -= *arg[0]
	jc_decc,
	jc_decs,
	jc_deci,
	jc_decl,
	jc_decf,
	jc_decd,

//取负指令【单操作数】，*arg[0] = -(*arg[0])
	jc_negc,
	jc_negs,
	jc_negi,
	jc_negl,
	jc_negf,
	jc_negd,

//乘法指令【双操作数】，*arg[0] *= *arg[1]
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

//除法指令【双操作数】，*arg[0] /= *arg[1]
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

//取模指令【双操作数】，*arg[0] %= *arg[1]
	jc_modc,
	jc_mods,
	jc_modi,
	jc_modl,
	jc_moduc,
	jc_modus,
	jc_modui,
	jc_modul,

//小于比较指令【双操作数】，*arg[0] = (*arg[1]) < 0
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

//小于等于比较指令【双操作数】，*arg[0] = (*arg[1]) <= 0
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

//等于比较指令【双操作数】，*arg[0] = (*arg[1]) == 0
	jc_eqc,
	jc_eqs,
	jc_eqi,
	jc_eql,
	jc_eqf,
	jc_eqd,

//不等于比较指令【双操作数】，*arg[0] = (*arg[1]) != 0
	jc_nec,
	jc_nes,
	jc_nei,
	jc_nel,
	jc_nef,
	jc_ned,

//大于等于比较指令【双操作数】，*arg[0] = (*arg[1]) >= 0
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

//大于比较指令【双操作数】，*arg[0] = (*arg[1]) > 0
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

//转换指令【双操作数】, *arg[0] = (type of arg[0])(*argv[1])
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

//移位指令【双操作数】, *arg[0] 移位操作 (0x000000FF&(*arg[0]));左移位不区分是
//否有符号，右移位区分是否有符号
	jc_lshuc, //操作是: <<=
	jc_lshus, //操作是: <<=
	jc_lshui, //操作是: <<=
	jc_lshul, //操作是: <<=
	jc_rshc,  //操作是: >>=
	jc_rshs,  //操作是: >>=
	jc_rshi,  //操作是: >>=
	jc_rshl,  //操作是: >>=
	jc_rshuc, //操作是: >>=
	jc_rshus, //操作是: >>=
	jc_rshui, //操作是: >>=
	jc_rshul, //操作是: >>=

//位与指令【双操作数】, *arg[0] &= *arg[1]
	jc_anduc,
	jc_andus,
	jc_andui,
	jc_andul,

//位或指令【双操作数】, *arg[0] |= *arg[1]
	jc_oruc,
	jc_orus,
	jc_orui,
	jc_orul,

//位取反指令【单操作数】, *arg[0] = ~*arg[0]
	jc_notuc,
	jc_notus,
	jc_notui,
	jc_notul,

//位异或指令【双操作数】, *arg[0] ^= *arg[1]
	jc_xoruc,
	jc_xorus,
	jc_xorui,
	jc_xorul
}JCVM_INS_CODE;

#endif
