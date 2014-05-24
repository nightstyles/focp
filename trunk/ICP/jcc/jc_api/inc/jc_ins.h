
#ifndef _jc_ins_h_
#define _jc_ins_h_

#include "jc_type.h"

///////////////////////////////////////////////////////////////////////////////
// ָ�������Ѱַ��ʽ����:
//		JCϵͳ��û�д�ͳ��ջ�����ǰ�ջ�ֽ���˹̶����ȵľֲ������κͲ����Ρ�
//	�����Σ��ڹ������ʱ����ʱ�Ρ�JCϵͳ֧���ⲿC��̬���ṩ�ķ��ţ�����Ѱַ��ʽ
//  ��Ϊ������Ѱַ(JC_HS)��JC_UN��ʾδ����Ѱַ��ʽ����JC_HSһ�������ڷ��ű��з�
//  �ŵ�������������˵���÷��ŵ�Ѱַ��ʽ��δ����˵��������ʱ����ȷ����
///////////////////////////////////////////////////////////////////////////////
typedef enum JC_INS_ARGOPT
{
	JC_UN, //δ����Ѱַ��ʽ�����������ָ���У�ֻ������ڷ��ű�ķ��������С�
	JC_IS, //������Ѱַ����������
	JC_DS, //���ݶ�Ѱַ
	JC_CS, //�����Ѱַ
	JC_LS, //�ֲ���Ѱַ
	JC_AS, //������Ѱַ
	JC_TS, //��ʱ��Ѱַ�����ڹ��캯�������Ķ�
	JC_SS, //���Ŷ�Ѱַ
	JC_HS, //������Ѱַ�����������ָ���У�ֻ������ڷ��ű�ķ��������С�
}JC_INS_ARGOPT;

///////////////////////////////////////////////////////////////////////////////
// ͳһ�Ļ������ݣ�
//	ע�⣬jc_long��64λ�ġ�������C���Բ�һ��һֱ��
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
// ͳһ��ָ��ṹ
//	ע�⣬���֧��˫��������δ�õĲ�������Ѱַ��ʽ������ΪJC_UN����δ������ʽ��
//	nArg���������ǿ�ѡ���ֶΣ����ݾ���ָ���ȷ����
///////////////////////////////////////////////////////////////////////////////
typedef struct CJcIns
{
	jc_ushort nOp;
	jc_uchar nOpt[2];
	jc_uint nArg[2];
}CJcIns;

