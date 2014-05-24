
#include "EvmFile.hpp"

FOCP_BEGIN();

CEvmFile::CEvmFile()
{
	m_nFileType = EVM_OBJ_FILE;//表示无效
	m_nCodeSize = 0;
	m_nDataSize = 0;
	m_nConstSize = 0;
	m_nSymCount = 0;
	m_nLibCount = 0;
	m_nHostCount = 0;
	m_nExternCount = 0;
	m_bLinked = 0;
	m_pCodeSegment = NULL;
	m_pConstSegment = NULL;
	m_pSymbolTable = NULL;
	m_pLibTable = NULL;
	m_pHostTable = NULL;
	m_pEntry = NULL;
}

CEvmFile::~CEvmFile()
{
	if(m_pCodeSegment)
	{
		delete[] m_pCodeSegment;
		m_pCodeSegment = NULL;
	}
	if(m_pConstSegment)
	{
		delete[] m_pConstSegment;
		m_pConstSegment = NULL;
	}
	if(m_pSymbolTable)
	{
		delete[] m_pSymbolTable;
		m_pSymbolTable = NULL;
	}
	if(m_pLibTable)
	{
		delete[] m_pLibTable;
		m_pLibTable = NULL;
	}
	if(m_pHostTable)
	{
		delete[] m_pHostTable;
		m_pHostTable = NULL;
	}
}

const CString& CEvmFile::GetFileName() const
{
	return m_oFileName;
}

