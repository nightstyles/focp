
#include "AFC.hpp"

#ifndef _AFW_LOG_CHOOSER_HPP_
#define _AFW_LOG_CHOOSER_HPP_

FOCP_BEGIN();

struct CLogMsg
{
	CString oHost;//0
	double nDate;//1
	CString oAppName;//2
	CString oModuleName;//3
	CString oFuncName;//4
	CString oFile;//5
	CString oInfo;//6
	uint32 nLevel, nDMN, nAIN, nLine;//789A
};

class CLogChoice
{
protected:
	uint32 m_nType;
	bool m_bNot;

public:
	CLogChoice(uint32 nType);
	virtual ~CLogChoice();

	void SetNot(bool bNot);
	void GetNot(bool &bNot);

	virtual bool Check(CLogMsg& oLog) = 0;
	virtual void GetChooser(CString& oChooser) = 0;
};

class CLogIntChoice: public CLogChoice
{
private:
	uint32 m_nValue;

public:
	CLogIntChoice(uint32 nType);
	virtual ~CLogIntChoice();

	void SetValue(uint32 nValue);
	void GetValue(uint32 &nValue);

	virtual bool Check(CLogMsg& oLog);
	virtual void GetChooser(CString& oChooser);
};

class CLogTimeChoice: public CLogChoice
{
private:
	bool m_bIncBeg, m_bIncEnd;
	double m_nBegin, m_nEnd;

public:
	CLogTimeChoice(uint32 nType);
	virtual ~CLogTimeChoice();

	void SetBegin(double nBegin, bool bInc);
	void GetBegin(double &nBegin, bool &bInc);

	void SetEnd(double nEnd, bool bInc);
	void GetEnd(double &nEnd, bool &bInc);

	virtual bool Check(CLogMsg& oLog);
	virtual void GetChooser(CString& oChooser);
};

class CLogTextChoice: public CLogChoice
{
private:
	CString m_nValue;

public:
	CLogTextChoice(uint32 nType);
	virtual ~CLogTextChoice();

	void SetValue(const CString &nValue);
	void GetValue(CString &nValue);

	virtual bool Check(CLogMsg& oLog);
	virtual void GetChooser(CString& oChooser);

private:
	void GetCaption(CString &oVal);
};

class CLogChoiceUnit
{
private:
	CLogChoice* m_pChoiceUnit[11];

public:
	CLogChoiceUnit();
	~CLogChoiceUnit();

	CLogChoice* GetChoice(uint32 nIdx, bool bCreate=false);
	bool Check(CLogMsg& oLog);
	void GetChooser(CString& oChooser);
	bool Empty();
};

class CLogChoiceGroup
{
private:
	CSingleList<CLogChoiceUnit*> m_oUnits;

public:
	CLogChoiceGroup();
	~CLogChoiceGroup();

	CLogChoice* NewChoice(uint32 nType);
	bool Check(CLogMsg& oLog);
	void GetChooser(CString& oChooser);
	bool Empty();
};

class CLogChooser
{
private:
	CMutex m_oMutex;
	CSingleList<CLogChoiceGroup*> m_oGroups;

public:
	CLogChooser();
	virtual ~CLogChooser();

	//(1)支持and与or以构建复杂的条件
	//(2)支持=与!=操作
	//(3)值用单引号扩起（两个连续单引号表示一个单引号值）
	//(4)日期的值格式为:[a,b];[a,b);(a,b];(a,b);
	//(5)Host,Date,App,Module,Func,File,Key,Level,DMN,AIN,Line
	void Clear();
	bool AddChooser(const char* sCond);
	void PopChooser();//返回是否还剩有选择器
	void GetChooser(CString& oChooser);
	uint32 GetFilter(const char* &sFilter);

	virtual bool Check(CLogMsg& oLog);

private:
	void AddGroup();
	void AddCond(uint32 nType, bool bNot, const CString& oVal1, const CString &oVal2=CString(), bool bIncBeg=true, bool bIncEnd=true);
	uint32 GetString(const char* &pStr, CString &oValue);
	uint32 GetInt(const char* &pStr, CString &oValue);
	uint32 GetDateTime(const char* &pStr, CString& oVal1, CString &oVal2, bool &bIncBeg, bool &bIncEnd);
	uint32 GetOperator(const char* &pStr, bool &bNot);
	uint32 GetIdentifier(const char* &pStr, CString &oIdentifier);
	void SkipWhiteSpace(const char* &pStr);
	uint32 GetItemType(const CString &oName);
	void RemoveEmptyGroup();
};

FOCP_END();

#endif
