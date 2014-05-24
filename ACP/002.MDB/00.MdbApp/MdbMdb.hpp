
#include "MdbApi.hpp"

#ifndef _MDB_MDB_HPP_
#define _MDB_MDB_HPP_

#define MDB_MDB_BEGIN() namespace MDB_MDB{
#define MDB_MDB_END() }

FOCP_BEGIN();
MDB_MDB_BEGIN();


/**********************************************
 * CMyProfileUpdateAttr for the table MyProfile
 **********************************************/
class CMyProfileUpdateAttr
{
	friend class CMyProfile;
private:
	CMdbPara* m_pPara;

	CMyProfileUpdateAttr();
	CMyProfileUpdateAttr(const CMyProfileUpdateAttr &o);
	CMyProfileUpdateAttr& operator=(const CMyProfileUpdateAttr &o);
	void Bind(CMdbAccess* pAccess);

public:
	void Clear();

	CMdbUpdateVal<int32,0>::TValue ont;
	CMdbUpdateVal<CString,1>::TValue two;
	CMdbUpdateVal<int32,2>::TValue three;
	CMdbUpdateVal<CString,3>::TValue four;
};

/**********************************************
 * CMyProfileInsertAttr for the table MyProfile
 **********************************************/
class CMyProfileInsertAttr
{
	friend class CMyProfile;
private:
	CMdbPara* m_pPara;

	CMyProfileInsertAttr();
	CMyProfileInsertAttr(const CMyProfileInsertAttr &o);
	CMyProfileInsertAttr& operator=(const CMyProfileInsertAttr &o);
	void Bind(CMdbAccess* pAccess);

public:
	void Clear();

	CMdbInsertVal<int32,0>::TValue ont;
	CMdbInsertVal<CString,1>::TValue two;
	CMdbInsertVal<int32,2>::TValue three;
	CMdbInsertVal<CString,3>::TValue four;
};

/**********************************************
 * CMyProfileCondAttr for the table MyProfile
 **********************************************/
class CMyProfileCondAttr
{
	friend class CMyProfileWhereAttr;
private:
	CMdbPara* m_pPara;

	CMyProfileCondAttr();
	void Bind(CMdbPara* pPara);

public:
	CMyProfileCondAttr(const CMyProfileCondAttr &o);
	CMyProfileCondAttr& operator=(const CMyProfileCondAttr &o);
	void Clear();

	CMdbCondVal<int32,0>::TValue ont;
	CMdbCondVal<CString,1>::TValue two;
	CMdbCondVal<int32,2>::TValue three;
	CMdbCondVal<CString,3>::TValue four;
};

/**********************************************
 * CMyProfileWhereAttr for the table MyProfile
 **********************************************/
class CMyProfileWhereAttr
{
	friend class CMyProfile;
private:
	CMdbParaSet* m_pParaSet;

	CMyProfileWhereAttr();
	CMyProfileWhereAttr(const CMyProfileWhereAttr &o);
	CMyProfileWhereAttr& operator=(const CMyProfileWhereAttr &o);
	void Bind(CMdbAccess* pAccess);

public:
	void Clear();
	uint32 GetGroupQuantity();
	void ExtendGroupQuantity(uint32 nIncQuantity);
	uint32 GetGroupSize(uint32 nGroupIdx);
	void ExtendGroupSize(uint32 nGroupIdx, uint32 nIncSize);
	CMyProfileCondAttr GetCond(uint32 nGroupIdx, uint32 nCondIdx);
};

/**********************************************
 * CMyProfileResultAttr for the table MyProfile
 **********************************************/
class CMyProfileResultAttr
{
	friend class CMyProfile;
private:
	CMdbResult* m_pPara;

	CMyProfileResultAttr();
	void Bind(CMdbResult* pPara);

public:
	CMyProfileResultAttr(const CMyProfileResultAttr &o);
	CMyProfileResultAttr& operator=(const CMyProfileResultAttr &o);

	CMdbResultVal<int32,0>::TValue ont;
	CMdbResultVal<CString,1>::TValue two;
	CMdbResultVal<int32,2>::TValue three;
	CMdbResultVal<CString,3>::TValue four;
};

/**********************************************
 * CMyProfileFilterAttr for the table MyProfile
 **********************************************/
class CMyProfileFilterAttr
{
	friend class CMyProfile;
private:
	CMdbFilter* m_pFilter;

	CMyProfileFilterAttr();
	CMyProfileFilterAttr(const CMyProfileFilterAttr &o);
	CMyProfileFilterAttr& operator=(const CMyProfileFilterAttr &o);
	void Bind(CMdbAccess* pAccess);

public:
	void Clear();

	CMdbSelect<0> ont;
	CMdbSelect<1> two;
	CMdbSelect<2> three;
	CMdbSelect<3> four;
};

/**********************************************
 * CMyProfile for the table MyProfile of the database Mdb
 **********************************************/
class CMyProfile
{
private:
	CMdbAccess* m_pAccess;
	bool m_bOwned;

	CMyProfile(const CMyProfile &o);
	CMyProfile& operator=(const CMyProfile &o);

public:
	CMyProfileUpdateAttr UpdateAttr;
	CMyProfileInsertAttr InsertAttr;
	CMyProfileWhereAttr WhereAttr;
	CMyProfileFilterAttr FilterAttr;

	CMyProfile(CMdbAccess* pAccess=NULL);
	~CMyProfile();
	bool Valid();
	void Clear();
	bool SetOrderBy(const char* sIndexName, bool bAsc=true);
	uint32 Update(uint32* pModifiedCount=NULL, uint32 nCaller=MDB_APP_CALLER);
	uint32 Delete(uint32* pDeletedCount=NULL, uint32 nCaller=MDB_APP_CALLER);
	uint32 Insert(uint32 nCaller=MDB_APP_CALLER);
	uint32 Truncate(uint32 nCaller=MDB_APP_CALLER);
	uint32 Query(uint32 nPageSize, uint32 nSkipCount, uint32 &nResultSize, uint32 nCaller=MDB_APP_CALLER);
	CMyProfileResultAttr GetResult(uint32 nIdx);
	uint32 Count(uint32 &nCount, uint32 nCaller=MDB_APP_CALLER);
	uint32 Exist(uint32& bExist, uint32 nCaller=MDB_APP_CALLER);
	const char* GetTableName();
	const char* GetDbName();
};

MDB_MDB_END();
FOCP_END();

#endif