bool CEvmFile::Load(const char* sFileName)
{
	CFile oFile;
	if(oFile.Open(sFileName, "r"))
	{
		FocpLog(FOCP_LOG_WARNING, ("Load(%s): open file failure", sFileName));
		return false;
	}

	CEvmFileHead oHead;
	if(sizeof(oHead) != oFile.Read(&oHead, sizeof(oHead)))
	{
		FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
		return false;
	}

	if(!CBinary::MemoryCompare(oHead.magic, "CEXE", 4))
		m_nFileType = EVM_EXE_FILE;
	else if(!CBinary::MemoryCompare(oHead.magic, "CDLL", 4))
		m_nFileType = EVM_DLL_FILE;
	else
	{
		FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
		return false;
	}
	bool bNeedConvert = ((oHead.endian!=0) != IsSmallEndian());
	if(bNeedConvert)
	{
		oHead.version0 = CBinary::U16Code(oHead.version0);
		oHead.version1 = CBinary::U16Code(oHead.version1);
		oHead.version2 = CBinary::U16Code(oHead.version2);
	}
	if(oHead.version0 > EVM_FILE_VERSION_0)
	{
		FocpLog(FOCP_LOG_WARNING, ("Load(%s): version too new", sFileName));
		return false;
	}
	else if(oHead.version0 == EVM_FILE_VERSION_0)
	{
		if(oHead.version1 > EVM_FILE_VERSION_1)
		{
			FocpLog(FOCP_LOG_WARNING, ("Load(%s): version too new", sFileName));
			return false;
		}
		else if(oHead.version1 == EVM_FILE_VERSION_1)
		{
			if(oHead.version2 > EVM_FILE_VERSION_2)
			{
				FocpLog(FOCP_LOG_WARNING, ("Load(%s): version too new", sFileName));
				return false;
			}
		}
	}

	ehc_uint pSection[2], nEntry, nTag = 0;
	bool bReadDone[] = {true,false,false,false,false,false,false,false};

	while(true)
	{
		if(sizeof(pSection) != oFile.Read(pSection, sizeof(pSection)))
		{
			FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
			return false;
		}
		if(bNeedConvert)
		{
			pSection[0] = CBinary::U32Code(pSection[0]);
			pSection[1] = (ehc_int)CBinary::U32Code((ehc_uint)pSection[1]);
		}
		if((pSection[0] >= EVM_INVALID_SECTION) || bReadDone[pSection[0]] || (pSection[0] <= nTag) || (pSection[1] % sizeof(ehc_uint)))
		{
			FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
			return false;
		}
		nTag = pSection[0];
		bReadDone[pSection[0]] = true;
		switch(nTag)
		{
		case EVM_CODESEG_SECTION:
			m_nCodeSize = pSection[1];
			m_pCodeSegment = NULL;
			if(m_nCodeSize)
			{
				m_pCodeSegment = new ehc_char[m_nCodeSize];
				if(m_nCodeSize != oFile.Read(m_pCodeSegment, m_nCodeSize))
				{
					FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
					return false;
				}
			}
			break;
		case EVM_DATASEG_SECTION:
			if(pSection[1] != (ehc_int)sizeof(ehc_uint))
			{
				FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
				return false;
			}
			if(sizeof(ehc_int) != oFile.Read(&m_nDataSize, sizeof(ehc_int)))
			{
				FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
				return false;
			}
			if(bNeedConvert)
				m_nDataSize = CBinary::U32Code(m_nDataSize);
			break;
		case EVM_CONSTSEG_SECTION:
			m_nConstSize = pSection[1];
			m_pConstSegment = NULL;
			if(m_nConstSize)
			{
				m_pConstSegment = new ehc_char[m_nConstSize];
				if(m_nConstSize != oFile.Read(m_pConstSegment, m_nConstSize))
				{
					FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
					return false;
				}
			}
			break;
		case EVM_SYMTAB_SECTION:
			if((pSection[1] % sizeof(CEvmSymbolItem)) || (nFileSize < pSection[1]))
			{
				FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
				return false;
			}
			m_nSymCount = pSection[1] / sizeof(CEvmSymbolItem);
			if(m_nSymCount)
			{
				CEhcSymbolItem* pSymbolTable = new CEhcSymbolItem[m_nSymCount];
				if(pSection[1] != oFile.Read(pSymbolTable, pSection[1]))
				{
					delete[] pSymbolTable;
					FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
					return false;
				}
				if(bNeedConvert)
				{
					for(ehc_int i=0; i<m_nSymCount; ++i)
					{
						pSymbolTable[i].name = CBinary::U32Code(pSymbolTable[i].name);
						pSymbolTable[i].type = CBinary::U32Code(pSymbolTable[i].type);
						pSymbolTable[i].opt = CBinary::U32Code(pSymbolTable[i].opt);
						pSymbolTable[i].arg = CBinary::U32Code(pSymbolTable[i].arg);
					}
				}
				m_pSymbolTable = new CEvmSymbolItem[m_nSymCount];
				for(ehc_int i=0; i<m_nSymCount; ++i)
				{
					if(pSymbolTable[i].name >= (ehc_uint)m_nConstSize)
					{
						delete[] pSymbolTable;
						FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
						return false;
					}
					m_pSymbolTable[i].name = m_pConstSegment + pSymbolTable[i].name;
					m_pSymbolTable[i].type = pSymbolTable[i].type;
					m_pSymbolTable[i].opt = pSymbolTable[i].opt;
					m_pSymbolTable[i].addr = NULL;
					m_pSymbolTable[i].file = NULL;
					m_pSymbolTable[i].idx = 0xFFFFFFFF;
					ehc_int nMaxLen = m_nConstSize - pSymbolTable[i].name;
					ehc_int nNameLen = (ehc_int)CString::StringLength(m_pSymbolTable[i].name, (ehc_uint)nMaxLen);
					if(nNameLen >= nMaxLen || !nNameLen)
					{//符号名不合法
						delete[] pSymbolTable;
						FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
						return false;
					}
					if(!(pSymbolTable[i].type&EVM_SHARE_SYM))
					{//可执行文件或动态库符号表中的符号应全是共享符号。
						delete[] pSymbolTable;
						FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
						return false;
					}
					m_pSymbolTable[i].type &= ~EVM_SHARE_SYM;
					switch(m_pSymbolTable[i].type)
					{
					default:
						delete[] pSymbolTable;
						FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
						return false;
					case EVM_VARIABLE_SYM:
						switch(m_pSymbolTable[i].opt)
						{
						default:
							delete[] pSymbolTable;
							FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
							return false;
						case EHC_UN: //外部变量符号，需要连接
							++m_nExternCount;
							break;
						case EHC_DS: //输出变量符号，可被连接
							if(pSymbolTable[i].arg >= m_nDataSize)
							{
								delete[] pSymbolTable;
								FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
								return false;
							}
							break;
						}
						break;
					case EVM_FUNCTION_SYM:
						switch(m_pSymbolTable[i].opt)
						{
						default:
							delete[] pSymbolTable;
							FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
							return false;
						case EHC_UN: //外部函数符号，需要连接
							++m_nExternCount;
							break;
						case EHC_CS: //输出函数符号，可被连接
							if(pSymbolTable[i].arg >= m_nCodeSize || (pSymbolTable[i].arg%sizeof(ehc_uint)))
							{//代码越界或无效指令边界
								delete[] pSymbolTable;
								FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
								return false;
							}
							m_pSymbolTable[i].addr = m_pCodeSegment + pSymbolTable[i].arg;
							break;
						case EHC_HS:
							break;
						}
						break;
					}
				}
				delete[] pSymbolTable;
			}
			break;
		case EVM_LIBTAB_SECTION:
			m_nLibCount = pSection[1] / sizeof(m_nLibCount);
			m_pLibTable = NULL;
			if(m_nLibCount)
			{
				ehc_uint* pLibTable = new ehc_uint[m_nLibCount];
				if(pSection[1] != oFile.Read(pLibTable, pSection[1]))
				{
					delete[] pLibTable;
					FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
					return false;
				}
				if(bNeedConvert)
				{
					for(ehc_int i=0; i<m_nLibCount; ++i)
						pLibTable[i] = CBinary::U32Code(pLibTable[i]);
				}
				m_pLibTable = new ehc_char*[m_nLibCount];
				for(ehc_int i=0; i<m_nLibCount; ++i)
				{
					if(pLibTable[i] >= (ehc_uint)m_nConstSize)
					{
						delete[] pLibTable;
						FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
						return false;
					}
					m_pLibTable[i] = m_pConstSegment + pLibTable[i];
					ehc_int nMaxLen = m_nConstSize - pLibTable[i];
					ehc_int nNameLen = (ehc_int)CString::StringLength(m_pLibTable[i], (ehc_uint)nMaxLen);
					if(nNameLen >= nMaxLen || !nNameLen)
					{//符号名不合法
						delete[] pLibTable;
						FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
						return false;
					}
				}
				delete[] pLibTable;
			}
			break;
		case EVM_HOSTTAB_SECTION:
			m_nHostCount = pSection[1] / sizeof(m_nHostCount);
			m_pHostTable = NULL;
			if(m_nHostCount)
			{
				ehc_uint* pHostTable = new ehc_uint[m_nHostCount];
				if(pSection[1] != oFile.Read(pHostTable, pSection[1]))
				{
					delete[] pHostTable;
					FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
					return false;
				}
				if(bNeedConvert)
				{
					for(ehc_int i=0; i<m_nHostCount; ++i)
						pHostTable[i] = CBinary::U32Code(pHostTable[i]);
				}
				m_pHostTable = new CDynamicLibrary[m_nHostCount];
				for(ehc_int i=0; i<m_nHostCount; ++i)
				{
					if(pHostTable[i] >= (ehc_uint)m_nConstSize)
					{
						delete[] pHostTable;
						FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
						return false;
					}
					ehc_har* name = m_pConstSegment + pHostTable[i];
					ehc_int nMaxLen = m_nConstSize - pHostTable[i];
					ehc_int nNameLen = (ehc_int)CString::StringLength(name, (ehc_uint)nMaxLen);
					if(nNameLen >= nMaxLen || !nNameLen)
					{//符号名不合法
						delete[] pHostTable;
						FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
						return false;
					}
					if(!m_pHostTable[i].Load(name))
					{
						delete[] pHostTable;
						FocpLog(FOCP_LOG_WARNING, ("Load(%s): load host file(%s) failure", sFileName, name));
						return false;
					}
				}
				delete[] pHostTable;
			}
			break;
		case EVM_ENTRY_SECTION:
			if(pSection[1] != (ehc_int)sizeof(ehc_uint))
			{
				FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
				return false;
			}
			if(sizeof(ehc_uint) != oFile.Read(&nEntry, sizeof(ehc_uint)))
			{
				FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
				return false;
			}
			if(bNeedConvert)
				nEntry = CBinary::U32Code(nEntry);
			if((nEntry >= m_nCodeSize) || (nEntry%sizeof(ehc_uint)))
			{
				FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid file", sFileName));
				return false;
			}
			m_pEntry = m_pCodeSegment + nEntry;
			break;
		}
	}

	//扫描符号表，对主机函数进行连接
	for(ehc_int i=0; i<m_nSymCount; ++i)
	{
		if(m_pSymbolTable[i].opt == EHC_HS)
		{
			for(ehc_int j=0; j<m_nHostCount; ++j)
			{
				m_pSymbolTable[i].addr = m_pHostTable[i].FindSymbol(m_pSymbolTable[i].name);
				if(m_pSymbolTable[i].addr)
					break;
			}
			if(m_pSymbolTable[i].addr == NULL)
			{
				FocpLog(FOCP_LOG_WARNING, ("Load(%s): host function(%s) link failure", sFileName, m_pSymbolTable[i].name));
				return false;
			}
		}
	}

	m_oFileName = sFileName;

	return true;
}

