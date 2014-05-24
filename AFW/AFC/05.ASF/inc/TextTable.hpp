
#include "AsfDef.hpp"

#ifndef _Asf_TextTable_Hpp_
#define _Asf_TextTable_Hpp_

FOCP_BEGIN();

/*
�ļ����ݸ�ʽ���塾�ı��ļ�����
	��1����#��ʼ����βע����
	��2������һ����ע����Ϊ�����У�������ע����Ϊ�����У�����Ҳ�൱��ע����
	��3������֮����ÿո��ˮƽ�Ʊ��������
	��4������֮�����[�س�]���з�
	��5������������C��ʶ���ʷ���
	��6������ֵ֧��Cת����������š�˫���ſ�ֱ��ʹ�á�:
				\w													=>		�ո�������
				\r													=>		�س�
				\n													=>		����
				\f													=>		��ҳ
				\b													=>		�˸�
				\v													=>		��ֱ�Ʊ��
				\t													=>		ˮƽ�Ʊ��
				\a													=>		����
				\[x|X]HH										=>		ʮ������ת��
				\�κ��ַ�										=> 		�κ��ַ�
	��6������ֵ��NULL[��Сд�޹�]�����ֵ�����ı�NULL������\NULL��ʾ
*/

class CApplication;
class CTextTable;
class CTextAccess;
class CInitConfigSystem;

FOCP_DETAIL_BEGIN();
struct ASF_API CFieldValue
{
	char* sVal;
	uint32 nSize;//������β0

	~CFieldValue();
	CFieldValue();

	CFieldValue(const CFieldValue& oVal);
	CFieldValue& operator=(const CFieldValue& oVal);

	void Parse(const CString &oValue);
	void EnCode(CString &oValue);
	void Set(const char * sIdxVal, uint32 nLength);
	bool Equal(const CFieldValue &oSrc, bool bCaseSensitive);
	void ReadHex(const uint8* &sStr, uint8 &nCh);
	void ToHex(char s[3], uint8 nCh);
};
FOCP_DETAIL_END();

class ASF_API CTextTable
{
	FOCP_FORBID_COPY(CTextTable);
	friend class CInitConfigSystem;
	friend class CTextAccess;
private:
	CVector< CString > m_oColInfo;
	CVector< CVector<FOCP_DETAIL_NAME::CFieldValue> > m_oTable;
	char m_sFileName[FOCP_MAX_PATH];
	bool m_bDirty;

public:
	CTextTable();
	~CTextTable();

	void Clear();

	bool LoadTable(const char * sFile);
	//bool CreateTable(const char * sFile, uint32 nColNum, const char **pColName);

	//�������
	uint32 GetRowCount();
	uint32 GetColCount();
	const char* GetCell(uint32 nRow, uint32 nCol, uint32 &nLength);//����NULL��ʾ��ֵ
	const char* GetColName(uint32 nCol);

	//����ļ�����
	void Truncate(bool bFlush=false);

	//���仯���µ��ļ��С�
	void Flush();

private:
	void ReadColTable(const char* pShift, CVector< CString > &oColTable);
	bool ReadCol(const char* &pShift, CString &oCol);
	bool CheckColInfo(CVector< CString > &oColTable);
	void CopyColInfo(CVector< CString > &oColTable);
	void ReadRecord(CVector<FOCP_DETAIL_NAME::CFieldValue>& oRecord, const CVector< CString > &oColTable);
	uint32 GetCol(const char* sColName);
};

class ASF_API CTextAccess
{
	FOCP_FORBID_COPY(CTextAccess);
	friend class CInitConfigSystem;
private:
	struct ASF_API CCondition
	{
		FOCP_DETAIL_NAME::CFieldValue oValue;
		bool bSensitive;
	};
	CRbMap<uint32, CCondition> m_oCondTable;
	CRbMap<uint32, CCondition> m_oAdditiveCond;
	CRbMap<uint32, FOCP_DETAIL_NAME::CFieldValue> m_oSetTable;
	CTextTable* m_pTable;
	uint32 m_nQueryRowNo;

public:
	CTextAccess(CTextTable* pTable=NULL);
	~CTextAccess();

	CTextTable& GetTable();

	//��������д���
	void OpenIdxVal();
	void OpenColVal();
	bool SetIdxVal(const char * sIdxName, const char * sIdxVal, bool bSensitive=true, uint32 nLength=0);//NULL��ʾ��ֵ
	bool SetColVal(const char * sColName, const char * sColVal, uint32 nLength=0);//NULL��ʾ��ֵ

