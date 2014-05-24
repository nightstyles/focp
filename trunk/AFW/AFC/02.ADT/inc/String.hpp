
#include "AdtDef.hpp"

#ifndef _ADT_STRING_HPP_
#define _ADT_STRING_HPP_

FOCP_BEGIN();

class ADT_API CString
{
private:
	uint32 m_nSize;
	char* m_pData;
	bool m_bOwned;

public:
	virtual ~CString();
	CString();
	CString(char nCh, uint32 nCount = 1);
	CString(const char* pStr, uint32 nCount=0);
	CString(const CString& oStr);
	CString(const CString& oStr, uint32 nIdx, uint32 nCount=0);

	CString& operator=(const char* pStr);
	CString& operator=(const CString& oStr);
	CString& operator=(char nCh);

	CString& operator+=(const char* pStr);
	CString& operator+=(const CString& oStr);
	CString& operator+=(char nCh);

	CString operator+(const char* pStr);
	CString operator+(const CString& oStr);
	CString operator+(char nCh);

	char& operator[](uint32 nIdx);

	void Clear();
	const char* GetStr() const;

	char* Detach(uint32 &nSize);
	CString& Bind(char* pStr, uint32 nCount=0);

	bool Empty() const;
	void Pack();

	uint32 GetSize() const;//不包括结尾'\0'
	void SetSize(uint32 nNewSize, char nFillChar='\0');

	CString& TrimLeft();
	CString& TrimRight();
	CString& Trim();

	CString& Append(const CString& oStr);
	CString& Append(char nCh, uint32 nCount=1);
	CString& Append(const char* pStr, uint32 nCount=0);

	CString& Assign(const CString& oStr);
	CString& Assign(char nCh, uint32 nCount=1);
	CString& Assign(const char* pStr, uint32 nCount=0);

	CString& Insert(uint32 nIdx, const CString& oStr);
	CString& Insert(uint32 nIdx, char nCh, uint32 nCount=1);
	CString& Insert(uint32 nIdx, const char* pStr, uint32 nCount=0);

	CString& Replace(uint32 nIdx, const CString& oStr);
	CString& Replace(uint32 nIdx, char nCh, uint32 nCount=1);
	CString& Replace(uint32 nIdx, const char* pStr, uint32 nCount=0);

	CString& Remove(uint32 nIdx, uint32 nCount);

	bool Equal(const CString& oStr, bool bSensitive=true) const;
	bool Equal(const char* pStr, bool bSensitive=true, uint32 nCount=0) const;

	int32 Compare(const CString& oStr, bool bSensitive=true) const;
	int32 Compare(const char* pStr, bool bSensitive=true, uint32 nCount=0) const;

	uint32 Find(char nCh, uint32 nIdx=0, bool bSensitive=true) const;
	uint32 Find(const char* pStr, uint32 nIdx=0, bool bSensitive=true) const;
	uint32 Find(const CString& oStr, uint32 nIdx=0, bool bSensitive=true) const;
	uint32 Find(const char* pStr, uint32 nCount, uint32 nIdx=0, bool bSensitive=true) const;

	//如果oDelimiters为空的话，以空白分割
	uint32 GetToken(CString& oToken, uint32 nIdx, const CString& oDelimiters=CString()) const;

	bool IsIdentifierOfC()const;
	void GetCIdentifier(CString& oIdentifier, uint32 &nIdx) const;

	CString& ToCString(bool bWithoutQuote=true);
	CString& ToUpper();
	CString& ToLower();

	void Swap(CString& oSrc);

	//基本字符串操作函数
	static char* StringCopy(char* sDst, const char* sSrc, uint32 nMaxCopy=0);//strcpy,strncpy
	static char* StringCatenate(char* sDst, const char* sSrc, uint32 nMaxCopy=0);//strcat,strncat
	static int32 StringCompare(const char* sLeft, const char* sRight, bool bSensitive=true, uint32 nMaxCmp=0);//strcmp, strcmpi
	static uint32 StringLength(const char* sStr, uint32 nMaxSize=0);//strlen
	static char* CharOfString(const char* sStr, char nCh, bool bReverse=false);//strchr, strrchr
	static char* StringOfString(const char* sStr, const char* sSubStr, bool bSensitive=true);//strstr
	static char* TokenOfString(const char* &sStr, const char* sDelimiters=NULL);//strtok, strspn????????
	static char* SetOfString(const char* sStr, const char* sCharSet, bool bReverse=false);//strcspn
	static const char* SkipSpace(const char* sStr);
	static CString GetCString(const char* sStr, bool bWithoutQuote=true, uint32 nCount=0);
	static void GetCIdentifier(CString& oIdentifier, const char* &sStr);

	//基本字符类型操作函数
	static bool IsAlnum(char c);
	static bool IsAlpha(char c);
	static bool IsControl(char c);
	static bool IsDigit(char c);
	static bool IsGraph(char c);
	static bool IsLower(char c);
	static bool IsUpper(char c);
	static bool IsPrint(char c);
	static bool IsPunct(char c);
	static bool IsSpace(char c);
	static bool IsXdigit(char c);
	static char ToLower(char c);
	static char ToUpper(char c);

	//基本转换函数
	static int32 Atoi(const char* s, const char** sEnd=NULL);
	static int64 Atoi64(const char* s, const char** sEnd=NULL);
	static double Atof(const char* s, const char** sEnd=NULL);

private:
	void TrimDetail(uint32 nMode);//1=TrimLeft, 2=TrimRight, 3=Trim
};

FOCP_END();

#endif //_ADT_STRING_HPP_
