
#include "MdbAccess.hpp"
#include "MdbDef.hpp"

#ifndef _MDB_IDX_HPP_
#define _MDB_IDX_HPP_

FOCP_BEGIN();

struct CMdbDataRecord
{
	CMdbDataRecord *pPrev, *pNext;
	uint8 pRecord[1];
};

class CMdbAbstractIndex;
class CMdbRowIndex;
class CMdbHashIndex;
class CMdbRbTreeIndex;

typedef uint32 (*FOnTravelIndex)(CMdbAbstractIndex* pIndex, CMdbDataRecord* pDataRecord, void* pPara, bool &bDelete);

class CMdbAbstractIndex
{
public:
	CMdbTableDef* m_pTabDef;
	CMdbIndexDef* m_pIdxDef;
	CMdbRowIndex* m_pRowIndex;
	CMdbAbstractIndex* m_pPrimaryIndex;
	uint32 m_nDetailIndexCount;
	CMdbAbstractIndex** m_pDetailIndexes;

public:
	CMdbAbstractIndex(CMdbTableDef* pTabDef, CMdbIndexDef* pIdxDef, CMdbRowIndex* pRowIndex);
	virtual ~CMdbAbstractIndex();

	void SetPrimaryIndex(CMdbAbstractIndex* pPrimaryIndex);

	void Truncate();
	virtual uint32 Query(CMdbSqlParaSet* pCondition, CMdbSqlFilter& oFilter, CMdbRecordSet &oResult, uint32 nSkipCount, bool bAsc=true) = 0;
	virtual uint32 Query(CMdbSqlParaSet * pCondition, uint32 &nCount) = 0;
	virtual uint32 Exist(CMdbSqlParaSet * pCondition, uint32& bExist) = 0;
	virtual uint32 Travel(CMdbSqlParaSet * pCondition, CMdbSqlFilter &oFilter,
						  FOnTravelIndex OnTravel, uint32& nCount, void* pPara=NULL) = 0;

	virtual uint32 AttachRecord(CMdbDataRecord* pDataRecord);
	virtual uint32 DetachRecord(CMdbDataRecord* pDataRecord);

	uint32 Exist(CMdbRecord* pRecord);

protected:
	virtual void Clear() = 0;

	bool ExistInPrimaryTable(CMdbRecord* pRecord);

	CMdbRecord* AllocRecord(CMdbRecordSet *pResult);

	uint32 IsCoincident(CMdbDataRecord* pDataRec,
						CMdbSqlParaSet* pCondition,
						CMdbSqlFilter& oFilter,
						CMdbRecordSet *pResult,
						uint32* pSkipCount,
						uint32* pCount,
						uint32* pMaxCount,
						uint32& nFull,
						bool &bCoincident,
						bool &bDeleted,
						FOnTravelIndex OnTravel = NULL,
						void* pTravelPara = NULL);
};

//----------------------------------------------------------
// CMdbRowIndex
//----------------------------------------------------------
class CMdbRowIndex: public CMdbAbstractIndex
{
public:
	uint32 m_nRecordCount;
	CMdbDataRecord *m_pHead, *m_pTail;//新记录往后追加
	CMutex m_oRecordLock[32];
	CRwMutex m_oTableLock;
	bool m_bWriteLock;

public:
	CMdbRowIndex(CMdbTableDef* pTabDef);
	virtual ~CMdbRowIndex();

	virtual uint32 Query(CMdbSqlParaSet* pCondition, CMdbSqlFilter& oFilter, CMdbRecordSet &oResult, uint32 nSkipCount, bool bAsc=true);
	virtual uint32 Query(CMdbSqlParaSet * pCondition, uint32 &nCount);
	virtual uint32 Exist(CMdbSqlParaSet * pCondition, uint32& bExist);
	virtual uint32 Travel(CMdbSqlParaSet * pCondition, CMdbSqlFilter &oFilter,
						  FOnTravelIndex OnTravel, uint32& nCount, void* pPara=NULL);

	virtual uint32 AttachRecord(CMdbDataRecord* pDataRecord);
	virtual uint32 DetachRecord(CMdbDataRecord* pDataRecord);

	CMdbDataRecord* NewDataRecord();
	void DelDataRecord(CMdbDataRecord* pRec);

	void LockRecord(CMdbDataRecord* pDataRecord);
	void UnLockRecord(CMdbDataRecord* pDataRecord);

	CMdbDataRecord* CloneRecord(CMdbDataRecord* pOld);

