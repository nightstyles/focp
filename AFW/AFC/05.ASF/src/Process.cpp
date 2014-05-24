
#include "Process.hpp"
#include "Argument.hpp"

#include <stdlib.h>
#ifdef UNIX
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/*
	Cygwin还需要调试
*/

FOCP_BEGIN();

static uint16 GetRunnerUdpPort()
{
	uint16 nPort = FOCP_RUNNER_UDPSERVICEPORT;
	const char* s = GetEnvVar("FOCP_RUNNER_UDPSERVICEPORT");
	if(s)
	{
		int32 d;
		if(StringScan(s, "%d", &d) == 1)
		{
			if(d > 0 && d < 65536)
				nPort = (uint16)d;
		}
	}
	return nPort;
}

static uint16 GetRunnerTcpPort()
{
	uint16 nPort = FOCP_RUNNER_TCPSERVICEPORT;
	const char* s = GetEnvVar("FOCP_RUNNER_TCPSERVICEPORT");
	if(s)
	{
		int32 d;
		if(StringScan(s, "%d", &d) == 1)
		{
			if(d > 0 && d < 65536)
				nPort = (uint16)d;
		}
	}
	return nPort;
}

ASF_API void StartApp(int32 nArgc, char* sArgv[], bool bLoop, uint16 nVirtualPort)
{
	CFile oFile;
	CFormatString oFileName, oVirtualPort;
	uint16 nServicePort = GetRunnerUdpPort();
	oFileName.Print("udp://127.0.0.1:%u16", nServicePort);
	if(oFile.Open(oFileName.GetStr()) == 0)
	{
		uint32 nOffset=0,nPkgSize = sizeof(uint32);
		const char* sFocpRunShellPath = GetEnvVar("FocpRunShellPath");
		char sRunPath[FOCP_MAX_PATH];
		CDiskFileSystem* pFs = CDiskFileSystem::GetInstance();
		if(pFs->GetFullPath(sFocpRunShellPath, sRunPath))
		{
			pFs->GetOsPathName(sRunPath);
			sFocpRunShellPath = sRunPath;
		}
		else
			sFocpRunShellPath = NULL;
		nPkgSize += CString::StringLength(sFocpRunShellPath)+1;
		uint32 xArgc = nArgc + 1;
		if(bLoop)
		{
			++xArgc;
			nPkgSize += 3;
		}
		if(nVirtualPort)
		{
			++xArgc;
			oVirtualPort.Print("-v%u16", nVirtualPort);
			nPkgSize += oVirtualPort.GetSize()+1;
#if defined(WINDOWS) || defined(CYGWIN_NT)
			++xArgc;
			nPkgSize += 3;//"-h"
#endif
		}
		int32 i;
		for(i=0; i<nArgc; ++i)
			nPkgSize += CString::StringLength(sArgv[i])+1;
		CBinary oBinary(nPkgSize);

		nPkgSize = sizeof(uint32);
		oBinary.Write(nOffset, (const uint8*)&xArgc, nPkgSize);
		nOffset+=nPkgSize;

		nPkgSize = CString::StringLength(sFocpRunShellPath)+1;
		oBinary.Write(nOffset, (const uint8*)sFocpRunShellPath, nPkgSize);
		nOffset+=nPkgSize;

		if(bLoop)
		{
			nPkgSize = 3;
			oBinary.Write(nOffset, (const uint8*)"-r", nPkgSize);
			nOffset+=nPkgSize;
		}

		if(nVirtualPort)
		{
			nPkgSize = oVirtualPort.GetSize()+1;
			oBinary.Write(nOffset, (const uint8*)oVirtualPort.GetStr(), nPkgSize);
			nOffset+=nPkgSize;
#if defined(WINDOWS) || defined(CYGWIN_NT)
			nPkgSize = 3;
			oBinary.Write(nOffset, (const uint8*)"-h", nPkgSize);
			nOffset+=nPkgSize;
#endif
		}
		for(i=0; i<nArgc; ++i)
		{
			nPkgSize = CString::StringLength(sArgv[i])+1;
			oBinary.Write(nOffset, (const uint8*)sArgv[i], nPkgSize);
			nOffset+=nPkgSize;
		}
		oFile.Write(oBinary.GetData(), oBinary.GetSize());
	}
}

