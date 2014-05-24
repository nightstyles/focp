
#include "MdbMdb.hpp"

FOCP_BEGIN();
MDB_MDB_BEGIN();


/**********************************************
 * CMyProfileUpdateAttr for the table MyProfile
 **********************************************/
CMyProfileUpdateAttr::CMyProfileUpdateAttr()
{
	m_pPara = NULL;
}

void CMyProfileUpdateAttr::Bind(CMdbAccess* pAccess)
{
	m_pPara = pAccess->GetUpdatePara();
	ont.Bind(m_pPara);
	two.Bind(m_pPara);
	three.Bind(m_pPara);
	four.Bind(m_pPara);
}

void CMyProfileUpdateAttr::Clear()
{
	m_pPara->Clear();
}

/**********************************************
 * CMyProfileInsertAttr for the table MyProfile
 **********************************************/
CMyProfileInsertAttr::CMyProfileInsertAttr()
{
	m_pPara = NULL;
}

void CMyProfileInsertAttr::Bind(CMdbAccess* pAccess)
{
	m_pPara = pAccess->GetInsertPara();
	ont.Bind(m_pPara);
	two.Bind(m_pPara);
	three.Bind(m_pPara);
	four.Bind(m_pPara);
}

void CMyProfileInsertAttr::Clear()
{
	m_pPara->Clear();
}

/**********************************************
 * CMyProfileCondAttr for the table MyProfile
 **********************************************/
CMyProfileCondAttr::CMyProfileCondAttr()
{
	m_pPara = NULL;
}

CMyProfileCondAttr::CMyProfileCondAttr(const CMyProfileCondAttr &o)
{
	m_pPara = o.m_pPara;
	ont.Bind(m_pPara);
	two.Bind(m_pPara);
	three.Bind(m_pPara);
	four.Bind(m_pPara);
}

CMyProfileCondAttr& CMyProfileCondAttr::operator=(const CMyProfileCondAttr &o)
{
	m_pPara = o.m_pPara;
	ont.Bind(m_pPara);
	two.Bind(m_pPara);
	three.Bind(m_pPara);
	four.Bind(m_pPara);
	return *this;
}

void CMyProfileCondAttr::Bind(CMdbPara* pPara)
{
	m_pPara = pPara;
	ont.Bind(m_pPara);
	two.Bind(m_pPara);
	three.Bind(m_pPara);
	four.Bind(m_pPara);
}

void CMyProfileCondAttr::Clear()
{
	m_pPara->Clear();
}

/**********************************************
 * CMyProfileWhereAttr for the table MyProfile
 **********************************************/
CMyProfileWhereAttr::CMyProfileWhereAttr()
{
	m_pParaSet = NULL;
}

void CMyProfileWhereAttr::Bind(CMdbAccess* pAccess)
{
	m_pParaSet = pAccess->GetQueryPara();
}

void CMyProfileWhereAttr::Clear()
{
	m_pParaSet->Clear();
}

uint32 CMyProfileWhereAttr::GetGroupQuantity()
{
	return m_pParaSet->GetParaCount();
}

void CMyProfileWhereAttr::ExtendGroupQuantity(uint32 nIncQuantity)
{
	for(uint32 i=0; i<nIncQuantity; ++i)
		 m_pParaSet->AddParaSet();
}

uint32 CMyProfileWhereAttr::GetGroupSize(uint32 nGroupIdx)
{
	bool bSet;
	if(nGroupIdx>=m_pParaSet->GetParaCount())
		return(0);
	CMdbParaSet* pGrp = (CMdbParaSet*)m_pParaSet->GetPara(nGroupIdx, bSet);
	return(pGrp->GetParaCount());
}

void CMyProfileWhereAttr::ExtendGroupSize(uint32 nGroupIdx, uint32 nIncSize)
{
	if(nGroupIdx < m_pParaSet->GetParaCount())
	{
		bool bSet;
		CMdbParaSet* pGrp = (CMdbParaSet*)m_pParaSet->GetPara(nGroupIdx, bSet);
		for(uint32 i=0; i<nIncSize; ++i)
		 	pGrp->AddPara();
	}
}

CMyProfileCondAttr CMyProfileWhereAttr::GetCond(uint32 nGroupIdx, uint32 nCondIdx)
{
	CMyProfileCondAttr oRet;
	if(nGroupIdx < m_pParaSet->GetParaCount())
	{
		bool bSet;
		CMdbParaSet* pGrp = (CMdbParaSet*)m_pParaSet->GetPara(nGroupIdx, bSet);
		if(nCondIdx < pGrp->GetParaCount())
		{
			oRet.Bind((CMdbPara*)pGrp->GetPara(nCondIdx, bSet));
		}
	}
	return oRet;
}

/**********************************************
 * CMyProfileResultAttr for the table MyProfile
 **********************************************/