	void LockTable(bool bRead);
	void UnLockTable(bool bRead);

protected:
	virtual void Clear();
	CMdbDataRecord* GetFirst();
	CMdbDataRecord* GetNext(CMdbDataRecord* pRec, bool Unlock=true);
};

//----------------------------------------------------------
// CMdbHashIndex
//----------------------------------------------------------
class CMdbHashIndex: public CMdbAbstractIndex
{
private:
	CFreeHash<CMdbDataRecord*> m_oHash;

public:
	virtual ~CMdbHashIndex();
	CMdbHashIndex(CMdbTableDef* pTabDef, CMdbIndexDef* pIdxDef, CMdbRowIndex* pRowIndex);

	virtual uint32 Query(CMdbSqlParaSet* pCondition, CMdbSqlFilter& oFilter, CMdbRecordSet &oResult, uint32 nSkipCount, bool bAsc=true);
	virtual uint32 Query(CMdbSqlParaSet * pCondition, uint32 &nCount);
	virtual uint32 Exist(CMdbSqlParaSet * pCondition, uint32& bExist);
	virtual uint32 Travel(CMdbSqlParaSet * pCondition, CMdbSqlFilter &oFilter,
						  FOnTravelIndex OnTravel, uint32& nCount, void* pPara=NULL);

	virtual uint32 AttachRecord(CMdbDataRecord* pDataRecord);
	virtual uint32 DetachRecord(CMdbDataRecord* pDataRecord);

	bool RecordIsEqual(CMdbDataRecord* a, CMdbDataRecord* b);

private:
	uint32 QueryHelp(CMdbSqlParaSet* pCondition,
					 CMdbSqlFilter& oFilter,
					 CMdbRecordSet *pResult,
					 uint32* pSkipCount,
					 uint32* pCount,
					 uint32* pMaxCount,
					 FOnTravelIndex OnTravel = NULL,
					 void* pPara = NULL);

	uint32 GetHashValue(CMdbSqlParaSet* pCondition, uint32 &nHashValue);
	uint32 GetHashValue(CMdbRecord &oRecord);

protected:
	virtual void Clear();
};

//----------------------------------------------------------
// CMdbRbTreeIndex
//----------------------------------------------------------
class CMdbRbTreeIndexField: public CMdbField
{
public:
	virtual ~CMdbRbTreeIndexField();
	CMdbRbTreeIndexField(uint32 nType, uint32 nSize);
};

struct CMdbRbCmpKey
{
	static int32 Compare(CMdbRbTreeIndexField* const * pLeft, CMdbRbTreeIndexField* const * pRight);
};

class CMdbRbTreeIndex: public CMdbAbstractIndex
{
private:
	uint32 m_nCurLevel;
	uint32 m_nType;
	uint32 m_nSize;
	CRbMap<CMdbRbTreeIndexField*, CMdbDataRecord*, CMdbRbCmpKey> m_oTree;

public:
	virtual ~CMdbRbTreeIndex();
	CMdbRbTreeIndex(CMdbTableDef* pTabDef, CMdbIndexDef* pIdxDef, CMdbRowIndex* pRowIndex, uint32 nLevel=0);

	virtual uint32 Query(CMdbSqlParaSet* pCondition, CMdbSqlFilter& oFilter, CMdbRecordSet &oResult, uint32 nSkipCount, bool bAsc=true);
	virtual uint32 Query(CMdbSqlParaSet * pCondition, uint32 &nCount);
	virtual uint32 Exist(CMdbSqlParaSet * pCondition, uint32& bExist);
	virtual uint32 Travel(CMdbSqlParaSet * pCondition, CMdbSqlFilter &oFilter,
						  FOnTravelIndex OnTravel, uint32& nCount, void* pPara=NULL);

	virtual uint32 AttachRecord(CMdbDataRecord* pDataRecord);
	virtual uint32 DetachRecord(CMdbDataRecord* pDataRecord);

private:
	uint32 QueryHelp(CMdbSqlParaSet* pCondition,
					 CMdbSqlFilter& oFilter,
					 CMdbRecordSet *pResult,
					 uint32* pSkipCount,
					 uint32* pCount,
					 uint32* pMaxCount,
					 bool bAsc,
					 FOnTravelIndex OnTravel = NULL,
					 void* pPara = NULL);

	uint32 AttachRecordHelp(CMdbDataRecord* pRecord);

protected:
	virtual void Clear();
};

FOCP_END();

#endif
