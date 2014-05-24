
#include "Binary.hpp"
#include "String.hpp"
#include "RbTree.hpp"

#ifndef _ADT_FORMATTER_HPP_
#define _ADT_FORMATTER_HPP_

FOCP_BEGIN();

class CFormatter;
class CFormatterMethod;

enum
{
	FOCP_FORMATER_MINUS_FLAG=1,
	FOCP_FORMATER_ZERO_FLAG=2,
	FOCP_FORMATER_PLUS_FLAG=4,
	FOCP_FORMATER_SPACE_FLAG=8,
	FOCP_FORMATER_SHARP_FLAG=16
};

//0：正常，-1：IO异常，-2：无效数据
typedef int32 (*FFormatRead)
(
	CFormatter* pFormatter,
	bool bIgnore,
	int32 nWidth, //default is -1, 应用需要根据不同转换类型修改成相应值
	CVaList& pArgList
);

//-1:IO异常，0：IO中断或满，>0：写入字节数
typedef int32 (* FFormatWrite)
(
	CFormatter* pFormatter,
	uint32 nFlags, //default is 0
	int32 nWidth, //default is 0;
	int32 nPrecision, //default is -1, 应用需要根据不同转换类型修改成相应值
	CVaList& pArgList
);

class ADT_API CFormatterMethod
{
	friend class CFormatter;
private:
	CRbMap<CString, FFormatRead> m_oReadTable;
	CRbMap<CString, FFormatWrite> m_oWriteTable;

public:
	CFormatterMethod(bool bDefault=true);
	virtual ~CFormatterMethod();

	bool SetReader(const char* sType, FFormatRead fRead);
	bool SetWriter(const char* sType, FFormatWrite fWrite);
};

class ADT_API CFormatter
{
	FOCP_FORBID_COPY(CFormatter);

	struct CReadBuffer
	{
		char pBuffer[128];
		CReadBuffer* pNext;
	};

protected:
	int32 m_nAlignPos;
	CFormatterMethod* m_pMehod;
	CReadBuffer* m_pReadBuffer;
	uint32 m_nReadOffset;
	uint32 m_nReadCount, m_nWriteCount;
	int32 m_nSuccess, m_nFlushCount;
	bool m_bLineBuf;

public:
	CFormatter(CFormatterMethod* pMethod=NULL);
	virtual ~CFormatter();


	void SetLineBuf(bool bLine);

	//调整对齐位置，用于二进制读取
	void SetAlignPos(int32 nAlignPos);
	int32 GetAlignPos();

	//格式化读，返回读的项数。
	//-1:IO错误, -3:参数错误
	int32 Scan(const char* sFormat, ...);

	//格式化写，返回写入的总长度。
	//-1:IO错误, -2:IO中断或满, -3:参数错误
	int32 Print(const char* sFormat, ...);

	//格式化读，返回读的项数。
	//-1:IO错误, -3:参数错误
	int32 ScanV(const char* sFormat, CVaList& pArgList);

	//格式化写，返回写入的总长度。
	//-1:IO错误, -2:IO中断或满, -3:参数错误
	int32 PrintV(const char* sFormat, CVaList& pArgList);

	//0：正常，-1：IO异常，-2：读到尾部
	int32 GetChar(char &nChar);

	//如果GetChar返回的字符不识别，可以退回。
	void UnGetChar(char nChar);

	//0：正常，-1：IO异常，-2：文件满或中断
	int32 PutChar(char nChar);

	//返回被读取的字符数量
	uint32 GetReadCount();

	//返回被写入的字符数量
	uint32 GetWriteCount();

protected:
	//0：正常，-1：IO异常，-2：读到尾部
	virtual int32 ReadChar(char &nChar)=0;

	//0：正常，-1：IO异常，-2：文件慢或中断
	virtual int32 WriteChar(char nChar)=0;