///////////////////////////////////////////////////////////////////////////////
// ָ������붨��
///////////////////////////////////////////////////////////////////////////////
typedef enum JCVM_INS_CODE
{
//�ղ���ָ��޲�������
	jc_non, 

//ֱ����תָ�������������ֱ����ת����һ���������ĵ�ַ
	jc_jmp, // pc = cs + arg[0]

//��������תָ�˫���������������һ����������ֵΪ�棬����ת���ڶ�����������
//��ַ��Ӧ���ڴ���Ρ�������ǰת��һ��ָ�˫������
	jc_jmptc, 
	jc_jmpts,
	jc_jmpti,
	jc_jmptl,
	jc_jmptf,
	jc_jmptd,

//��������תָ�˫���������������һ����������ֵΪ�٣�����ת���ڶ�����������
//��ַ��Ӧ���ڴ���Ρ�������ǰת��һ��ָ�˫������
	jc_jmpfc,
	jc_jmpfs,
	jc_jmpfi,
	jc_jmpfl,
	jc_jmpff,
	jc_jmpfd,

//����תָ�˫������������һ��������ָ��ֵ���ڶ���������ָ����ת������ֵ��
//ƥ�������ת����Ӧ������ָ���ĵ�ַ����ת��Ľṹ��
//  <�������n> <��һ��ƥ����ƫ��> {��ת��ַ} ... {ƥ����}
//	���е�һ��<ƥ����>��Ҫ����Ȼ�߽���룬���ԣ�������Ҫ�ڱ�ͷָ��ƫ�ơ�
//	<��һ��ƥ����>����ƥ�䣬������ģ���Ӧ��һ����ת��ַ��ȱʡ��ת��ַ����
//  <ƥ����>����С�������У�ƥ��ʱ���ö��ַ���
	jc_jtabc,
	jc_jtabs,
	jc_jtabi,
	jc_jtabl,

//��������ָ�
	jc_newarg, //�½�������ָ����������������ݵ�һ��������������������ʱ��
	jc_delarg, //ɾ��������ָ��޲����������ͷ���ʱ���ڴ档
	jc_newstk, //�½��ֲ���ָ����������������ݵ�һ������������������ֲ���
	jc_delstk, //ɾ���ֲ���ָ��޲����������ͷžֲ����ڴ档
	jc_call,   //��������ָ����������������ݵ�һ������������ȡ������ַ�����е��á�
			   //����ĺ�������������������Ҳ������JC������
	jc_ret,	   //��������ָ��޲���������

// ȡ��ַָ�˫��������
	jc_lea,	//ȡ���ڶ����������ĵ�ַ����һ����������ֵ��

//ѭ��ָ�����������������mov��load��save�����ڴ��ȡ������
	jc_rep, //ȡ����һ����������������Ϊѭ��������

//�ڴ洫��ָ�˫����������memcpy(arg[0], arg[1], rep?rep:1);
	jc_movc,
	jc_movs,
	jc_movi,
	jc_movl,

//�ڴ洫��ָ�˫����������memcpy(arg[0], *arg[1], rep?rep:1);
	jc_loadc,
	jc_loads,
	jc_loadi,
	jc_loadl,

//�ڴ洫��ָ�˫����������memcpy(*arg[0], arg[1], rep?rep:1);
	jc_savec,
	jc_saves,
	jc_savei,
	jc_savel,

//�ڴ潻��ָ�˫����������memswap(arg[0], arg[1], rep?rep:1);��JC�����в���ʹ�ø�ָ�
	jc_xchgc,
	jc_xchgs,
	jc_xchgi,
	jc_xchgl,

//�ӷ�ָ�˫����������*arg[0] += *arg[1]
	jc_addc,
	jc_adds,
	jc_addi,
	jc_addl,
	jc_addf,
	jc_addd,

//����ָ�������������*arg[0] += *arg[0]
	jc_incc,
	jc_incs,
	jc_inci,
	jc_incl,
	jc_incf,
	jc_incd,

//����ָ�˫����������*arg[0] -= *arg[1]
	jc_subc,
	jc_subs,
	jc_subi,
	jc_subl,
	jc_subf,
	jc_subd,

//�ݼ�ָ�������������*arg[0] -= *arg[0]
	jc_decc,
	jc_decs,
	jc_deci,
	jc_decl,
	jc_decf,
	jc_decd,

//ȡ��ָ�������������*arg[0] = -(*arg[0])
	jc_negc,
	jc_negs,
	jc_negi,
	jc_negl,
	jc_negf,
	jc_negd,

//�˷�ָ�˫����������*arg[0] *= *arg[1]
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

//����ָ�˫����������*arg[0] /= *arg[1]
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

//ȡģָ�˫����������*arg[0] %= *arg[1]
	jc_modc,
	jc_mods,
	jc_modi,
	jc_modl,
	jc_moduc,
	jc_modus,
	jc_modui,
	jc_modul,

//С�ڱȽ�ָ�˫����������*arg[0] = (*arg[1]) < 0
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

//С�ڵ��ڱȽ�ָ�˫����������*arg[0] = (*arg[1]) <= 0
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

//���ڱȽ�ָ�˫����������*arg[0] = (*arg[1]) == 0
	jc_eqc,
	jc_eqs,
	jc_eqi,
	jc_eql,
	jc_eqf,
	jc_eqd,

//�����ڱȽ�ָ�˫����������*arg[0] = (*arg[1]) != 0
	jc_nec,
	jc_nes,
	jc_nei,
	jc_nel,
	jc_nef,
	jc_ned,

//���ڵ��ڱȽ�ָ�˫����������*arg[0] = (*arg[1]) >= 0
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

//���ڱȽ�ָ�˫����������*arg[0] = (*arg[1]) > 0
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

//ת��ָ�˫��������, *arg[0] = (type of arg[0])(*argv[1])
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

//��λָ�˫��������, *arg[0] ��λ���� (0x000000FF&(*arg[0]));����λ��������
//���з��ţ�����λ�����Ƿ��з���
	jc_lshuc, //������: <<=
	jc_lshus, //������: <<=
	jc_lshui, //������: <<=
	jc_lshul, //������: <<=
	jc_rshc,  //������: >>=
	jc_rshs,  //������: >>=
	jc_rshi,  //������: >>=
	jc_rshl,  //������: >>=
	jc_rshuc, //������: >>=
	jc_rshus, //������: >>=
	jc_rshui, //������: >>=
	jc_rshul, //������: >>=

//λ��ָ�˫��������, *arg[0] &= *arg[1]
	jc_anduc,
	jc_andus,
	jc_andui,
	jc_andul,

//λ��ָ�˫��������, *arg[0] |= *arg[1]
	jc_oruc,
	jc_orus,
	jc_orui,
	jc_orul,

//λȡ��ָ�����������, *arg[0] = ~*arg[0]
	jc_notuc,
	jc_notus,
	jc_notui,
	jc_notul,

//λ���ָ�˫��������, *arg[0] ^= *arg[1]
	jc_xoruc,
	jc_xorus,
	jc_xorui,
	jc_xorul
}JCVM_INS_CODE;

#endif
