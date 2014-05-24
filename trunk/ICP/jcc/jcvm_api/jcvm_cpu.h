
#ifndef _jc_vm_h_
#define _jc_vm_h_

#include "jcvm_api.h"

typedef struct CJcCaseItem CJcCaseItem;
typedef struct CJcVmWorker CJcVmWorker;

struct CJcCaseItem
{
	jc_uint nCount;
	jc_uint nStart;
	jc_uint nDefault;
};

struct CJcVmWorker
{
	jc_uint bStop;
	jc_uint bRuning;
	jc_void* hEvent;
	CJcVmRegister* pRegister;
};

jc_uint InitializeJcVm(jc_uint nCpuNum);
jc_void PowerOffJcVm();
jc_void HaltJcVmCpu(jc_uint nCpuIdx);
jc_void WaitJcVm();
CJcVmRegister* SwitchJcVm(jc_uint nCpuIdx, CJcVmRegister* pNewRegister);
CJcVmRegister* GetJcVmRegister(jc_uint nCpuIdx);

#endif
