
#include "AfcThread.hpp"

#include <string.h>

#if defined(UNIX)
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
static pthread_key_t TlsAlloc()
{
	pthread_key_t key;
	pthread_key_create(&key, NULL);
	return key;
}
static void TlsFree(pthread_key_t dwTlsIndex)
{
	pthread_key_delete(dwTlsIndex);
}
static void TlsSetValue(pthread_key_t dwTlsIndex, void* lpTlsValue)
{
	pthread_setspecific(dwTlsIndex, lpTlsValue);
}
static void* TlsGetValue(pthread_key_t dwTlsIndex)
{
	return pthread_getspecific(dwTlsIndex);
}
#endif

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

CThreadMutex::CThreadMutex()
{
#if defined(WINDOWS)
	InitializeCriticalSection(&m_oMutex);
#endif

#if defined(UNIX)
	pthread_mutexattr_init(&m_oMutex.attr);
	pthread_mutexattr_settype(&m_oMutex.attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_oMutex.mutex, &m_oMutex.attr);
#endif
}

CThreadMutex::~CThreadMutex()
{
#if defined(WINDOWS)
	DeleteCriticalSection(&m_oMutex);
#endif

#if defined(UNIX)
	pthread_mutex_destroy(&m_oMutex.mutex);
	pthread_mutexattr_destroy(&m_oMutex.attr);
#endif
}

void CThreadMutex::Enter()
{
#if defined(WINDOWS)
	EnterCriticalSection(&m_oMutex);
#endif

#if defined(UNIX)
	pthread_mutex_lock(&m_oMutex.mutex);
#endif
}

void CThreadMutex::Leave()
{
#if defined(WINDOWS)
	LeaveCriticalSection(&m_oMutex);
#endif

#if defined(UNIX)
	pthread_mutex_unlock(&m_oMutex.mutex);
#endif
}

CRecursiveTest::CRecursiveTest()
{
	m_nKey = TlsAlloc();
}

CRecursiveTest::~CRecursiveTest()
{
	TlsFree(m_nKey);
	CRbTreeNode* idx = m_oSet.First();
	CRbTreeNode* end = m_oSet.End();
	for(; idx!=end; idx=m_oSet.GetNext(idx))
	{
		CRbTree<void*>* &pTree = m_oSet.GetItem(idx);
		delete pTree;
	}
}

bool CRecursiveTest::IsRecursive(void* pData)
{
	CRbTree<void*>* pContext = (CRbTree<void*>*)TlsGetValue(m_nKey);
	if(pContext == NULL)
	{
		pContext = new CRbTree<void*>;
		TlsSetValue(m_nKey, pContext);
		m_oMutex.Enter();
		m_oSet.Insert(pContext);
		m_oMutex.Leave();
	}
	CRbTreeNode* idx = pContext->Find(pData);
	bool bRet = (idx != pContext->End());
	if(bRet == false)
		pContext->Insert(pData);
	return bRet;
}

void CRecursiveTest::UnRecursive(void* pData)
{
	CRbTree<void*>* pContext = (CRbTree<void*>*)TlsGetValue(m_nKey);
	if(pContext)
	{
		CRbTreeNode* idx = pContext->Find(pData);
		if(idx != pContext->End())
			pContext->Remove(idx);
	}
}

CString::CString()
{
	m_sStr = NULL;
}

CString::CString(const CString& oSrc)
{
	m_sStr = NULL;
	operator=(oSrc.m_sStr);
}

CString::~CString()
{
	if(m_sStr)
	{
		delete[] m_sStr;
		m_sStr = NULL;
	}
}

bool CString::Empty()
{
	return (m_sStr == NULL);
}

const char* CString::GetStr()
{
	if(m_sStr)
		return m_sStr;
	return "";
}

CString& CString::operator=(const char* s)
{
	if(s != m_sStr)
	{
		char* r = NULL;
		if(s)
		{
			r = new char[strlen(s)+1];
			strcpy(r, s);
		}
		if(m_sStr)
			delete[] m_sStr;
		m_sStr = r;
	}
	return *this;
}

CString& CString::operator+=(const char* s)
{
	if(s)
	{
		uint32 nLen = strlen(s);
		if(m_sStr)
			nLen += strlen(m_sStr);
		char* r = new char[nLen+1];
		if(m_sStr)
			strcpy(r, m_sStr);
		else
			r[0] = '\0';
		strcat(r, s);
		if(m_sStr)
			delete[] m_sStr;
		m_sStr = r;
	}
	return *this;
}

FOCP_DETAIL_END();

FOCP_END();
