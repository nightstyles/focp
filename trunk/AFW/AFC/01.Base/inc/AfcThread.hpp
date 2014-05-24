
#include "AfcBase.hpp"
#include "RbTree.hpp"

#ifdef WINDOWS
#include <windows.h>
typedef CRITICAL_SECTION CRITICAL_SECTION_UX;
#endif

#ifdef UNIX
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
struct CRITICAL_SECTION_UX
{
	pthread_mutexattr_t attr;
	pthread_mutex_t mutex;
};
#endif

#ifndef __AFC_THREAD_HPP_
#define __AFC_THREAD_HPP_

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

class CThreadMutex
{
private:
	CRITICAL_SECTION_UX m_oMutex;

public:
	CThreadMutex();
	~CThreadMutex();
	void Enter();
	void Leave();
};

class CRecursiveTest
{
private:
#if defined(WINDOWS)
	DWORD m_nKey;
#endif
#if defined(UNIX)
	pthread_key_t m_nKey;
#endif
	CRbTree<CRbTree<void*>*> m_oSet;
	CThreadMutex m_oMutex;

public:
	CRecursiveTest();
	~CRecursiveTest();
	bool IsRecursive(void* pData);
	void UnRecursive(void* pData);
};

class CString
{
private:
	char* m_sStr;

public:
	CString();
	CString(const CString& oSrc);
	~CString();
	bool Empty();
	const char* GetStr();
	CString& operator=(const char* s);
	CString& operator+=(const char* s);
};

FOCP_DETAIL_END();

FOCP_END();

#endif
