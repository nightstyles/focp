
#include "VmmControl.hpp"

FOCP_BEGIN();

CVmmControl::CVmmControl()
{
	m_pStatus = NULL;
}

CVmmControl::~CVmmControl()
{
	if(m_pStatus)
		delete[] m_pStatus;
}

int32 CVmmControl::Startup(const char* sControlFile, uint32 nStatusNum)
{
	bool bNew = false;
	m_nStatusNum = nStatusNum;
	m_pStatus = new uint8[nStatusNum];
	if(!m_pStatus)
	{
		FocpError(("VmmControl::Startup(%s) no enough memory", sControlFile));
		return 1;
	}
	CString oFileName("disk://");
	oFileName += sControlFile;
	sControlFile = oFileName.GetStr();
	if(m_nControlFile.Open(sControlFile, "w"))
	{
		if(m_nControlFile.Open(sControlFile, "c"))
		{
			FocpError(("VmmControl::Startup(%s) open vmm control file failure", sControlFile));
			return 1;
		}
		bNew = true;
	}
	if(bNew)
	{
		CBinary::MemorySet(m_pStatus, 0, m_nStatusNum);
		if(m_nControlFile.Write(m_pStatus, m_nStatusNum) != (int32)m_nStatusNum)
		{
			FocpError(("VmmControl::Startup(%s) write vmm control file failure", sControlFile));
			return 1;
		}
	}
	else
	{
		if(m_nControlFile.Read(m_pStatus, m_nStatusNum) != (int32)m_nStatusNum)
		{
			FocpError(("VmmControl::Startup(%s) read vmm control file failure", sControlFile));
			return 1;
		}
	}
	return 0;
}

uint8 CVmmControl::GetStatus(uint32 nIdx)
{
	if(m_nStatusNum <= nIdx)
		throw 1;
	return m_pStatus[nIdx];
}

void CVmmControl::SetStatus(uint32 nIdx, uint8 nStatus)
{
	if(m_nStatusNum <= nIdx)
		throw 1;
	m_oMutex.Enter();
	if(nStatus != m_pStatus[nIdx])
	{
		m_pStatus[nIdx] = nStatus;
		m_nControlFile.Seek(FOCP_SEEK_SET, nIdx);
		m_nControlFile.Write(m_pStatus + nIdx, 1);
	}
	m_oMutex.Leave();
}

FOCP_END();