ASF_API int32 System(const char* sCmd)
{
//#if defined(WINDOWS) || defined(CYGWIN_NT)
	return system(sCmd);
/*#else
	int pid;
	CArguments oArg;
	oArg.SetCmdLine(sCmd);
	if(0 == (pid=vfork()))
	{
		int fd, fdtablesize;
		fdtablesize = getdtablesize();
		for(fd = 3; fd < fdtablesize; fd++)
			close(fd);
		if (setsid() < 0)
			setpgid(getpid(), getpid());
		int r = execv(oArg.GetArgv()[0], oArg.GetArgv()+1);
		_exit(r);
		return (0);
	}
	else
		return waitpid(pid, NULL, 0);
#endif*/
}

CPipeOpen::CPipeOpen(bool bThread)
	:m_oMutex(bThread),m_oEvent(bThread),m_oCooperator((CCooperateFunction*)this, bThread)
{
	m_bQuit = true;
	m_bThread = bThread;
}

CPipeOpen::~CPipeOpen()
{
}

int32 CPipeOpen::ReadOutput(char* pBuf, uint16 nBufSize, bool &bError)
{
	int32 nRet;
	COutputMsg* pMsg;
	if(pBuf == NULL || !nBufSize)
		return 0;
	m_oEvent.Wait(1000);
	m_oMutex.Enter();
	pMsg = m_oOutputMsgList.Head();
	if(pMsg)
	{
		bError = pMsg->bError?true:false;
		uint32 nRest = pMsg->nSize - pMsg->nOffset;
		if(nBufSize > nRest)
			nBufSize = nRest;
		CBinary::MemoryCopy(pBuf, pMsg->sBuf+pMsg->nOffset, nBufSize);
		pMsg->nOffset += nBufSize;
		if(pMsg->nOffset == pMsg->nSize)
		{
			m_oOutputMsgList.Pop();
			if(!m_oOutputMsgList.GetSize())
				m_oEvent.Reset();
		}
		nRet = nBufSize;
	}
	else if(m_bQuit)
		nRet = -1;
	else
		nRet = 0;
	m_oMutex.Leave();
	return nRet;
}

int32 CPipeOpen::WriteInput(const char* pBuf)
{
	uint32 nBufSize = CString::StringLength(pBuf);
	if(nBufSize==0 || nBufSize > 1024)
		return 0;
	if(m_bQuit)
		return -1;
	uint16 nHead[2] = {2, (uint16)nBufSize};
	//0:文件中断、磁盘满等因素，-1:IO异常，>0:返回长度
	int32 nRet = m_oFile.Write(nHead, 4);
	if(nRet != 4)
	{
		m_oFile.Close();
		return -1;
	};
	nRet = m_oFile.Write(pBuf, nBufSize);
	if(nRet <= 0)
	{
		m_oFile.Close();
		return -1;
	}
	return nRet;
}

static bool TcpRecv(CFile& oFile, char *pBuf, int32 nBufLen)
{
	while(nBufLen)
	{
		int32 nRet = oFile.Read(pBuf, nBufLen);
		if(nRet <= 0)
			return false;
		nBufLen -= nRet;
		pBuf += nRet;
	}
	return true;
}

