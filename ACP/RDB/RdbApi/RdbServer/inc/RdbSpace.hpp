
#include "RdbDef.hpp"

#ifndef _RDB_SPACE_HPP_
#define _RDB_SPACE_HPP_

FOCP_BEGIN();

class CDataBaseSpace;
class CTableSpace;
class CDataTable;
class CRdbField;
class CRdbRecord;

class CDataBaseSpace: public CVirtualAllocator
{
public:
	void* m_pSpace;

public:
	CDataBaseSpace();
	virtual ~CDataBaseSpace();

	bool CreateObject(int32 bInMemory);
	void DestroyObject();

	uint64 GetThis();
	CDataBaseSpace& SetThis(uint64 nThis);

	int32 InMemory();

	virtual uint64 Allocate(uint32 nSaveSize, int32 nInMemory);
	virtual void DeAllocate(uint64 nAddr);
	virtual void DeAllocateAll();
};

//------------------------------------------------------------------------
// CTableSpace
//------------------------------------------------------------------------
struct CVarFieldStruct
{
	uint64 nAddr;
	uint32 nSize;
};

class CTableSpace
{
public:
	uint64 m_nThis;
	CMutex m_oMutex;
	uint64 m_nMaxRowId;
	CDataBaseSpace m_oDiskSpace;
	CTableDefine* m_pTabDef;

public:
	~CTableSpace();
	CTableSpace(CTableDefine* pTabDef);

	CDataBaseSpace& GetDiskSpace();

	CTableSpace& SetThis(uint64 nThis);
	uint64 GetThis();

	bool CreateObject();
	void DestroyObject();

	void Truncate();

	void ReadVarField(CVarFieldStruct& oVar, uint32 nRecSize, char* pBuf);
	bool RemoveVarField(CVarFieldStruct& oVar, uint32 nRecSize);
	bool WriteVarField(CVarFieldStruct& oVar, uint32 nRecSize, char* pBuf, uint32 nBufSize);

public:
	uint64 AllocateRecord();
	void DeAllocateRecord(uint64 nRecord);
	uint64 ApplyRowid();
};

//------------------------------------------------------------------------
// CKernelIndexItem
//------------------------------------------------------------------------
#define RDB_MAX_RECORD_LOCK 16
struct CKernelIndexItem
{
	uint64 nRowId;
	uint64 nRecord;
	uint32 nJobId;

	operator uint32();
	CKernelIndexItem& operator=(uint32);
	CKernelIndexItem& operator=(const CKernelIndexItem& oSrc);
};

//------------------------------------------------------------------------
// CDataTable
//------------------------------------------------------------------------
class CDataTable: public CVirtualGetKey<CKernelIndexItem, uint64>
{
public:
	enum {RDB_HASH_BLOCKSIZE = 16};
	typedef CVmmHash<CKernelIndexItem, RDB_HASH_BLOCKSIZE> THash;

	class CIterator
	{
	private:
		THash* m_pKernelIndex;
		THash::CHashIterator m_oIt;
		CKernelIndexItem* m_pValue;
		CMutex* m_pLock;
		bool m_bLocked;

	public:
		~CIterator();
		CIterator();
		CIterator(THash::CHashIterator& oIt, THash* pKernelIndex, CMutex* pLock);
		CIterator(const CIterator& oIt);

		void Detach();

		CIterator& operator=(const CIterator& oIt);

		bool operator==(const CIterator& oIt);
		bool operator!=(const CIterator& oIt);

		CIterator& operator++ ();
		CIterator operator++ (int);

		CKernelIndexItem* GetValue();
		THash::CHashIterator& GetIterator();

		void UnLock();

		void KillSelf();
	};

	struct CRdbHashInfo
	{
		uint64 nAddr;
		CVmmHashInfo oInfo;
	};

public:
	CMutex m_pLock[RDB_MAX_RECORD_LOCK];
	uint64 m_nThis;
	THash m_oKernelIndex;
	CTableDefine* m_pTabDef;
	CTableSpace* m_pTableSpace;

public:
	virtual ~CDataTable();
	CDataTable(CTableDefine* pTabDef, CTableSpace* pTableSpace);

	CDataTable& SetThis(uint64 nThis);
	uint64 GetThis();

	bool CreateObject();
	void DestroyObject();

	void Truncate();

	CIterator Begin();
	CIterator End();
	CIterator Find(uint64 nRowId);
	CIterator Insert();

	CIterator& Erase(CIterator& oIt);

	CIterator& ActivateRecord(CIterator& it, bool &bUpgradeFailure);
};

//-----------------------------------------------------------------------------
// CRdbField
//-----------------------------------------------------------------------------
class CRdbField
{
public:
	CTableSpace* m_pTableSpace;
	CRdbRecord* m_pRecord;
	uint32* m_pFlag;
	char* m_pData;
	uint32 m_nBit;
	CRdbFieldDef* m_pBaseAttr;
	const char* m_sLogName;

public:
	CRdbField();
	~CRdbField();

	void Initialize(CRdbRecord* pRecord, CFieldDefine* pFieldDef, uint32 nRecHeadSize);

	bool IsNull();
	void SetNull();
	void GetField(CField* pField);
	uint32 SetField(CField* pField);
	uint32 SetField(char* sValue);

	void CopyField(CRdbField* pField, uint32 nFieldSize);

	const char* GetLogName();
};

//-----------------------------------------------------------------------------
// CRdbRecord
//-----------------------------------------------------------------------------
class CRdbRecord
{
public:
	CTableDefine* m_pTabDef;
	CTableSpace* m_pTableSpace;
	CKernelIndexItem* m_pKernelItem;
	char* m_pRecordData;
	CRdbField* m_pFieldTable;
	uint32 m_nFieldCount;
	uint32 m_nRecHeadSize;
	uint32 m_nDirty;

public:
	CRdbRecord(CTableSpace* pTableSpace, CTableDefine * pTabDef, CKernelIndexItem* pKernelItem);
	~CRdbRecord();

	CRdbField* GetField(uint32 nFieldNo);

	void Clear(CSqlParameter* pInsertAttr, uint32 nCount);
	void Clear();

	uint64 GetRowId();

	void GetRecord(CRecord* pRecord, CSqlFilter & oFilter);
	uint32 CopyRecord(CRecord& oRecord, CSqlParameter* pInsertAttr);

	const char* GetLogName();

	void ClearDirty();
};

FOCP_END();

#endif
