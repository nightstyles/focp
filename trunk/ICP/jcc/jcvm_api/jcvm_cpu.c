
#include "jcvm_cpu.h"
#include "jc_ins.h"

//----------------------------------------------------------
// instruction implement
//----------------------------------------------------------
jc_char* GetJcVmDataAddr(jc_char* SS, jc_uint nArg);
jc_char* GetJcVmProcAddr(jc_char* SS, jc_uint nArg, CJcVmSegment* pSegment);

#define GetAddr(reg, ins, n) ((ins->nOpt[n]==JC_SS)?GetJcVmDataAddr(reg->SS, ins->nArg[n]):(seg[ins->nOpt[n]]+ins->nArg[n]))
#define GetFunAddr(reg, ins, ps) ((ins->nOpt[0]==JC_SS)?GetJcVmProcAddr(reg->SS, ins->nArg[0], ps):(seg[ins->nOpt[0]]+ins->nArg[0]))

#define JCVM_NON() ++reg->PC

#define JCVM_JMP() reg->PC = (jc_uint*)(reg->CS + ins->nArg[0])

#define JCVM_JMPT(type) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		if(!p0)\
		{\
			reg->PC += 3;\
			ret = JCVM_INVALID_ADDRESS;\
		}\
		else if(*(type*)p0)\
			reg->PC = (jc_uint*)GetAddr(reg, ins, 1);\
		else\
			reg->PC += 3;\
	}while(0)

#define JCVM_JMPF(type)\
	do{\
		p0 = GetAddr(reg, ins, 0);\
		if(!p0)\
		{\
			reg->PC += 3;\
			ret = JCVM_INVALID_ADDRESS;\
		}\
		else if(*(type*)p0)\
			reg->PC += 3;\
		else\
			reg->PC = (jc_uint*)GetAddr(reg, ins, 1);\
	}while(0)

#define JCVM_JTAB(type) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		if(!p0 || !p1)\
		{\
			reg->PC += 3;\
			ret = JCVM_INVALID_ADDRESS;\
		}\
		else\
		{\
			type v1 = *(type*)p0;\
			CJcCaseItem* pTab = (CJcCaseItem*)p1;\
			type* v2 = (type*)((jc_char*)pTab + pTab->nStart);\
			jc_uint* pAddr = &pTab->nDefault;\
			jc_uint i=1, j=pTab->nCount-1, k;\
			if(v1<v2[i] || v1>v2[j])\
				k=0;\
			else if(v1 == v2[i])\
				k=i;\
			else if(v1 == v2[j])\
				k=j;\
			else while(true)\
			{\
				k = ((i+j)>>1);\
				if(k == i)\
				{\
					k=0;\
					break;\
				}\
				if(v1 == v2[k])\
					break;\
				if(v1 < v2[k])\
					j=k;\
				else i=k;\
			}\
			reg->PC = (jc_uint*)(reg->CS + pAddr[k]);\
		}\
	}while(0)

#define JCVM_NEWARG() \
	do{\
		jc_uint nSize = ins->nArg[0];\
		reg->PC += 2;\
		if(nSize)\
		{\
			reg->TS = (jc_char*)g_oInterface.Malloc(nSize);\
			if(!reg->TS)\
				ret = JCVM_LACK_MEMORY;\
		}\
		else\
			reg->TS = NULL;\
	}while(0)

#define JCVM_DELARG() \
	do{\
		if(reg->TS)\
		{\
			g_oInterface.Free(reg->TS);\
			reg->TS = NULL;\
		}\
		++reg->PC;\
	}while(0)

#define JCVM_NEWSTK() \
	do{\
		jc_uint nSize = ins->nArg[0];\
		reg->PC += 2;\
		if(nSize)\
		{\
			reg->LS = (jc_char*)g_oInterface.Malloc(nSize);\
			if(!reg->LS)\
				ret = JCVM_LACK_MEMORY;\
		}\
		else\
			reg->LS = NULL;\
	}while(0)

#define JCVM_DELSTK() \
	do{\
		if(reg->LS)\
		{\
			g_oInterface.Free(reg->LS);\
			reg->LS = NULL;\
		}\
		++reg->PC;\
	}while(0)

