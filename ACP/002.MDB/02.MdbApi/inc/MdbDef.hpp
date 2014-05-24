
#include "MdbAccess.hpp"

#ifndef _MDB_DEF_h_
#define _MDB_DEF_h_

FOCP_BEGIN();

struct CMdbFieldDefine;
struct CMdbTableDefine;
struct CMdbIndexDefine;
struct CMdbDefine;

//-----------------------------------------------------
// CMdbFieldDefine
//-----------------------------------------------------
struct MCI_API CMdbFieldDefine: public CMdbFieldDef
{
	CMdbFieldDefine();
	~CMdbFieldDefine();

	void Initialize(CMdbFieldDef* pFieldDef);

	uint32 Invalid();

	bool IsSame(CMdbFieldDef* pFieldDef);

	bool Write(CMemoryStream & oStream);
	bool Read(CMemoryStream & oStream);
};

//-----------------------------------------------------
// CMdbTableDefine
//-----------------------------------------------------
struct MCI_API CMdbTableDefine: public CMdbTableDef
{
	CMdbTableDefine();
	~CMdbTableDefine();

	uint32 Initialize(CMdbDefine* pDbDef, CMdbTableDef* pTableDef);

	uint32 GetFieldNo(const char* sFieldName);

	static uint32 ComputeFieldRecSize(CMdbFieldDef* pFieldDefine);
	static uint32 ComputeFieldOffset(uint32 nOffset, CMdbFieldDef* pFieldDefine);

	uint32 AddIndex(const char* sDbName, CMdbIndexDef* pIndexDefine);

	bool IsIndexField(uint32 nFieldNo);
	uint32 GetIndexFieldNo(CMdbIndexDef* pIdxDef, uint32 nFieldNo);

	bool Write(CMemoryStream & oStream);
	bool Read(CMdbDefine* pDbDef, CMemoryStream & oStream);

private:
	uint32 AddFieldDefine(const char* sDbName, CMdbFieldDef* pFieldDefine, uint32 nFieldNo);
	bool CheckRepeateField(CMdbFieldDef* pFieldDefine);
};

//-----------------------------------------------------
// CMdbIndexDefine
//-----------------------------------------------------
struct MCI_API CMdbIndexDefine: public CMdbIndexDef
{
	CMdbIndexDefine();
	~CMdbIndexDefine();

	uint32 Initialize(CMdbDefine* pDbDef, CMdbIndexDef* pIndexDef);

	bool Write(CMemoryStream & oStream);
	bool Read(CMdbDefine* pDbDef, CMemoryStream & oStream);

private:
	uint32 AddDetail(const char* sDbName, CMdbIndexDefine * pIndexDefine);
};

//-----------------------------------------------------
// CMdbDefine
//-----------------------------------------------------
struct MCI_API CMdbDefine
{
	enum
	{
		MDB_TABLE_DEFINE,
		MDB_INDEX_DEFINE
	};

	struct CMdbItemDef
	{
		int32 nType;
		union tag_def_t
		{
			CMdbTableDef oTabDef;
			CMdbIndexDef oIdxDef;
		} oItemDef;
	};

private:
	const char* m_sDbName;
	CRbMap<CString, CMdbItemDef, CNameCompare> m_oMdbDef;
	CString m_oTableList, m_oIndexList;

public:
	CMdbDefine(const char* sDbName);
	~CMdbDefine();

	const char* GetDbName();

	uint32 CreateTable(CMdbTableDef* pDefine);
	uint32 CreateIndex(CMdbIndexDef* pDefine);

	CMdbTableDef* GetTableDefine(const char* sTableName);
	CMdbIndexDef* GetIndexDefine(const char* sIdxName);

	const char* GetTableList();
	const char* GetIndexList();

	uint32 CreateTable(const char* sTableName, CMdbTableDefine *&pDefine);
	uint32 CreateIndex(const char* sIndexName, CMdbIndexDefine *&pDefine);

// bool Write(CMemoryStream & oStream);
// bool Read(CMdbDefine* pDbDef, CMemoryStream & oStream);

private:
	void MakeTableList(const char* sTableName);
	void MakeIndexList(const char* sIndexName);
};

FOCP_END();

#endif
