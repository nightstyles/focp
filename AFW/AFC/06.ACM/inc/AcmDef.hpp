
#include "../../05.ASF/ASF.hpp"

#ifndef _ACM_DEFINE_HPP_
#define _ACM_DEFINE_HPP_

#ifdef ACM_EXPORTS
#define ACM_API FOCP_EXPORT
#else
#define ACM_API FOCP_IMPORT
#endif

FOCP_BEGIN();

enum
{
	//UDPģ�鶨��
	ACM_SERIAL_MODULE = 0,
	ACM_TOKEN_MODULE = 1,

	//UDP�������
	ASM_SYSTEM_PLUGIN = 0,
	ASM_MDBREP_PLUGIN = 1,

	//�����嶨��,ֻ���õ�16λ
	ACM_VIP_TOKEN = 1,//��16λ���ڱ�ʾvip����
	ACM_MDB_REPLICATION_TOKEN = 2,
	ACM_MDB_STORAGE_TOKEN = 3,

	//TCPģ�鶨��
	ACM_HELLO_MODULE = 0,
	MDB_SERVER_MODULE = 1,
	MDB_TRANSFER_MODULE = 2,

	//TCPϵͳMAGIC
	ACM_TCP_MAGIC = 0xABCDEFAB,

	//��������
	ACM_TCPSVR_ABILITY = 1,
};

FOCP_END();

#endif