#define JCVM_CALL() \
	do{\
		CJcVmSegment ps;\
		ps.CS = reg->CS;\
		jc_char* f = GetFunAddr(reg, ins, &ps); \
		if(ins->nOpt[0]==JC_DS) \
			f = *(jc_char**)f; \
		reg->PC += 2; \
		if(!f) \
			ret = JCVM_INVALID_ADDRESS;\
		else if(!ps.CS)\
			((FOnHost)f)(nCpuIdx, reg->TS);\
		else\
		{\
			CJcVmRegister* pNewReg = (CJcVmRegister*)g_oInterface.Malloc(sizeof(CJcVmRegister));\
			if(!pNewReg)\
				ret = JCVM_LACK_MEMORY;\
			else\
			{\
				pNewReg->FP = reg;\
				pNewReg->AS = reg->TS;\
				pNewReg->CP = reg->CP;\
				if(ps.CS != reg->CS)\
				{\
					pNewReg->IS = ps.IS;\
					pNewReg->DS = ps.DS;\
					pNewReg->CS = ps.CS;\
					pNewReg->SS = ps.SS;\
				}\
				else\
				{\
					pNewReg->IS = reg->IS;\
					pNewReg->DS = reg->DS;\
					pNewReg->CS = reg->CS;\
					pNewReg->SS = reg->SS;\
				}\
				pNewReg->LS = 0;\
				pNewReg->TS = 0;\
				pNewReg->PC = (jc_uint*)f;\
				pWorker->m_pRegister = pNewReg;\
			}\
		}\
	}while(0)

#define JCVM_RET()\
	do{\
		pWorker->m_pRegister = reg->FP;\
		g_oInterface.Free(reg);\
	}while(0)

#define JCVM_LEA() \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		if(!p0)\
			ret = JCVM_INVALID_ADDRESS;\
		else \
			*(jc_char**)p0 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
	}while(0)

#define JCVM_REP() rep = ins->nArg[0]; ++reg->PC

#define JCVM_MOV(t) \
	do{\
		jc_uint i;\
		t* dst = (t*)GetAddr(reg, ins, 0);\
		t* src = (t*)GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!dst || !src)\
			ret = JCVM_INVALID_ADDRESS;\
		if(rep)\
		{\
			for(i=0; i<rep; ++i)\
				dst[i] = src[i];\
			rep = 0;\
		}\
		else\
			*dst = *src;\
	}while(0)

#define JCVM_LOAD(t)\
	do{\
		jc_uint i;\
		t* dst = (t*)GetAddr(reg, ins, 0);\
		t** src1 = (t**)GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!dst || !src1 || !src1[0])\
			ret = JCVM_INVALID_ADDRESS;\
		t* src = *src1;\
		if(rep)\
		{\
			for(i=0; i<rep; ++i)\
				dst[i] = src[i];\
			rep = 0;\
		}\
		else\
			*dst = *src;\
	}while(0)

#define JCVM_SAVE(t)\
	do{\
		jc_uint i;\
		t** dst1 = (t**)GetAddr(reg, ins, 0);\
		t* src = (t*)GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!dst1 || !dst1[0] || !src)\
			ret = JCVM_INVALID_ADDRESS;\
		t* dst = *dst1;\
		if(rep)\
		{\
			for(i=0; i<rep; ++i)\
				dst[i] = src[i];\
			rep = 0;\
		}\
		else\
			*dst = *src;\
	}while(0)

#define JCVM_XCHG(t)\
	do{\
		jc_uint i;\
		t* dst = (t*)GetAddr(reg, ins, 0);\
		t* src = (t*)GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!dst || !src)\
			ret = JCVM_INVALID_ADDRESS;\
		if(rep)\
		{\
			for(i=0; i<rep; ++i)\
			{\
				t x = dst[i];\
				dst[i] = src[i];\
				src[i] = x;\
			}\
			rep = 0;\
		}\
		else\
		{\
			t x = *dst;\
			*dst = *src;\
			*src = x;\
		}\
	}while(0)

#define JCVM_BINARY_0(t, op)\
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!p0 || !p1)\
			ret = JCVM_INVALID_ADDRESS;\
		else\
			*(t*)p0 op *(t*)p1;\
	}while(0);

