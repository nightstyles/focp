
#include "MdbDef.hpp"
#include "AFC.hpp"

FOCP_BEGIN();

static CMutex g_oMdbMutex;
static CRbMap<CString, CMdb*, CNameCompare> m_oMdbTable;
static CString g_oDbList;

void CMdbResult::GetNumber(void* v, register uint32 nFieldNo)
{
	switch(GetType(nFieldNo))
	{
	case MDB_INT8_FIELD:
		*(int8*)v = GetInt8(nFieldNo);
		break;
	case MDB_INT16_FIELD:
		*(int16*)v = GetInt16(nFieldNo);
		break;
	case MDB_INT32_FIELD:
		*(int32*)v = GetInt32(nFieldNo);
		break;
	case MDB_INT64_FIELD:
		*(int64*)v = GetInt64(nFieldNo);
		break;
	case MDB_UINT8_FIELD:
		*(uint8*)v = GetUInt8(nFieldNo);
		break;
	case MDB_UINT16_FIELD:
		*(uint16*)v = GetUInt16(nFieldNo);
		break;
	case MDB_UINT32_FIELD:
		*(uint32*)v = GetUInt32(nFieldNo);
		break;
	case MDB_UINT64_FIELD:
		*(uint64*)v = GetUInt64(nFieldNo);
		break;
	case MDB_FLOAT_FIELD:
		*(float*)v = GetFloat(nFieldNo);
		break;
	case MDB_DOUBLE_FIELD:
	case MDB_DATETIME_FIELD:
		*(double*)v = GetDouble(nFieldNo);
		break;
	case MDB_DATE_FIELD:
	case MDB_TIME_FIELD:
		*(int32*)v = GetInt32(nFieldNo);
		break;
	}
}

void CMdbPara::SetNumber(register uint32 nFieldNo, register void* pVal, register uint32 nOperator)
{
	switch(GetType(nFieldNo))
	{
	case MDB_INT8_FIELD:
		SetInt8(nFieldNo, *(int8*)pVal, nOperator);
		break;
	case MDB_INT16_FIELD:
		SetInt16(nFieldNo, *(int16*)pVal, nOperator);
		break;
	case MDB_INT32_FIELD:
		SetInt32(nFieldNo, *(int32*)pVal, nOperator);
		break;
	case MDB_INT64_FIELD:
		SetInt64(nFieldNo, *(int64*)pVal, nOperator);
		break;
	case MDB_UINT8_FIELD:
		SetUInt8(nFieldNo, *(uint8*)pVal, nOperator);
		break;
	case MDB_UINT16_FIELD:
		SetUInt16(nFieldNo, *(uint16*)pVal, nOperator);
		break;
	case MDB_UINT32_FIELD:
		SetUInt32(nFieldNo, *(uint32*)pVal, nOperator);
		break;
	case MDB_UINT64_FIELD:
		SetUInt64(nFieldNo, *(uint64*)pVal, nOperator);
		break;
	case MDB_FLOAT_FIELD:
		SetFloat(nFieldNo, *(float*)pVal, nOperator);
		break;
	case MDB_DOUBLE_FIELD:
		SetDouble(nFieldNo, *(double*)pVal, nOperator);
		break;
	case MDB_DATE_FIELD:
		SetDate(nFieldNo, *(CDate*)pVal, nOperator);
		break;
	case MDB_TIME_FIELD:
		SetTime(nFieldNo, *(CTime*)pVal, nOperator);
		break;
	case MDB_DATETIME_FIELD:
		SetDateTime(nFieldNo, *(CDateTime*)pVal, nOperator);
		break;
	}
}

CMdb::CMdb(const char* sName)
{
	CRbTreeNode* pEnd = m_oMdbTable.End();
	g_oMdbMutex.Enter();
	CRbTreeNode* pIt = m_oMdbTable.Find(sName);
	if(pIt != pEnd)
		FocpAbort(("CMdb::CMdb(%s) failure: repeate register dbname", sName));
	pIt = m_oMdbTable.Insert(CString(sName), this);
	m_sDbName = m_oMdbTable.GetKey(pIt).GetStr();
	if(g_oDbList.GetSize())
		g_oDbList += ",";
	g_oDbList += sName;
	g_oMdbMutex.Leave();
}

CMdb::~CMdb()
{
	g_oMdbMutex.Enter();
	m_oMdbTable.Remove(m_sDbName);
	g_oMdbMutex.Leave();
}

