
#include "MdbRep.hpp"

FOCP_BEGIN();

CMdbReplicator::CMdbReplicator():CMdbExtPlugIn(MDB_EXTEND_PLUGIN_REPLICATOR)
{
}

CMdbReplicator::~CMdbReplicator()
{
}

void CMdbReplicator::OnFree(void* pAttr, uint32 nAttrType/*0=字段属性，1表属性，2索引属性*/)
{
	if(nAttrType == 1)
	{
		CMdbReplicateTableAttr* pTabAttr = (CMdbReplicateTableAttr*)pAttr;
		if(pTabAttr->pWhere)
			delete (CMdbSqlParaSet*)pTabAttr->pWhere;
		delete pTabAttr;
	}
}

bool CMdbReplicator::WriteString(CMemoryStream &oStream, const char* sStr)
{
	uint32 nSize = CString::StringLength(sStr);
	bool bRet = oStream.Write(nSize);
	if(bRet && nSize)
		bRet = (nSize == oStream.Write((void*)sStr, nSize));
	return bRet;
}

bool CMdbReplicator::ReadString(CMemoryStream &oStream, CString &oStr)
{
	uint32 nSize;
	oStr.Clear();
	bool bRet = oStream.Read(nSize);
	if(bRet && nSize)
	{
		CString oTmp('A', nSize);
		bRet = (nSize==oStream.Read((void*)oTmp.GetStr(), nSize));
		if(bRet)
			oStr.Swap(oTmp);
	}
	return bRet;
}

void CMdbReplicator::OnInsert(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	CMdbReplicator* pReplicator = (CMdbReplicator*)pContext;
	pReplicator->OnInsert(pAccess, nCaller);
}

void CMdbReplicator::OnDelete(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	CMdbReplicator* pReplicator = (CMdbReplicator*)pContext;
	pReplicator->OnDelete(pAccess, nCaller);
}

void CMdbReplicator::OnUpdate(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	CMdbReplicator* pReplicator = (CMdbReplicator*)pContext;
	pReplicator->OnUpdate(pAccess, nCaller);
}

void CMdbReplicator::OnTruncate(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	CMdbReplicator* pReplicator = (CMdbReplicator*)pContext;
	pReplicator->OnUpdate(pAccess, nCaller);
}

bool CMdbReplicator::OnDeleteBefore(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	CMdbReplicator* pReplicator = (CMdbReplicator*)pContext;
	return pReplicator->OnDeleteBefore(pAccess, nCaller);
}

bool CMdbReplicator::OnUpdateBefore(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	CMdbReplicator* pReplicator = (CMdbReplicator*)pContext;
	return pReplicator->OnUpdateBefore(pAccess, nCaller);
}

void CMdbReplicator::OnInsert(CMdbAccess* pAccess, uint32 nCaller)
{
	if(nCaller & MDB_REPLICATE_CALLER)
		return;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbReplicateTableAttr* pTabRepAttr = (CMdbReplicateTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR];
	if(pTabRepAttr == NULL)
		return;
	if(pTabRepAttr->bReplicativeTable == false)
		return;
	CMdbSqlPara* pInsertAttr = &((CMdbTableAccess*)pAccess)->m_oInsertAttr;
	CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pTabRepAttr->pWhere;
	if(pCond && !pCond->IsCoincident(pInsertAttr->m_oRecord))
		return;

	CAcmSequenceModule* pRepModule = m_oMdbUdpTable[pAccess->GetMdb()];
	CAsmDataMsg oMsg;
	uint32 nParaCount;
	CMemoryStream oStream((char*)oMsg.sBody, sizeof(oMsg.sBody));
	const char* sDbName = pAccess->GetMdb()->GetDbName();
	const char* sTabName = pAccess->GetTableName();
	bool bRet = WriteString(oStream, sDbName);
	if(bRet)bRet = WriteString(oStream, sTabName);
	if(bRet)bRet = pInsertAttr->WriteForRep(oStream, nParaCount);
	if(bRet)
	{
		if(nParaCount)
		{
			oMsg.oHead.nPlugIn = ASM_MDBREP_PLUGIN;
			oMsg.oHead.nOp = MDB_REPLICATE_INSERT;
			oMsg.nSize = oStream.GetPosition() + ASM_UDP_HEADSIZE;
			pRepModule->Send(oMsg);
		}
	}
	else
		FocpWarn(("CMdbReplicator::OnInsert(%s.%s) pack failure", sDbName, sTabName));
}

