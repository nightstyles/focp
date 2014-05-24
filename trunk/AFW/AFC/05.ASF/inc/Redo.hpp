
#include "AsfDef.hpp"

#ifndef _Afs_Redo_Hpp_
#define _Afs_Redo_Hpp_

FOCP_BEGIN();

struct CRedoControlInfo
{
	uint32 nFileNo;
	uint32 nPos;
	uint8 nSequence;
};

typedef void (*FOnDoRedo)(char* pMsg, uint32 nMsgSize);
class ASF_API CRedoLog
{
private:
	CMutex m_oMutex;
	CFormatString m_oPath;
	CFile m_nFile;
	CFile m_nCtrlFile;
	CRedoControlInfo m_oControlInfo;
	FOnDoRedo m_OnRedo;
	uint32 m_nLogFileSize;
	uint32 m_nLogFileNum;

public:
	CRedoLog();
	~CRedoLog();

	uint32 Create(char* sRdoFile, char* sPath, FOnDoRedo OnRedo, uint32 nLogFileSize, uint32 nLogFileNum);

	uint32 WriteRedo(char* sRedo, uint32 nSize);
	uint32 WriteRedoStream(CMemoryStream* pStream);

	void Redo();
	void Reset();

private:
	void Commit();
	uint32 OpenRedoFile(bool bNotCreate=false);
	void CloseRedoFile();
};

FOCP_END();

#endif
