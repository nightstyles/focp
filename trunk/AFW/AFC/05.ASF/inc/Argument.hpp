
#include "AsfDef.hpp"

#ifndef _Asf_Argument_Hpp_
#define _Asf_Argument_Hpp_

FOCP_BEGIN();

/*
	AppName {[选项标识 选项字符]参数}
	选项标识: -
	选项标识、选项字符和参数是紧紧相连的。
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

	int32 GetOption(char c, char**pArg=NULL);//如果存在选项，则返回该选项所在的位置【并携带选项参数返回】，否则返回0，
	char WalkOption(int32 &nIdx, char* &pArg);//遍历选项，并返回参数。结束时，返回选项为'\0'

private:
	void ParseCmdline(const char* sCmdLine, char **argv, char *args, int32 *pArgc, int32 *pCharNum);
};

FOCP_END();

#endif //_Afc_Argument_Hpp_
