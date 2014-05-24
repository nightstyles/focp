
#include "EvmCpu.hpp"

FOCP_BEGIN();

#include "EvmIns.h"

//----------------------------------------------------------
// instruction implement
//----------------------------------------------------------
ehc_char* GetEhcVmDataAddr(ehc_void* reg, ehc_uint nArg);
ehc_char* GetEhcVmProcAddr(ehc_void* reg, ehc_uint nArg, CEhcVmSegment* pSegment);

#define GetAddr(reg, ins, n) ((ins->nOpt[n]==EHC_SS)?GetEhcVmDataAddr(reg, ins->nArg[n]):(seg[ins->nOpt[n]]+ins->nArg[n]))
#define GetFunAddr(reg, ins, ps) ((ins->nOpt[0]==EHC_SS)?GetEhcVmProcAddr(reg, ins->nArg[0], ps):(seg[ins->nOpt[0]]+ins->nArg[0]))

#define EVM_NON() ++reg->PC

#define EVM_JMP() reg->PC = (ehc_uint*)(reg->CS + ins->nArg[0])

#define EVM_JMPT(type) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		if(!p0)\
		{\
			reg->PC += 3;\
			ret = EVM_INVALID_ADDRESS;\
		}\
		else if(*(type*)p0)\
			reg->PC = (ehc_uint*)GetAddr(reg, ins, 1);\
		else\
			reg->PC += 3;\
	}while(0)

#define EVM_JMPF(type)\
	do{\
		p0 = GetAddr(reg, ins, 0);\
		if(!p0)\
		{\
			reg->PC += 3;\
			ret = EVM_INVALID_ADDRESS;\
		}\
		else if(*(type*)p0)\
			reg->PC += 3;\
		else\
			reg->PC = (ehc_uint*)GetAddr(reg, ins, 1);\
	}while(0)

#define EVM_JTAB(type) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		if(!p0 || !p1)\
		{\
			reg->PC += 3;\
			ret = EVM_INVALID_ADDRESS;\
		}\
		else\
		{\
			type v1 = *(type*)p0;\
			CEhcCaseItem* pTab = (CEhcCaseItem*)p1;\
			type* v2 = (type*)((ehc_char*)pTab + pTab->nStart);\
			ehc_uint* pAddr = &pTab->nDefault;\
			ehc_uint i=1, j=pTab->nCount-1, k;\
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
			reg->PC = (ehc_uint*)(reg->CS + pAddr[k]);\
		}\
	}while(0)

#define EVM_NEWARG() \
	do{\
		ehc_uint nSize = ins->nArg[0];\
		reg->PC += 2;\
		if(nSize)\
		{\
			reg->TS = (ehc_char*)CMalloc::Malloc(nSize);\
			if(!reg->TS)\
				ret = EVM_LACK_MEMORY;\
		}\
		else\
			reg->TS = NULL;\
	}while(0)

#define EVM_DELARG() \
	do{\
		if(reg->TS)\
		{\
			CMalloc::Free(reg->TS);\
			reg->TS = NULL;\
		}\
		++reg->PC;\
	}while(0)

#define EVM_NEWSTK() \
	do{\
		ehc_uint nSize = ins->nArg[0];\
		reg->PC += 2;\
		if(nSize)\
		{\
			reg->LS = (ehc_char*)CMalloc::Malloc(nSize);\
			if(!reg->LS)\
				ret = EVM_LACK_MEMORY;\
		}\
		else\
			reg->LS = NULL;\
	}while(0)

#define EVM_DELSTK() \
	do{\
		if(reg->LS)\
		{\
			CMalloc::Free(reg->LS);\
			reg->LS = NULL;\
		}\
		++reg->PC;\
	}while(0)

