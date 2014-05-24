
#include "RdbPara.hpp"
#include "VmmHash.hpp"
#include "VmmNTree.hpp"
#include "RdbSpace.hpp"

#ifndef _RDB_IDX_HPP_
#define _RDB_IDX_HPP_

FOCP_BEGIN();

class CAbstractIndex;
class CRdbRowIndex;
class CRdbHashIndex;
class CBaseRbTreeIndex;
class CRdbRbTreeIndex;
class CRdbPrefixIndex;

//----------------------------------------------------------
// CAbstractIndex
//----------------------------------------------------------
class CAbstractIndex;
typedef uint32 (*FOnTravelIndex)(CAbstractIndex* pIndex, CDataTable::CIterator &oIt, CRecord* pRecord, void* pPara);
class CAbstractIndex
{
public:
	CTableDefine* m_pTabDef;
	CIndexDefine* m_pIdxDef;
	CTableSpace* m_pTableSpace;
	CDataTable* m_pDataTable;

	CAbstractIndex* m_pPrimaryIndex;
	uint32 m_nDetailIndexCount;
	CAbstractIndex** m_pDetailIndexes;

public:
	CAbstractIndex(CTableDefine* pTabDef, 
		CIndexDefine* pIdxDef, 
		CTableSpace* pTableSpace,
		CDataTable* pDataTable);
	virtual ~CAbstractIndex();
	
	CAbstractIndex* GetPrimaryIndex();
	void SetPrimaryIndex(CAbstractIndex* pPrimaryIndex);
	uint32 GetDetailIndexCount();
	CAbstractIndex* GetDetailIndex(uint32 nIndexNo);

	CDataTable* GetDataTable();
	CTableDefine* GetTableDefine();
	CIndexDefine* GetIndexDefine();
	CTableSpace* GetTableSpace();

	virtual bool CreateObject()= 0;
	virtual void DestroyObject()= 0;
	virtual CAbstractIndex& SetThis(uint64 nThis)= 0;
	virtual uint64 GetThis() = 0;

	virtual void Truncate() = 0;
	uint32 LoadAllData();

	virtual uint32 Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount) = 0;
	virtual uint32 QueryCount(CSqlParameterSet * pCondition, uint32 &nCount) = 0;
	virtual uint32 Exist(CSqlParameterSet * pCondition, uint32& bExist) = 0;
	virtual uint32 Travel(CSqlParameterSet * pCondition, CSqlFilter &oFilter, bool bUpdate, FOnTravelIndex OnTravel, uint32& nCount, void* pPara=NULL) = 0;

	virtual uint32 AttachRecord(CRecord* pRecord);
	virtual uint32 DetachRecord(CRecord* pRecord);

	const char* GetLogName();

protected:
	void BuildCond(CSqlParameterSet& oCond, CSqlFilter* pFilter, CRecord* pRecord);
	bool ExistInPrimaryTable(CSqlFilter* pFilter, CRecord* pRecord);

	uint32 AllocRecord(CRecordSet *pResult, CRecord* &pRecord);
	uint32 IsCoincident(CRecord* &pRecord, 
		uint64 nRowId, 
		CSqlParameterSet* pCondition, 
		CSqlFilter& oFilter, 
		CRecordSet *pResult, 
		uint32* pSkipCount, 
		uint32* pCount, 
		uint32* pMaxCount,
		uint32& nFull,
		bool bUpdate,
		FOnTravelIndex = NULL,
		void* pTravelPara = NULL);

private:
	void AddDetailIndex(CAbstractIndex* pIndex);
	void DelDetailIndex(CAbstractIndex* pIndex);
};

//----------------------------------------------------------
// CRdbRowIndex
//----------------------------------------------------------
class CRdbRowIndex: public CAbstractIndex
{
public:
	CDataTable m_oDataTable;

public:
	CRdbRowIndex(CTableDefine* pTabDef, CTableSpace* pTableSpace);
	virtual ~CRdbRowIndex();

	virtual bool CreateObject();
	virtual void DestroyObject();
	virtual CAbstractIndex& SetThis(uint64 nThis);
	virtual uint64 GetThis();

	virtual void Truncate();