void CEvmFile::Link(CEvmFile* pFile)
{
	if(m_nExternCount == 0)
		return;
	for(ehc_int i=0; i<m_nSymCount; ++i)
	{
		if(m_pSymbolTable[i].opt == EHC_UN)
		{
			for(ehc_int j=0; j<pFile->m_nSymCount; ++j)
			{
				if((m_pSymbolTable[i].type == pFile->m_pSymbolTable[j].type) && 
					(pFile->m_pSymbolTable[j].opt != EHC_UN) && 
					!CString::StringCompare(m_pSymbolTable[i].name, pFile->m_pSymbolTable[j].name))
				{
					if(m_pSymbolTable[i].type == EVM_VARIABLE_SYM)
					{
						if(pFile->m_pSymbolTable[j].opt == EHC_DS)
						{
							m_pSymbolTable[i].idx = j;
							m_pSymbolTable[i].file = pFile;
							break;
						}
					}
					else if(pFile->m_pSymbolTable[j].opt == EHC_CS)
					{
						m_pSymbolTable[i].addr = pFile->m_pSymbolTable[j];
						m_pSymbolTable[i].file = pFile;
						break;
					}
				}
			}
		}
	}
}

bool CEvmFile::CheckLink()
{
	if(m_bLinked)
		return true;

	if(m_nExternCount == 0)
	{
		m_bLinked = 1;
		return true;
	}

	for(ehc_int i=0; i<m_nSymCount; ++i)
	{
		if(m_pSymbolTable[i].opt == EHC_UN && (m_pSymbolTable[i].file==NULL))
			return false;
	}

	m_bLinked = 1;
	return true;
}