void CMdbReplicator::OnDelete(CMdbAccess* pAccess, uint32 nCaller)
{
	if(nCaller & MDB_REPLICATE_CALLER)
		return;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbReplicateTableAttr* pTabRepAttr = (CMdbReplicateTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR];
	if(pTabRepAttr == NULL)
		return;
	if(pTabRepAttr->bReplicativeTable == false)
		return;
	CMdbSqlParaSet* pCond = &((CMdbTableAccess*)pAccess)->m_oIdxAttr;
	CAcmSequenceModule* pRepModule = m_oMdbUdpTable[pAccess->GetMdb()];
	CAsmDataMsg oMsg;
	CMemoryStream oStream((char*)oMsg.sBody, sizeof(oMsg.sBody));
	const char* sDbName = pAccess->GetMdb()->GetDbName();
	const char* sTabName = pAccess->GetTableName();
	bool bRet = WriteString(oStream, sDbName);
	if(bRet)bRet = WriteString(oStream, sTabName);
	if(bRet)bRet = pCond->Write(oStream);
	if(bRet)
	{
		oMsg.oHead.nPlugIn = ASM_MDBREP_PLUGIN;
		oMsg.oHead.nOp = MDB_REPLICATE_DELETE;
		oMsg.nSize = oStream.GetPosition() + ASM_UDP_HEADSIZE;
		pRepModule->Send(oMsg);
	}
	else
		FocpWarn(("CMdbReplicator::OnDelete(%s.%s) pack failure", sDbName, sTabName));
}

void CMdbReplicator::OnUpdate(CMdbAccess* pAccess, uint32 nCaller)
{
	if(nCaller & MDB_REPLICATE_CALLER)
		return;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbReplicateTableAttr* pTabRepAttr = (CMdbReplicateTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR];
	if(pTabRepAttr == NULL)
		return;
	if(pTabRepAttr->bReplicativeFields == false)
		return;
	CMdbSqlParaSet* pCond = &((CMdbTableAccess*)pAccess)->m_oIdxAttr;
	CMdbSqlPara* pSet = &((CMdbTableAccess*)pAccess)->m_oSetAttr;
	CAcmSequenceModule* pRepModule = m_oMdbUdpTable[pAccess->GetMdb()];
	CAsmDataMsg oMsg;
	uint32 nParaCount;
	CMemoryStream oStream((char*)oMsg.sBody, sizeof(oMsg.sBody));
	const char* sDbName = pAccess->GetMdb()->GetDbName();
	const char* sTabName = pAccess->GetTableName();
	bool bRet = WriteString(oStream, sDbName);
	if(bRet)bRet = WriteString(oStream, sTabName);
	if(bRet)bRet = pCond->Write(oStream);
	if(bRet)bRet = pSet->WriteForRep(oStream, nParaCount);
	if(bRet)
	{
		if(nParaCount)
		{
			oMsg.oHead.nPlugIn = ASM_MDBREP_PLUGIN;
			oMsg.oHead.nOp = MDB_REPLICATE_UPDATE;
			oMsg.nSize = oStream.GetPosition() + ASM_UDP_HEADSIZE;
			pRepModule->Send(oMsg);
		}
	}
	else
		FocpWarn(("CMdbReplicator::OnDelete(%s.%s) pack failure", sDbName, sTabName));
}

void CMdbReplicator::OnTruncate(CMdbAccess* pAccess, uint32 nCaller)
{
	if(nCaller & MDB_REPLICATE_CALLER)
		return;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbReplicateTableAttr* pTabRepAttr = (CMdbReplicateTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR];
	if(pTabRepAttr == NULL)
		return;
	if(pTabRepAttr->bReplicativeTable == false)
		return;
	CAcmSequenceModule* pRepModule = m_oMdbUdpTable[pAccess->GetMdb()];
	CAsmDataMsg oMsg;
	CMemoryStream oStream((char*)oMsg.sBody, sizeof(oMsg.sBody));
	const char* sDbName = pAccess->GetMdb()->GetDbName();
	const char* sTabName = pAccess->GetTableName();
	bool bRet = WriteString(oStream, sDbName);
	if(bRet)bRet = WriteString(oStream, sTabName);
	if(bRet)
	{
		oMsg.oHead.nPlugIn = ASM_MDBREP_PLUGIN;
		oMsg.oHead.nOp = MDB_REPLICATE_UPDATE;
		oMsg.nSize = oStream.GetPosition() + ASM_UDP_HEADSIZE;
		pRepModule->Send(oMsg);
	}
	else
		FocpWarn(("CMdbReplicator::OnDelete(%s.%s) pack failure", sDbName, sTabName));
}

bool CMdbReplicator::OnDeleteBefore(CMdbAccess* pAccess, uint32 nCaller)
{
	if(!(nCaller&MDB_REPLICATE_CALLER))
		return true;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbReplicateTableAttr* pTabRepAttr = (CMdbReplicateTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR];
	if(pTabRepAttr == NULL)
		return false;
	if(pTabRepAttr->bReplicativeTable == false)
		return false;
	CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pTabRepAttr->pWhere;
	if(pCond)
	{
		CMdbRecord* pRecord = ((CMdbTableAccess*)pAccess)->m_oGetAttr.GetRecord(0);
		if(!pCond->IsCoincident(*pRecord))
			return false;
	}
	return true;
}

