
#include "Service.hpp"
#include "LongJmpWin32.hpp"

#ifdef WINDOWS
#include <malloc.h>
#else
#include <alloca.h>
#endif

FOCP_BEGIN();

static CMutex g_oServiceScheduleMutex;
static CService* g_pService = NULL;

//���ļ��еĺ���ֻ����ȫ�ֺ���ʵ��

void CService::Switch(CService * pService)
{
	CServiceManager* pServiceManager = CServiceManager::GetInstance();

	if(pServiceManager->m_oStackAttr.bCallDown)
		pService->m_pStackBottom = (ulong*)(void*)(&pService) + 1;
	else
		pService->m_pStackBottom = (ulong*)(void*)(&pService);

	pService->m_pCurBreak = pService->m_pServiceBreak;

	//�ú�������������ϴ���ֻ����һ���ֲ�������
#if defined(MSVC)
	if(FocpSetJmp(pService->m_pCurBreak->oBreak))
#else
	if(setjmp(pService->m_pCurBreak->oBreak))
#endif
	{
		//resume
		pService = g_pService;
		g_oServiceScheduleMutex.Leave();
		pServiceManager = CServiceManager::GetInstance();//��Ҫ��ȡ
		if(pServiceManager->m_oStackAttr.bCallDown)//�ָ�ջ
			ArrayCopy(pService->m_pStackBottom, pService->m_pStackBackup, pService->m_pStackTop - pService->m_pStackBottom);
		else
			ArrayCopy(pService->m_pStackTop, pService->m_pStackBackup, pService->m_pStackBottom - pService->m_pStackTop);
		pService->m_pCurBreak = NULL;
		CBufferManager::GetInstance()->DeAllocateBuffer(pService->m_pStackBackup);
	}
	else
	{
		//suspend
		ulong nStackLen;
		if(pServiceManager->m_oStackAttr.bCallDown)
			nStackLen = pService->m_pStackTop - pService->m_pStackBottom;
		else
			nStackLen = pService->m_pStackBottom - pService->m_pStackTop;
		pService->m_pStackBackup = (ulong*)CBufferManager::GetInstance()->AllocateBuffer(nStackLen*sizeof(ulong));
		if(pServiceManager->m_oStackAttr.bCallDown)
			ArrayCopy(pService->m_pStackBackup, pService->m_pStackBottom, nStackLen);
		else
			ArrayCopy(pService->m_pStackBackup, pService->m_pStackTop, nStackLen);
#if defined(MSVC)
		FocpLongJmp(pService->m_pTopBreak->oBreak, 1);
#else
		longjmp(pService->m_pTopBreak->oBreak, 1);
#endif
	}
}

void CServiceManager::Run(CServiceManager* pServiceManager, uint32 nStep)
{
	CService* pService;
	//�ú����в������������ֲ����������Էŵ������Ա�ж��塣

#if defined(MSVC)
	FocpSetJmp(pServiceManager->m_pTopBreak->oBreak);
#else
	setjmp(pServiceManager->m_pTopBreak->oBreak);
#endif
	while((pService = pServiceManager->m_oReadyQueue.Pop()))
	{
		if(pService->m_pCurBreak)
		{
			g_oServiceScheduleMutex.Enter();
			g_pService = pService;
#if defined(MSVC)
			FocpLongJmp(pService->m_pCurBreak->oBreak, 1);
#else
			longjmp(pService->m_pCurBreak->oBreak, 1);
#endif
		}
		else
		{
			pService->m_pTopBreak = pServiceManager->m_pTopBreak;
			if(pServiceManager->m_oStackAttr.bCallDown)
				pService->m_pStackTop = (ulong*)(void*)&pService;
			else
				pService->m_pStackTop = ((ulong*)(void*)&pService)+1;
			if(nStep == 0)
			{
				if(!pServiceManager->m_bResult)
					continue;
				if(!pService->Initialize())
				{
					pServiceManager->m_bResult = false;
					pService->StateNotice(true);
				}
			}
			else if(nStep == 1)
			{
				if(!pServiceManager->m_bResult)
					continue;
				if(!pService->Start())
				{
					pServiceManager->m_bResult = false;
					pService->StateNotice(true);
				}
			}
			else
			{
				pService->Stop();
				pService->StateNotice(true);
			}
		}
	}
}

FOCP_END();