	virtual uint32 Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount);
	virtual uint32 QueryCount(CSqlParameterSet * pCondition, uint32 &nCount);
	virtual uint32 Exist(CSqlParameterSet * pCondition, uint32& bExist);
	virtual uint32 Travel(CSqlParameterSet * pCondition, CSqlFilter &oFilter, bool bUpdate, FOnTravelIndex OnTravel, uint32& nCount, void* pPara=NULL);
};

//----------------------------------------------------------
// CRdbHashIndex
//----------------------------------------------------------
struct CRdbHashInfo
{
	uint64 nAddr[2];
	CVmmHashInfo oInfo;
};
class CRdbHashIndex: public CAbstractIndex
{
public:
	enum {RDB_HASH_BLOCKSIZE = 8};
	uint64 m_nThis;
	CVmmHash<uint64, RDB_HASH_BLOCKSIZE> m_oHash;
	CDataBaseSpace m_oAllocator;
	typedef CVmmHash<uint64, RDB_HASH_BLOCKSIZE>::THashBlock THashBlock;
	CSqlFilter m_oFilter;

public:
	virtual ~CRdbHashIndex();
	CRdbHashIndex(CTableDefine* pTabDef,
		CIndexDefine* pIdxDef, 
		CTableSpace* pTableSpace,
		CDataTable* pDataTable);

	virtual bool CreateObject();
	virtual void DestroyObject();
	virtual CAbstractIndex& SetThis(uint64 nThis);
	virtual uint64 GetThis();

	virtual void Truncate();

	virtual uint32 Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount);
	virtual uint32 QueryCount(CSqlParameterSet * pCondition, uint32 &nCount);
	virtual uint32 Exist(CSqlParameterSet * pCondition, uint32& bExist);
	virtual uint32 Travel(CSqlParameterSet * pCondition, CSqlFilter &oFilter, bool bUpdate, FOnTravelIndex OnTravel, uint32& nCount, void* pPara=NULL);

	virtual uint32 AttachRecord(CRecord* pRecord);
	virtual uint32 DetachRecord(CRecord* pRecord);

	bool RecordIsEqual(CRecord* pRecord, uint64 nRowId, CSqlFilter* pFilter);

private:
	uint32 QueryHelp(CSqlParameterSet* pCondition, 
		CSqlFilter& oFilter, 
		CRecordSet *pResult, 
		uint32* pSkipCount, 
		uint32* pCount, 
		uint32* pMaxCount,
		bool bUpdate=false, 
		FOnTravelIndex OnTravel = NULL,
		void* pPara = NULL);
};

//----------------------------------------------------------
// CRdbRbTreeIndex
//----------------------------------------------------------
class CIndexKeyValue: public CField
{
public:
	uint32 m_nRecSize;

public:
	virtual ~CIndexKeyValue();
	CIndexKeyValue();
	CIndexKeyValue(const CIndexKeyValue& oSrc);
	CIndexKeyValue& operator=(const CIndexKeyValue& oSrc);

	void Create(uint32 nType, uint32 nSize, CTableDefine* pTabDef);
	void Load(uint64 nThis);
	void Load(void* p);
	void Save(uint64 nThis);
};

struct CRdbRbTreeItem
{
	uint64 nHandle;//rowid or index handle;
};

