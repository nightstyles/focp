
#include "AcmSystemPlugIn.hpp"

FOCP_BEGIN();

CAsmStream::~CAsmStream()
{
}

CAsmStream::CAsmStream()
{
	m_bFirst = true;
	m_bFail = true;
	m_bReleased = true;
	m_nSeq = 0;
	m_nReadPos = 0;
	m_nTruncatePos = 0;
	m_pAssembler = NULL;
}

CAsmStream::CAsmStream(const CAsmStream& oSrc)
{
	m_bFirst = true;
	m_bFail = true;
	m_bReleased = true;
	m_nSeq = 0;
	m_nReadPos = 0;
	m_nTruncatePos = 0;
	m_pAssembler = NULL;
}

CAsmStream& CAsmStream::operator=(const CAsmStream& oSrc)
{
	return *this;
}

uint32 CAsmStream::Read(void* buf, uint32 nbuflen)
{
	if(nbuflen == (uint32)(-1) || !buf)
		return 0;
	m_oEvent.Wait(1000);
	m_oMutex.Enter();
	if(m_bFail)
	{
		m_oMutex.Leave();
		return -1;
	}
	m_oStream.SetPosition(m_nReadPos);
	uint32 nRet = m_oStream.Read(buf, nbuflen);
	if(nRet == 0)
		m_oEvent.Reset();
	else
	{
		uint32 nPos1 = m_oStream.GetPosition();
		m_oStream.LeftTrim();
		m_nReadPos = m_oStream.GetPosition();
		uint32 nMove = nPos1 - m_nReadPos;
		if(m_nTruncatePos >= nMove)
			m_nTruncatePos -= nMove;
		else
			m_nTruncatePos = 0;
	}
	m_oMutex.Leave();
	return nRet;
}

void CAsmStream::Assemble(CAsmDataMsg& oMsg)
{
	uint32 nSeq = oMsg.oHead.nSequence;
	uint32 nPackNo = CBinary::U16Code(*(uint16*)oMsg.sBody);
	m_oMutex.Enter();
	uint32 nSize = m_oStream.GetSize();
	if(m_bFirst)
	{
		if(nPackNo)
		{
			m_oMutex.Leave();
			return;
		}
		m_bFirst = false;
	}
	else if(m_bFail == false)
	{
		uint32 nNext = (m_nSeq + 1) % ADT_MAX_SEQUENCE;
		if(nSeq != nNext)
		{//³ö´íÁË
			m_bFail = true;
			m_oStream.SetPosition(m_nTruncatePos);
			m_oStream.Truncate();
			nSize = m_nTruncatePos;
			if(m_nReadPos > m_nTruncatePos)
				m_nReadPos = m_nTruncatePos;
			if(nSize == m_nReadPos)
				m_oEvent.Reset();
		}
	}
	if(nPackNo == 0)
	{
		if(m_bFail)
			m_bFail = false;
		else if(oMsg.oHead.nOp == ASM_SYSPLUGIN_OP_BIGPACK)//Auto Call
		{
			if(nSize)
			{
				uint8* pBuf = new uint8[nSize];
				m_oStream.SetPosition(0);
				m_oStream.Read(pBuf, nSize);
				m_pAssembler->m_pSequenceModule->ProcessMessage(*(CAsmDataMsg*)pBuf);
				delete[] pBuf;
				m_oStream.SetPosition(0);
				m_oStream.Truncate();
				nSize = 0;
			}
		}
		else
			m_nTruncatePos = nSize;
	}
	if(m_bFail == false)
	{
		uint32 nCount = oMsg.nSize-ASM_UDP_HEADSIZE-2;
		if(nCount)
		{
			if(oMsg.oHead.nOp == ASM_SYSPLUGIN_OP_STREAM && m_bReleased)//Common Stream
			{
				m_bReleased = false;
				if(0 == m_pAssembler->m_oAccepts.GetSize())
					m_pAssembler->m_oEvent.Set();
				m_pAssembler->m_oAccepts.Push(this);
			}
			m_nSeq = nSeq;
			if(nSize == m_nReadPos)
				m_oEvent.Set();
			m_oStream.SetPosition(nSize);
			if(oMsg.oHead.nOp == ASM_SYSPLUGIN_OP_BIGPACK && nPackNo == 0)
			{
				m_oStream.Write(&oMsg.oHead, ACM_UDP_HEADSIZE+4);
				uint16* pPlugIn = (uint16*)oMsg.sBody+1;
				pPlugIn[0] = CBinary::U16Code(pPlugIn[0]);//nPlugIn
				pPlugIn[1] = CBinary::U16Code(pPlugIn[1]);//nOp
			}
			m_oStream.Write(oMsg.sBody+2, nCount);
		}
	}
	m_oMutex.Leave();
}