const char* CMdb::GetDbList()
{
	return g_oDbList.GetStr();
}

CMdb* CMdb::GetMdb(const char* sDbName)
{
	CMdb* pDb = NULL;
	CRbTreeNode* pEnd = m_oMdbTable.End();
	g_oMdbMutex.Enter();
	CRbTreeNode* pIt = m_oMdbTable.Find(sDbName);
	if(pIt != pEnd)
		pDb = m_oMdbTable.GetItem(pIt);
	g_oMdbMutex.Leave();
	return pDb;
}

const char* CMdb::GetDbName()
{
	return m_sDbName;
}

void RemoveAccessTable();

void CMdb::RemoveAll()
{
	RemoveAccessTable();
	g_oMdbMutex.Enter();
	while(m_oMdbTable.GetSize())
	{
		CRbTreeNode* pIt = m_oMdbTable.First();
		CMdb* pDb = m_oMdbTable.GetItem(pIt);
		delete pDb;
	}
	g_oMdbMutex.Leave();
}

CMdbLocalInterface* CMdb::GetLocalInterface()
{
	return NULL;
}

static const char* GetMdbTypeSymbol(uint32 nType)
{
	const char* sRet = NULL;
	switch(nType)
	{
	case MDB_INT8_FIELD:
		sRet = "int8";
		break;
	case MDB_INT16_FIELD:
		sRet = "int16";
		break;
	case MDB_INT32_FIELD:
		sRet = "int32";
		break;
	case MDB_INT64_FIELD:
		sRet = "int64";
		break;
	case MDB_UINT8_FIELD:
		sRet = "uint8";
		break;
	case MDB_UINT16_FIELD:
		sRet = "uint16";
		break;
	case MDB_UINT32_FIELD:
		sRet = "uint32";
		break;
	case MDB_UINT64_FIELD:
		sRet = "uint64";
		break;
	case MDB_FLOAT_FIELD:
		sRet = "float";
		break;
	case MDB_DOUBLE_FIELD:
		sRet = "double";
		break;
	case MDB_CHAR_FIELD:
	case MDB_LCHAR_FIELD:
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
		sRet = "CString";
		break;
	case MDB_RAW_FIELD:
	case MDB_VARRAW_FIELD:
		sRet = "void";
		break;
	case MDB_DATE_FIELD:
		sRet = "CDate";
		break;
	case MDB_TIME_FIELD:
		sRet = "CTime";
		break;
	case MDB_DATETIME_FIELD:
		sRet = "CDateTime";
		break;
	}
	return sRet;
}

static void CreateInsertUpdateAttrHppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile, bool bInsert)
{
	uint32 nCol;
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = bInsert?"Insert":"Update";
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("class C%s%sAttr\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	friend class C%s;\n", sTableName);
	oFile.Print("private:\n");
	oFile.Print("	CMdbPara* m_pPara;\n");
	oFile.Print("\n");
	oFile.Print("	C%s%sAttr();\n", sTableName, sModeName);
	oFile.Print("	C%s%sAttr(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("	C%s%sAttr& operator=(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("	void Bind(CMdbAccess* pAccess);\n");
	oFile.Print("\n");
	oFile.Print("public:\n");
	oFile.Print("	void Clear();\n");
	oFile.Print("\n");
	for(nCol=0; nCol<pTabDef->nFieldCount; ++nCol)
	{
		CMdbFieldDef* pField = pTabDef->pFieldDefines + nCol;
		oFile.Print("	CMdb%sVal<%s,%u>::TValue %s;\n", sModeName, GetMdbTypeSymbol(pField->nType), nCol, pField->sFieldName);
	}
	oFile.Print("};\n");
}

static void CreateInsertUpdateAttrCppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile, bool bInsert)
{
	uint32 nCol;
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = bInsert?"Insert":"Update";
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("C%s%sAttr::C%s%sAttr()\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara = NULL;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::Bind(CMdbAccess* pAccess)\n", sTableName, sModeName);
	oFile.Print("{\n");
	if(bInsert)
		oFile.Print("	m_pPara = pAccess->GetInsertPara();\n");
	else
		oFile.Print("	m_pPara = pAccess->GetUpdatePara();\n");
	for(nCol=0; nCol<pTabDef->nFieldCount; ++nCol)
	{
		CMdbFieldDef* pField = pTabDef->pFieldDefines + nCol;
		oFile.Print("	%s.Bind(m_pPara);\n", pField->sFieldName);
	}
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::Clear()\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara->Clear();\n");
	oFile.Print("}\n");
}

static void CreateCondAttrHppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	uint32 nCol;
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = "Cond";
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("class C%s%sAttr\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	friend class C%sWhereAttr;\n", sTableName);
	oFile.Print("private:\n");
	oFile.Print("	CMdbPara* m_pPara;\n");
	oFile.Print("\n");
	oFile.Print("	C%s%sAttr();\n", sTableName, sModeName);
	oFile.Print("	void Bind(CMdbPara* pPara);\n");
	oFile.Print("\n");
	oFile.Print("public:\n");
	oFile.Print("	C%s%sAttr(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("	C%s%sAttr& operator=(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("	void Clear();\n");
	oFile.Print("\n");
	for(nCol=0; nCol<pTabDef->nFieldCount; ++nCol)
	{
		CMdbFieldDef* pField = pTabDef->pFieldDefines + nCol;
		oFile.Print("	CMdb%sVal<%s,%u>::TValue %s;\n", sModeName, GetMdbTypeSymbol(pField->nType), nCol, pField->sFieldName);
	}
	oFile.Print("};\n");
}

static void CreateCondAttrCppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	uint32 nCol;
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = "Cond";
	CFormatString oFmt;
	for(nCol=0; nCol<pTabDef->nFieldCount; ++nCol)
	{
		CMdbFieldDef* pField = pTabDef->pFieldDefines + nCol;
		oFmt.Print("	%s.Bind(m_pPara);\n", pField->sFieldName);
	}
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("C%s%sAttr::C%s%sAttr()\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara = NULL;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("C%s%sAttr::C%s%sAttr(const C%s%sAttr &o)\n", sTableName, sModeName, sTableName, sModeName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara = o.m_pPara;\n");
	oFile.Print("%s", oFmt.GetStr());
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("C%s%sAttr& C%s%sAttr::operator=(const C%s%sAttr &o)\n", sTableName, sModeName, sTableName, sModeName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara = o.m_pPara;\n");
	oFile.Print("%s", oFmt.GetStr());
	oFile.Print("	return *this;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::Bind(CMdbPara* pPara)\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara = pPara;\n");
	oFile.Print("%s", oFmt.GetStr());
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::Clear()\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara->Clear();\n");
	oFile.Print("}\n");
}

static void CreateWhereAttrHppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = "Where";
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("class C%s%sAttr\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	friend class C%s;\n", sTableName);
	oFile.Print("private:\n");
	oFile.Print("	CMdbParaSet* m_pParaSet;\n");
	oFile.Print("\n");
	oFile.Print("	C%s%sAttr();\n", sTableName, sModeName);
	oFile.Print("	C%s%sAttr(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("	C%s%sAttr& operator=(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("	void Bind(CMdbAccess* pAccess);\n");
	oFile.Print("\n");
	oFile.Print("public:\n");
	oFile.Print("	void Clear();\n");
	oFile.Print("	uint32 GetGroupQuantity();\n");
	oFile.Print("	void ExtendGroupQuantity(uint32 nIncQuantity);\n");
	oFile.Print("	uint32 GetGroupSize(uint32 nGroupIdx);\n");
	oFile.Print("	void ExtendGroupSize(uint32 nGroupIdx, uint32 nIncSize);\n");
	oFile.Print("	C%sCondAttr GetCond(uint32 nGroupIdx, uint32 nCondIdx);\n", sTableName);
	oFile.Print("};\n");
}

static void CreateWhereAttrCppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = "Where";
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("C%s%sAttr::C%s%sAttr()\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pParaSet = NULL;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::Bind(CMdbAccess* pAccess)\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pParaSet = pAccess->GetQueryPara();\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::Clear()\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pParaSet->Clear();\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("uint32 C%s%sAttr::GetGroupQuantity()\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	return m_pParaSet->GetParaCount();\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::ExtendGroupQuantity(uint32 nIncQuantity)\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	for(uint32 i=0; i<nIncQuantity; ++i)\n");
	oFile.Print("		 m_pParaSet->AddParaSet();\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("uint32 C%s%sAttr::GetGroupSize(uint32 nGroupIdx)\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	bool bSet;\n");
	oFile.Print("	if(nGroupIdx>=m_pParaSet->GetParaCount())\n");
	oFile.Print("		return(0);\n");
	oFile.Print("	CMdbParaSet* pGrp = (CMdbParaSet*)m_pParaSet->GetPara(nGroupIdx, bSet);\n");
	oFile.Print("	return(pGrp->GetParaCount());\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::ExtendGroupSize(uint32 nGroupIdx, uint32 nIncSize)\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	if(nGroupIdx < m_pParaSet->GetParaCount())\n");
	oFile.Print("	{\n");
	oFile.Print("		bool bSet;\n");
	oFile.Print("		CMdbParaSet* pGrp = (CMdbParaSet*)m_pParaSet->GetPara(nGroupIdx, bSet);\n");
	oFile.Print("		for(uint32 i=0; i<nIncSize; ++i)\n");
	oFile.Print("		 	pGrp->AddPara();\n");
	oFile.Print("	}\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("C%sCondAttr C%s%sAttr::GetCond(uint32 nGroupIdx, uint32 nCondIdx)\n", sTableName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	C%sCondAttr oRet;\n", sTableName);
	oFile.Print("	if(nGroupIdx < m_pParaSet->GetParaCount())\n");
	oFile.Print("	{\n");
	oFile.Print("		bool bSet;\n");
	oFile.Print("		CMdbParaSet* pGrp = (CMdbParaSet*)m_pParaSet->GetPara(nGroupIdx, bSet);\n");
	oFile.Print("		if(nCondIdx < pGrp->GetParaCount())\n");
	oFile.Print("		{\n");
	oFile.Print("			oRet.Bind((CMdbPara*)pGrp->GetPara(nCondIdx, bSet));\n");
	oFile.Print("		}\n");
	oFile.Print("	}\n");
	oFile.Print("	return oRet;\n");
	oFile.Print("}\n");
}

static void CreateResultAttrHppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	uint32 nCol;
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = "Result";
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("class C%s%sAttr\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	friend class C%s;\n", sTableName);
	oFile.Print("private:\n");
	oFile.Print("	CMdbResult* m_pPara;\n");
	oFile.Print("\n");
	oFile.Print("	C%s%sAttr();\n", sTableName, sModeName);
	oFile.Print("	void Bind(CMdbResult* pPara);\n");
	oFile.Print("\n");
	oFile.Print("public:\n");
	oFile.Print("	C%s%sAttr(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("	C%s%sAttr& operator=(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("\n");
	for(nCol=0; nCol<pTabDef->nFieldCount; ++nCol)
	{
		CMdbFieldDef* pField = pTabDef->pFieldDefines + nCol;
		oFile.Print("	CMdb%sVal<%s,%u>::TValue %s;\n", sModeName, GetMdbTypeSymbol(pField->nType), nCol, pField->sFieldName);
	}
	oFile.Print("};\n");
}

static void CreateResultAttrCppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	uint32 nCol;
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = "Result";
	CFormatString oFmt;
	for(nCol=0; nCol<pTabDef->nFieldCount; ++nCol)
	{
		CMdbFieldDef* pField = pTabDef->pFieldDefines + nCol;
		oFmt.Print("	%s.Bind(m_pPara);\n", pField->sFieldName);
	}
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("C%s%sAttr::C%s%sAttr()\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara = NULL;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("C%s%sAttr::C%s%sAttr(const C%s%sAttr &o)\n", sTableName, sModeName, sTableName, sModeName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara = o.m_pPara;\n");
	oFile.Print("%s", oFmt.GetStr());
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("C%s%sAttr& C%s%sAttr::operator=(const C%s%sAttr &o)\n", sTableName, sModeName, sTableName, sModeName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara = o.m_pPara;\n");
	oFile.Print("%s", oFmt.GetStr());
	oFile.Print("	return *this;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::Bind(CMdbResult* pPara)\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pPara = pPara;\n");
	oFile.Print("%s", oFmt.GetStr());
	oFile.Print("}\n");
}

static void CreateFilterAttrHppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	uint32 nCol;
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = "Filter";
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("class C%s%sAttr\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	friend class C%s;\n", sTableName);
	oFile.Print("private:\n");
	oFile.Print("	CMdbFilter* m_pFilter;\n");
	oFile.Print("\n");
	oFile.Print("	C%s%sAttr();\n", sTableName, sModeName);
	oFile.Print("	C%s%sAttr(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("	C%s%sAttr& operator=(const C%s%sAttr &o);\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("	void Bind(CMdbAccess* pAccess);\n");
	oFile.Print("\n");
	oFile.Print("public:\n");
	oFile.Print("	void Clear();\n");
	oFile.Print("\n");
	for(nCol=0; nCol<pTabDef->nFieldCount; ++nCol)
	{
		CMdbFieldDef* pField = pTabDef->pFieldDefines + nCol;
		oFile.Print("	CMdbSelect<%u> %s;\n", nCol, pField->sFieldName);
	}
	oFile.Print("};\n");
}

static void CreateFilterAttrCppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	uint32 nCol;
	char* sTableName = pTabDef->sTableName;
	const char* sModeName = "Filter";
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s%sAttr for the table %s\n", sTableName, sModeName, sTableName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("C%s%sAttr::C%s%sAttr()\n", sTableName, sModeName, sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pFilter = NULL;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::Bind(CMdbAccess* pAccess)\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pFilter = pAccess->GetResultFilter();\n");
	for(nCol=0; nCol<pTabDef->nFieldCount; ++nCol)
	{
		CMdbFieldDef* pField = pTabDef->pFieldDefines + nCol;
		oFile.Print("	%s.Bind(m_pFilter);\n", pField->sFieldName);
	}
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s%sAttr::Clear()\n", sTableName, sModeName);
	oFile.Print("{\n");
	oFile.Print("	m_pFilter->Clear();\n");
	oFile.Print("}\n");
}

static void CreateTableHppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	CreateInsertUpdateAttrHppCode(sMdbName, pTabDef, oFile, 0);
	CreateInsertUpdateAttrHppCode(sMdbName, pTabDef, oFile, 1);
	CreateCondAttrHppCode(sMdbName, pTabDef, oFile);
	CreateWhereAttrHppCode(sMdbName, pTabDef, oFile);
	CreateResultAttrHppCode(sMdbName, pTabDef, oFile);
	CreateFilterAttrHppCode(sMdbName, pTabDef, oFile);

	char* sTableName = pTabDef->sTableName;
	oFile.Print("\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s for the table %s of the database %s\n", sTableName, sTableName, sMdbName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("class C%s\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("private:\n");
	oFile.Print("	CMdbAccess* m_pAccess;\n");
	oFile.Print("	bool m_bOwned;\n");
	oFile.Print("\n");
	oFile.Print("	C%s(const C%s &o);\n", sTableName, sTableName);
	oFile.Print("	C%s& operator=(const C%s &o);\n", sTableName, sTableName);
	oFile.Print("\n");
	oFile.Print("public:\n");
	oFile.Print("	C%sUpdateAttr UpdateAttr;\n", sTableName);
	oFile.Print("	C%sInsertAttr InsertAttr;\n", sTableName);
	oFile.Print("	C%sWhereAttr WhereAttr;\n", sTableName);
	oFile.Print("	C%sFilterAttr FilterAttr;\n", sTableName);
	oFile.Print("\n");
	oFile.Print("	C%s(CMdbAccess* pAccess=NULL);\n", sTableName);
	oFile.Print("	~C%s();\n", sTableName);
	oFile.Print("	bool Valid();\n");
	oFile.Print("	void Clear();\n");
	oFile.Print("	bool SetOrderBy(const char* sIndexName, bool bAsc=true);\n");
	oFile.Print("	uint32 Update(uint32* pModifiedCount=NULL, uint32 nCaller=MDB_APP_CALLER);\n");
	oFile.Print("	uint32 Delete(uint32* pDeletedCount=NULL, uint32 nCaller=MDB_APP_CALLER);\n");
	oFile.Print("	uint32 Insert(uint32 nCaller=MDB_APP_CALLER);\n");
	oFile.Print("	uint32 Truncate(uint32 nCaller=MDB_APP_CALLER);\n");
	oFile.Print("	uint32 Query(uint32 nPageSize, uint32 nSkipCount, uint32 &nResultSize, uint32 nCaller=MDB_APP_CALLER);\n");
	oFile.Print("	C%sResultAttr GetResult(uint32 nIdx);\n", sTableName);
	oFile.Print("	uint32 Count(uint32 &nCount, uint32 nCaller=MDB_APP_CALLER);\n");
	oFile.Print("	uint32 Exist(uint32& bExist, uint32 nCaller=MDB_APP_CALLER);\n");
	oFile.Print("	const char* GetTableName();\n");
	oFile.Print("	const char* GetDbName();\n");
	oFile.Print("};\n");
}

static void CreateTableCppCode(const char* sMdbName, CMdbTableDef* pTabDef, CFormatFile &oFile)
{
	CreateInsertUpdateAttrCppCode(sMdbName, pTabDef, oFile, 0);
	CreateInsertUpdateAttrCppCode(sMdbName, pTabDef, oFile, 1);
	CreateCondAttrCppCode(sMdbName, pTabDef, oFile);
	CreateWhereAttrCppCode(sMdbName, pTabDef, oFile);
	CreateResultAttrCppCode(sMdbName, pTabDef, oFile);
	CreateFilterAttrCppCode(sMdbName, pTabDef, oFile);

	char* sTableName = pTabDef->sTableName;
	oFile.Print("\n");
	oFile.Print("static CMdb* g_pMdb = NULL;\n");
	oFile.Print("/**********************************************\n");
	oFile.Print(" * C%s for the table %s of the database %s\n", sTableName, sTableName, sMdbName);
	oFile.Print(" **********************************************/\n");
	oFile.Print("C%s::C%s(CMdbAccess* pAccess)\n", sTableName, sTableName);
	oFile.Print("{\n");
	oFile.Print("	m_bOwned = pAccess?false:true;\n");
	oFile.Print("	if(pAccess)\n");
	oFile.Print("	{\n");
	oFile.Print("		CMdb* pMdb = pAccess->GetMdb();\n");
	oFile.Print("		if(CString::StringCompare(pMdb->GetDbName(), \"%s\", false))\n", sMdbName);
	oFile.Print("			Abort();//Design Error\n");
	oFile.Print("		if(CString::StringCompare(pAccess->GetTableName(), \"%s\", false))\n", sTableName);
	oFile.Print("			Abort();//Design Error\n");
	oFile.Print("		m_pAccess = pAccess;\n");
	oFile.Print("	}\n");
	oFile.Print("	else\n");
	oFile.Print("	{\n");
	oFile.Print("		if(g_pMdb == NULL)\n");
	oFile.Print("			g_pMdb = CMdb::GetMdb(\"%s\");\n", sMdbName);
	oFile.Print("		m_pAccess = g_pMdb->QueryAccess(\"%s\");\n", sTableName);
	oFile.Print("	}\n");
	oFile.Print("	UpdateAttr.Bind(m_pAccess);\n");
	oFile.Print("	InsertAttr.Bind(m_pAccess);\n");
	oFile.Print("	WhereAttr.Bind(m_pAccess);\n");
	oFile.Print("	FilterAttr.Bind(m_pAccess);\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("C%s::~C%s()\n", sTableName, sTableName);
	oFile.Print("{\n");
	oFile.Print("	if(m_pAccess && m_bOwned)\n");
	oFile.Print("		m_pAccess->Release();\n");
	oFile.Print("	m_pAccess = NULL;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("bool C%s::Valid()\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return m_pAccess!=NULL;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("void C%s::Clear()\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	m_pAccess->Clear();\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("bool C%s::SetOrderBy(const char* sIndexName, bool bAsc)\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return m_pAccess->SetOrderBy(sIndexName, bAsc);\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("uint32 C%s::Update(uint32* pModifiedCount, uint32 nCaller)\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return m_pAccess->Update(pModifiedCount, nCaller);\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("uint32 C%s::Delete(uint32* pDeletedCount, uint32 nCaller)\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return m_pAccess->Delete(pDeletedCount, nCaller);\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("uint32 C%s::Insert(uint32 nCaller)\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return m_pAccess->Insert(nCaller);\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("uint32 C%s::Truncate(uint32 nCaller)\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return m_pAccess->Truncate(nCaller);\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("uint32 C%s::Query(uint32 nPageSize, uint32 nSkipCount, uint32 &nResultSize, uint32 nCaller)\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	uint32 nRet = m_pAccess->Query(nPageSize, nSkipCount, nCaller);\n");
	oFile.Print("	nResultSize = m_pAccess->GetResultSet()->GetResultCount();\n");
	oFile.Print("	return nRet;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("C%sResultAttr C%s::GetResult(uint32 nIdx)\n", sTableName, sTableName);
	oFile.Print("{\n");
	oFile.Print("	C%sResultAttr oRet;\n", sTableName);
	oFile.Print("	CMdbResultSet* pSet = m_pAccess->GetResultSet();\n");
	oFile.Print("	if(nIdx < pSet->GetResultCount())\n");
	oFile.Print("		oRet.Bind(pSet->GetResult(nIdx));\n");
	oFile.Print("	return oRet;\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("uint32 C%s::Count(uint32 &nCount, uint32 nCaller)\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return m_pAccess->Count(nCount, nCaller);\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("uint32 C%s::Exist(uint32 &bExist, uint32 nCaller)\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return m_pAccess->Exist(bExist, nCaller);\n");
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("const char* C%s::GetTableName()\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return \"%s\";\n", sTableName);
	oFile.Print("}\n");
	oFile.Print("\n");
	oFile.Print("const char* C%s::GetDbName()\n", sTableName);
	oFile.Print("{\n");
	oFile.Print("	return \"%s\";\n", sMdbName);
	oFile.Print("}\n");
}

void CMdb::CreateCppCode()
{
	char sHppFile[256];
	char sCppFile[256];

	CFormatFile oHppFile;
	CFormatFile oCppFile;

	CRbTreeNode* pEnd = m_oMdbTable.End();
	g_oMdbMutex.Enter();
	CRbTreeNode* pIt = m_oMdbTable.First();
	for(; pIt!=pEnd; pIt=m_oMdbTable.GetNext(pIt))
	{
		CMdb* pMdb = m_oMdbTable.GetItem(pIt);
		const char* sMdbName = pMdb->GetDbName();

		CString oUpperMdbName(sMdbName);
		oUpperMdbName.ToUpper();

		StringPrint(sHppFile, "Mdb%s.hpp", sMdbName);
		StringPrint(sCppFile, "Mdb%s.cpp", sMdbName);

		oHppFile.Open(sHppFile, "wcd");
		oCppFile.Open(sCppFile, "wcd");

		oHppFile.SetLineBuf(false);
		oCppFile.SetLineBuf(false);

		oHppFile.Print("\n");
		oHppFile.Print("#include \"MdbApi.hpp\"\n");
		oHppFile.Print("\n");
		oHppFile.Print("#ifndef _MDB_%s_HPP_\n", oUpperMdbName.GetStr());
		oHppFile.Print("#define _MDB_%s_HPP_\n", oUpperMdbName.GetStr());
		oHppFile.Print("\n");
		oHppFile.Print("#define MDB_%s_BEGIN() namespace MDB_%s{\n", oUpperMdbName.GetStr(), oUpperMdbName.GetStr());
		oHppFile.Print("#define MDB_%s_END() }\n", oUpperMdbName.GetStr());
		oHppFile.Print("\n");
		oHppFile.Print("FOCP_BEGIN();\n");
		oHppFile.Print("MDB_%s_BEGIN();\n", oUpperMdbName.GetStr());
		oHppFile.Print("\n");

		oCppFile.Print("\n");
		oCppFile.Print("#include \"Mdb%s.hpp\"\n", sMdbName);

		oCppFile.Print("\n");
		oCppFile.Print("FOCP_BEGIN();\n");
		oCppFile.Print("MDB_%s_BEGIN();\n", oUpperMdbName.GetStr());
		oCppFile.Print("\n");

		CString oTabList(pMdb->GetTableList());
		char *pShift, *sTableName = (char*)oTabList.GetStr();
		while(sTableName)
		{
			pShift = (char*)CString::CharOfString(sTableName, ',');
			if(pShift)
				pShift[0] = 0;
			CMdbTableDef* pTabDef = pMdb->GetTableDefine(sTableName);
			CreateTableHppCode(sMdbName, pTabDef, oHppFile);
			CreateTableCppCode(sMdbName, pTabDef, oCppFile);
			sTableName = pShift;
			if(sTableName)
			{
				sTableName[0] = ',';
				++sTableName;
			}
		}

		oHppFile.Print("\n");
		oHppFile.Print("MDB_%s_END();\n", oUpperMdbName.GetStr());
		oHppFile.Print("FOCP_END();\n");
		oHppFile.Print("\n");

		oHppFile.Print("#endif\n");
		oHppFile.Print("\n");

		oCppFile.Print("\n");
		oCppFile.Print("MDB_%s_END();\n", oUpperMdbName.GetStr());
		oCppFile.Print("FOCP_END();\n");
		oCppFile.Print("\n");

		oHppFile.Close();
		oCppFile.Close();
	}
	g_oMdbMutex.Leave();
}

FOCP_END();