bool CMdbReplicator::OnUpdateBefore(CMdbAccess* pAccess, uint32 nCaller)
{
	if(!(nCaller&MDB_REPLICATE_CALLER))
		return true;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbReplicateTableAttr* pTabRepAttr = (CMdbReplicateTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR];
	if(pTabRepAttr == NULL)
		return false;
	if(pTabRepAttr->bReplicativeFields == false)
		return false;
	CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pTabRepAttr->pWhere;
	if(pCond)
	{
		CMdbRecord* pRecord = ((CMdbTableAccess*)pAccess)->m_oGetAttr.GetRecord(1);
		if(!pCond->IsCoincident(*pRecord))
			return false;
	}
	return true;
}

void CMdbReplicator::ProcessAsmPlugInMsg(CAcmSequenceModule* pModule, CAsmDataMsg& oMsg)
{
	switch(oMsg.oHead.nOp)
	{
	case MDB_REPLICATE_INSERT:
		ProcessRepInsert(oMsg);
		break;
	case MDB_REPLICATE_DELETE:
		ProcessRepDelete(oMsg);
		break;
	case MDB_REPLICATE_UPDATE:
		ProcessRepUpdate(oMsg);
		break;
	case MDB_REPLICATE_TRUNCATE:
		ProcessRepTruncate(oMsg);
		break;
	}
}

void CMdbReplicator::ProcessRepInsert(CAsmDataMsg& oMsg)
{
	CString oDbName, oTableName;
	CMemoryStream oStream((char*)oMsg.sBody, oMsg.nSize - ASM_UDP_HEADSIZE);
	bool bRet = ReadString(oStream, oDbName);
	if(bRet)bRet = ReadString(oStream, oTableName);
	if(bRet)
	{
		CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
		if(pDb)
		{
			CMdbAccess* pAccess = pDb->QueryAccess(oTableName.GetStr());
			if(pAccess)
			{
				CMdbSqlPara* pInsert = (CMdbSqlPara*)pAccess->GetInsertPara();
				bRet = pInsert->Read(oStream);
				if(bRet)
					pAccess->Insert(MDB_REPLICATE_CALLER);
				pAccess->Release();
			}
		}
	}
}

void CMdbReplicator::ProcessRepDelete(CAsmDataMsg& oMsg)
{
	CString oDbName, oTableName;
	CMemoryStream oStream((char*)oMsg.sBody, oMsg.nSize - ASM_UDP_HEADSIZE);
	bool bRet = ReadString(oStream, oDbName);
	if(bRet)bRet = ReadString(oStream, oTableName);
	if(bRet)
	{
		CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
		if(pDb)
		{
			CMdbAccess* pAccess = pDb->QueryAccess(oTableName.GetStr());
			if(pAccess)
			{
				CMdbSqlParaSet* pDelete = (CMdbSqlParaSet*)pAccess->GetQueryPara();
				bRet = pDelete->Read(oStream);
				if(bRet)
					pAccess->Delete(NULL, MDB_REPLICATE_CALLER);
				pAccess->Release();
			}
		}
	}
}

void CMdbReplicator::ProcessRepUpdate(CAsmDataMsg& oMsg)
{
	CString oDbName, oTableName;
	CMemoryStream oStream((char*)oMsg.sBody, oMsg.nSize - ASM_UDP_HEADSIZE);
	bool bRet = ReadString(oStream, oDbName);
	if(bRet)bRet = ReadString(oStream, oTableName);
	if(bRet)
	{
		CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
		if(pDb)
		{
			CMdbAccess* pAccess = pDb->QueryAccess(oTableName.GetStr());
			if(pAccess)
			{
				CMdbSqlParaSet* pUpdate = (CMdbSqlParaSet*)pAccess->GetQueryPara();
				CMdbSqlPara* pSet = (CMdbSqlPara*)pAccess->GetUpdatePara();
				bRet = pUpdate->Read(oStream);
				if(bRet)bRet = pSet->Read(oStream);
				if(bRet)
					pAccess->Update(NULL, MDB_REPLICATE_CALLER);
				pAccess->Release();
			}
		}
	}
}

void CMdbReplicator::ProcessRepTruncate(CAsmDataMsg& oMsg)
{
	CString oDbName, oTableName;
	CMemoryStream oStream((char*)oMsg.sBody, oMsg.nSize - ASM_UDP_HEADSIZE);
	bool bRet = ReadString(oStream, oDbName);
	if(bRet)bRet = ReadString(oStream, oTableName);
	if(bRet)
	{
		CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
		if(pDb)
		{
			CMdbAccess* pAccess = pDb->QueryAccess(oTableName.GetStr());
			if(pAccess)
			{
				pAccess->Truncate(MDB_REPLICATE_CALLER);
				pAccess->Release();
			}
		}
	}
}