void CAsmStream::Clear()
{
	m_oMutex.Enter();
	m_bFirst = true;
	m_bFail = true;
	m_oStream.SetPosition(m_nTruncatePos);
	m_oStream.Truncate();
	if(m_nReadPos > m_nTruncatePos)
		m_nReadPos = m_nTruncatePos;
	if(m_nTruncatePos == m_nReadPos)
		m_oEvent.Reset();
	m_oMutex.Leave();
}

void CAsmStream::Release()
{
	m_bReleased = true;
}

CAsmSystemPlugIn::CAsmSystemPlugIn(CAcmSequenceModule* pModule)
{
	m_pSequenceModule = pModule;
	pModule->RegisterPlugIn(ASM_SYSTEM_PLUGIN, this);
}

CAsmSystemPlugIn::~CAsmSystemPlugIn()
{
	m_pSequenceModule->DeRegisterPlugIn(ASM_SYSTEM_PLUGIN);
}

bool CAsmSystemPlugIn::Send(CAsmDataMsg &oMsg, uint32 nNode)
{
	return m_pSequenceModule->Send4(oMsg, nNode);
}

bool CAsmSystemPlugIn::Send(uint8* pStream, uint32 nStreamSize, uint32 nNode, uint16 *pPackNo)
{
	return m_pSequenceModule->Send5(pStream, nStreamSize, nNode, pPackNo);
}

CAsmStream* CAsmSystemPlugIn::Accept(uint32 &nNodeId)
{
	CAsmStream* pStream;
	m_oEvent.Wait(1000);
	m_oMutex.Enter();
	if(m_oAccepts.Pop(pStream))
	{
		if(m_oAccepts.GetSize() == 0)
			m_oEvent.Reset();
	}
	else
		pStream = NULL;
	m_oMutex.Leave();
	return pStream;
}

void CAsmSystemPlugIn::ProcessAsmPlugInMsg(CAcmSequenceModule* pModule, CAsmDataMsg& oMsg)
{
	uint32 nNode = oMsg.oHead.nNode;
	uint16 nOp = oMsg.oHead.nOp;
	if(nOp == ASM_SYSPLUGIN_OP_STREAM || nOp == ASM_SYSPLUGIN_OP_BIGPACK)
	{
		CRbMap<uint32, CAsmStream>* pStreams;
		if(nOp == ASM_SYSPLUGIN_OP_STREAM)
			pStreams = &m_oCommonStreams;
		else
			pStreams = &m_oAsmDataStreams;
		CRbTreeNode* pEnd = pStreams->End();
		CAsmStream* pStream;
		m_oMutex.Enter();
		CRbTreeNode* pIt = pStreams->Find(nNode);
		if(pIt == pEnd)
		{
			CAsmStream& oStream = (*pStreams)[nNode];
			oStream.m_pAssembler = this;
			pStream = &oStream;
		}
		else
		{
			CAsmStream& oStream = pStreams->GetItem(pIt);
			pStream = &oStream;
		}
		pStream->Assemble(oMsg);
		m_oMutex.Leave();
	}
}

void CAsmSystemPlugIn::Clear(uint32 nNode)
{
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oAsmDataStreams.Find(nNode);
	if(pIt != m_oAsmDataStreams.End())
	{
		CAsmStream& oStream = m_oAsmDataStreams.GetItem(pIt);
		oStream.Clear();
	}
	pIt = m_oCommonStreams.Find(nNode);
	if(pIt != m_oCommonStreams.End())
	{
		CAsmStream& oStream = m_oCommonStreams.GetItem(pIt);
		oStream.Clear();
	}
	m_oMutex.Leave();
}

FOCP_END();
