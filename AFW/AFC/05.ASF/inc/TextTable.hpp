
#include "AsfDef.hpp"

#ifndef _Asf_TextTable_Hpp_
#define _Asf_TextTable_Hpp_

FOCP_BEGIN();

/*
文件数据格式定义【文本文件】：
	【1】：#开始的行尾注释行
	【2】：第一个非注释行为列名行，其它非注释行为数据行，空行也相当于注释行
	【3】：列之间采用空格或水平制表符隔开。
	【4】：行之间采用[回车]换行符
	【5】：列名采用C标识符词法。
	【6】：列值支持C转义符表达【单引号、双引号可直接使用】:
				\w													=>		空格【新增】
				\r													=>		回车
				\n													=>		换行
				\f													=>		换页
				\b													=>		退格
				\v													=>		垂直制表符
				\t													=>		水平制表符
				\a													=>		警告
				\[x|X]HH										=>		十六进制转义
				\任何字符										=> 		任何字符
	【6】：空值：NULL[大小写无关]，如果值就是文本NULL，就用\NULL表示
*/

class CApplication;
class CTextTable;
class CTextAccess;
class CInitConfigSystem;

FOCP_DETAIL_BEGIN();
struct ASF_API CFieldValue
{
	char* sVal;
	uint32 nSize;//包括结尾0

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

	//遍历表格
	uint32 GetRowCount();
	uint32 GetColCount();
	const char* GetCell(uint32 nRow, uint32 nCol, uint32 &nLength);//返回NULL表示空值
	const char* GetColName(uint32 nCol);

	//清空文件数据
	void Truncate(bool bFlush=false);

	//将变化更新到文件中。
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

	//按条件读写表格
	void OpenIdxVal();
	void OpenColVal();
	bool SetIdxVal(const char * sIdxName, const char * sIdxVal, bool bSensitive=true, uint32 nLength=0);//NULL表示空值
	bool SetColVal(const char * sColName, const char * sColVal, uint32 nLength=0);//NULL表示空值

	void Insert(bool bFlush=false);//根据SetColVal
	void Update(bool bFlush=false);//根据SetIdxVal和SetColVal
	void Delete(bool bFlush=false);//根据SetIdxVal
	bool Query();//根据SetIdxVal
	const char* GetVal(const char * sColName, uint32 &nLength);//根据Query或QueryNext,返回NULL表示空值

private:
	void OpenAdditiveIdxVal();
	bool SetAdditiveIdxVal(const char * sIdxName, const char * sIdxVal, bool bSensitive=true, uint32 nLength=0);//NULL表示空值
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
	预定义表属性:
		MinRecordNum 最小记录数
		MaxRecordNum 最大记录数
		DynamicMode：
			0固定表格，不允许对表格进行增删，必须有出厂数据
			1静态表格，在系统运行期间修改无效或不允许修改。
			2动态表格，在系统运行期间的修改可实时生效。
	预定义字段属性:
		DataType:
			string, 			普通文本
			int,					整型，int32的取值范围
			uint,					无符号整型，uint32的取值范围
			long,					长整型，int64的取值范围
			ulong,				无符号长整型，uint64的取值范围
			bool,					取值范围为'1'、'0'、'true'、'false'、'yes'、'no'
			datetime			UTC时间格式的字符串
			base64,				base64编码串
			binary,				二进制数据串
		RangeList:
			（1）为空表示不另外添加范围限制。
			（2）由{}括起来的多个范围组合，范围之间用逗号分隔
			（3）范围可由冒号分隔，如
				（A）a:b，表示最小为a，最大为b
				（b）a:，表示最小为a，无最大限制
				（C）:b，表示最大为b，无最小限制
				（D）c，表示为单值c，即c:c的简写
			（4）string的范围表示长度范围
			（5）int,uint, long, ulong的范围表示数值的取值范围。
			（6）datetime，base64，binary无取值范围。
			（7）范围列表还可单独RageList表格中定义，这里仅仅引用范围名称即可。
		EnumList：
			（1）由{}括起来的符合数据格式和取值范围的枚举列表，进一步限制其取值
			（2）枚举列表还可单独EnumList表格中定义，这里仅仅引用枚举名称即可。
		DefaultValue：
			符合数据格式和取值范围的缺省值。
		NotNull：0可以为空，1不可以为空。
		DynamicMode：
			0只读字段，表示该字段是只读字段，不可以修改。
			1静态字段，在系统运行期间修改无效或不允许修改。
			2动态字段，表示该字段的修改可实时生效。
		SystemMode：
			0系统侧，表示该数据存储为网管系统侧。
			1应用侧，表示该数据存储于应用系统侧。
		StorageMode：
			0持久性数据，需要持久保存，系统重启时，能恢复出来。
			1临时性数据，仅存在于类型中，系统重启后就丢失了，可重新获取。
			2控制性数据，连内存中都不存在，需要预定义一个无意义的缺省值。
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
	有三个配置文件存放路径：
		（1）私有配置：    存放在$(AppCfgHome)/$(AppName)/cfg/目录下
		（2）网管基础配置：存放在$(NmsCfgHome)/目录下
		（2）网管引用配置：存放在$(NmsCfgHome)/$(AppName)/目录下
	有三张系统表:
		（1）网络规划配置：Network:固定放在基础配置下
				网络号	NetworkNo	UINT32	!=0xFFFFFFFF
				物理IP	PhysicalIp	IpAddress
		（2）子系统规划配置：SubSystem:固定放在基础配置下
				子系统号类型	SubsystemType	UINT32	>=1
				子系统号位长	SubsystemBits	UINT32	1..(32-DependOffset)
				子系统号偏移	SubsystemOffset	UINT32	0..31(从左到右)
				子系统号描述	SubsystemDesc	DisplayString	0..64
		（2）配置部署表：ConfigDeploy：固定放在私有配置下
				表名              TableName	Displaystring	1..256
				部署              Deploy		ENUM        0:私有配置； 1:网管应用配置； 2:网管基础配置；
				子系统号类型      SubsystemType	UINT32		0:表示不按子系统过滤。非0：按子系统过滤。
	数据过滤规则：
	（1）Deploy=0，不做任何过滤。表格可以不含“子系统号”字段和“应用类型”字段，如果要含，均为0xFFFFFFFF【建议包含】。
	（2）Deploy=1/2
			a.当SubsystemNoType为0时，不做子系统号过滤。数据表格可以不含“子系统号”字段，如果要含，就为0xFFFFFFFF【建议包含】。
			b.当SubsystemNoType非0时，数据表格需含‘子系统号’字段，从‘子系统表’提取子系统号，再按子系统号进行过滤。
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

	//提供该函数的目的是，应用可以自主读取配置数据。
	bool OpenConfig(CTextAccess& oAccess, const char* sTableName, bool bOptional=false);

private:
	bool WalkDeployTable(CTextAccess& oAccess, CTableItem &oItem);
	bool LoadTable(CTableItem &oItem);
	bool QuerySubsystem(CSubsystemItem &oItem, uint32 nSubsystemType);
	uint32 GetSubsystemNo(CSubsystemItem &oItem, uint32 nNetworkNo);
};

FOCP_END();

#endif //_Afc_TextTable_Hpp_