void CMdbReplicator::ProcessAcmModuleMsg(CAcmTcpLink* pLink, uint32 nCmd, CMemoryStream& oStream)
{
	if(nCmd != MDB_TRANSFER_INSERT_REQUEST)
		return;
	CString oDbName;
	bool bRet = ReadString(oStream, oDbName);
	if(bRet)
	{
		CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
		if(pDb)
		{
			CString oTabList(pDb->GetTableList());
			char*pShift, *sTableName = (char*)oTabList.GetStr();
			while(sTableName)
			{
				pShift = (char*)CString::CharOfString(sTableName, ',');
				if(pShift)
					pShift[0] = 0;
				TransferTable(pLink, pDb, sTableName);
				sTableName = pShift;
				if(sTableName)
				{
					sTableName[0] = ',';
					++sTableName;
				}
			}
			TransferRecord(pLink, pDb, NULL, NULL, NULL);
		}
	}
}

void CMdbReplicator::TransferTable(CAcmTcpLink* pLink, CMdb* pDb, char* sTableName)
{
	CMdbAccess* pAccess = pDb->QueryAccess(sTableName);
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbReplicateTableAttr* pTabRepAttr = (CMdbReplicateTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR];
	if(pTabRepAttr == NULL)
	{
		pAccess->Release();
		return;
	}
	if(pTabRepAttr->bReplicativeTable == false && pTabRepAttr->bReplicativeFields == false)
	{
		pAccess->Release();
		return;
	}

	CMdbSqlParaSet* pParaSet = (CMdbSqlParaSet*)pAccess->GetQueryPara();
	CMdbSqlFilter* pFilter = (CMdbSqlFilter*)pAccess->GetResultFilter();
	uint32 nFieldCount = pTabDef->nFieldCount;
	for(uint32 i=0; i<nFieldCount; ++i)
	{
		CMdbFieldAttr* pAttr = pTabDef->pFieldDefines[i].pExtendAttr;
		if(pAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR])
			pFilter->SetField(i);
	}
	if(!pFilter->GetParaCount())
	{
		pAccess->Release();
		return;
	}
	if(pTabRepAttr->pWhere)
		pParaSet->SetFrom((CMdbSqlParaSet*)pTabRepAttr->pWhere);

	register uint32 nRecordCount;
	register CMdbSqlPara** pAddPara = NULL;
	register CMdbResultSet* pResultSet = pAccess->GetResultSet();
	uint32 nCondCount = pParaSet->GetParaCount();
	uint32 nIdxFieldCount = pTabRepAttr->pTransferIdx->pExtendAttr->nFieldCount;
	uint32* pIdxFields = pTabRepAttr->pTransferIdx->pExtendAttr->pFields;

	while(!pAccess->Query(1024, 0) && (nRecordCount = pResultSet->GetResultCount()))
	{
		register uint32 i, j;
		for(i=0; i<nRecordCount; ++i)
		{
			CMdbResult* pRecord = pResultSet->GetResult(i);
			TransferRecord(pLink, pDb, (CMdbTableAccess*)pAccess, (CMdbRecord*)pRecord, pFilter);
		}
		if(nRecordCount < 1024)
			break;
		if(!pAddPara)
		{
			bool bParaSet;
			if(!nCondCount)
			{
				pParaSet->AddParaSet();
				nCondCount++;
			}
			pAddPara = new CMdbSqlPara*[nCondCount];
			for(i=0; i<nCondCount; ++i)
				pAddPara[i] = (CMdbSqlPara*)((CMdbSqlParaSet*)pParaSet->GetPara(i, bParaSet))->AddPara();
		}
		register CMdbRecord* pRecord = (CMdbRecord*)pResultSet->GetResult(i);
		for(i=0; i<nCondCount; ++i)
		{
			register CMdbSqlPara* pIdxPara = pAddPara[i];
			for(j=0; j<nIdxFieldCount; ++j)
			{
				register uint32 nCol = pIdxFields[j];
				pIdxPara->SetFrom(nCol, pRecord->GetField(nCol), MDB_SQLPARA_OPERATOR_MORE);
			}
		}
	}
	pAccess->Release();
	if(pAddPara)
		delete[] pAddPara;
}

