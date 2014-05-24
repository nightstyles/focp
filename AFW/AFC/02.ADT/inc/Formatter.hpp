
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

//0��������-1��IO�쳣��-2����Ч����
typedef int32 (*FFormatRead)
(
	CFormatter* pFormatter,
	bool bIgnore,
	int32 nWidth, //default is -1, Ӧ����Ҫ���ݲ�ͬת�������޸ĳ���Ӧֵ
	CVaList& pArgList
);

//-1:IO�쳣��0��IO�жϻ�����>0��д���ֽ���
typedef int32 (* FFormatWrite)
(
	CFormatter* pFormatter,
	uint32 nFlags, //default is 0
	int32 nWidth, //default is 0;
	int32 nPrecision, //default is -1, Ӧ����Ҫ���ݲ�ͬת�������޸ĳ���Ӧֵ
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

	//��������λ�ã����ڶ����ƶ�ȡ
	void SetAlignPos(int32 nAlignPos);
	int32 GetAlignPos();

	//��ʽ���������ض���������
	//-1:IO����, -3:��������
	int32 Scan(const char* sFormat, ...);

	//��ʽ��д������д����ܳ��ȡ�
	//-1:IO����, -2:IO�жϻ���, -3:��������
	int32 Print(const char* sFormat, ...);

	//��ʽ���������ض���������
	//-1:IO����, -3:��������
	int32 ScanV(const char* sFormat, CVaList& pArgList);

	//��ʽ��д������д����ܳ��ȡ�
	//-1:IO����, -2:IO�жϻ���, -3:��������
	int32 PrintV(const char* sFormat, CVaList& pArgList);

	//0��������-1��IO�쳣��-2������β��
	int32 GetChar(char &nChar);

	//���GetChar���ص��ַ���ʶ�𣬿����˻ء�
	void UnGetChar(char nChar);

	//0��������-1��IO�쳣��-2���ļ������ж�
	int32 PutChar(char nChar);

	//���ر���ȡ���ַ�����
	uint32 GetReadCount();

	//���ر�д����ַ�����
	uint32 GetWriteCount();

protected:
	//0��������-1��IO�쳣��-2������β��
	virtual int32 ReadChar(char &nChar)=0;

	//0��������-1��IO�쳣��-2���ļ������ж�
	virtual int32 WriteChar(char nChar)=0;

	//������ˢ�µ��ļ��У�PrintV���Զ����á�
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
