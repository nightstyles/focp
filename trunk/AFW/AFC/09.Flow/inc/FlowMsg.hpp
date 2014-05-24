
#include "FlowDef.hpp"
#include "../../08.Rule/Rule.hpp"

#ifndef _FLW_MSG_HPP_
#define _FLW_MSG_HPP_

FOCP_BEGIN();

class CMsgFieldType;
class CMsgFieldDef;
class CMsgStruct;
class CMsgVectorType;
class CMsgField;
class CMsgRecord;
class CMsgVector;
class CMsgSystem;

enum
{
    FLW_NULL,
    FLW_INT8, FLW_UINT8,
    FLW_INT16, FLW_UINT16,
    FLW_INT32, FLW_UINT32,
    FLW_INT64, FLW_UINT64,
    FLW_FLOAT32, FLW_FLOAT64,
    FLW_DATE, FLW_TIME, FLW_DATETIME,
    FLW_STRING, FLW_VSTRING,
    FLW_STRUCT
};

class FLW_API CMsgFieldType: public CRuleFileInfo
{
public:
    CMsgFieldType(CMsgSystem* pMsgSystem);
    virtual ~CMsgFieldType();

    virtual uint32 GetType() const;
    virtual uint32 GetSize() const;
    virtual const char* GetName() const;

    static const char* GetTypeName(uint32 nType);

protected:
    void Register(CMsgSystem* pMsgSystem);
};

class FLW_API CMsgFieldDef
{
    friend class CMsgStruct;
private:
	CString m_oName;
	CString m_oDefault;
	CMsgFieldType* m_pType;
	uint32 m_nOffset;
	uint32 m_nMaxLen;//仅针对字符串类型
	uint32 m_nUnits;//如果为0表示为非向量，否则为向量
	CMsgStruct* m_pStruct;
	bool m_bMandatory;

    void GetFieldAttr(const char* sStructName, CString& oAttr) const;

public:
    CMsgFieldDef(CMsgFieldType* pType, const char* sFieldName);
    ~CMsgFieldDef();

    const CMsgFieldType* GetType() const;
    const char* GetName() const;
    uint32 GetOffset() const;

    void SetDefault(const char* sDefault);
    const char* GetDefault() const;

    void SetMandatory(bool bMandatory=true);
    bool IsMandatory() const;

    void SetVector(uint32 nSize);
    uint32 GetVector() const;

    void SetMaxLen(uint32 nLen);
    uint32 GetMaxLen() const;

    const char* GetStructName()const;

    void Missing(const char* pAddInfo) const;
};

class FLW_API CMsgStruct: public CMsgFieldType
{
    friend class CMsgSystem;
private:
    CString m_oName;
    CVector< CAutoPointer<CMsgFieldDef> > m_oFields;
    uint32 m_nSize;
    bool m_bUnion, m_bImplemented;

    void CreateCppStructCode(CFileFormatter& oFmt, bool bHpp) const;
    void CreateCppMsgCode(CFileFormatter& oFmt, uint32 nMsgId, bool bHpp) const;

public:
    CMsgStruct(CMsgSystem* pMsgSystem, const char* sName, bool bUnion);
    virtual ~CMsgStruct();

    bool IsUnion() const;

    virtual uint32 GetType() const;
    virtual uint32 GetSize() const;
    virtual const char* GetName() const;

    uint32 GetFieldCount() const;
    const CMsgFieldDef* GetField(uint32 nIdx) const;
    const CMsgFieldDef* FindField(const char* sName) const;
    uint32 GetFieldNo(const char* sName) const;

    bool AddField(CMsgFieldDef* pField);
    bool Implemented() const;
    void FinishDefine();
};

class FLW_API CMsgField
{
    friend class CMsgVector;
    friend class CMsgRecord;
private:
    const CMsgFieldDef* m_pFieldDef;
    uint32 m_nSize;
    uint32* m_pFlag;
    uint32 m_nBit;
    void* m_pData;

    CMsgRecord* m_pRecord;
    CMsgVector* m_pVector;

    void Initialize(const CMsgFieldDef* pFieldDef, uint32 nSize,
                    void* pRecordData, uint32 nFieldNo, uint32 nOffset);//??????

