
#include "EvmDef.hpp"

#ifndef _EVM_CPU_HPP_
#define _EVM_CPU_HPP_

FOCP_BEGIN();

enum
{
	EVM_INVALID_ADDRESS = 1,
	EVM_LACK_MEMORY,
	EVM_DIVDE_ZERO,
	EVM_MEMORY_BEYOND,
	EVM_POWER_OFF,
	EVM_OTHER_EXCEPT
};

struct CEvmSegment
{
	ehc_char *IS, *DS, *CS, *SS;
};

struct CEvmRegister
{
	ehc_void* CP;
	ehc_char* IS;
	ehc_char* DS;
	ehc_char* CS;
	ehc_char* LS;
	ehc_char* AS;
	ehc_char* TS;
	ehc_char* SS;
	ehc_uint* PC;
	struct CEvmRegister *FP, *pNext;
};

struct CEvmCaseItem
{
	ehc_uint nCount;
	ehc_uint nStart;
	ehc_uint nDefault;
};

#ifndef EVM_SYMBOL_ITEM
#define EVM_SYMBOL_ITEM
struct CEvmSymbolItem
{
	ehc_char* name;//符号名
	ehc_char* addr;//符号地址,外部变量地址为空
	ehc_void* file;//外部文件指针
	ehc_uint type; //符号类型
	ehc_uint opt;  //符号属性
	ehc_uint idx;  //外部变量地址索引
};
#endif

typedef ehc_void (*FOnHost)(ehc_char* pArg, ehc_void*, ehc_void*);
typedef ehc_void (*FSignal)(ehc_uint nCode, ehc_void*, ehc_void*);

class EVM_API CEvmCpu: public CCooperateFunction
{
private:
	CMutex m_oMutex;
	CEvent m_oEvent;
	CBaseSingleList<CEvmRegister> m_oReadyQueue;
	CThreadPool m_oCpus;
	FSignal OnSignal;

public:
	CEvmCpu();
	~CEvmCpu();

	static CEvmCpu* GetInstance();

	void Initialzie(FSignal OnSignal, ehc_uint nCpuNum);
	void Cleanup();

	bool Start();
	void Stop(bool bBlock=true);

	ehc_void Resume(CEvmRegister* pRegister);

private:
	CEvmRegister* Pop();

protected:
	virtual void MainProc(CCooperator* pCooperator, bool &bRunning);
};

FOCP_END();

#endif