void CMdbReplicator::TransferRecord(CAcmTcpLink* pLink, CMdb* pDb, CMdbTableAccess* pAccess, CMdbRecord* pRecord, CMdbSqlFilter* pFilter)
{
	CTcpHead oHead;
	CMemoryStream oStream;
	oHead.nSize = 0;
	oHead.nMagic = ACM_TCP_MAGIC;
	oHead.nModule = MDB_TRANSFER_MODULE;
	oHead.nCmd = MDB_TRANSFER_INSERT_RESPONSE;
	oStream.Write(oHead.nSize);
	oStream.Write(oHead.nMagic);
	oStream.Write(oHead.nModule);
	oStream.Write(oHead.nCmd);
	if(pRecord)
	{
		oStream.Write((uint32)(0));
		const char* sDbName = pDb->GetDbName();
		const char* sTabName = pAccess->GetTableName();
		WriteString(oStream, sDbName);
		WriteString(oStream, sTabName);
		pFilter->Write(oStream);
		pRecord->Write(oStream, *pFilter);
	}
	else
		oStream.Write((uint32)(1));
	oStream.SetPosition(0);
	oStream.Write(oStream.GetSize());
	pLink->Send(oStream);
}

static CEvent g_oTransferEvent;
static bool g_bTransferEnd = false;
static uint32 g_nTransferCount = 0;

bool CMdbReplicator::Transfer()
{
	CRbTreeNode* pEnd = m_oUdpMdbTable.End();
	CRbTreeNode* pIt = m_oUdpMdbTable.First();
	FocpInfo(("Transfer Begin"));
	for(; pIt!=pEnd; pIt=m_oUdpMdbTable.GetNext(pIt))
	{
		CAsmNode oNode;
		uint32 nRemoteNode;
		CSingleList<CMdb*> &oMdbs = m_oUdpMdbTable.GetItem(pIt);
		CAcmSequenceModule* pModule = m_oUdpMdbTable.GetKey(pIt);
		CAcmUdp* pUdp = pModule->GetUdp();

		uint32 nDomain = pUdp->GetDomain();
		CAcmMutex oMutex(nDomain, ACM_MDB_REPLICATION_TOKEN);
		uint32 nReLoop = 0;

		FocpInfo(("Transfer Domain(%u) Begin", nDomain));
loop:
		while(true)
		{
			if(oMutex.Enter())
				break;
		}

		uint32 nTry = 0;
		while(true)
		{
			nRemoteNode = pModule->QueryServiceNode(oNode);
			if(nRemoteNode == pUdp->GetNode())
			{
				if(nTry < 5)
				{
					++nTry;
					CCooperator::Sleep(1000);
					continue;
				}
			}
			break;
		}

		if(nRemoteNode == pUdp->GetNode())
		{
			FocpInfo(("Transfer This is the first node(%u)", nRemoteNode));
			if(!CMdbFirstNode::GetInstance()->OnFirstNode(nDomain))
			{
				oMutex.Leave();
				return false;
			}
			pModule->ActiveService();
			oMutex.Leave();
			FocpInfo(("Transfer Domain(%u) End", nDomain));
			continue;
		}

		oMutex.Leave();

		CMdbFirstNode::GetInstance()->OnOtherNode(nDomain);

		CIpAddr oAddr;
		pUdp->GetNodeAddr(nRemoteNode, oAddr);
		CAcmTcpClient oClient(this);
		oClient.Initialize(oAddr.nAddr, oNode.nTcpPort);

		FocpInfo(("Transfer From Node %u", nRemoteNode));
		if(!oClient.Start(false))
		{
			oClient.Stop();
			oClient.Cleanup();
			if(nReLoop >= 3)
				return false;
			++nReLoop;
			goto loop;
		}
		void* pIt2 = oMdbs.First();
		for(; pIt2; pIt2=oMdbs.GetNext(pIt2))
		{
			g_oTransferEvent.Reset();
			CMdb* pDb = oMdbs.GetItem(pIt2);
			g_bTransferEnd = false;
			g_nTransferCount = 0;
			FocpInfo(("Transfer(%s) Begin", pDb->GetDbName()));
			SendTransferRequest(&oClient, pDb);
			g_oTransferEvent.Wait();
			FocpInfo(("Transfer(%s) End, Total %u Records", pDb->GetDbName(), g_nTransferCount));
			if(!g_bTransferEnd)
			{
				oClient.Stop();
				oClient.Cleanup();
				FocpError(("Transfer(%s) Break", pDb->GetDbName()));
				return false;
			}
		}
		pModule->ActiveService();
		oClient.Stop();

		FocpInfo(("Transfer Domain(%u) End", nDomain));
	}
	FocpInfo(("Transfer End"));

	return true;
}

void CMdbReplicator::SendTransferRequest(CAcmTcpClient* pClient, CMdb* pDb)
{
	CTcpHead oHead;
	CMemoryStream oStream;
	oHead.nSize = 0;
	oHead.nMagic = ACM_TCP_MAGIC;
	oHead.nModule = MDB_TRANSFER_MODULE;
	oHead.nCmd = MDB_TRANSFER_INSERT_REQUEST;
	oStream.Write(oHead.nSize);
	oStream.Write(oHead.nMagic);
	oStream.Write(oHead.nModule);
	oStream.Write(oHead.nCmd);
	const char* sDbName = pDb->GetDbName();
	WriteString(oStream, sDbName);
	oStream.SetPosition(0);
	oStream.Write(oStream.GetSize());
	pClient->Send(oStream);
}