	void Insert(bool bFlush=false);//����SetColVal
	void Update(bool bFlush=false);//����SetIdxVal��SetColVal
	void Delete(bool bFlush=false);//����SetIdxVal
	bool Query();//����SetIdxVal
	const char* GetVal(const char * sColName, uint32 &nLength);//����Query��QueryNext,����NULL��ʾ��ֵ

private:
	void OpenAdditiveIdxVal();
	bool SetAdditiveIdxVal(const char * sIdxName, const char * sIdxVal, bool bSensitive=true, uint32 nLength=0);//NULL��ʾ��ֵ
	bool CompareRecord(uint32 nRowNo);
};

FOCP_DETAIL_BEGIN();

class ASF_API CTableInfo
{
private:
	CTextTable m_oTable;
	CTextAccess m_oAccess;

public:
	CTableInfo();
	~CTableInfo();

	bool LoadTable(uint32 nDeploy);

	const char* GetAttr(const char* sTableName, const char* sAttrName);
};

FOCP_DETAIL_END();

/*
	Ԥ���������:
		MinRecordNum ��С��¼��
		MaxRecordNum ����¼��
		DynamicMode��
			0�̶���񣬲�����Ա�������ɾ�������г�������
			1��̬�����ϵͳ�����ڼ��޸���Ч�������޸ġ�
			2��̬�����ϵͳ�����ڼ���޸Ŀ�ʵʱ��Ч��
	Ԥ�����ֶ�����:
		DataType:
			string, 			��ͨ�ı�
			int,					���ͣ�int32��ȡֵ��Χ
			uint,					�޷������ͣ�uint32��ȡֵ��Χ
			long,					�����ͣ�int64��ȡֵ��Χ
			ulong,				�޷��ų����ͣ�uint64��ȡֵ��Χ
			bool,					ȡֵ��ΧΪ'1'��'0'��'true'��'false'��'yes'��'no'
			datetime			UTCʱ���ʽ���ַ���
			base64,				base64���봮
			binary,				���������ݴ�
		RangeList:
			��1��Ϊ�ձ�ʾ��������ӷ�Χ���ơ�
			��2����{}�������Ķ����Χ��ϣ���Χ֮���ö��ŷָ�
			��3����Χ����ð�ŷָ�����
				��A��a:b����ʾ��СΪa�����Ϊb
				��b��a:����ʾ��СΪa�����������
				��C��:b����ʾ���Ϊb������С����
				��D��c����ʾΪ��ֵc����c:c�ļ�д
			��4��string�ķ�Χ��ʾ���ȷ�Χ
			��5��int,uint, long, ulong�ķ�Χ��ʾ��ֵ��ȡֵ��Χ��
			��6��datetime��base64��binary��ȡֵ��Χ��
			��7����Χ�б��ɵ���RageList����ж��壬����������÷�Χ���Ƽ��ɡ�
		EnumList��
			��1����{}�������ķ������ݸ�ʽ��ȡֵ��Χ��ö���б���һ��������ȡֵ
			��2��ö���б��ɵ���EnumList����ж��壬�����������ö�����Ƽ��ɡ�
		DefaultValue��
			�������ݸ�ʽ��ȡֵ��Χ��ȱʡֵ��
		NotNull��0����Ϊ�գ�1������Ϊ�ա�
		DynamicMode��
			0ֻ���ֶΣ���ʾ���ֶ���ֻ���ֶΣ��������޸ġ�
			1��̬�ֶΣ���ϵͳ�����ڼ��޸���Ч�������޸ġ�
			2��̬�ֶΣ���ʾ���ֶε��޸Ŀ�ʵʱ��Ч��
		SystemMode��
			0ϵͳ�࣬��ʾ�����ݴ洢Ϊ����ϵͳ�ࡣ
			1Ӧ�ò࣬��ʾ�����ݴ洢��Ӧ��ϵͳ�ࡣ
		StorageMode��
			0�־������ݣ���Ҫ�־ñ��棬ϵͳ����ʱ���ָܻ�������
			1��ʱ�����ݣ��������������У�ϵͳ������Ͷ�ʧ�ˣ������»�ȡ��
			2���������ݣ����ڴ��ж������ڣ���ҪԤ����һ���������ȱʡֵ��
*/

class ASF_API CDataTable
{
private:
	CTextTable m_oDataTable;
	CRbMap<CString, CString, CNameCompare> m_oTableInfo;
	CRbMap<CString, CRbMap<CString, CString, CNameCompare>, CNameCompare> m_oFieldInfo;

public:
	CDataTable();
	~CDataTable();