#define JCVM_ADD(t) JCVM_BINARY_0(t, +=)
#define JCVM_SUB(t) JCVM_BINARY_0(t, -=)
#define JCVM_MUL(t) JCVM_BINARY_0(t, *=)
#define JCVM_DIV(t) JCVM_BINARY_0(t, /=)
#define JCVM_MOD(t) JCVM_BINARY_0(t, %=)
#define JCVM_AND(t) JCVM_BINARY_0(t, &=)
#define JCVM_OR(t)  JCVM_BINARY_0(t, |=)
#define JCVM_XOR(t) JCVM_BINARY_0(t, ^=)

#define JCVM_INC(t) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		reg->PC += 2;\
		if(!p0)\
			ret = JCVM_INVALID_ADDRESS;\
		else\
			++(*(t*)p0);\
	}while(0)

#define JCVM_DEC(t) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		reg->PC += 2;\
		if(!p0)\
			ret = JCVM_INVALID_ADDRESS;\
		else\
			--(*(t*)p0);\
	}while(0)

#define JCVM_NEG(t) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		reg->PC += 2;\
		if(!p0)\
			ret = JCVM_INVALID_ADDRESS;\
		else\
			*(t*)p0 = -(*(t*)p0);\
	}while(0)

#define JCVM_NOT(t) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		reg->PC += 2;\
		if(!p0)\
			ret = JCVM_INVALID_ADDRESS;\
		else\
			*(t*)p0 = ~(*(t*)p0);\
	}while(0)

#define JCVM_CMP(t,op) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!p0 || !p1)\
			ret = JCVM_INVALID_ADDRESS;\
		else\
			*(jc_uint*)p0 = (((*(t*)p1) op (t)0)?1:0);\
	}while(0)

#define JCVM_CVT(t1,t2) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!p0 || !p1)\
			ret = JCVM_INVALID_ADDRESS;\
		else\
			*(t2*)p0 = (t2)(*(t1*)p1);\
	}while(0)

#define JCVM_SH(t,op) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!p0 || !p1)\
			ret = JCVM_INVALID_ADDRESS;\
		else\
			*(t*)p0 op (0x000000FF&(*(jc_uint*)p1));\
	}while(0)

//----------------------------------------------------------
// global virtual machine
//----------------------------------------------------------
extern CJcVmInterface g_oInterface;
static jc_uint g_nWorker = 0;
static jc_uint g_nPowerOff = 1;
static CJcVmWorker* g_pWorker = NULL;
static jc_void JCVM_CALL JcVmThread(CJcVmWorker* pWorker);

//----------------------------------------------------------
// virtual machine class implement
//----------------------------------------------------------

jc_uint InitializeJcVm(jc_uint nCpuNum)
{
	jc_uint i;
	if(nCpuNum == 0)
		nCpuNum = 5;
	g_nWorker = nCpuNum;
	g_nPowerOff = 0;
	g_pWorker = (CJcVmWorker*)g_oInterface.Malloc(g_nWorker*sizeof(CJcVmWorker));
	if(g_pWorker == NLL)
		return 1;
	for(i=0; i<g_nWorker; ++i)
	{
		g_pWorker[i].bStop = 0;
		g_pWorker[i].bRuning = 0;
		g_pWorker[i].hEvent = NULL;
		g_pWorker[i].pRegister = NULL;
	}
	for(i=0; i<g_nWorker; ++i)
	{
		g_pWorker[i].hEvent = g_oInterface.CreateEvent();
		if(g_pWorker[i].hEvent == NULL)
			break;
		if(g_oInterface.CreateThread(g_pWorker+i, (FThreadProc)JcVmThread))
			break;
		while(g_pWorker[i].bRuning == 0)
			g_oInterface.Sleep(100);
	}
	return 0;
}

jc_void PowerOffJcVm()
{
	g_nPowerOff = 1;
}

jc_void HaltJcVmCpu(jc_uint nCpuIdx)
{
	g_pWorker[nCpuIdx].bStop = 1;
}

jc_void CleanupJcVm()
{
	jc_uint i;
	while(1)
	{
		for(i=0; i<g_nWorker; ++i)
		{
			if(g_pWorker[i].bRuning)
				break;
		}
		if(i == g_nWorker)
			break;
		g_oInterface.Sleep(100);
	}
	for(i=0; i<g_nWorker; ++i)
		g_oInterface.DestroyEvent(g_pWorker[i].hEvent);
	g_nWorker = 0;
	if(g_pWorker)
	{
		g_oInterface.Free(g_pWorker);
		g_pWorker = NULL;
	}
}