bool CMdbReplicator::OnReConnect(CAcmTcpClient* pClient)
{
	g_oTransferEvent.Set();
	return false;
}

uint32 CMdbReplicator::OnLogin(CAcmTcpClient* pClient, bool bReLogin)
{
	if(bReLogin)
		return 1;//Failure
	return 0;
}

void CMdbReplicator::ProcessMsg(CAcmTcpClient* pClient, CTcpHead& oHead, CMemoryStream &oStream)
{
	uint32 nEnd;
	if(oHead.nMagic != ACM_TCP_MAGIC)
		return;
	if(oHead.nModule != MDB_TRANSFER_MODULE)
		return;
	if(oHead.nCmd != MDB_TRANSFER_INSERT_RESPONSE)
		return;
	if(!oStream.Read(nEnd))
		return;
	if(nEnd)
	{
		g_bTransferEnd = true;
		g_oTransferEvent.Set();
		return;
	}
	CString oDbName, oTabName;
	bool bRet = ReadString(oStream, oDbName);
	if(bRet)bRet = ReadString(oStream, oTabName);
	if(!bRet)
		return;
	CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
	if(!pDb)
		return;
	CMdbAccess* pAccess = pDb->QueryAccess(oTabName.GetStr());
	if(!pAccess)
		return;
	CMdbSqlPara* pInsert = (CMdbSqlPara*)pAccess->GetInsertPara();
	if(!pInsert->ReadFilterAndRecord(oStream) || !pInsert->GetParaCount())
	{
		pAccess->Release();
		return;
	}
	uint32 nRet = pAccess->Insert(MDB_TRANSFER_CALLER|MDB_REPLICATE_CALLER);
	if(nRet)
	{
		CMdbSqlPara* pSet = (CMdbSqlPara*)pAccess->GetUpdatePara();
		CMdbSqlParaSet* pIdx = (CMdbSqlParaSet*)pAccess->GetQueryPara();
		if(pIdx->Insert2Update(pInsert, pSet))
			pAccess->Update(NULL, MDB_TRANSFER_CALLER|MDB_REPLICATE_CALLER);
	}
	g_nTransferCount++;
	if(!(g_nTransferCount%5000))
		FocpInfo(("CMdbReplicator::MdbTranfer(%s): %u", oDbName.GetStr(), g_nTransferCount));
	pAccess->Release();
}

bool CMdbReplicator::ConfigDomain(uint32 nDomain, const char* sDbName)
{
	CAcmUdp * pUdp = CAcmUdp::QueryUdp(nDomain);
	if(!pUdp)
	{
		FocpError(("CMdbReplicator::ConfigDomain(%u, %s): Domain isnot exist", nDomain, sDbName));
		return false;
	}
	CMdb * pDb = CMdb::GetMdb(sDbName);
	if(!pDb)
	{
		FocpError(("CMdbReplicator::ConfigDomain(%u, %s): Database isnot exist", nDomain, sDbName));
		return false;
	}
	CMdbLocalInterface* pItf = pDb->GetLocalInterface();
	if(!pItf)
	{
		FocpError(("CMdbReplicator::ConfigDomain(%u, %s): Database isnot local", nDomain, sDbName));
		return false;
	}
	CAcmSequenceModule* pModule = (CAcmSequenceModule*)pUdp->QueryModule(ACM_SERIAL_MODULE);
	if(!pModule)
	{
		FocpError(("CMdbReplicator::ConfigDomain(%u, %s): Domain is invalid", nDomain, sDbName));
		return false;
	}
	if(!pModule->SupportRecv() || !pModule->SupportMultiCastSend())
	{
		FocpError(("CMdbReplicator::ConfigDomain(%u, %s): Domain is invalid", nDomain, sDbName));
		pUdp->ReleaseModule(ACM_SERIAL_MODULE);
		return false;
	}
	CRbTreeNode* pNode1 = m_oMdbUdpTable.Find(pDb);
	if(pNode1 != m_oMdbUdpTable.End())
	{
		if(pModule != m_oMdbUdpTable.GetItem(pNode1))
		{
			FocpError(("CMdbReplicator::ConfigDomain(%u, %s): redefine database's replication domain", nDomain, sDbName));
			pUdp->ReleaseModule(ACM_SERIAL_MODULE);
			return false;
		}
		pUdp->ReleaseModule(ACM_SERIAL_MODULE);
		return true;
	}
	m_oMdbUdpTable[pDb] = pModule;
	CSingleList<CMdb*>& oMdbs = m_oUdpMdbTable[pModule];
	oMdbs.Push(pDb);
	if(oMdbs.GetSize() == 1)
		pModule->RegisterPlugIn(ASM_MDBREP_PLUGIN, this);
	else
		pUdp->ReleaseModule(ACM_SERIAL_MODULE);
	pDb->GetLocalInterface()->RegInsertDbTrigger(CMdbReplicator::OnInsert, this, MDB_APP_CALLER);
	pDb->GetLocalInterface()->RegDeleteDbTrigger(CMdbReplicator::OnDelete, this, MDB_APP_CALLER);
	pDb->GetLocalInterface()->RegUpdateDbTrigger(CMdbReplicator::OnUpdate, this, MDB_APP_CALLER);
	pDb->GetLocalInterface()->RegTruncateDbTrigger(CMdbReplicator::OnTruncate, this, MDB_APP_CALLER);

	CString oTableList(pDb->GetTableList());
	char *pShift, *sTableName = (char*)oTableList.GetStr();
	while(sTableName)
	{
		pShift = (char*)CString::CharOfString(sTableName, ',');
		if(pShift)
			pShift[0] = 0;
		CMdbTableDef* pTabDef = pDb->GetTableDefine(sTableName);
		CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
		CMdbReplicateTableAttr* pTabRepAttr = (CMdbReplicateTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR];
		if(pTabRepAttr)
		{
			pItf->RegDeleteBeforeTrigger(sTableName, CMdbReplicator::OnDeleteBefore, this, MDB_REPLICATE_CALLER);
			pItf->RegUpdateBeforeTrigger(sTableName, CMdbReplicator::OnUpdateBefore, this, MDB_REPLICATE_CALLER);
		}
		sTableName = pShift;
		if(sTableName)
		{
			sTableName[0] = ',';
			++sTableName;
		}
	}
	return true;
}