void CPipeOpen::MainProc(CCooperator* pCooperator, bool &bRunning)
{
	m_bQuit = false;
	while(bRunning)
	{
		COutputMsg* pMsg = m_oOutputMsgList.Tail();
		if(!TcpRecv(m_oFile, pMsg->sBuf, 4))
		{
			delete pMsg;
			break;
		}
		unsigned short nMode = ((unsigned short*)pMsg->sBuf)[0];
		if(nMode != 3 && nMode != 4)//StdOut/StdErr Msg
		{
			delete pMsg;
			break;
		}
		unsigned short nSize = ((unsigned short*)pMsg->sBuf)[1];
		if(nSize == 0 || nSize > 1024)//invalid StdIn Msg Len
		{
			delete pMsg;
			break;
		}
		if(!TcpRecv(m_oFile, pMsg->sBuf, (int)nSize))
		{
			delete pMsg;
			break;
		}
		pMsg->bError = (nMode == 4);
		m_oMutex.Enter();
		m_oOutputMsgList.Push();
		if(m_oOutputMsgList.GetSize() == 1)
			m_oEvent.Set();
		m_oMutex.Leave();
	}
	m_bQuit = true;
}

bool CPipeOpen::StartApp(int32 nArgc, char* sArgv[], uint16 nVirtualPort)
{
	CFormatString oFileName;
	if(nVirtualPort == 0 || nArgc <= 0 || !m_bQuit)
		return false;

	m_oFile.Close();
	uint16 nServicePort = GetRunnerUdpPort();
	oFileName.Print("udp://127.0.0.1:%u16|127.0.0.1:0", nServicePort);
	if(m_oFile.Open(oFileName.GetStr()))//, m_bThread?"w":"wf")
		return false;
	CString oBindName = m_oFile.GetFileName().oBindName;
	char* pMsg= new char[4096];
	char* pShift = pMsg;
	int32 xArgc = nArgc + 2;//RunShell -v
#if defined(WINDOWS) || defined(CYGWIN_NT)
	++xArgc;//-h
#endif
	++xArgc;//nVirtualPort
	CBinary::MemoryCopy(pShift, &xArgc, sizeof(xArgc));
	pShift += sizeof(xArgc);
	const char* sFocpRunShellPath = GetEnvVar("FocpRunShellPath");
	char sRunPath[FOCP_MAX_PATH];
	CDiskFileSystem* pFs = CDiskFileSystem::GetInstance();
	if(pFs->GetFullPath(sFocpRunShellPath, sRunPath))
	{
		pFs->GetOsPathName(sRunPath);
		sFocpRunShellPath = sRunPath;
	}
	else
		sFocpRunShellPath = NULL;
	CString::StringCopy(pShift, sFocpRunShellPath);
	pShift += CString::StringLength(pShift)+1;
	CIpAddrList oAddrList;
	CFile::GetIpAddrList(oBindName.GetStr(), oAddrList);
	StringPrint(pShift, "-c%u16", oAddrList.nPort);
	pShift += CString::StringLength(pShift)+1;
	StringPrint(pShift, "-v%u16", nVirtualPort);
	pShift += CString::StringLength(pShift)+1;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	CString::StringCopy(pShift, "-h");
	pShift += CString::StringLength(pShift)+1;
#endif
	int32 i;
	for(i=0; i<nArgc; ++i)
	{
		CString::StringCopy(pShift, sArgv[i]);
		pShift += CString::StringLength(pShift)+1;
	}
	if(pShift-pMsg > 4096)
	{
		m_oFile.Close();
		return false;
	}
	m_oFile.Write(pMsg, pShift-pMsg);
	char bMaybe;
	if(1 != m_oFile.Read(&bMaybe, 1, 5000) || bMaybe != '1')
	{
		m_oFile.Close();
		return false;
	}

	delete pMsg;
	m_oFile.Close();
	nServicePort = GetRunnerTcpPort();
	oFileName.Print("udp://127.0.0.1:%u16", nServicePort);
	if(m_oFile.Open(oFileName.GetStr(), m_bThread?"w":"wf"))
		return false;
	uint16 pCmd[2] = {5, nVirtualPort};
	if(4 != m_oFile.Write(pCmd, 4))
	{
		m_oFile.Close();
		return false;
	}
	if(!TcpRecv(m_oFile, (char*)pCmd, 4) || pCmd[0] != 6 || pCmd[1] != 1)
	{
		m_oFile.Close();
		return false;
	}

	m_oCooperator.Start();
	return true;
}

FOCP_END();
