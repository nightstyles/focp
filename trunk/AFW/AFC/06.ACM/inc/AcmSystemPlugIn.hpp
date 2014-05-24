
#include "AcmSequence.hpp"

#ifndef _ACM_SYSTEM_PLUGIN_HPP_
#define _ACM_SYSTEM_PLUGIN_HPP_

FOCP_BEGIN();

#ifndef ASM_SYSPLUGIN_OP
#define ASM_SYSPLUGIN_OP
enum//ϵͳ�����������
{
	ASM_SYSPLUGIN_OP_STREAM = 0,	//Common Stream
	ASM_SYSPLUGIN_OP_BIGPACK = 1,	//���
};
#endif

class ACM_API CAsmStream
{
	friend class CAsmSystemPlugIn;
private:
	bool m_bFirst, m_bFail, m_bReleased;
	uint32 m_nSeq, m_nReadPos, m_nTruncatePos;
	CEvent m_oEvent;
	CMutex m_oMutex;
	CMemoryStream m_oStream;
	CAsmSystemPlugIn* m_pAssembler;

public:
	~CAsmStream();
	CAsmStream();
	CAsmStream(const CAsmStream& oSrc);
	CAsmStream& operator=(const CAsmStream& oSrc);

	//�������İ�
	uint32 Read(void* buf, uint32 nbuflen);
	void Release();//ʹ���꣬��ҪRelease�����ܵ���delete����

private:
	void Assemble(CAsmDataMsg& oMsg);
	void Clear();
};

class ACM_API CAsmSystemPlugIn: public CAsmPlugIn
{
	friend class CAsmStream;
private:
	CMutex m_oMutex;
	CAcmSequenceModule* m_pSequenceModule;
	CRbMap<uint32, CAsmStream> m_oAsmDataStreams;
	CRbMap<uint32, CAsmStream> m_oCommonStreams;
	CSingleList<CAsmStream*> m_oAccepts;
	CEvent m_oEvent;

public:
	CAsmSystemPlugIn(CAcmSequenceModule* pModule);
	virtual ~CAsmSystemPlugIn();

	//���CAsmDataMsg::sBody����ACM_MAX_UDP-ASM_UDP_HEADSIZE�����Զ����в��
	//����ֱ�ӵ���CAcmSequenceModule����
	//�յ�CAsmDataMsg�������ݣ�������Զ�����CAcmSequenceModule::ProcessMessage
	bool Send(CAsmDataMsg &oMsg, uint32 nNode=(uint32)(-1));

	//������ͨ���ݣ����һ������ֶ�η��ͣ���ָʾpPackNo,
	//������ذ���Ϊ0����pPackNo[0] == 0��,��ô�´�Ӧ��Ϊ�����İ����͡�
	bool Send(uint8* pStream, uint32 nStreamSize, uint32 nNode=(uint32)(-1), uint16 *pPackNo=NULL);

	//������ͨ����
	CAsmStream* Accept(uint32 &nNodeId);//nNodeId���������

protected:
	virtual void ProcessAsmPlugInMsg(CAcmSequenceModule* pModule, CAsmDataMsg& oMsg);
	virtual void Clear(uint32 nNode);
};

FOCP_END();

#endif
