
#include "AsfDef.hpp"

#ifndef _Asf_Argument_Hpp_
#define _Asf_Argument_Hpp_

FOCP_BEGIN();

/*
	AppName {[ѡ���ʶ ѡ���ַ�]����}
	ѡ���ʶ: -
	ѡ���ʶ��ѡ���ַ��Ͳ����ǽ��������ġ�
*/
class ASF_API CArguments
{
	FOCP_FORBID_COPY(CArguments);
	friend struct CSingleInstance<CArguments>;
private:
	int32 m_nArgc;
	char** m_sArgv;
	char m_sCmdLine[2*FOCP_MAX_PATH];
	bool m_bOwnArgv;

public:
	CArguments();
	~CArguments();

	static CArguments* GetInstance();

	void SetArgv(int32 argc, char*argv[], bool bIgnoreArgv0=true);
	void SetCmdLine(const char* sCmdLine);

	int32 GetArgc();
	char* const* GetArgv();
	const char* GetCmdLine();

	int32 GetOption(char c, char**pArg=NULL);//�������ѡ��򷵻ظ�ѡ�����ڵ�λ�á���Я��ѡ��������ء������򷵻�0��
	char WalkOption(int32 &nIdx, char* &pArg);//����ѡ������ز���������ʱ������ѡ��Ϊ'\0'

private:
	void ParseCmdline(const char* sCmdLine, char **argv, char *args, int32 *pArgc, int32 *pCharNum);
};

FOCP_END();

#endif //_Afc_Argument_Hpp_
