
#if defined(WINDOWS) || defined(CYGWIN_NT)

#include <../../02.ADT/ADT.hpp>

#include <windows.h>
#include <nb30.h>

FOCP_BEGIN();

// 因为是通过NetAPI来获取网卡信息，所以需要包含其题头文件nb30.h #include < nb30.h >
typedef struct _ASTAT_
{
	ADAPTER_STATUS adapt;
	NAME_BUFFER NameBuff [30];
} ASTAT, * PASTAT;

// 定义一个存放返回网卡信息的变量
// 输入参数：lana_num为网卡编号，一般地，从0开始，但在Windows 2000中并不一定是连续分配的
static int getmac_one (int lana_num, char *buf, ASTAT &Adapter)
{
	NCB ncb;
	UCHAR uRetCode;

	CBinary::MemorySet( &ncb, 0, sizeof(ncb) );
	ncb.ncb_command = NCBRESET;
	ncb.ncb_lana_num = lana_num;
	// 指定网卡号

	// 首先对选定的网卡发送一个NCBRESET命令，以便进行初始化
	uRetCode = Netbios( &ncb );

	CBinary::MemorySet( &ncb, 0, sizeof(ncb) );
	ncb.ncb_command = NCBASTAT;
	ncb.ncb_lana_num = lana_num;     // 指定网卡号

	CString::StringCopy( (char *)ncb.ncb_callname, "*               " );
	ncb.ncb_buffer = (uint8 *) &Adapter;

	// 指定返回的信息存放的变量
	ncb.ncb_length = sizeof(Adapter);

	// 接着，可以发送NCBASTAT命令以获取网卡的信息
	uRetCode = Netbios( &ncb );
	if ( uRetCode == 0 )
	{
		CBinary::MemoryCopy(buf, Adapter.adapt.adapter_address, 6);
		return 0;
	}
	return 1;
}

bool GetWinMac(char sMac[7], bool bWalk)
{
	NCB ncb;
	ASTAT Adapter;
	UCHAR uRetCode;
	LANA_ENUM lana_enum;

	CBinary::MemorySet( &ncb, 0, sizeof(ncb) );
	ncb.ncb_command = NCBENUM;

	ncb.ncb_buffer = (uint8*) &lana_enum;
	ncb.ncb_length = sizeof(lana_enum);

	// 向网卡发送NCBENUM命令，以获取当前机器的网卡信息，如有多少个网卡、每张网卡的编号等
	bool nret  = false;
	sMac[6] = '\0';
	uRetCode = Netbios( &ncb );
	if( uRetCode == 0 )
	{
		for( int i=0; i< lana_enum.length; ++i)
		{
			if(!getmac_one( lana_enum.lana[i], sMac, Adapter))
			{
				nret = true;
				break;
			}
			if(!bWalk)
				break;
		}
	}
	return nret;
}

FOCP_END();

#endif
