
#include "mdb.hpp"
#include "MdbMdb.hpp"

FOCP_BEGIN();

CMdbApplication::CMdbApplication()
{
}

CMdbApplication::~CMdbApplication()
{
}

bool CMdbApplication::StartServiceBefore()//启动服务前
{
	return true;
}

bool CMdbApplication::StartServiceAfter()//启动服务后
{
	MDB_MDB::CMyProfile oProfile;
	oProfile.InsertAttr.ont = 10;
	oProfile.InsertAttr.two = "123";
	oProfile.InsertAttr.three = 3;
	oProfile.InsertAttr.four = "456";
	oProfile.Insert();
	oProfile.WhereAttr.ExtendGroupQuantity(1);
	oProfile.WhereAttr.ExtendGroupSize(0, 1);
	MDB_MDB::CMyProfileCondAttr oCond = oProfile.WhereAttr.GetCond(0, 0);
	oCond.ont == 10;
	uint32 nSize = 0;
	oProfile.Query(1024, 0, nSize);
	MDB_MDB::CMyProfileResultAttr oRec = oProfile.GetResult(0);
	Print("MyProfile.ont=%d\n", (int32)oRec.ont);
	Print("MyProfile.two=%s\n", ((CString)oRec.two).GetStr());
	Print("MyProfile.three=%d\n", (int32)oRec.three);
	Print("MyProfile.four=%s\n", ((CString)oRec.four).GetStr());
	oProfile.UpdateAttr.four = "890";
	oProfile.Update();
	oProfile.Query(1024, 0, nSize);
	oRec = oProfile.GetResult(0);
	Print("MyProfile.ont=%d\n", (int32)oRec.ont);
	Print("MyProfile.two=%s\n", ((CString)oRec.two).GetStr());
	Print("MyProfile.three=%d\n", (int32)oRec.three);
	Print("MyProfile.four=%s\n", ((CString)oRec.four).GetStr());
	oProfile.Delete();
	oProfile.Query(1024, 0, nSize);
	if(nSize)
	{
		Print("MyProfile.ont=%d\n", (int32)oRec.ont);
		Print("MyProfile.two=%s\n", ((CString)oRec.two).GetStr());
		Print("MyProfile.three=%d\n", (int32)oRec.three);
		Print("MyProfile.four=%s\n", ((CString)oRec.four).GetStr());
	}
	return true;
}

void CMdbApplication::StopServiceBefore()//停止服务前
{
}

void CMdbApplication::StopServiceAfter()//停止服务后
{
}

FOCP_END();