CEvmFileManager::CEvmFileManager()
{
}

CEvmFileManager::~CEvmFileManager()
{
	Clear();
}

void CEvmFileManager::Clear()
{
	CRbTreeNode* pEnd = m_oFiles.End();
	CRbTreeNode* pIt = m_oFiles.First;
	for(; pIt!=pEnd; pIt=m_oFiles.GetNext(pIt))
	{
		CNode& oFile = m_oFiles.GetItem(pIt);
		delete oFile.pFile;
	}
	m_oFiles.Clear();
}

const CString* CEvmFileManager::CGetFileName::GetKey(const CNode& oFile)
{
	return &oFile.pFile->GetFileName();
}

void CEvmFileManager::GetFullName(const char* sFileName, char* sFullName)
{
	sFullName[0] = '\0';
	if(sFileName == NULL || !sFileName[0])
		return;
	CPathDetailInfo oDetailInfo;
	CDiskFileSystem* pFileSystem = CDiskFileSystem::GetInstance();
	if(CString::CharOfString(sFileName, '/') || CString::CharOfString(sFileName, '\\'))
	{
		if(!pFileSystem->GetFullPath(sFileName, sFullName))
		{
			sFullName[0] = '\0';
			FocpLog(FOCP_LOG_WARNING, ("Load(%s): invalid filename", sFileName));
			return;
		}
	}
	else
	{
		const char* sPrefixName = NULL, *sPostfixName = ".e";
		if(!pFileSystem->SearchFile("EVM_PATH", sFileName, sFullName, sPrefixName, sPostfixName))
		{
			sPostfixName = ".d";
			if(!pFileSystem->SearchFile("EVM_LDPATH", sFileName, sFullName, sPrefixName, sPostfixName))
			{
				sFullName[0] = '\0';
				FocpLog(FOCP_LOG_WARNING, ("Load(%s): search file failure", sFileName));
				return;
			}
		}
	}
}

CEvmFile* CEvmFileManager::Load(const char* sFileName)
{
	char sFullName[FOCP_MAX_PATH];
	GetFullName(sFileName, sFullName);
	if(sLibName2[0] == '\0')
		return NULL;
	CEvmFile* pRet = NULL;
	CRbTreeNode* pEnd = m_oFiles.End();
	CRbTreeNode* pIt = m_oFiles.Find(sFullName);
	if(pIt != pEnd)
	{
		CNode& oFile = m_oFiles.GetItem(pIt);
		pRet = oFile.pFile;
		++oFile.nCounter;
	}
	else
	{
		pRet = new CEvmFile;
		if(pRet->Load(sFullName))
		{
			CNode oFile = {pRet, 1};
			m_oFiles.Insert(oFile);
		}
		else
		{
			delete pRet;
			pRet = NULL;
		}
	}
	return pRet;
}

void CEvmFileManager::UnLoad(CEvmFile* pFile)
{
	if(pFile == NULL)
		return;
	CRbTreeNode* pEnd = m_oFiles.End();
	CRbTreeNode* pIt = m_oFiles.Find(oFile.GetFileName());
	if(pIt != pEnd())
	{
		CNode& oFile = m_oFiles.GetItem(pIt);
		if(oFile.pFile == pFile)
		{
			--oFile.nCounter;
			if(!oFile.nCounter)
			{
				delete oFile.pFile;
				m_oFiles.Remove(pIt);
			}
		}
	}
}

FOCP_END();