	//将缓冲刷新到文件中，PrintV中自动调用。
	virtual void Flush();

private:
	int32 ReadSet(int32 nWidth, const CString &oSet, CString &oStr);
	void GetReadSet(const char* sFormat, uint32 &nFormatLen, CString& oSet);
	FFormatRead GetReadInterface(const char* sFormat, bool &bIgnore, int32& nWidth, uint32 &nFormatLen, CString& oSet);
	FFormatWrite GetWriteInterface(const char* sFormat, uint32& nFlags, int32& nWidth, int32& nPrecision, uint32& nFormatLen);
	void ReadWidth(const char* sFormat, int32& nWidth, uint32 &nWidthLen, bool bSupportStar=false, int32 nDefault=0);
};

class ADT_API CStringFormatter: public CFormatter
{
private:
	CString* m_pString;
	int32 m_nPos;
	bool m_bHaveCr;

public:
	CStringFormatter(CString* pString, int32 nOffset=0, CFormatterMethod* pMethod=NULL);
	virtual ~CStringFormatter();

	int32 GetPos();

protected:
	virtual int32 ReadChar(char &nChar);
	virtual int32 WriteChar(char nChar);
};

class ADT_API CBinaryFormatter: public CFormatter
{
private:
	CBinary* m_pBinary;
	int32 m_nPos;
	bool m_bText, m_bHaveCr;

public:
	CBinaryFormatter(CBinary* pBinary, int32 nOffset=0, bool bText=true, CFormatterMethod* pMethod=NULL);
	virtual ~CBinaryFormatter();

	int32 GetPos();

protected:
	virtual int32 ReadChar(char &nChar);
	virtual int32 WriteChar(char nChar);
};

class ADT_API CStreamFormatter: public CFormatter
{
private:
	CMemoryStream* m_pStream;
	bool m_bText, m_bHaveCr;

public:
	CStreamFormatter(CMemoryStream* pStream, int32 nOffset=0, bool bText=true, CFormatterMethod* pMethod=NULL);
	virtual ~CStreamFormatter();

	int32 GetPos();

protected:
	virtual int32 ReadChar(char &nChar);
	virtual int32 WriteChar(char nChar);
};


ADT_API int32 StringScanEx(const char* pBuf, uint32 nBufLen, const char* sFormat, ...);
ADT_API int32 StringPrintEx(char* pBuf, uint32 nBufLen, const char* sFormat, ...);
ADT_API int32 StringScanExV(const char* pBuf, uint32 nBufLen, const char* sFormat, CVaList& pArgList);
ADT_API int32 StringPrintExV(char* pBuf, uint32 nBufLen, const char* sFormat, CVaList& pArgList);

ADT_API int32 StringScan(const char* pBuf, const char* sFormat, ...);
ADT_API int32 StringPrint(char* pBuf, const char* sFormat, ...);
ADT_API int32 StringScanV(const char* pBuf, const char* sFormat, CVaList& pArgList);
ADT_API int32 StringPrintV(char* pBuf, const char* sFormat, CVaList& pArgList);

class ADT_API CFormatString: public CString, public CStringFormatter
{
public:
	CFormatString(CFormatterMethod* pMethod=NULL);
	CFormatString(char nCh, uint32 nCount = 1, CFormatterMethod* pMethod=NULL);
	CFormatString(const char* pStr, uint32 nCount=0, CFormatterMethod* pMethod=NULL);
	CFormatString(const CString& oStr, CFormatterMethod* pMethod=NULL);
	CFormatString(const CString& oStr, uint32 nIdx, uint32 nCount=0, CFormatterMethod* pMethod=NULL);
	virtual ~CFormatString();
};

class ADT_API CFormatBinary: public CBinary, public CBinaryFormatter
{
public:
	CFormatBinary(bool bText=true, CFormatterMethod* pMethod=NULL);
	CFormatBinary(const CBinary& oSrc, bool bText=true, CFormatterMethod* pMethod=NULL);
	CFormatBinary(uint32 nBufSize, bool bText=true, CFormatterMethod* pMethod=NULL);
	CFormatBinary(uint8* pData, uint32 nDataLen, bool bText=true, CFormatterMethod* pMethod=NULL);
	CFormatBinary(const CBinary& oSrc, uint32 nOff, uint32 nSize, bool bText=true, CFormatterMethod* pMethod=NULL);
	virtual ~CFormatBinary();
};

FOCP_END();

#endif //_ADT_FORMATTER_HPP_