class CBaseRbTreeIndex:
public CVirtualGetKey<CRdbRbTreeItem, CIndexKeyValue>,
public CVirtualCompare<CIndexKeyValue>,
public CVirtualAccess,
public CAbstractIndex
{
public:
	uint32 m_nTotalLevel;
	uint32 m_nCurLevel;
	uint32 m_nType, m_nSize, m_nKeySize;
	CVmmRbTree<CRdbRbTreeItem, CIndexKeyValue> m_oTree;
	CDataBaseSpace* m_pSpace; // for CIndexKeyContainer & m_oTree
	CVirtualObjectPool<CBaseRbTreeIndex, CBaseRbTreeIndex>* m_pIndexPool;
	CSqlFilter* m_pFilter;

private:
	// for CVirtualAccess
	void Clear(void* pBuf);

	// for CVirtualGetKey
	virtual CIndexKeyValue GetKey(CRdbRbTreeItem& oSrc);

	// for CVirtualCompare
	virtual int32 Compare(CIndexKeyValue&l, CIndexKeyValue&r);

public:
	// for TObjectFactory of m_pIndexPool
	CBaseRbTreeIndex* QueryObject(uint64 nThis);

public:
	virtual ~CBaseRbTreeIndex();
	CBaseRbTreeIndex(CTableDefine* pTabDef,
		CIndexDefine* pIdxDef, 
		CTableSpace* pTableSpace,
		CDataTable* pDataTable,
		CDataBaseSpace* pSpace,
		CVirtualObjectPool<CBaseRbTreeIndex, CBaseRbTreeIndex>* pIndexPool,
		CSqlFilter* pFilter,
		uint32 nLevel=0);

	virtual bool CreateObject();
	virtual void DestroyObject();
	virtual CAbstractIndex& SetThis(uint64 nThis);
	virtual uint64 GetThis();

	virtual void Truncate();

	virtual uint32 Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount);
	virtual uint32 QueryCount(CSqlParameterSet * pCondition, uint32 &nCount);
	virtual uint32 Exist(CSqlParameterSet * pCondition, uint32& bExist);
	virtual uint32 Travel(CSqlParameterSet * pCondition, CSqlFilter &oFilter, bool bUpdate, FOnTravelIndex OnTravel, uint32& nCount, void* pPara=NULL);

	virtual uint32 AttachRecord(CRecord* pRecord);
	virtual uint32 DetachRecord(CRecord* pRecord);

private:
	uint32 QueryHelp(CSqlParameterSet* pCondition, 
		CSqlFilter& oFilter, 
		CRecordSet *pResult, 
		uint32* pSkipCount, 
		uint32* pCount, 
		uint32* pMaxCount,
		bool bUpdate = false, 
		FOnTravelIndex OnTravel = NULL,
		void* pPara = NULL);
	uint32 AttachRecordHelp(CRecord* pRecord);
};

class CRdbRbTreeIndex: public CBaseRbTreeIndex
{
public:
	uint64 m_nThis;
	CDataBaseSpace m_oSpace;
	CVirtualObjectPool<CBaseRbTreeIndex, CBaseRbTreeIndex> m_oIndexPool;

public:
	virtual ~CRdbRbTreeIndex();
	CRdbRbTreeIndex(CTableDefine* pTabDef,
		CIndexDefine* pIdxDef, 
		CTableSpace* pTableSpace,
		CDataTable* pDataTable);

	virtual bool CreateObject();
	virtual void DestroyObject();
	virtual CAbstractIndex& SetThis(uint64 nThis);
	virtual uint64 GetThis();
};

//----------------------------------------------------------
// CRdbPrefixIndex
//----------------------------------------------------------
class CRdbPrefixIndex: public CAbstractIndex
{
public:
	uint64 m_nThis;
	CVmmNTree<uint64> m_oTree;
	CDataBaseSpace m_oAllocator;
	typedef CVmmNTree<uint64>::TNTreeNode TNTreeNode;
	CSqlFilter m_oFilter;

public:
	virtual ~CRdbPrefixIndex();
	CRdbPrefixIndex(CTableDefine* pTabDef,
		CIndexDefine* pIdxDef, 
		CTableSpace* pTableSpace,
		CDataTable* pDataTable);

	virtual bool CreateObject();
	virtual void DestroyObject();
	virtual CAbstractIndex& SetThis(uint64 nThis);
	virtual uint64 GetThis();

	virtual void Truncate();

	virtual uint32 Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount);
	virtual uint32 QueryCount(CSqlParameterSet * pCondition, uint32 &nCount);
	virtual uint32 Exist(CSqlParameterSet * pCondition, uint32& bExist);
	virtual uint32 Travel(CSqlParameterSet * pCondition, CSqlFilter &oFilter, bool bUpdate, FOnTravelIndex OnTravel, uint32& nCount, void* pPara=NULL);

	virtual uint32 AttachRecord(CRecord* pRecord);
	virtual uint32 DetachRecord(CRecord* pRecord);

private:
	uint32 QueryHelp(char* sKey, uint32 nKeyLen,
		CSqlParameterSet* pCondition,
		CSqlFilter& oFilter, 
		CRecordSet *pResult, 
		uint32* pSkipCount, 
		uint32* pCount, 
		uint32* pMaxCount,
		bool bUpdate = false,
		FOnTravelIndex OnTravel = NULL,
		void* pPara = NULL);
};

FOCP_END();

#endif
