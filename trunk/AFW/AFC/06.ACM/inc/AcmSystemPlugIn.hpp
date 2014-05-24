
#include "AcmSequence.hpp"

#ifndef _ACM_SYSTEM_PLUGIN_HPP_
#define _ACM_SYSTEM_PLUGIN_HPP_

FOCP_BEGIN();

#ifndef ASM_SYSPLUGIN_OP
#define ASM_SYSPLUGIN_OP
enum//系统插件操作定义
{
	ASM_SYSPLUGIN_OP_STREAM = 0,	//Common Stream
	ASM_SYSPLUGIN_OP_BIGPACK = 1,	//大包
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

	//读出来的包
	uint32 Read(void* buf, uint32 nbuflen);
	void Release();//使用完，需要Release，不能调用delete操作

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

	//如果CAsmDataMsg::sBody超过ACM_MAX_UDP-ASM_UDP_HEADSIZE，将自动进行拆包
	//否则直接调用CAcmSequenceModule发包
	//收到CAsmDataMsg类型数据，组包后将自动调用CAcmSequenceModule::ProcessMessage
	bool Send(CAsmDataMsg &oMsg, uint32 nNode=(uint32)(-1));

	//发送普通数据，如果一个包想分多次发送，请指示pPackNo,
	//如果返回包号为0（即pPackNo[0] == 0）,那么下次应作为独立的包发送。
	bool Send(uint8* pStream, uint32 nStreamSize, uint32 nNode=(uint32)(-1), uint16 *pPackNo=NULL);

	//侦听普通数据
	CAsmStream* Accept(uint32 &nNodeId);//nNodeId是输出参数

protected:
	virtual void ProcessAsmPlugInMsg(CAcmSequenceModule* pModule, CAsmDataMsg& oMsg);
	virtual void Clear(uint32 nNode);
};

FOCP_END();

#endif