#define EVM_CALL() \
	do{\
		CEhcVmSegment ps;\
		ps.CS = reg->CS;\
		ehc_char* f = GetFunAddr(reg, ins, &ps); \
		if(ins->nOpt[0]==EHC_DS) \
			f = *(ehc_char**)f; \
		reg->PC += 2; \
		if(!f) \
			ret = EVM_INVALID_ADDRESS;\
		else if(!ps.CS)\
			((FOnHost)f)(reg->TS, this, &reg);\
		else\
		{\
			CEvmRegister* pNewReg = (CEvmRegister*)CMalloc::Malloc(sizeof(CEvmRegister));\
			if(!pNewReg)\
				ret = EVM_LACK_MEMORY;\
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
				pNewReg->PC = (ehc_uint*)f;\
				pWorker->m_pRegister = pNewReg;\
			}\
		}\
	}while(0)

#define EVM_RET()\
	do{\
		pWorker->m_pRegister = reg->FP;\
		CMalloc::Free(reg);\
	}while(0)

#define EVM_LEA() \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		if(!p0)\
			ret = EVM_INVALID_ADDRESS;\
		else \
			*(ehc_char**)p0 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
	}while(0)

#define EVM_REP() rep = ins->nArg[0]; ++reg->PC

#define EVM_MOV(t) \
	do{\
		ehc_uint i;\
		t* dst = (t*)GetAddr(reg, ins, 0);\
		t* src = (t*)GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!dst || !src)\
			ret = EVM_INVALID_ADDRESS;\
		if(rep)\
		{\
			for(i=0; i<rep; ++i)\
				dst[i] = src[i];\
			rep = 0;\
		}\
		else\
			*dst = *src;\
	}while(0)

#define EVM_LOAD(t)\
	do{\
		ehc_uint i;\
		t* dst = (t*)GetAddr(reg, ins, 0);\
		t** src1 = (t**)GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!dst || !src1 || !src1[0])\
			ret = EVM_INVALID_ADDRESS;\
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

#define EVM_SAVE(t)\
	do{\
		ehc_uint i;\
		t** dst1 = (t**)GetAddr(reg, ins, 0);\
		t* src = (t*)GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!dst1 || !dst1[0] || !src)\
			ret = EVM_INVALID_ADDRESS;\
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

#define EVM_XCHG(t)\
	do{\
		ehc_uint i;\
		t* dst = (t*)GetAddr(reg, ins, 0);\
		t* src = (t*)GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!dst || !src)\
			ret = EVM_INVALID_ADDRESS;\
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

#define EVM_BINARY_0(t, op)\
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!p0 || !p1)\
			ret = EVM_INVALID_ADDRESS;\
		else\
			*(t*)p0 op *(t*)p1;\
	}while(0);

#define EVM_ADD(t) EVM_BINARY_0(t, +=)
#define EVM_SUB(t) EVM_BINARY_0(t, -=)
#define EVM_MUL(t) EVM_BINARY_0(t, *=)
#define EVM_DIV(t) EVM_BINARY_0(t, /=)
#define EVM_MOD(t) EVM_BINARY_0(t, %=)
#define EVM_AND(t) EVM_BINARY_0(t, &=)
#define EVM_OR(t)  EVM_BINARY_0(t, |=)
#define EVM_XOR(t) EVM_BINARY_0(t, ^=)

#define EVM_INC(t) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		reg->PC += 2;\
		if(!p0)\
			ret = EVM_INVALID_ADDRESS;\
		else\
			++(*(t*)p0);\
	}while(0)

#define EVM_DEC(t) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		reg->PC += 2;\
		if(!p0)\
			ret = EVM_INVALID_ADDRESS;\
		else\
			--(*(t*)p0);\
	}while(0)

#define EVM_NEG(t) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		reg->PC += 2;\
		if(!p0)\
			ret = EVM_INVALID_ADDRESS;\
		else\
			*(t*)p0 = -(*(t*)p0);\
	}while(0)

#define EVM_NOT(t) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		reg->PC += 2;\
		if(!p0)\
			ret = EVM_INVALID_ADDRESS;\
		else\
			*(t*)p0 = ~(*(t*)p0);\
	}while(0)

#define EVM_CMP(t,op) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!p0 || !p1)\
			ret = EVM_INVALID_ADDRESS;\
		else\
			*(ehc_uint*)p0 = (((*(t*)p1) op (t)0)?1:0);\
	}while(0)