	bool LoadTable(uint32 nDeploy, const char* sTableName);

	const char* GetTableAttr(const char* sAttrName);
	const char* GetFieldAttr(const char* sFieldName, const char* sAttrName);

	CTextTable& GetTable();
};

/*
	�����������ļ����·����
		��1��˽�����ã�    �����$(AppCfgHome)/$(AppName)/cfg/Ŀ¼��
		��2�����ܻ������ã������$(NmsCfgHome)/Ŀ¼��
		��2�������������ã������$(NmsCfgHome)/$(AppName)/Ŀ¼��
	������ϵͳ��:
		��1������滮���ã�Network:�̶����ڻ���������
				�����	NetworkNo	UINT32	!=0xFFFFFFFF
				����IP	PhysicalIp	IpAddress
		��2����ϵͳ�滮���ã�SubSystem:�̶����ڻ���������
				��ϵͳ������	SubsystemType	UINT32	>=1
				��ϵͳ��λ��	SubsystemBits	UINT32	1..(32-DependOffset)
				��ϵͳ��ƫ��	SubsystemOffset	UINT32	0..31(������)
				��ϵͳ������	SubsystemDesc	DisplayString	0..64
		��2�����ò����ConfigDeploy���̶�����˽��������
				����              TableName	Displaystring	1..256
				����              Deploy		ENUM        0:˽�����ã� 1:����Ӧ�����ã� 2:���ܻ������ã�
				��ϵͳ������      SubsystemType	UINT32		0:��ʾ������ϵͳ���ˡ���0������ϵͳ���ˡ�
	���ݹ��˹���
	��1��Deploy=0�������κι��ˡ������Բ�������ϵͳ�š��ֶκ͡�Ӧ�����͡��ֶΣ����Ҫ������Ϊ0xFFFFFFFF�������������
	��2��Deploy=1/2
			a.��SubsystemNoTypeΪ0ʱ��������ϵͳ�Ź��ˡ����ݱ����Բ�������ϵͳ�š��ֶΣ����Ҫ������Ϊ0xFFFFFFFF�������������
			b.��SubsystemNoType��0ʱ�����ݱ���躬����ϵͳ�š��ֶΣ��ӡ���ϵͳ����ȡ��ϵͳ�ţ��ٰ���ϵͳ�Ž��й��ˡ�
*/

class ASF_API CInitConfigSystem
{
	FOCP_FORBID_COPY(CInitConfigSystem);
	friend struct CSingleInstance<CInitConfigSystem>;
	friend class CApplication;
	struct CTableItem
	{
		CString oTableName;
		uint32 nDeploy;
		uint32 nNetworkType;
		uint32 nSubsystemType;
		CTextTable* pTable;

		CTableItem();
		CTableItem(const CTableItem& oSrc);
		CTableItem& operator=(const CTableItem& oSrc);
		~CTableItem();
	};
	struct CSubsystemItem
	{
		uint32 nSubsystemType;
		uint32 nSubsystemBits;
		uint32 nSubsystemOffset;
		uint32 nNetworkType;
		CString oSubsystemDesc;
	};
private:
	char m_sAppCfgHome[FOCP_MAX_PATH];
	char m_sNmsBaseCfgHome[FOCP_MAX_PATH];
	char m_sNmsAppCfgHome[FOCP_MAX_PATH];
	CTextTable m_oSubsystemConfig;
	CTextTable m_oConfigDeploy;
	CSubsystemItem m_oSystemSsn[4];
	uint32 m_nNetworkNo1, m_nNetworkNo2;
	CRbMap<CString, CTableItem, CNameCompare> m_oTextTables;

private:
	CInitConfigSystem();

public:
	~CInitConfigSystem();

	static CInitConfigSystem* GetInstance();

	bool Initialize(const char* sNmsCfgHome, const char* sAppCfgHome, const char* sAppName);

	bool Load();
	void UnLoad();

	//�ṩ�ú�����Ŀ���ǣ�Ӧ�ÿ���������ȡ�������ݡ�
	bool OpenConfig(CTextAccess& oAccess, const char* sTableName, bool bOptional=false);

private:
	bool WalkDeployTable(CTextAccess& oAccess, CTableItem &oItem);
	bool LoadTable(CTableItem &oItem);
	bool QuerySubsystem(CSubsystemItem &oItem, uint32 nSubsystemType);
	uint32 GetSubsystemNo(CSubsystemItem &oItem, uint32 nNetworkNo);
};

FOCP_END();

#endif //_Afc_TextTable_Hpp_
