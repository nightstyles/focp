
#include "EvmDef.hpp"

#ifndef _EVM_FILE_HPP_
#define _EVM_FILE_HPP_

FOCP_BEGIN();

enum
{
	EVM_ALIGN_SECTION = 0,
	EVM_CODESEG_SECTION,
	EVM_DATASEG_SECTION,
	EVM_CONSTSEG_SECTION,
	EVM_SYMTAB_SECTION,
	EVM_LIBTAB_SECTION,
	EVM_HOSTTAB_SECTION,
	EVM_ENTRY_SECTION,
	EVM_INVALID_SECTION,

	EVM_OBJ_FILE = 0,
	EVM_LIB_FILE,
	EVM_DLL_FILE,
	EVM_EXE_FILE,
};

enum
{
	EVM_VARIABLE_SYM = 1,
	EVM_FUNCTION_SYM = 2,
	EVM_SHARE_SYM  = 4
};

struct CEvmFileHead
{
	ehc_char magic[4];
	ehc_uchar endian, bites;
	ehc_ushort version0, version1, version2;
};

struct CEvmSection
{
	ehc_uint tag;
	ehc_int len;
	ehc_char v[1];
};

struct CEhcSymbolItem
{
	ehc_uint name, type, opt, arg;
};

enum
{
	EVM_FILE_VERSION_0 = 1,
	EVM_FILE_VERSION_1 = 0,
	EVM_FILE_VERSION_2 = 0
};

#ifndef EVM_SYMBOL_ITEM
#define EVM_SYMBOL_ITEM
struct CEvmSymbolItem
{
	ehc_char* name;//符号名
	ehc_char* addr;//符号地址,外部变量地址为空
	ehc_void* file;//外部文件指针
	ehc_uint type; //符号类型
	ehc_uint opt;  //符号属性
	ehc_uint idx;  //外部变量地址索引
};
#endif

class CEvmFile;
class CEvmFileManager;
class CEvmModule;
class CEvmProcess;
class CEvmProcessManager;

class EVM_API CEvmFile
{
	friend class CEvmFileManager;
	friend class CEvmModule;
	friend class CEvmProcess;
	friend ehc_char* GetEhcVmProcAddr(ehc_void* reg, ehc_uint nArg, CEhcVmSegment* pSegment);
private:
	CString m_oFileName;
	ehc_uint m_nFileType;
	ehc_int m_nCodeSize;
	ehc_int m_nDataSize;
	ehc_int m_nConstSize;
	ehc_int m_nSymCount;
	ehc_int m_nLibCount;
	ehc_int m_nHostCount;
	ehc_int m_nExternCount;
	ehc_int m_bLinked;
	ehc_char* m_pCodeSegment;
	ehc_char* m_pConstSegment;
	CEvmSymbolItem* m_pSymbolTable;
	ehc_char** m_pLibTable;
	CDynamicLibrary* m_pHostTable;
	ehc_char* m_pEntry;

public:
	CEvmFile();
	~CEvmFile();

	const CString& GetFileName() const;

private:
	bool Load(const char* sFileName);
	void Link(CEvmFile* pFile);
	bool CheckLink();
};

class EVM_API CEvmFileManager
{
	friend class CEvmProcess;
	friend class CEvmProcessManager;
private:
	struct CNode
	{
		CEvmFile* pFile;
		uint32 nCounter;
	};
	struct CGetFileName
	{
		static const CString* GetKey(const CNode& oFile);
	};
	CRbTree<CString, CNode, CGetFileName> m_oFiles;

public:
	CEvmFileManager();
	~CEvmFileManager();

	CEvmFile* Load(const char* sFileName);
	void UnLoad(CEvmFile* pFile);

private:
	void Clear();
	static void GetFullName(const char* sFileName);
};

FOCP_END();

#endif
