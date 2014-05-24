
#include "AdtDef.hpp"

#ifndef _ADT_EXCEPTION_HPP_
#define _ADT_EXCEPTION_HPP_

FOCP_BEGIN();

class ADT_API CException
{
public:
	CException();
	virtual ~CException();
	virtual const char* What() const;
};

class ADT_API CStdException: public CException
{
private:
	const char* m_sInfo;

public:
	CStdException(const char* sInfo);
	CStdException(const CStdException& oSrc);

	CStdException& operator=(const CStdException& oSrc);
	virtual const char* What() const;
};

FOCP_END();

#define FOCP_EXCEPTION_CLASS(ExceptionClass) \
class C##ExceptionClass: public FOCP_NAME::CException \
{ \
public: \
	virtual const char* What() const \
	{\
		return FOCP_STRING_DEFINE(C##ExceptionClass);\
	}\
}

#ifndef FOCP_ALLOW_EXPTION
#define FocpThrow(ExceptionName) FocpAbort((FOCP_STRING_DEFINE(C##ExceptionName)))
#define FocpFuncThrow(FuncName) FocpAbort((FOCP_STRING_DEFINE(FuncName)))
#else
#define FocpThrow(ExceptionName) \
	do{ \
		const char* sInfo = FOCP_STRING_DEFINE(C##ExceptionName) ": \'" __FILE__ "\': [" FOCP_STRING_DEFINE(__LINE__) "]"; \
		throw FOCP_NAME::CStdException(sInfo); \
	}while(0)

#define FocpFuncThrow(FuncName) \
	do{ \
		const char* sInfo = FOCP_STRING_DEFINE(FuncName) ": \'" __FILE__ "\': [" FOCP_STRING_DEFINE(__LINE__) "]"; \
		throw FOCP_NAME::CStdException(sInfo); \
	}while(0)
#endif


#endif