#define EVM_CVT(t1,t2) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!p0 || !p1)\
			ret = EVM_INVALID_ADDRESS;\
		else\
			*(t2*)p0 = (t2)(*(t1*)p1);\
	}while(0)

#define EVM_CVT_U2D() \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!p0 || !p1)\
			ret = EVM_INVALID_ADDRESS;\
		else\
		{\
			register ehc_long x = *(ehc_long*)p1; \
			if(x >= 0)\
				*(ehc_double*)p0 = (ehc_double)x;\
			else\
				*(ehc_double*)p0 = (ehc_double(x&FOCP_INT64_MAX) + FOCP_INT64_MAX) + 1;\
		}\
	}while(0)

#define EVM_SH(t,op) \
	do{\
		p0 = GetAddr(reg, ins, 0);\
		p1 = GetAddr(reg, ins, 1);\
		reg->PC += 3;\
		if(!p0 || !p1)\
			ret = EVM_INVALID_ADDRESS;\
		else\
			*(t*)p0 op (0x000000FF&(*(ehc_uint*)p1));\
	}while(0)

//----------------------------------------------------------
// global virtual machine
//----------------------------------------------------------
CEvmCpu::CEvmCpu():
	m_oReadyQueue(FocpFieldOffset(CEvmRegister, pNext))
{
	OnSignal = NULL;
	m_oEvent.Reset();
}

CEvmCpu::~CEvmCpu()
{
}

CEvmCpu* CEvmCpu::GetInstance()
{
	return CSingleInstance<CEvmCpu>::GetInstance();
}

void CEvmCpu::Initialzie(FSignal OnSignal, ehc_uint nCpuNum)
{
	this->OnSignal = OnSignal;
	m_oCpus.Initialzie(nCpuNum, this);
}

void CEvmCpu::Cleanup()
{
	m_oCpus.Cleanup();
}

bool CEvmCpu::Start()
{
	return m_oCpus.Start();
}

void CEvmCpu::Stop(bool bBlock)
{
	m_oCpus.Stop();
}

ehc_void CEvmCpu::Resume(CEvmRegister* pRegister)
{
	m_oMutex.Enter();
	m_oReadyQueue.Push(pRegister);
	if(m_oReadyQueue.GetSize() == 1)
		m_oEvent.Set();
	m_oMutex.Leave();
}

CEvmRegister* CEvmCpu::Pop()
{
	m_oEvent.Wait(1000);
	m_oMutex.Enter();
	CEvmRegister* pReg = m_oReadyQueue.Pop();
	if(pReg && (m_oReadyQueue.GetSize()==0))
		m_oEvent.Reset();
	m_oMutex.Leave();
	return pReg;
}