CJcVmRegister* SwitchJcVm(jc_uint nCpuIdx, CJcVmRegister* pNewRegister)
{
	CJcVmRegister* pRet = g_pWorker[nCpuIdx].m_pRegister;
	if(pRet && !pNewRegister)
		g_oInterface.ResetEvent(g_pWorker[nCpuIdx].hEvent);
	if(pNewRegister && !pRet)
		g_oInterface.SetEvent(g_pWorker[nCpuIdx].hEvent);
}

CJcVmRegister* GetJcVmRegister(jc_uint nCpuIdx)
{
	return g_pWorker[nCpuIdx].m_pRegister;
}

static jc_void JCVM_CALL JcVmThread(CJcVmWorker* pWorker)
{
	CJcIns* ins;
	CJcVmRegister* reg;
	jc_char** seg, *p0, *p1;
	jc_uint nCpuIdx, ret, rep;
	jc_uint nPowerOff=0, nTerminal=0;
	
	nCpuIdx = pWorker - g_pWorker;

	pWorker->bRuning = 1;
	while(pWorker->bStop == 0)
	{
		WaitForSingleObject(pWorker->hEvent, 1000);
		while(reg = pWorker->m_pRegister && pWorker->bStop == 0)
		{
			ret = 0;
			rep = 0;
			seg = (jc_char**)reg;
			ins = (CJcIns*)reg->PC;
			try
			{
				if(g_nPowerOff && nPowerOff == 0)
					nPowerOff = 1;
				else switch(ins->nOp)
				{
				case jc_non:
					JCVM_NON();
					break;
				case jc_jmp:
					JCVM_JMP();
					break;
				case jc_jmptc:
					JCVM_JMPT(jc_char);
					break;
				case jc_jmpts:
					JCVM_JMPT(jc_short);
					break;
				case jc_jmpti:
					JCVM_JMPT(jc_int);
					break;
				case jc_jmptl:
					JCVM_JMPT(jc_long);
					break;
				case jc_jmptf:
					JCVM_JMPT(jc_float);
					break;
				case jc_jmptd:
					JCVM_JMPT(jc_double);
					break;
				case jc_jmpfc:
					JCVM_JMPF(jc_char);
					break;
				case jc_jmpfs:
					JCVM_JMPF(jc_short);
					break;
				case jc_jmpfi:
					JCVM_JMPF(jc_int);
					break;
				case jc_jmpfl:
					JCVM_JMPF(jc_long);
					break;
				case jc_jmpff:
					JCVM_JMPF(jc_float);
					break;
				case jc_jmpfd:
					JCVM_JMPF(jc_double);
					break;
				case jc_jtabc:
					JCVM_JTAB(jc_uchar);
					break;
				case jc_jtabs:
					JCVM_JTAB(jc_ushort);
					break;
				case jc_jtabi:
					JCVM_JTAB(jc_uint);
					break;
				case jc_jtabl:
					JCVM_JTAB(jc_ulong);
					break;
				case jc_newarg:
					JCVM_NEWARG();
					break;
				case jc_delarg:
					JCVM_DELARG();
					break;
				case jc_newstk:
					JCVM_NEWSTK();
					break;
				case jc_delstk:
					JCVM_DELSTK();
					break;
				case jc_call:
					JCVM_CALL();
					break;
				case jc_ret:
					JCVM_RET();
					break;
				case jc_lea:
					JCVM_LEA();
					break;
				case jc_rep:
					JCVM_REP();
					break;
				case jc_movc:
					JCVM_MOV(jc_uchar);
					break;
				case jc_movs:
					JCVM_MOV(jc_ushort);
					break;
				case jc_movi:
					JCVM_MOV(jc_uint);
					break;
				case jc_movl:
					JCVM_MOV(jc_ulong);
					break;
				case jc_loadc:
					JCVM_LOAD(jc_uchar);
					break;
				case jc_loads:
					JCVM_LOAD(jc_ushort);
					break;
				case jc_loadi:
					JCVM_LOAD(jc_uint);
					break;
				case jc_loadl:
					JCVM_LOAD(jc_ulong);
					break;
				case jc_savec:
					JCVM_SAVE(jc_uchar);
					break;
				case jc_saves:
					JCVM_SAVE(jc_ushort);
					break;
				case jc_savei:
					JCVM_SAVE(jc_uint);
					break;
				case jc_savel:
					JCVM_SAVE(jc_ulong);
					break;
				case jc_xchgc:
					JCVM_XCHG(jc_uchar);
					break;
				case jc_xchgs:
					JCVM_XCHG(jc_ushort);
					break;
				case jc_xchgi:
					JCVM_XCHG(jc_uint);
					break;
				case jc_xchgl:
					JCVM_XCHG(jc_ulong);
					break;
				case jc_addc:
					JCVM_ADD(jc_uchar);
					break;
				case jc_adds:
					JCVM_ADD(jc_ushort);
					break;
				case jc_addi:
					JCVM_ADD(jc_uint);
					break;
				case jc_addl:
					JCVM_ADD(jc_ulong);
					break;
				case jc_addf:
					JCVM_ADD(jc_float);
					break;
				case jc_addd:
					JCVM_ADD(jc_double);
					break;
				case jc_incc:
					JCVM_INC(jc_char);
					break;
				case jc_incs:
					JCVM_INC(jc_short);
					break;
				case jc_inci:
					JCVM_INC(jc_int);
					break;
				case jc_incl:
					JCVM_INC(jc_long);
					break;
				case jc_subc:
					JCVM_SUB(jc_uchar);
					break;
				case jc_subs:
					JCVM_SUB(jc_ushort);
					break;
				case jc_subi:
					JCVM_SUB(jc_uint);
					break;
				case jc_subl:
					JCVM_SUB(jc_ulong);
					break;
				case jc_subf:
					JCVM_SUB(jc_float);
					break;
				case jc_subd:
					JCVM_SUB(jc_double);
					break;
				case jc_decc:
					JCVM_DEC(jc_char);
					break;
				case jc_decs:
					JCVM_DEC(jc_short);
					break;
				case jc_deci:
					JCVM_DEC(jc_int);
					break;
				case jc_decl:
					JCVM_DEC(jc_long);
					break;
				case jc_negc:
					JCVM_NEG(jc_char);
					break;
				case jc_negs:
					JCVM_NEG(jc_short);
					break;
				case jc_negi:
					JCVM_NEG(jc_int);
					break;
				case jc_negl:
					JCVM_NEG(jc_long);
					break;
				case jc_negf:
					JCVM_NEG(jc_float);
					break;
				case jc_negd:
					JCVM_NEG(jc_double);
					break;
				case jc_mulc:
					JCVM_MUL(jc_char);
					break;
				case jc_muls:
					JCVM_MUL(jc_short);
					break;
				case jc_muli:
					JCVM_MUL(jc_int);
					break;
				case jc_mull:
					JCVM_MUL(jc_long);
					break;
				case jc_mulf:
					JCVM_MUL(jc_float);
					break;
				case jc_muld:
					JCVM_MUL(jc_double);
					break;
				case jc_muluc:
					JCVM_MUL(jc_uchar);
					break;
				case jc_mulus:
					JCVM_MUL(jc_ushort);
					break;
				case jc_mului:
					JCVM_MUL(jc_uint);
					break;
				case jc_mulul:
					JCVM_MUL(jc_ulong);
					break;
				case jc_divc:
					JCVM_DIV(jc_char);
					break;
				case jc_divs:
					JCVM_DIV(jc_short);
					break;
				case jc_divi:
					JCVM_DIV(jc_int);
					break;
				case jc_divl:
					JCVM_DIV(jc_long);
					break;
				case jc_divf:
					JCVM_DIV(jc_float);
					break;
				case jc_divd:
					JCVM_DIV(jc_double);
					break;
				case jc_divuc:
					JCVM_DIV(jc_uchar);
					break;
				case jc_divus:
					JCVM_DIV(jc_ushort);
					break;
				case jc_divui:
					JCVM_DIV(jc_uint);
					break;
				case jc_divul:
					JCVM_DIV(jc_ulong);
					break;
				case jc_modc:
					JCVM_MOD(jc_char);
					break;
				case jc_mods:
					JCVM_MOD(jc_short);
					break;
				case jc_modi:
					JCVM_MOD(jc_int);
					break;
				case jc_modl:
					JCVM_MOD(jc_long);
					break;
				case jc_moduc:
					JCVM_MOD(jc_uchar);
					break;
				case jc_modus:
					JCVM_MOD(jc_ushort);
					break;
				case jc_modui:
					JCVM_MOD(jc_uint);
					break;
				case jc_modul:
					JCVM_MOD(jc_ulong);
					break;
				case jc_ltc:
					JCVM_CMP(jc_char, <);
					break;
				case jc_lts:
					JCVM_CMP(jc_short, <);
					break;
				case jc_lti:
					JCVM_CMP(jc_int, <);
					break;
				case jc_ltl:
					JCVM_CMP(jc_long, <);
					break;
				case jc_ltf:
					JCVM_CMP(jc_float, <);
					break;
				case jc_ltd:
					JCVM_CMP(jc_double, <);
					break;
				case jc_ltuc:
					JCVM_CMP(jc_uchar, <);
					break;
				case jc_ltus:
					JCVM_CMP(jc_ushort, <);
					break;
				case jc_ltui:
					JCVM_CMP(jc_uint, <);
					break;
				case jc_ltul:
					JCVM_CMP(jc_ulong, <);
					break;
				case jc_lec:
					JCVM_CMP(jc_char, <=);
					break;
				case jc_les:
					JCVM_CMP(jc_short, <=);
					break;
				case jc_lei:
					JCVM_CMP(jc_int, <=);
					break;
				case jc_lel:
					JCVM_CMP(jc_long, <=);
					break;
				case jc_lef:
					JCVM_CMP(jc_float, <=);
					break;
				case jc_led:
					JCVM_CMP(jc_double, <=);
					break;
				case jc_leuc:
					JCVM_CMP(jc_uchar, <=);
					break;
				case jc_leus:
					JCVM_CMP(jc_ushort, <=);
					break;
				case jc_leui:
					JCVM_CMP(jc_uint, <=);
					break;
				case jc_leul:
					JCVM_CMP(jc_ulong, <=);
					break;
				case jc_eqc:
					JCVM_CMP(jc_uchar, ==);
					break;
				case jc_eqs:
					JCVM_CMP(jc_ushort, ==);
					break;
				case jc_eqi:
					JCVM_CMP(jc_uint, ==);
					break;
				case jc_eql:
					JCVM_CMP(jc_ulong, ==);
					break;
				case jc_eqf:
					JCVM_CMP(jc_float, ==);
					break;
				case jc_eqd:
					JCVM_CMP(jc_double, ==);
					break;
				case jc_nec:
					JCVM_CMP(jc_uchar, !=);
					break;
				case jc_nes:
					JCVM_CMP(jc_ushort, !=);
					break;
				case jc_nei:
					JCVM_CMP(jc_uint, !=);
					break;
				case jc_nel:
					JCVM_CMP(jc_ulong, !=);
					break;
				case jc_nef:
					JCVM_CMP(jc_float, !=);
					break;
				case jc_ned:
					JCVM_CMP(jc_double, !=);
					break;
				case jc_gec:
					JCVM_CMP(jc_char, >=);
					break;
				case jc_ges:
					JCVM_CMP(jc_short, >=);
					break;
				case jc_gei:
					JCVM_CMP(jc_int, >=);
					break;
				case jc_gel:
					JCVM_CMP(jc_long, >=);
					break;
				case jc_gef:
					JCVM_CMP(jc_float, >=);
					break;
				case jc_ged:
					JCVM_CMP(jc_double, >=);
					break;
				case jc_geuc:
					JCVM_CMP(jc_uchar, >=);
					break;
				case jc_geus:
					JCVM_CMP(jc_ushort, >=);
					break;
				case jc_geui:
					JCVM_CMP(jc_uint, >=);
					break;
				case jc_geul:
					JCVM_CMP(jc_ulong, >=);
					break;
				case jc_gtc:
					JCVM_CMP(jc_char, >);
					break;
				case jc_gts:
					JCVM_CMP(jc_short, >);
					break;
				case jc_gti:
					JCVM_CMP(jc_int, >);
					break;
				case jc_gtl:
					JCVM_CMP(jc_long, >);
					break;
				case jc_gtf:
					JCVM_CMP(jc_float, >);
					break;
				case jc_gtd:
					JCVM_CMP(jc_double, >);
					break;
				case jc_gtuc:
					JCVM_CMP(jc_uchar, >);
					break;
				case jc_gtus:
					JCVM_CMP(jc_ushort, >);
					break;
				case jc_gtui:
					JCVM_CMP(jc_uint, >);
					break;
				case jc_gtul:
					JCVM_CMP(jc_ulong, >);
					break;
				case jc_c2s:
					JCVM_CVT(jc_char, jc_short);
					break;
				case jc_s2i:
					JCVM_CVT(jc_short, jc_int);
					break;
				case jc_i2l:
					JCVM_CVT(jc_int, jc_long);
					break;
				case jc_l2i:
					JCVM_CVT(jc_long, jc_int);
					break;
				case jc_i2s:
					JCVM_CVT(jc_int, jc_short);
					break;
				case jc_s2c:
					JCVM_CVT(jc_short, jc_char);
					break;
				case jc_uc2us:
					JCVM_CVT(jc_uchar, jc_ushort);
					break;
				case jc_us2ui:
					JCVM_CVT(jc_ushort, jc_uint);
					break;
				case jc_ui2ul:
					JCVM_CVT(jc_uint, jc_ulong);
					break;
				case jc_ul2ui:
					JCVM_CVT(jc_ulong, jc_uint);
					break;
				case jc_ui2us:
					JCVM_CVT(jc_uint, jc_ushort);
					break;
				case jc_us2uc:
					JCVM_CVT(jc_ushort, jc_uchar);
					break;
				case jc_f2d:
					JCVM_CVT(jc_float, jc_double);
					break;
				case jc_d2f:
					JCVM_CVT(jc_double, jc_float);
					break;
				case jc_f2i:
					JCVM_CVT(jc_float, jc_int);
					break;
				case jc_i2f:
					JCVM_CVT(jc_int, jc_float);
					break;
				case jc_d2l:
					JCVM_CVT(jc_double, jc_long);
					break;
				case jc_l2d:
					JCVM_CVT(jc_long, jc_double);
					break;
				case jc_lshuc:
					JCVM_SH(jc_uchar,<<=);
					break;
				case jc_lshus:
					JCVM_SH(jc_ushort,<<=);
					break;
				case jc_lshui:
					JCVM_SH(jc_uint,<<=);
					break;
				case jc_lshul:
					JCVM_SH(jc_ulong,<<=);
					break;
				case jc_rshc:
					JCVM_SH(jc_char,>>=);
					break;
				case jc_rshs:
					JCVM_SH(jc_short,>>=);
					break;
				case jc_rshi:
					JCVM_SH(jc_int,>>=);
					break;
				case jc_rshl:
					JCVM_SH(jc_long,>>=);
					break;
				case jc_rshuc:
					JCVM_SH(jc_uchar,>>=);
					break;
				case jc_rshus:
					JCVM_SH(jc_ushort,>>=);
					break;
				case jc_rshui:
					JCVM_SH(jc_uint,>>=);
					break;
				case jc_rshul:
					JCVM_SH(jc_ulong,>>=);
					break;
				case jc_anduc:
					JCVM_AND(jc_uchar);
					break;
				case jc_andus:
					JCVM_AND(jc_ushort);
					break;
				case jc_andui:
					JCVM_AND(jc_uint);
					break;
				case jc_andul:
					JCVM_AND(jc_ulong);
					break;
				case jc_oruc:
					JCVM_OR(jc_uchar);
					break;
				case jc_orus:
					JCVM_OR(jc_ushort);
					break;
				case jc_orui:
					JCVM_OR(jc_uint);
					break;
				case jc_orul:
					JCVM_OR(jc_ulong);
					break;
				case jc_notuc:
					JCVM_NOT(jc_uchar);
					break;
				case jc_notus:
					JCVM_NOT(jc_ushort);
					break;
				case jc_notui:
					JCVM_NOT(jc_uint);
					break;
				case jc_notul:
					JCVM_NOT(jc_ulong);
					break;
				case jc_xoruc:
					JCVM_XOR(jc_uchar);
					break;
				case jc_xorus:
					JCVM_XOR(jc_ushort);
					break;
				case jc_xorui:
					JCVM_XOR(jc_uint);
					break;
				case jc_xorul:
					JCVM_XOR(jc_ulong);
					break;
				}
			}
			catch(...)
			{
				ret = JCVM_OTHER_EXCEPT;
			}
			if(nPowerOff && nTerminal==0)
			{
				nTerminal = 1;
				ret = JCVM_POWER_OFF;
			}
			if(ret)
				g_oInterface.Signal(nCpuIdx, ret);
		}
	}
	pWorker->bRuning = 0;
}
