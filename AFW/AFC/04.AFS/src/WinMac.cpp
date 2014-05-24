
#if defined(WINDOWS) || defined(CYGWIN_NT)

#include <../../02.ADT/ADT.hpp>

#include <windows.h>
#include <nb30.h>

FOCP_BEGIN();

// ��Ϊ��ͨ��NetAPI����ȡ������Ϣ��������Ҫ��������ͷ�ļ�nb30.h #include < nb30.h >
typedef struct _ASTAT_
{
	ADAPTER_STATUS adapt;
	NAME_BUFFER NameBuff [30];
} ASTAT, * PASTAT;

// ����һ����ŷ���������Ϣ�ı���
// ���������lana_numΪ������ţ�һ��أ���0��ʼ������Windows 2000�в���һ�������������
static int getmac_one (int lana_num, char *buf, ASTAT &Adapter)
{
	NCB ncb;
	UCHAR uRetCode;

	CBinary::MemorySet( &ncb, 0, sizeof(ncb) );
	ncb.ncb_command = NCBRESET;
	ncb.ncb_lana_num = lana_num;
	// ָ��������

	// ���ȶ�ѡ������������һ��NCBRESET����Ա���г�ʼ��
	uRetCode = Netbios( &ncb );

	CBinary::MemorySet( &ncb, 0, sizeof(ncb) );
	ncb.ncb_command = NCBASTAT;
	ncb.ncb_lana_num = lana_num;     // ָ��������

	CString::StringCopy( (char *)ncb.ncb_callname, "*               " );
	ncb.ncb_buffer = (uint8 *) &Adapter;

	// ָ�����ص���Ϣ��ŵı���
	ncb.ncb_length = sizeof(Adapter);

	// ���ţ����Է���NCBASTAT�����Ի�ȡ��������Ϣ
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

	// ����������NCBENUM����Ի�ȡ��ǰ������������Ϣ�����ж��ٸ�������ÿ�������ı�ŵ�
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