bool CMdbReplicator::CreateReplicator(const char* sDbName, const char* sTableName, const char* sTransferIdx,
									  bool bTableReplicative, const char* sFields, bool bWithout, const char* sCond)
{
	CMdb * pDb = CMdb::GetMdb(sDbName);
	if(!pDb)
	{
		FocpError(("CMdbReplicator::CreateReplicator(%s.%s): Database isnot exist", sDbName, sTableName));
		return false;
	}
	if(!pDb->GetLocalInterface())
	{
		FocpError(("CMdbReplicator::CreateReplicator(%s.%s): Database isnot local", sDbName, sTableName));
		return false;
	}
	CMdbAccess* pAccess = pDb->QueryAccess(sTableName);
	if(!pAccess)
	{
		FocpError(("CMdbReplicator::CreateReplicator(%s.%s): Table isnot exist", sDbName, sTableName));
		return false;
	}
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	bool bFieldReplicative = false;
	CString oFields(sFields);
	CRbTree<CString, CString, CGetKey<CString, CString>, CNameCompare> oList;
	char*pShift, *sField = (char*)oFields.GetStr();
	while(sField && sField[0])
	{
		pShift = (char*)CString::CharOfString(sField, ',');
		if(pShift)
			pShift[0] = 0;
		CString oField(sField);
		oField.Trim();
		if(oField.Empty())
		{
			FocpError(("CMdbReplicator::CreateReplicator(%s.%s.%s): Field is whitespace", sDbName, sTableName, sField));
			if(pShift)
				pShift[0] = ',';
			pAccess->Release();
			return false;
		}
		uint32 nCol = pAccess->GetFieldNo(oField.GetStr());
		if(nCol == (uint32)(-1))
		{
			FocpError(("CMdbReplicator::CreateReplicator(%s.%s.%s): Field isnot exist", sDbName, sTableName, oField.GetStr()));
			if(pShift)
				pShift[0] = ',';
			pAccess->Release();
			return false;
		}
		oList.Insert(oField);
		sField = pShift;
		if(sField)
		{
			sField[0] = ',';
			++sField;
		}
	}
	uint32 nSize = oList.GetSize();
	if(bWithout)
	{
		if(nSize < pTabDef->nFieldCount)
			bFieldReplicative = true;
	}
	else
	{
		if(nSize)
			bFieldReplicative = true;
	}
	if(!bTableReplicative && !bFieldReplicative)
	{
		pAccess->Release();
		FocpError(("CMdbReplicator::CreateReplicator(%s.%s): There aren't any replication option", sDbName, sTableName));
		return false;
	}

	uint32 i;
	CMdbIndexDef* pTransferIdx = NULL;
	CMdbTableAttr* pExtendAttr = pTabDef->pExtendAttr;
	for(i=0; i<pExtendAttr->nIndexCount; ++i)
	{
		CMdbIndexDef* pIdxDef = pExtendAttr->pIndexDefineSet[i];
		if( (pIdxDef->nQualifier & MDB_UNIQUE_INDEX) && (pIdxDef->nArithmetic & MDB_RANGE_INDEX) )
		{
			if(!CString::StringCompare(pIdxDef->sIndexName, sTransferIdx, false))
			{
				pTransferIdx = pIdxDef;
				break;
			}
		}
	}
	if(!pTransferIdx)
	{
		pAccess->Release();
		FocpError(("CMdbReplicator::CreateReplicator(%s.%s): There aren't unique range index", sDbName, sTableName));
		return false;
	}

	CMdbSqlParaSet* pWhere = new CMdbSqlParaSet(pTabDef);
	if(!pWhere->SetFromWhere(sCond))
	{
		delete pWhere;
		pAccess->Release();
		return false;
	}
	if(pWhere->IsEmpty())
	{
		delete pWhere;
		pWhere = NULL;
	}
	CMdbReplicateTableAttr* pTabAttr = new CMdbReplicateTableAttr;
	pTabAttr->bReplicativeTable = bTableReplicative;
	pTabAttr->bReplicativeFields = bFieldReplicative;
	pTabAttr->pWhere = pWhere;
	pTabAttr->pTransferIdx = pTransferIdx;
	pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR] = pTabAttr;
	if(bFieldReplicative)
	{
		if(bWithout)
		{
			for(uint32 i=0; i<pTabDef->nFieldCount; ++i)
			{
				if(oList.Find(pTabDef->pFieldDefines[i].sFieldName) == oList.End())
					pTabDef->pFieldDefines[i].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR] = (void*)1;
			}
		}
		else
		{
			for(uint32 i=0; i<pTabDef->nFieldCount; ++i)
			{
				if(oList.Find(pTabDef->pFieldDefines[i].sFieldName) != oList.End())
					pTabDef->pFieldDefines[i].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR] = (void*)1;
			}
		}
	}

	uint32 nIdxFieldCount = pTransferIdx->pExtendAttr->nFieldCount;
	uint32* pIdxFields = pTransferIdx->pExtendAttr->pFields;
	for(i=0; i<nIdxFieldCount; ++i)
	{
		if(!pTabDef->pFieldDefines[pIdxFields[i]].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR])
		{
			pAccess->Release();
			FocpError(("CMdbReplicator::CreateReplicator(%s.%s): transfer index field doesn't support replication", sDbName, sTableName));
			return false;
		}
	}

	pAccess->Release();
	return true;
}