CMyProfileResultAttr::CMyProfileResultAttr()
{
	m_pPara = NULL;
}

CMyProfileResultAttr::CMyProfileResultAttr(const CMyProfileResultAttr &o)
{
	m_pPara = o.m_pPara;
	ont.Bind(m_pPara);
	two.Bind(m_pPara);
	three.Bind(m_pPara);
	four.Bind(m_pPara);
}

CMyProfileResultAttr& CMyProfileResultAttr::operator=(const CMyProfileResultAttr &o)
{
	m_pPara = o.m_pPara;
	ont.Bind(m_pPara);
	two.Bind(m_pPara);
	three.Bind(m_pPara);
	four.Bind(m_pPara);
	return *this;
}

void CMyProfileResultAttr::Bind(CMdbResult* pPara)
{
	m_pPara = pPara;
	ont.Bind(m_pPara);
	two.Bind(m_pPara);
	three.Bind(m_pPara);
	four.Bind(m_pPara);
}

/**********************************************
 * CMyProfileFilterAttr for the table MyProfile
 **********************************************/
CMyProfileFilterAttr::CMyProfileFilterAttr()
{
	m_pFilter = NULL;
}

void CMyProfileFilterAttr::Bind(CMdbAccess* pAccess)
{
	m_pFilter = pAccess->GetResultFilter();
	ont.Bind(m_pFilter);
	two.Bind(m_pFilter);
	three.Bind(m_pFilter);
	four.Bind(m_pFilter);
}

void CMyProfileFilterAttr::Clear()
{
	m_pFilter->Clear();
}

static CMdb* g_pMdb = NULL;
/**********************************************
 * CMyProfile for the table MyProfile of the database Mdb
 **********************************************/
CMyProfile::CMyProfile(CMdbAccess* pAccess)
{
	m_bOwned = pAccess?false:true;
	if(pAccess)
	{
		CMdb* pMdb = pAccess->GetMdb();
		if(CString::StringCompare(pMdb->GetDbName(), "Mdb", false))
			Abort();//Design Error
		if(CString::StringCompare(pAccess->GetTableName(), "MyProfile", false))
			Abort();//Design Error
		m_pAccess = pAccess;
	}
	else
	{
		if(g_pMdb == NULL)
			g_pMdb = CMdb::GetMdb("Mdb");
		m_pAccess = g_pMdb->QueryAccess("MyProfile");
	}
	UpdateAttr.Bind(m_pAccess);
	InsertAttr.Bind(m_pAccess);
	WhereAttr.Bind(m_pAccess);
	FilterAttr.Bind(m_pAccess);
}

CMyProfile::~CMyProfile()
{
	if(m_pAccess && m_bOwned)
		m_pAccess->Release();
	m_pAccess = NULL;
}

bool CMyProfile::Valid()
{
	return m_pAccess!=NULL;
}

void CMyProfile::Clear()
{
	m_pAccess->Clear();
}

bool CMyProfile::SetOrderBy(const char* sIndexName, bool bAsc)
{
	return m_pAccess->SetOrderBy(sIndexName, bAsc);
}

uint32 CMyProfile::Update(uint32* pModifiedCount, uint32 nCaller)
{
	return m_pAccess->Update(pModifiedCount, nCaller);
}

uint32 CMyProfile::Delete(uint32* pDeletedCount, uint32 nCaller)
{
	return m_pAccess->Delete(pDeletedCount, nCaller);
}

uint32 CMyProfile::Insert(uint32 nCaller)
{
	return m_pAccess->Insert(nCaller);
}

uint32 CMyProfile::Truncate(uint32 nCaller)
{
	return m_pAccess->Truncate(nCaller);
}

uint32 CMyProfile::Query(uint32 nPageSize, uint32 nSkipCount, uint32 &nResultSize, uint32 nCaller)
{
	uint32 nRet = m_pAccess->Query(nPageSize, nSkipCount, nCaller);
	nResultSize = m_pAccess->GetResultSet()->GetResultCount();
	return nRet;
}

CMyProfileResultAttr CMyProfile::GetResult(uint32 nIdx)
{
	CMyProfileResultAttr oRet;
	CMdbResultSet* pSet = m_pAccess->GetResultSet();
	if(nIdx < pSet->GetResultCount())
		oRet.Bind(pSet->GetResult(nIdx));
	return oRet;
}

uint32 CMyProfile::Count(uint32 &nCount, uint32 nCaller)
{
	return m_pAccess->Count(nCount, nCaller);
}

uint32 CMyProfile::Exist(uint32 &bExist, uint32 nCaller)
{
	return m_pAccess->Exist(bExist, nCaller);
}

const char* CMyProfile::GetTableName()
{
	return "MyProfile";
}

const char* CMyProfile::GetDbName()
{
	return "Mdb";
}

MDB_MDB_END();
FOCP_END();