    void Initialize(const CMsgFieldDef* pFieldDef, uint32 nSize, void* pData, bool bInVector);//????????

    bool Write(CMemoryStream & oStream);
    bool Read(CMemoryStream & oStream);

    CMsgField();

public:
    ~CMsgField();

    const CMsgFieldType* GetType() const;

    bool IsVector() const;
    bool IsMandatory() const;
    bool IsNull() const;

    int8 GetInt8() const;
    int16 GetInt16() const;
    int32 GetInt32() const;
    int64 GetInt64() const;
    uint8 GetUInt8() const;
    uint16 GetUInt16() const;
    uint32 GetUInt32() const;
    uint64 GetUInt64() const;
    float GetFloat() const;
    double GetDouble() const;
    char* GetString(uint32 * pStrLen) const;
    CDate GetDate() const;
    CTime GetTime() const;
    CDateTime GetDateTime() const;
    uint32 GetStringSize() const;
    void GetAsString(char * pString) const;

    void SetNull();
    void SetInt8(int8 v);
    void SetInt16(int16 v);
    void SetInt32(int32 v);
    void SetInt64(int64 v);
    void SetUInt8(uint8 v);
    void SetUInt16(uint16 v);
    void SetUInt32(uint32 v);
    void SetUInt64(uint64 v);
    void SetFloat(float v);
    void SetDouble(double v);
    void SetString(const char* v);
    void SetDate(const CDate& oDate);
    void SetTime(const CTime& oTime);
    void SetDateTime(const CDateTime& oDateTime);
    void SetFromString(const char* v);

    CMsgRecord* GetRecord();
    CMsgVector* GetVector();

    static void DumpLevel(CFormatter& oFmt, uint32 nLevel);
    void Dump(CFormatter& oFmt, uint32 nLevel);
    bool Check(bool bAll=false);

    void Clear();

    //interface for auto-code
    void SetNumber(void* pVal);
    void GetNumber(void* pVal);
    void* RefNumber();

private:
    int8& RefInt8();
    int16& RefInt16();
    int32& RefInt32();
    int64& RefInt64();
    uint8& RefUInt8();
    uint16& RefUInt16();
    uint32& RefUInt32();
    uint64& RefUInt64();
    float& RefFloat();
    double& RefDouble();
};

class FLW_API CMsgRecord
{
    friend class CMsgField;
    friend class CMsgSystem;
    friend class CMessage;
private:
    char* m_pData;
    uint32 m_nSize;
    uint32 m_nCount;
    const CMsgStruct* m_pType;
    CMsgField* m_pFieldTable;

private:
    CMsgRecord(const CMsgStruct* pType);

    bool Write(CMemoryStream & oStream);
    bool Read(CMemoryStream & oStream);

public:
    ~CMsgRecord();

    const CMsgStruct* GetStructType() const;

    CMsgField* GetField(uint32 nFieldNo);
    CMsgField* GetField(const char* sFieldName);

    bool IsUnion() const;
    uint32 GetUnionField() const;

    void Dump(CFormatter& oFmt, uint32 nLevel);
    bool Check(bool bAll=false);

    void Clear();
};

class FLW_API CMsgVector
{
    friend class CMsgField;
private:
    uint32 m_nCapacity;
    uint32 m_nUnitSize;
    uint32 m_nCount;
    char* m_pData;
    const CMsgFieldDef* m_pFieldDef;
    CMsgField* m_pFieldTable;

    CMsgVector(const CMsgFieldDef* pFieldDef, uint32 nCapacity);

    bool Write(CMemoryStream & oStream);
    bool Read(CMemoryStream & oStream);

public:
    ~CMsgVector();

    const CMsgFieldType* GetType() const;

    uint32 GetCapacity() const;

    void SetSize(uint32 nSize);
    uint32 GetSize() const;
    CMsgField* GetItem(uint32 nIdx);

    void Dump(CFormatter& oFmt, uint32 nLevel);
    bool Check(bool bAll=false);
};

