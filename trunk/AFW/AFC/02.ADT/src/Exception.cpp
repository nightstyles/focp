
#include "Exception.hpp"

FOCP_BEGIN();

CException::CException()
{
}

CException::~CException()
{
}

const char* CException::What()const
{
	return "CException";
}

CStdException::CStdException(const char* sInfo)
{
	m_sInfo = sInfo;
}

const char* CStdException::What()const
{
	return m_sInfo;
}

CStdException::CStdException(const CStdException& oSrc)
{
	m_sInfo = oSrc.m_sInfo;
}

CStdException& CStdException::operator=(const CStdException& oSrc)
{
	if(this != &oSrc)
		m_sInfo = oSrc.m_sInfo;
	return *this;
}

FOCP_END();
