
#ifndef _AFC_CONVENTION_HPP_
#define _AFC_CONVENTION_HPP_

#ifdef MSVC
#if (_MSC_VER < 1300)
#pragma warning(disable:4786)
#pragma warning(disable:4251)
#pragma warning(disable:4355)
#pragma warning(disable:4275)
#else
#pragma warning( disable : 4290 )
#pragma warning( disable : 4251 )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4355 )
#endif
#endif

#if defined(WINDOWS) || defined(CYGWIN_NT)
#define FOCP_CALL __stdcall
#else
#define FOCP_CALL
#endif

#if defined(WINDOWS) || defined(CYGWIN_NT)
#define FOCP_EXPORT __declspec(dllexport)
#else
#define FOCP_EXPORT
#endif

#if defined(WINDOWS) || defined(CYGWIN_NT)
#define FOCP_IMPORT __declspec(dllimport)
#else
#define FOCP_IMPORT
#endif

#define FOCP_NAME Focp
#define FOCP_DETAIL_NAME AFC

#define FOCP_BEGIN() namespace FOCP_NAME{
#define FOCP_END() }

#define FOCP_DETAIL_BEGIN() namespace FOCP_DETAIL_NAME{
#define FOCP_DETAIL_END() }

#define FOCP_PRIVATE_BEGIN() namespace{
#define FOCP_PRIVATE_END() }

#define USING_FOCP_SPACE() using namespace FOCP_NAME

#define FOCP_C_BEGIN() extern "C"{
#define FOCP_C_END() }

#ifndef FOCP_STRING_DEFINE
#define FOCP_STRING_DEFINE2(S) #S
#define FOCP_STRING_DEFINE(S) FOCP_STRING_DEFINE2(S)
#endif

#ifndef NULL
#define NULL 0ul
#endif

#define FocpFieldOffset(TType, oFieldName) ((char*)&(((TType*)0x10)->oFieldName)-(char*)0x10)

//需要包含系统库，必须慎重
#include <stdarg.h>
#include <new>
#include <typeinfo>

#include "Integer.hpp"

#ifdef MSVC

#include <stdlib.h> //为了重载VC6的new/delete

inline void* __cdecl operator new(size_t nSize) throw(std::bad_alloc)
{
	if(nSize == 0)
		throw std::bad_alloc();
	void* p = calloc(nSize, 1);
	if(p == NULL)
		throw std::bad_alloc();
	return p;
}

inline void *__cdecl operator new(size_t nSize, const std::nothrow_t&) throw()
{
	if(nSize == 0)
		return NULL;
	return calloc(nSize, 1);
}

inline void* __cdecl operator new[](size_t nSize) throw(std::bad_alloc)
{
	if(nSize == 0)
		throw std::bad_alloc();
	void* p = calloc(nSize, 1);
	if(p == NULL)
		throw std::bad_alloc();
	return p;
}

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t nSize, void *_P)
{
	char * s = (char*)_P;
	for(size_t i=0; i<nSize; ++i)
		s[i] = '\0';
	return (_P);
}
#endif

inline void __cdecl operator delete(void * p) throw()
{
	if(p)
		free(p);
}

inline void __cdecl operator delete[](void * p) throw()
{
	if(p)
		free(p);
}

inline void operator delete(void*p, const std::nothrow_t&)
{
	delete p;
}

#endif

FOCP_BEGIN();

struct CVaList
{
	va_list args;
};

#define VaStart(oVaList, oArg) va_start(oVaList.args, oArg)
inline void VaEnd(CVaList& oVaList)
{
	va_end(oVaList.args);
}
template<typename T> T VaArg(CVaList& oVaList)
{
	return va_arg(oVaList.args, T);
}

template<typename TData> void Swap(TData& oDst, TData&oSrc)
{
	if(&oDst != &oSrc)
	{
		TData oTmp = oDst;
		oDst = oSrc;
		oSrc = oTmp;
	}
};

template<typename TData> struct CSizeOf
{
	enum {SIZE=sizeof(*(TData*)0x10)};
};

template<typename TType1, typename TType2> union CUnion
{
	TType1 oVal1;
	TType2 oVal2;
};

FOCP_END();

#define FOCP_SIZE_OF(type) (FOCP_NAME::CSizeOf<type>::SIZE)
#define MakeAlign(nSize, nAlign) (((nSize) + (nAlign) - 1) & ~((nAlign) - 1))

#endif