class FLW_API CMsgSystem
{
    friend class CMsgFieldType;
private:
    CRbMap<CString, CAutoPointer<CMsgFieldType>, CNameCompare> m_oSystemTypes;
    CRbMap<uint32, CMsgStruct*> m_oMsgTypes;
    CMsgStruct m_oAliasTable;

public:
    CMsgSystem();
    ~CMsgSystem();

    static CMsgSystem* GetInstance();

    CMsgFieldType* GetType(const char* sName) const;

    bool Export(uint32 nMsgId, const char* sMsgType);

    CMsgRecord* AllocMsg(uint32 nMsgId);
    CMsgRecord* Parse(CMemoryStream& oStream, uint32 &nMsgId, bool bIncHead=true);
    bool Pack(CMemoryStream& oStream, CMsgRecord* pMsg, uint32 nMsgId, bool bBuildHead=true);

    uint32 Compile(CFile& oErrFile, CFile& oLangSyntax, CFile &oMsgSyntax, uint32 &nWarning);

    void CreateCppCode(CFile& oHppFile, CFile& oCppFile, const char* sProtocolName);

private:
    void RegType(CMsgFieldType* pType);
    CMsgStruct* GetMsgType(uint32 nMsgId) const;
    bool GetMsgId(CMsgStruct* pStruct, uint32 &nMsgId);
    static void CreateStringConst(CRuleStack &oStack, CToken* pConst, CString& oStr);
    static void CreateIntConst(CRuleStack &oStack, CToken* pConst, uint64 &ul, uint32 &nConstBits, bool bSigned);
    static void CreateFloatConst(CRuleStack &oStack, double& d, CToken* pConst);
    static void CreateStruct(CRuleStack &oStack);
    static void CreateMessage(CRuleStack &oStack);
    static void CloseStruct(CRuleStack &oStack);
    static void CreateField(CRuleStack &oStack);
    static void CreateAlias(CRuleStack &oStack);
};

/*
消息文法：
	文法 := {结构|消息};
	结构 := 'struct|union' 类型名 [ '{' {字段 ';'} '}' ] ';'
		当没有定义结构体时表示向前说明
		不能用于全局的消息定义，只能作为字段类型使用。
	消息 := 'message' 类型名 = 整数 ';'
		整数用以指示MsgId,这里的类型名，必须是结构体名，而非联合体名
	字段 := [mandatory] 类型 字段名 [ '['整数']' ] [=默认值]
		整数，用以指定向量最大单元数，如果不指定，表示不是向量
		默认值，表示为空时的默认值，只有简单类型才允许默认值
		Mandatory、向量、默认值三个设置选项是互斥的
		联合体字段不支持mandatory选项和缺省值设置。
	类型 := int8 | int16 | int32 | int64 | uint8 | uint16 | uint32 | uint64 | float | double |
			string(整数) | vstring(整数) | date | time | datetime | 结构类型 | 消息类型
			string&vstring与数据库的char&varchar类似
*/
/*
版本兼容：
	（1）新增字段必须是可选的或向量类型。
	（2）新版协议定义不能修改字段类型(包括其大小)，也不能删除字段，否则必须冷升级。
	（3）想作废可选字段或向量字段，废弃不用即可，但不能删除。
	（4）想作废必选字段，必须删除字段，并进行冷升级。
如果要使用CreateCppCode自动生成所需代码，建议：
	（1）结构类型名和字段名均取有意义的一个单词或多个单词的组合。
	（2）每个单词多余1个字母，且首字母大写，其它字母小写。
	（3）名字中不要包含下划线
另外：
*/
FLW_API uint32 CompileMsg(CFile& oErrFile, CFile& oLangSyntax, CFile &oMsgSyntax, uint32 &nWarning, CMsgSystem* pSystem=NULL);

//------------------------------------------------------
// interface for auto-code
//------------------------------------------------------
FLW_API void CreateMsgCode(CFile& oHppFile, CFile& oCppFile, const char* sProtocolName, CMsgSystem* pSystem=NULL);

class FLW_API CMsgStructInst
{
    friend class CMessage;
protected:
    CMsgRecord* m_pStruct;

public:
    CMsgStructInst(CMsgRecord* pStruct);
    CMsgStructInst(const CMsgStructInst& oInst);
    ~CMsgStructInst();
    bool Valid() const;
};