bool CMdbReplicator::RegTransferServer()
{
	CService* pService = CServiceManager::GetInstance()->QueryService("AcmService");
	if(!pService)
	{
		FocpLog(FOCP_LOG_ERROR, ("MdbService need AcmService for CMdbReplicator, but there isn't it"));
		return false;
	}
	if(!pService->HaveAbility(ACM_TCPSVR_ABILITY))
	{
		FocpLog(FOCP_LOG_ERROR, ("MdbService need AcmService's tcp server ability for MdbSvrModule, but there isn't it"));
		return false;
	}
	CAcmTcpServer::GetInstance()->RegisterModule(MDB_TRANSFER_MODULE, this);
	return true;
}

void CMdbReplicator::Cleanup()
{
	CAcmTcpServer::GetInstance()->DeRegisterModule(MDB_TRANSFER_MODULE);
	CRbTreeNode* pIt = m_oUdpMdbTable.First();
	for(; pIt != m_oUdpMdbTable.End(); pIt = m_oUdpMdbTable.GetNext(pIt))
	{
		CAcmSequenceModule* pModule = m_oUdpMdbTable.GetKey(pIt);
		pModule->DeRegisterPlugIn(ASM_MDBREP_PLUGIN);
		pModule->GetUdp()->ReleaseModule(ACM_SERIAL_MODULE);
	}
	m_oMdbUdpTable.Clear();
	m_oUdpMdbTable.Clear();
}

CMdbReplicator* CMdbReplicator::GetInstance()
{
	return CSingleInstance<CMdbReplicator>::GetInstance();
}

FOCP_END();

FOCP_C_BEGIN();

MDBREP_API bool MdbTransfer()
{
	return FOCP_NAME::CMdbReplicator::GetInstance()->Transfer();
}

MDBREP_API bool ConfigMdbDomain(const char* sDbName, FOCP_NAME::uint32 nDomain)
{
	return FOCP_NAME::CMdbReplicator::GetInstance()->ConfigDomain(nDomain, sDbName);
}

MDBREP_API bool CreateMdbReplicator(const char* sDbName, const char* sTableName, const char* sTransferIdx, bool bTableReplicative, bool bWithout, const char* sFields, const char* sCond)
{
	return FOCP_NAME::CMdbReplicator::GetInstance()->CreateReplicator(sDbName, sTableName, sTransferIdx, bTableReplicative, sFields, bWithout, sCond);
}

MDBREP_API bool RegisterMdbTransfer()
{
	return FOCP_NAME::CMdbReplicator::GetInstance()->RegTransferServer();
}

MDBREP_API void CleanupMdbReplication()
{
	FOCP_NAME::CMdbReplicator::GetInstance()->Cleanup();
}

FOCP_C_END();