void CEvmCpu::MainProc(CCooperator* pCooperator, bool &bRunning)
{
	register ehc_uint nCpuIdx = m_oCpus.GetCooperatorIndex(pCooperator);

	while(true)
	{
		register CEvmRegister* reg = Pop();
		if(reg == NULL)
		{
			if(!bRunning)
				break;
			continue;
		}
		while(reg)
		{
			register CEhcIns* ins = (CEhcIns*)reg->PC;
			register ehc_char** seg = (ehc_char**)reg, *p0, *p1;
			register ehc_uint ret = 0, rep = 0;
			register ehc_uint nPowerOff=0, nTerminal=0;
			try
			{
				if(bRunning == false && nPowerOff == 0)
					nPowerOff = 1;
				else switch(ins->nOp)
				{
				case ehc_non:
					EVM_NON();
					break;
				case ehc_jmp:
					EVM_JMP();
					break;
				case ehc_jmptc:
					EVM_JMPT(ehc_char);
					break;
				case ehc_jmpts:
					EVM_JMPT(ehc_short);
					break;
				case ehc_jmpti:
					EVM_JMPT(ehc_int);
					break;
				case ehc_jmptl:
					EVM_JMPT(ehc_long);
					break;
				case ehc_jmptf:
					EVM_JMPT(ehc_float);
					break;
				case ehc_jmptd:
					EVM_JMPT(ehc_double);
					break;
				case ehc_jmpfc:
					EVM_JMPF(ehc_char);
					break;
				case ehc_jmpfs:
					EVM_JMPF(ehc_short);
					break;
				case ehc_jmpfi:
					EVM_JMPF(ehc_int);
					break;
				case ehc_jmpfl:
					EVM_JMPF(ehc_long);
					break;
				case ehc_jmpff:
					EVM_JMPF(ehc_float);
					break;
				case ehc_jmpfd:
					EVM_JMPF(ehc_double);
					break;
				case ehc_jtabc:
					EVM_JTAB(ehc_uchar);
					break;
				case ehc_jtabs:
					EVM_JTAB(ehc_ushort);
					break;
				case ehc_jtabi:
					EVM_JTAB(ehc_uint);
					break;
				case ehc_jtabl:
					EVM_JTAB(ehc_ulong);
					break;
				case ehc_newarg:
					EVM_NEWARG();
					break;
				case ehc_delarg:
					EVM_DELARG();
					break;
				case ehc_newstk:
					EVM_NEWSTK();
					break;
				case ehc_delstk:
					EVM_DELSTK();
					break;
				case ehc_call:
					EVM_CALL();
					break;
				case ehc_ret:
					EVM_RET();
					break;
				case ehc_lea:
					EVM_LEA();
					break;
				case ehc_rep:
					EVM_REP();
					break;
				case ehc_movc:
					EVM_MOV(ehc_uchar);
					break;
				case ehc_movs:
					EVM_MOV(ehc_ushort);
					break;
				case ehc_movi:
					EVM_MOV(ehc_uint);
					break;
				case ehc_movl:
					EVM_MOV(ehc_ulong);
					break;
				case ehc_loadc:
					EVM_LOAD(ehc_uchar);
					break;
				case ehc_loads:
					EVM_LOAD(ehc_ushort);
					break;
				case ehc_loadi:
					EVM_LOAD(ehc_uint);
					break;
				case ehc_loadl:
					EVM_LOAD(ehc_ulong);
					break;
				case ehc_savec:
					EVM_SAVE(ehc_uchar);
					break;
				case ehc_saves:
					EVM_SAVE(ehc_ushort);
					break;
				case ehc_savei:
					EVM_SAVE(ehc_uint);
					break;
				case ehc_savel:
					EVM_SAVE(ehc_ulong);
					break;
				case ehc_xchgc:
					EVM_XCHG(ehc_uchar);
					break;
				case ehc_xchgs:
					EVM_XCHG(ehc_ushort);
					break;
				case ehc_xchgi:
					EVM_XCHG(ehc_uint);
					break;
				case ehc_xchgl:
					EVM_XCHG(ehc_ulong);
					break;
				case ehc_addc:
					EVM_ADD(ehc_uchar);
					break;
				case ehc_adds:
					EVM_ADD(ehc_ushort);
					break;
				case ehc_addi:
					EVM_ADD(ehc_uint);
					break;
				case ehc_addl:
					EVM_ADD(ehc_ulong);
					break;
				case ehc_addf:
					EVM_ADD(ehc_float);
					break;
				case ehc_addd:
					EVM_ADD(ehc_double);
					break;
				case ehc_incc:
					EVM_INC(ehc_char);
					break;
				case ehc_incs:
					EVM_INC(ehc_short);
					break;
				case ehc_inci:
					EVM_INC(ehc_int);
					break;
				case ehc_incl:
					EVM_INC(ehc_long);
					break;
				case ehc_subc:
					EVM_SUB(ehc_uchar);
					break;
				case ehc_subs:
					EVM_SUB(ehc_ushort);
					break;
				case ehc_subi:
					EVM_SUB(ehc_uint);
					break;
				case ehc_subl:
					EVM_SUB(ehc_ulong);
					break;
				case ehc_subf:
					EVM_SUB(ehc_float);
					break;
				case ehc_subd:
					EVM_SUB(ehc_double);
					break;
				case ehc_decc:
					EVM_DEC(ehc_char);
					break;
				case ehc_decs:
					EVM_DEC(ehc_short);
					break;
				case ehc_deci:
					EVM_DEC(ehc_int);
					break;
				case ehc_decl:
					EVM_DEC(ehc_long);
					break;
				case ehc_negc:
					EVM_NEG(ehc_char);
					break;
				case ehc_negs:
					EVM_NEG(ehc_short);
					break;
				case ehc_negi:
					EVM_NEG(ehc_int);
					break;
				case ehc_negl:
					EVM_NEG(ehc_long);
					break;
				case ehc_negf:
					EVM_NEG(ehc_float);
					break;
				case ehc_negd:
					EVM_NEG(ehc_double);
					break;
				case ehc_mulc:
					EVM_MUL(ehc_char);
					break;
				case ehc_muls:
					EVM_MUL(ehc_short);
					break;
				case ehc_muli:
					EVM_MUL(ehc_int);
					break;
				case ehc_mull:
					EVM_MUL(ehc_long);
					break;
				case ehc_mulf:
					EVM_MUL(ehc_float);
					break;
				case ehc_muld:
					EVM_MUL(ehc_double);
					break;
				case ehc_muluc:
					EVM_MUL(ehc_uchar);
					break;
				case ehc_mulus:
					EVM_MUL(ehc_ushort);
					break;
				case ehc_mului:
					EVM_MUL(ehc_uint);
					break;
				case ehc_mulul:
					EVM_MUL(ehc_ulong);
					break;
				case ehc_divc:
					EVM_DIV(ehc_char);
					break;
				case ehc_divs:
					EVM_DIV(ehc_short);
					break;
				case ehc_divi:
					EVM_DIV(ehc_int);
					break;
				case ehc_divl:
					EVM_DIV(ehc_long);
					break;
				case ehc_divf:
					EVM_DIV(ehc_float);
					break;
				case ehc_divd:
					EVM_DIV(ehc_double);
					break;
				case ehc_divuc:
					EVM_DIV(ehc_uchar);
					break;
				case ehc_divus:
					EVM_DIV(ehc_ushort);
					break;
				case ehc_divui:
					EVM_DIV(ehc_uint);
					break;
				case ehc_divul:
					EVM_DIV(ehc_ulong);
					break;
				case ehc_modc:
					EVM_MOD(ehc_char);
					break;
				case ehc_mods:
					EVM_MOD(ehc_short);
					break;
				case ehc_modi:
					EVM_MOD(ehc_int);
					break;
				case ehc_modl:
					EVM_MOD(ehc_long);
					break;
				case ehc_moduc:
					EVM_MOD(ehc_uchar);
					break;
				case ehc_modus:
					EVM_MOD(ehc_ushort);
					break;
				case ehc_modui:
					EVM_MOD(ehc_uint);
					break;
				case ehc_modul:
					EVM_MOD(ehc_ulong);
					break;
				case ehc_ltc:
					EVM_CMP(ehc_char, <);
					break;
				case ehc_lts:
					EVM_CMP(ehc_short, <);
					break;
				case ehc_lti:
					EVM_CMP(ehc_int, <);
					break;
				case ehc_ltl:
					EVM_CMP(ehc_long, <);
					break;
				case ehc_ltf:
					EVM_CMP(ehc_float, <);
					break;
				case ehc_ltd:
					EVM_CMP(ehc_double, <);
					break;
				case ehc_ltuc:
					EVM_CMP(ehc_uchar, <);
					break;
				case ehc_ltus:
					EVM_CMP(ehc_ushort, <);
					break;
				case ehc_ltui:
					EVM_CMP(ehc_uint, <);
					break;
				case ehc_ltul:
					EVM_CMP(ehc_ulong, <);
					break;
				case ehc_lec:
					EVM_CMP(ehc_char, <=);
					break;
				case ehc_les:
					EVM_CMP(ehc_short, <=);
					break;
				case ehc_lei:
					EVM_CMP(ehc_int, <=);
					break;
				case ehc_lel:
					EVM_CMP(ehc_long, <=);
					break;
				case ehc_lef:
					EVM_CMP(ehc_float, <=);
					break;
				case ehc_led:
					EVM_CMP(ehc_double, <=);
					break;
				case ehc_leuc:
					EVM_CMP(ehc_uchar, <=);
					break;
				case ehc_leus:
					EVM_CMP(ehc_ushort, <=);
					break;
				case ehc_leui:
					EVM_CMP(ehc_uint, <=);
					break;
				case ehc_leul:
					EVM_CMP(ehc_ulong, <=);
					break;
				case ehc_eqc:
					EVM_CMP(ehc_uchar, ==);
					break;
				case ehc_eqs:
					EVM_CMP(ehc_ushort, ==);
					break;
				case ehc_eqi:
					EVM_CMP(ehc_uint, ==);
					break;
				case ehc_eql:
					EVM_CMP(ehc_ulong, ==);
					break;
				case ehc_eqf:
					EVM_CMP(ehc_float, ==);
					break;
				case ehc_eqd:
					EVM_CMP(ehc_double, ==);
					break;
				case ehc_nec:
					EVM_CMP(ehc_uchar, !=);
					break;
				case ehc_nes:
					EVM_CMP(ehc_ushort, !=);
					break;
				case ehc_nei:
					EVM_CMP(ehc_uint, !=);
					break;
				case ehc_nel:
					EVM_CMP(ehc_ulong, !=);
					break;
				case ehc_nef:
					EVM_CMP(ehc_float, !=);
					break;
				case ehc_ned:
					EVM_CMP(ehc_double, !=);
					break;
				case ehc_gec:
					EVM_CMP(ehc_char, >=);
					break;
				case ehc_ges:
					EVM_CMP(ehc_short, >=);
					break;
				case ehc_gei:
					EVM_CMP(ehc_int, >=);
					break;
				case ehc_gel:
					EVM_CMP(ehc_long, >=);
					break;
				case ehc_gef:
					EVM_CMP(ehc_float, >=);
					break;
				case ehc_ged:
					EVM_CMP(ehc_double, >=);
					break;
				case ehc_geuc:
					EVM_CMP(ehc_uchar, >=);
					break;
				case ehc_geus:
					EVM_CMP(ehc_ushort, >=);
					break;
				case ehc_geui:
					EVM_CMP(ehc_uint, >=);
					break;
				case ehc_geul:
					EVM_CMP(ehc_ulong, >=);
					break;
				case ehc_gtc:
					EVM_CMP(ehc_char, >);
					break;
				case ehc_gts:
					EVM_CMP(ehc_short, >);
					break;
				case ehc_gti:
					EVM_CMP(ehc_int, >);
					break;
				case ehc_gtl:
					EVM_CMP(ehc_long, >);
					break;
				case ehc_gtf:
					EVM_CMP(ehc_float, >);
					break;
				case ehc_gtd:
					EVM_CMP(ehc_double, >);
					break;
				case ehc_gtuc:
					EVM_CMP(ehc_uchar, >);
					break;
				case ehc_gtus:
					EVM_CMP(ehc_ushort, >);
					break;
				case ehc_gtui:
					EVM_CMP(ehc_uint, >);
					break;
				case ehc_gtul:
					EVM_CMP(ehc_ulong, >);
					break;
				case ehc_c2s:
					EVM_CVT(ehc_char, ehc_short);
					break;
				case ehc_s2i:
					EVM_CVT(ehc_short, ehc_int);
					break;
				case ehc_i2l:
					EVM_CVT(ehc_int, ehc_long);
					break;
				case ehc_l2i:
					EVM_CVT(ehc_long, ehc_int);
					break;
				case ehc_i2s:
					EVM_CVT(ehc_int, ehc_short);
					break;
				case ehc_s2c:
					EVM_CVT(ehc_short, ehc_char);
					break;
				case ehc_uc2us:
					EVM_CVT(ehc_uchar, ehc_ushort);
					break;
				case ehc_us2ui:
					EVM_CVT(ehc_ushort, ehc_uint);
					break;
				case ehc_ui2ul:
					EVM_CVT(ehc_uint, ehc_ulong);
					break;
				case ehc_ul2ui:
					EVM_CVT(ehc_ulong, ehc_uint);
					break;
				case ehc_ui2us:
					EVM_CVT(ehc_uint, ehc_ushort);
					break;
				case ehc_us2uc:
					EVM_CVT(ehc_ushort, ehc_uchar);
					break;
				case ehc_f2d:
					EVM_CVT(ehc_float, ehc_double);
					break;
				case ehc_d2f:
					EVM_CVT(ehc_double, ehc_float);
					break;
				case ehc_f2i:
					EVM_CVT(ehc_float, ehc_int);
					break;
				case ehc_i2f:
					EVM_CVT(ehc_int, ehc_float);
					break;
				case ehc_ui2f:
					EVM_CVT(ehc_uint, ehc_float);
					break;
				case ehc_f2ui:
					EVM_CVT(ehc_float, ehc_uint);
					break;
				case ehc_d2l:
					EVM_CVT(ehc_double, ehc_long);
					break;
				case ehc_l2d:
					EVM_CVT(ehc_long, ehc_double);
					break;
				case ehc_ul2d:
#ifdef MSVC
					EVM_CVT_U2D();
#else
					EVM_CVT(ehc_ulong, ehc_double);
#endif
					break;
				case ehc_d2ul:
					EVM_CVT(ehc_double, ehc_ulong);
					break;
				case ehc_lshuc:
					EVM_SH(ehc_uchar,<<=);
					break;
				case ehc_lshus:
					EVM_SH(ehc_ushort,<<=);
					break;
				case ehc_lshui:
					EVM_SH(ehc_uint,<<=);
					break;
				case ehc_lshul:
					EVM_SH(ehc_ulong,<<=);
					break;
				case ehc_rshc:
					EVM_SH(ehc_char,>>=);
					break;
				case ehc_rshs:
					EVM_SH(ehc_short,>>=);
					break;
				case ehc_rshi:
					EVM_SH(ehc_int,>>=);
					break;
				case ehc_rshl:
					EVM_SH(ehc_long,>>=);
					break;
				case ehc_rshuc:
					EVM_SH(ehc_uchar,>>=);
					break;
				case ehc_rshus:
					EVM_SH(ehc_ushort,>>=);
					break;
				case ehc_rshui:
					EVM_SH(ehc_uint,>>=);
					break;
				case ehc_rshul:
					EVM_SH(ehc_ulong,>>=);
					break;
				case ehc_anduc:
					EVM_AND(ehc_uchar);
					break;
				case ehc_andus:
					EVM_AND(ehc_ushort);
					break;
				case ehc_andui:
					EVM_AND(ehc_uint);
					break;
				case ehc_andul:
					EVM_AND(ehc_ulong);
					break;
				case ehc_oruc:
					EVM_OR(ehc_uchar);
					break;
				case ehc_orus:
					EVM_OR(ehc_ushort);
					break;
				case ehc_orui:
					EVM_OR(ehc_uint);
					break;
				case ehc_orul:
					EVM_OR(ehc_ulong);
					break;
				case ehc_notuc:
					EVM_NOT(ehc_uchar);
					break;
				case ehc_notus:
					EVM_NOT(ehc_ushort);
					break;
				case ehc_notui:
					EVM_NOT(ehc_uint);
					break;
				case ehc_notul:
					EVM_NOT(ehc_ulong);
					break;
				case ehc_xoruc:
					EVM_XOR(ehc_uchar);
					break;
				case ehc_xorus:
					EVM_XOR(ehc_ushort);
					break;
				case ehc_xorui:
					EVM_XOR(ehc_uint);
					break;
				case ehc_xorul:
					EVM_XOR(ehc_ulong);
					break;
				}
			}catch(...)
			{
				ret = EVM_OTHER_EXCEPT;
			}
			if(nPowerOff && nTerminal==0)
			{
				nTerminal = 1;
				ret = EVM_POWER_OFF;
			}
			if(ret)
				OnSignal(ret, this, &reg);
		}
	}
}

FOCP_END();