class FLW_API CMessage
{
private:
    CMessage(const CMessage& o);
    CMessage& operator=(const CMessage& o);

protected:
    CMsgSystem* m_pMsgSystem;
    uint32 m_nMsgId;
    CMsgStructInst m_oInst;

public:
    ~CMessage();
    CMessage(uint32 nMsgId, CMsgSystem* pMsgSystem = NULL);
    CMessage(CMemoryStream& oStream, CMsgSystem* pMsgSystem = NULL);
	CMessage(uint32 nMsgId, CMemoryStream& oStream, CMsgSystem* pMsgSystem = NULL);

    bool Check();
    bool Pack(CMemoryStream& oStream, bool bBuildHead=true);

    void Dump(CFormatter& oFmt);

    uint32 GetMsgId() const;
    bool Valid() const;

protected:
    void Clear();
};

template<typename TValueType, uint32 nCol, uint32 nMaxLen, bool bOption, uint32 nCapacity> class CMsgValue
{
public:
	class CMsgBaseField
	{
	private:
		CMsgBaseField(const CMsgBaseField& o);
        inline CMsgField* GetField()
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            return pRec->GetField(nCol);
        }

	public:
		inline CMsgBaseField()
		{
		}

        inline CString GetAsString()
        {
        	CMsgField* pField = GetField();
        	uint32 nLen = pField->GetStringSize();
        	CString oRet('A', nLen);
            pField->GetAsString((char*)oRet.GetStr());
            return oRet;
        }

        inline void SetFromString(const char* v)
        {
            GetField()->SetFromString(v);
        }

        inline void SetFromString(const CString& v)
        {
            GetField()->SetFromString(v.GetStr());
        }
	};

    template<typename TData> class CMsgNumber: public CMsgBaseField
    {
    private:
        inline CMsgField* GetField()
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            return pRec->GetField(nCol);
        }
    public:
        inline CMsgNumber()
        {
        }

        inline CMsgNumber<TData>& operator=(const CMsgNumber<TData>& v)
        {
            if(this != &v)
                Set(((CMsgNumber<TData>&)v).Get());
            return *this;
        }

        inline CMsgNumber<TData>& operator=(TData v)
        {
            GetField()->SetNumber(&v);
            return *this;
        }

        inline TData Get()
        {
            TData v;
            GetField()->GetNumber(&v);
            return v;
        }

        inline void Set(TData v)
        {
            GetField()->SetNumber(&v);
        }

        inline operator TData()
        {
            TData v;
            GetField()->GetNumber(&v);
            return v;
        }
    };

    class CMsgString: public CMsgBaseField
    {
    private:
        inline CMsgField* GetField()
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            return pRec->GetField(nCol);
        }

    public:
        inline CMsgString()
        {
        }

        inline CMsgString& operator=(const CMsgString& v)
        {
            if(this != &v)
            {
                uint32 nLen;
                Set(((CMsgString&)v).Get(nLen));
            }
            return *this;
        }

        inline CMsgString& operator=(const char* v)
        {
            GetField()->SetString(v);
            return *this;
        }

        inline CMsgString& operator=(const CString& v)
        {
            GetField()->SetString(v.GetStr());
            return *this;
        }

        inline const char* Get(uint32 &nLen)
        {
            return GetField()->GetString(&nLen);
        }

        inline void Set(const char* v)
        {
            GetField()->SetString(v);
        }

        inline void Set(const CString& v)
        {
            GetField()->SetString(v.GetStr());
        }

        inline operator CString()
        {
            uint32 nLen;
            const char* sStr = GetField()->GetString(&nLen);
            return CString(sStr, nLen);
        }

        inline uint32 GetMaxLen()
        {
            return nMaxLen;
        }
    };

    template<typename TData> class CMsgTime: public CMsgBaseField
    {
    private:
        inline CMsgField* GetField()
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            return pRec->GetField(nCol);
        }

    public:
        inline CMsgTime()
        {
        }

        inline CMsgTime<TData>& operator=(const CMsgTime<TData>& v)
        {
            if(this != &v)
                Set(((CMsgNumber<TData>&)v).Get());
            return *this;
        }

        inline CMsgTime<TData>& operator=(const TData& v)
        {
            GetField()->SetNumber((void*)&v);
            return *this;
        }

        inline TData Get()
        {
            TData v;
            GetField()->GetNumber(&v);
            return v;
        }

        inline void Set(const TData& v)
        {
            GetField()->SetNumber((void*)&v);
        }

        inline operator TData()
        {
            TData v;
            GetField()->GetNumber(&v);
            return v;
        }
    };

    template<typename TData> class CMsgStructVal
    {
    private:
        CMsgStructVal(const CMsgStructVal<TData>& o);

        inline CMsgField* GetField()
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            return pRec->GetField(nCol);
        }

    public:
        inline CMsgStructVal()
        {
        }

        TData Get()
        {
            return TData(GetField()->GetRecord());
        }

        operator TData()
        {
            return TData(GetField()->GetRecord());
        }
    };

	class CMsgBaseOptionVal
	{
	private:
		CMsgBaseOptionVal(const CMsgBaseOptionVal& o);
        inline CMsgField* GetField()
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            return pRec->GetField(nCol);
        }
     public:
		inline CMsgBaseOptionVal()
		{
		}

		inline bool IsNull()
		{
			return GetField()->IsNull();
		}

		inline void SetNull()
		{
			GetField()->SetNull();
		}
	};

	template<typename TData> class CMsgOptionNumber: public CMsgNumber<TData>, public CMsgBaseOptionVal
	{
	private:
		CMsgOptionNumber(const CMsgOptionNumber<TData>& o);

     public:
		inline CMsgOptionNumber()
		{
		}

        inline CMsgOptionNumber<TData>& operator=(const CMsgOptionNumber<TData>& v)
        {
			CMsgNumber<TData>::operator=(v);
			return *this;
        }

        inline CMsgOptionNumber<TData>& operator=(TData v)
        {
			CMsgNumber<TData>::operator=(v);
            return *this;
        }
	};

	class CMsgOptionString: public CMsgString, public CMsgBaseOptionVal
	{
	private:
		CMsgOptionString(const CMsgOptionString& o);

     public:
		inline CMsgOptionString()
		{
		}

        inline CMsgOptionString& operator=(const CMsgOptionString& v)
        {
			CMsgString::operator=(v);
            return *this;
        }

        inline CMsgOptionString& operator=(const char* v)
        {
			CMsgString::operator=(v);
            return *this;
        }

        inline CMsgOptionString& operator=(const CString& v)
        {
			CMsgString::operator=(v);
            return *this;
        }
	};

	template<typename TData> class CMsgOptionTime: public CMsgTime<TData>, public CMsgBaseOptionVal
	{
	private:
		CMsgOptionTime(const CMsgOptionTime<TData>& o);

     public:
		inline CMsgOptionTime()
		{
		}

        inline CMsgOptionTime<TData>& operator=(const CMsgOptionTime<TData>& v)
        {
			CMsgTime<TData>::operator=(v);
            return *this;
        }

        inline CMsgTime<TData>& operator=(const TData& v)
        {
			CMsgTime<TData>::operator=(v);
            return *this;
        }
	};

	template<typename TData> class CMsgOptionStructVal: public CMsgStructVal<TData>, public CMsgBaseOptionVal
	{
	private:
		CMsgOptionStructVal(const CMsgOptionStructVal<TData>& o);

     public:
		inline CMsgOptionStructVal()
		{
		}
	};

    class CMsgBaseVectorField
    {
    private:
    	CMsgBaseVectorField(const CMsgBaseVectorField& o);
    	CMsgBaseVectorField& operator=(const CMsgBaseVectorField& o);

		inline CMsgField* GetField(uint32 nIdx)
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            CMsgField* pField = pRec->GetField(nCol);
            CMsgVector* pVec = pField->GetVector();
            return pVec->GetItem(nIdx);
        }

	public:
		inline CMsgBaseVectorField()
		{
		}

        inline CString GetAsString(uint32 nIdx)
        {
        	CMsgField* pField = GetField(nIdx);
        	uint32 nLen = pField->GetStringSize();
        	CString oRet('A', nLen);
            pField->GetAsString((char*)oRet.GetStr());
            return oRet;
        }

        inline void SetFromString(uint32 nIdx, const char* v)
        {
            GetField(nIdx)->SetFromString(v);
        }

        inline void SetFromString(uint32 nIdx, const CString& v)
        {
            GetField(nIdx)->SetFromString(v.GetStr());
        }

        inline uint32 GetCapacity()
        {
            return nCapacity;
        }

        inline uint32 GetVectorSize()
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            CMsgField* pField = pRec->GetField(nCol);
            if(pField->IsNull())
                return 0;
            CMsgVector* pVec = pField->GetVector();
            return pVec->GetSize();
        }

        inline void SetVectorSize(uint32 nSize)
        {
        	uint32 nSize2 = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize2 - sizeof(CMsgRecord*));
            CMsgField* pField = pRec->GetField(nCol);
            if(nSize == 0)
                pField->SetNull();
            else
                pField->GetVector()->SetSize(nSize);
        }
    };

    template<typename TData> class CMsgNumberVec: public CMsgBaseVectorField
    {
    private:
        inline CMsgField* GetField(uint32 nIdx)
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            CMsgField* pField = pRec->GetField(nCol);
            CMsgVector* pVec = pField->GetVector();
            return pVec->GetItem(nIdx);
        }

        CMsgNumberVec(const CMsgNumberVec<TData>& o);
        CMsgNumberVec<TData>& operator=(const CMsgNumberVec<TData>& v);

    public:
        inline CMsgNumberVec()
        {
        }

        inline TData Get(uint32 nIdx)
        {
            TData v;
            GetField(nIdx)->GetNumber(&v);
            return v;
        }

        inline void Set(uint32 nIdx, TData v)
        {
            GetField(nIdx)->SetNumber(&v);
        }

        inline TData& operator[](uint32 nIdx)
        {
            return *(TData*)GetField(nIdx)->RefNumber();
        }
    };

    class CMsgStringVec: public CMsgBaseVectorField
    {
    private:
        inline CMsgField* GetField(uint32 nIdx)
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            CMsgField* pField = pRec->GetField(nCol);
            CMsgVector* pVec = pField->GetVector();
            return pVec->GetItem(nIdx);
        }

    public:
        inline CMsgStringVec()
        {
        }

        inline const char* Get(uint32 nIdx, uint32 &nLen)
        {
            return GetField(nIdx)->GetString(&nLen);
        }

        inline void Set(uint32 nIdx, const char* v)
        {
            GetField(nIdx)->SetString(v);
        }

        inline void Set(uint32 nIdx, const CString& v)
        {
            GetField(nIdx)->SetString(v.GetStr());
        }

        inline CString operator[](uint32 nIdx)
        {
            uint32 nLen;
            const char* sStr = GetField(nIdx)->GetString(&nLen);
            return CString(sStr, nLen);
        }

        inline uint32 GetMaxLen()
        {
            return nMaxLen;
        }
    };

    template<typename TData> class CMsgTimeVec: public CMsgBaseVectorField
    {
    private:
        inline CMsgField* GetField(uint32 nIdx)
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            CMsgField* pField = pRec->GetField(nCol);
            CMsgVector* pVec = pField->GetVector();
            return pVec->GetItem(nIdx);
        }

    public:
        inline CMsgTimeVec()
        {
        }

        inline TData Get(uint32 nIdx)
        {
            TData v;
            GetField(nIdx)->GetNumber(&v);
            return v;
        }

        inline void Set(uint32 nIdx, const TData& v)
        {
            GetField(nIdx)->SetNumber((void*)&v);
        }

        inline TData& operator[](uint32 nIdx)
        {
            return *(TData*)GetField(nIdx)->RefNumber();
        }
    };

    template<typename TData> class CMsgStructValVec
    {
    private:
        inline CMsgField* GetField(uint32 nIdx)
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            CMsgField* pField = pRec->GetField(nCol);
            CMsgVector* pVec = pField->GetVector();
            return pVec->GetItem(nIdx);
        }

        CMsgStructValVec(const CMsgStructValVec<TData>& o);
        CMsgStructValVec<TData>& operator=(const CMsgStructValVec<TData>& v);

    public:
        inline CMsgStructValVec()
        {
        }

        inline TData Get(uint32 nIdx)
        {
            return TData(GetField(nIdx)->GetRecord());
        }

        inline TData operator[](uint32 nIdx)
        {
            return TData(GetField(nIdx)->GetRecord());
        }

        inline uint32 GetCapacity()
        {
            return nCapacity;
        }

        inline uint32 GetVectorSize()
        {
        	uint32 nSize = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize - sizeof(CMsgRecord*));
            CMsgField* pField = pRec->GetField(nCol);
            if(pField->IsNull())
                return 0;
            CMsgVector* pVec = pField->GetVector();
            return pVec->GetSize();
        }

        inline void SetVectorSize(uint32 nSize)
        {
        	uint32 nSize2 = nCol * sizeof(*this);
        	CMsgRecord* pRec = *(CMsgRecord**)((char*)this - nSize2 - sizeof(CMsgRecord*));
            CMsgField* pField = pRec->GetField(nCol);
            if(nSize == 0)
                pField->SetNull();
            else
                pField->GetVector()->SetSize(nSize);
        }
    };

    template<typename TData FOCP_FAKE_DEFAULT_TYPE(S)> struct CMandatoryHelper
    {
        typedef CMsgStructVal<TData> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<int8 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<int8> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<int16 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<int16> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<int32 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<int32> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<int64 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<int64> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<uint8 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<uint8> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<uint16 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<uint16> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<uint32 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<uint32> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<uint64 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<uint64> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<float FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<float> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<double FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumber<double> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<CDate FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgTime<CDate> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<CTime FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgTime<CTime> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<CDateTime FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgTime<CDateTime> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CMandatoryHelper<CString FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgString TValue;
    };

    template<typename TData FOCP_FAKE_DEFAULT_TYPE(S)> struct COptionHelper
    {
        typedef CMsgOptionStructVal<TData> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<int8 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<int8> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<int16 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<int16> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<int32 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<int32> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<int64 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<int64> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<uint8 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<uint8> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<uint16 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<uint16> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<uint32 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<uint32> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<uint64 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<uint64> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<float FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<float> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<double FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionNumber<double> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<CDate FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionTime<CDate> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<CTime FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionTime<CTime> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<CDateTime FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionTime<CDateTime> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct COptionHelper<CString FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgOptionString TValue;
    };

    template<typename TData FOCP_FAKE_DEFAULT_TYPE(S)> struct CVectorHelper
    {
        typedef CMsgStructValVec<TData> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<int8 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<int8> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<int16 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<int16> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<int32 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<int32> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<int64 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<int64> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<uint8 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<uint8> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<uint16 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<uint16> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<uint32 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<uint32> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<uint64 FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<uint64> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<float FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<float> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<double FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgNumberVec<double> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<CDate FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgTimeVec<CDate> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<CTime FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgTimeVec<CTime> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<CDateTime FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgTimeVec<CDateTime> TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CVectorHelper<CString FOCP_FAKE_TYPE_ARG(S)>
    {
        typedef CMsgStringVec TValue;
    };

    template<bool bOption2 FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
    {
        typedef typename CMandatoryHelper<TValueType>::TValue TValue;
    };
    template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>
    {
        template<uint32 nCapacity2 FOCP_FAKE_DEFAULT_TYPE(S2)> struct CHelp2
        {
            typedef typename CVectorHelper<TValueType>::TValue TValue;
        };
        template<FOCP_FAKE_TYPE(S2)> struct CHelp2<0 FOCP_FAKE_TYPE_ARG(S2)>
        {
            typedef typename COptionHelper<TValueType>::TValue TValue;
        };
        typedef typename CHelp2<nCapacity>::TValue TValue;
    };

public:
	typedef typename CHelper<bOption>::TValue TValue;
};

FOCP_END();

#endif
