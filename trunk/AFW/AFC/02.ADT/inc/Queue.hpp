
#include "Malloc.hpp"
#include "RbTree.hpp"

#ifndef _ADT_QUEUE_HPP_
#define _ADT_QUEUE_HPP_

FOCP_BEGIN();

template<typename TData> class CQueue
{
private:
	uint32 m_nSize, m_nHead, m_nTail;
	uint32 m_nCapacity;
	TData* m_pItems;

public:
	inline ~CQueue()
	{
		Clear();
		if(m_pItems)
			CMalloc::Free(m_pItems);
		m_nSize = 0;
		m_nCapacity = 0;
		m_pItems = NULL;
	}

	inline CQueue(uint32 nCapacity=0)
	{
		if(nCapacity==0)
			nCapacity = 1024;
		m_nSize = 0;
		m_nCapacity = nCapacity;
		m_pItems = (TData*)CMalloc::Malloc((m_nCapacity+1)*FOCP_SIZE_OF(TData));
		m_nHead = 0;
		m_nTail = 0;
	}

	CQueue(const CQueue<TData>& oSrc)
	{
		uint32 i;
		m_nSize = oSrc.m_nSize;
		m_nCapacity = oSrc.m_nCapacity;
		m_pItems = (TData*)CMalloc::Malloc((m_nCapacity+1)*FOCP_SIZE_OF(TData));
		m_nHead = 0;
		m_nTail = 0;
		for(i=0; i<m_nSize; ++i)
			Push(oSrc[i]);
	}

	CQueue<TData>& operator=(const CQueue<TData>& oSrc)
	{
		if(this != &oSrc)
		{
			CQueue oVec(oSrc);
			Swap(oVec);
		}
		return *this;
	}

	void Clear()
	{
		uint32 i;
		for(i=0; i<m_nSize; ++i)
			At(i)->~TData();
		m_nSize = 0;
		m_nHead = 0;
		m_nTail = 0;
	}

	uint32 GetCapacity() const
	{
		return m_nCapacity;
	}

	uint32 GetSize() const
	{
		return m_nSize;
	}

	TData* At(uint32 nIdx)
	{
		if(nIdx >= m_nSize)
			return NULL;
		nIdx += m_nHead;
		nIdx %= m_nCapacity;
		return m_pItems + nIdx;
	}

	const TData* At(uint32 nIdx) const
	{
		if(nIdx >= m_nSize)
			return NULL;
		nIdx += m_nHead;
		nIdx %= m_nCapacity;
		return m_pItems + nIdx;
	}

	TData& operator[](uint32 nIdx)
	{
		return *At(nIdx);
	}

	const TData& operator[](uint32 nIdx) const
	{
		return *At(nIdx);
	}

	//可指定从尾部加还是头加，注意并不阻塞
	bool Push(const TData& oData, bool bAppend=true)
	{
		if(m_nSize == m_nCapacity)
			return false;
		uint32 nNext;
		if(bAppend)
			nNext = m_nTail;
		else
			nNext = ((m_nHead-1+m_nCapacity) % m_nCapacity);
		TData* pData = m_pItems + nNext;
		new(pData) TData(oData);
		if(bAppend)
			m_nTail = (m_nTail+1)% m_nCapacity;
		else
			m_nHead = nNext;
		++m_nSize;
		return true;
	}

	//可指定从尾部还是头部取对象。注意并不阻塞
	bool Pop(TData& oData, bool bFromHead=true)
	{
		if(m_nSize == 0)
			return false;
		uint32 nIdx;

		if(bFromHead)
			nIdx = m_nHead;
		else
			nIdx = ((m_nTail-1+m_nCapacity) % m_nCapacity);
		TData* pData = m_pItems + nIdx;
		oData = *pData;
		pData->~TData();

		if(bFromHead)
			m_nHead = (m_nHead+1)% m_nCapacity;
		else
			m_nTail = nIdx;
		--m_nSize;

		return true;
	}

	//将对象插入nIdx之前，如果nIdx超出范围，则插到尾部。
	//注意并不阻塞
	bool Insert(const TData& oData, uint32 nIdx=(uint32)(-1))
	{
		uint32 i;
		if(m_nSize >= m_nCapacity)
			return false;

		if(nIdx > m_nSize)
			nIdx = m_nSize;

		uint32 nDistance = (m_nTail - m_nHead + m_nCapacity) % m_nCapacity;
		if(nIdx >= (nDistance>>2))
		{
			new(m_pItems + m_nTail) TData();
			m_nTail = (m_nTail+1)%m_nCapacity;
			++m_nSize;
			for(i=m_nSize-1; i>nIdx; --i)
				operator[](i) = operator[](i-1);
		}
		else
		{
			m_nHead = (m_nHead-1+m_nCapacity)%m_nCapacity;
			new(m_pItems + m_nHead) TData();
			++m_nSize;

			for(i=0; i<nIdx; ++i)
				operator[](i) = operator[](i+1);
		}
		operator[](nIdx) = oData;

		return true;
	}

	//删除指定元素,如果nIdx超出范围，则不起任何效应【返回false】
	//注意并不阻塞
	bool Remove(TData& oData, uint32 nIdx)
	{
		uint32 i;
		if(nIdx >= m_nSize)
			return false;

		uint32 nDistance = (m_nTail - m_nHead + m_nCapacity) % m_nCapacity;
		oData = operator[](nIdx);
		if(nIdx < (nDistance>>2))
		{
			for(i=nIdx; i; --i)
				operator[](i) = operator[](i-1);
			m_pItems[m_nHead].~TData();
			m_nHead = (m_nHead+1)%m_nCapacity;
		}
		else
		{
			for(i=nIdx+1; i<m_nSize; ++i)
				operator[](i-1) = operator[](i);
			m_pItems[m_nTail].~TData();
			m_nTail = (m_nTail-1+m_nCapacity)%m_nCapacity;
		}
		--m_nSize;
		return true;
	}

	//删除指定元素,如果nIdx超出范围，则不起任何效应【返回false】
	//注意并不阻塞
	bool Remove(uint32 nIdx)
	{
		uint32 i;
		if(nIdx >= m_nSize)
			return false;

		uint32 nDistance = (m_nTail - m_nHead + m_nCapacity) % m_nCapacity;
		if(nIdx < (nDistance>>2))
		{
			for(i=nIdx; i; --i)
				operator[](i) = operator[](i-1);
			m_pItems[m_nHead].~TData();
			m_nHead = (m_nHead+1)%m_nCapacity;
		}
		else
		{
			for(i=nIdx+1; i<m_nSize; ++i)
				operator[](i-1) = operator[](i);
			m_pItems[m_nTail].~TData();
			m_nTail = (m_nTail-1+m_nCapacity)%m_nCapacity;
		}
		--m_nSize;
		return true;
	}

	void Swap(CQueue<TData> &oSrc)
	{
		if(this != &oSrc)
		{
			::FOCP_NAME::Swap(m_nSize, oSrc.m_nSize);
			::FOCP_NAME::Swap(m_nHead, oSrc.m_nHead);
			::FOCP_NAME::Swap(m_nTail, oSrc.m_nTail);
			::FOCP_NAME::Swap(m_nCapacity, oSrc.m_nCapacity);
			::FOCP_NAME::Swap(m_pItems, oSrc.m_pItems);
		}
	}
};

/*
struct TElement
{
	uint32 GetNode();//获得消息中的发送发节点号
	uint32 GetSequence();//获得消息序号
	void SetSequence(uint32 nSeq);//for CSendWindow
	void Swap(TElement& oItem);

	TElement();
	TElement(const TElement& oSrc);
	TElement& operator=(const TElement& oSrc);
};

template<typename TElement> struct TContext
{
	void OnRequestMsg(uint64 nWindowId, uint32 nCount, uint32 *pSeq);
	void OnLostMsg(uint64 nWindowId, uint32 nSeq);
	void OnClearRecvWindow(nWindowId);
	void OnMoveRecvWindow(uint64 nWindowId, uint32 nSeq);
	void OnMessage(TElement& oMsg);
};
*/

enum
{
	ADT_MIN_ACK_SIZE = 100,
	ADT_MAX_SEQUENCE = 1000000000
};
template<typename TElement, typename TContext, typename TMutex> class CSequenceRecvQueue
{
	struct CRecvWindow;

	friend struct CSequenceRecvQueue<TElement,TContext,TMutex>::CRecvWindow;

	struct CRecvWindow
	{
		struct CItem
		{
			uint32 nPos;
			uint8 nAckTimes;//最大请求丢包次数，
			uint8 nValid;//0=无效；1=有效
		};
		bool m_bFirst, m_bMove;
		CSequenceRecvQueue<TElement, TContext,TMutex>* m_pSequenceQueue;
		uint32 m_nHead, m_nTail;//sequence
		uint32 m_nInvalid;//丢包个数
		CItem* m_pWindow;
		uint64 m_nWindowId;
		uint32 m_nMove;
		CRecvWindow* m_pNext;

		inline CRecvWindow()
		{
			m_pSequenceQueue = NULL;
			m_bFirst = true;
			m_bMove = false;
			m_nHead = 0;
			m_nTail = 0;
			m_nInvalid = 0;
			m_pWindow = NULL;
			m_nWindowId = 0;
			m_nMove = 0;
		}

		inline CRecvWindow(const CRecvWindow& oSrc)
		{
			m_pSequenceQueue = oSrc.m_pSequenceQueue;
			m_bFirst = oSrc.m_bFirst;
			m_bMove = oSrc.m_bMove;
			m_nHead = oSrc.m_nHead;
			m_nTail = oSrc.m_nTail;
			m_nInvalid = oSrc.m_nInvalid;
			m_pWindow = oSrc.m_pWindow;
			m_nWindowId = oSrc.m_nWindowId;
			m_nMove = oSrc.m_nMove;
		}

		inline CRecvWindow& operator=(const CRecvWindow& oSrc)
		{
			m_pSequenceQueue = oSrc.m_pSequenceQueue;
			m_bFirst = oSrc.m_bFirst;
			m_bMove = oSrc.m_bMove;
			m_nHead = oSrc.m_nHead;
			m_nTail = oSrc.m_nTail;
			m_nInvalid = oSrc.m_nInvalid;
			m_pWindow = oSrc.m_pWindow;
			m_nWindowId = oSrc.m_nWindowId;
			m_nMove = oSrc.m_nMove;
			return *this;
		}

		inline ~CRecvWindow()
		{
			Clear();
			delete[] m_pWindow;
		}

		inline void Init(CSequenceRecvQueue<TElement, TContext,TMutex>* pSequenceQueue, uint64 nWinId)
		{
			m_pSequenceQueue = pSequenceQueue;
			m_pWindow = new CItem[pSequenceQueue->m_nWindowSize];
			m_nWindowId = nWinId;
		}

		//0:success; -1:failure
		inline uint32 PutIn(TElement& oItem)
		{
			uint32 nDis1, nDis2, nCur, nRet;
			uint32 nSeq = oItem.GetSequence();
			uint32 nSize = m_pSequenceQueue->m_nWindowSize;
			uint32 nRange = m_pSequenceQueue->m_nRange;

			nDis1 = (nSeq - m_nHead + nRange)%nRange;
			if(nDis1 >= nSize)
			{
				if(nDis1 <= nRange-nSize)
					Clear();
				else
					return(0);
			}
			if(m_bFirst)
			{
				m_nHead = nSeq;
				m_nTail = nSeq;
				m_bFirst = false;
			}
			nDis1 = (nSeq - m_nHead + nRange)%nRange;
			nDis2 = (m_nTail - m_nHead + nRange)%nRange;
			if(nDis1 >= nDis2)
			{
				uint8 nAckTimes = m_pSequenceQueue->m_nMaxAckTimes;
				for(; m_nTail!=nSeq;)
				{
					nCur = m_nTail%nSize;
					nRet = m_pSequenceQueue->PutInVac();
					if(((int32)nRet) < 0)
					{
						Clear();
						return(-1);
					}
					m_pWindow[nCur].nPos = nRet;
					m_pWindow[nCur].nValid = 0;
					m_pWindow[nCur].nAckTimes = nAckTimes;
					++m_nInvalid;
					m_nTail = (m_nTail+1)%nRange;
				}
				nCur = nSeq%nSize;
				nRet = m_pSequenceQueue->PutInSQ(oItem);
				if(m_bFirst)
					return(0);
				if(((int32)nRet) < 0)
				{
					Clear();
					return(-1);
				}
				m_pWindow[nCur].nPos = nRet;
				m_pWindow[nCur].nValid = 1;
				m_pWindow[nCur].nAckTimes = 0;
				m_nTail = (m_nTail+1)%nRange;
			}
			else
			{
				nCur = nSeq%nSize;
				if(!m_pWindow[nCur].nValid)
				{
					nRet = m_pSequenceQueue->PutInSQ(oItem, m_pWindow[nCur].nPos);
					if(m_bFirst)
						return(0);
					if(((int32)nRet) < 0)
					{
						Clear();
						return(-1);
					}
					m_pWindow[nCur].nValid = 1;
					m_pWindow[nCur].nAckTimes = 0;
					--m_nInvalid;
				}
			}
			MoveRecvWindow();
			return(0);
		}

		inline uint32 PutInNull(uint32 nSeq)
		{
			uint32 nDis1, nDis2, nCur, nRet;
			uint32 nSize = m_pSequenceQueue->m_nWindowSize;
			uint32 nRange = m_pSequenceQueue->m_nRange;

			nDis1 = (nSeq - m_nHead + nRange)%nRange;
			if(nDis1 >= nSize)
			{
				if(nDis1 <= nRange-nSize)
					Clear();
				else
					return (0);
			}
			if(m_bFirst)
			{
				m_nHead = nSeq;
				m_nTail = nSeq;
				m_bFirst = false;
			}
			nDis1 = (nSeq - m_nHead + nRange)%nRange;
			nDis2 = (m_nTail - m_nHead + nRange)%nRange;
			if(nDis1 >= nDis2)
			{
				uint8 nAckTimes = m_pSequenceQueue->m_nMaxAckTimes;
				for(; m_nTail!=nSeq;)
				{
					nCur = m_nTail%nSize;
					nRet = m_pSequenceQueue->PutInVac();
					if(((int32)nRet) < 0)
					{
						Clear();
						return(-1);
					}
					m_pWindow[nCur].nPos = nRet;
					m_pWindow[nCur].nValid = 0;
					m_pWindow[nCur].nAckTimes = nAckTimes;
					++m_nInvalid;
					m_nTail = (m_nTail+1)%nRange;
				}
				nCur = nSeq%nSize;
				nRet = m_pSequenceQueue->PutInNull();
				if(m_bFirst)
					return(0);
				if(((int32)nRet) < 0)
				{
					Clear();
					return(-1);
				}
				m_pWindow[nCur].nPos = nRet;
				m_pWindow[nCur].nValid = 1;
				m_pWindow[nCur].nAckTimes = 0;
				m_nTail = (m_nTail+1)%nRange;
			}
			else
			{
				nCur = nSeq%nSize;
				if(!m_pWindow[nCur].nValid)
				{
					nRet = m_pSequenceQueue->PutInVac(m_pWindow[nCur].nPos);
					if(m_bFirst)
						return(0);
					if(((int32)nRet) < 0)
					{
						Clear();
						return(-1);
					}
					m_pWindow[nCur].nValid = 1;
					m_pWindow[nCur].nAckTimes = 0;
					--m_nInvalid;
				}
			}
			MoveRecvWindow();
			return(0);
		}

		inline uint32 PutInNack(TElement& oItem)
		{
			uint32 nSeq = oItem.GetSequence();
			uint32 nSize = m_pSequenceQueue->m_nWindowSize;
			uint32 nRange = m_pSequenceQueue->m_nRange;
			if(m_bFirst)
				return(-1);
			if((nSeq - m_nHead + nRange)%nRange >= nSize)
				return(0);
			uint32 nCur = nSeq % nSize;
			if(m_pWindow[nCur].nValid)
				return(0);
			uint32 nDis1 = (nSeq - m_nHead + nRange)%nRange;
			uint32 nDis2 = (m_nTail - m_nHead + nRange)%nRange;
			if(nDis1 < nDis2)
			{
				uint32 nRet = m_pSequenceQueue->PutInSQ(oItem, m_pWindow[nCur].nPos);
				if(((int32)nRet) < 0)
					return(-1);
				m_pWindow[nCur].nValid = 1;
				m_pWindow[nCur].nAckTimes = 0;
				--m_nInvalid;
				if(nSeq == m_nHead)
					MoveRecvWindow();
			}
			return(0);
		}

		inline uint32 PutInNack(uint32 nSeq)
		{
			TContext * pContext = m_pSequenceQueue->m_pContext;
			uint32 nSize = m_pSequenceQueue->m_nWindowSize;
			uint32 nRange = m_pSequenceQueue->m_nRange;
			if(m_bFirst)
				return(-1);
			if((nSeq - m_nHead + nRange)%nRange >= nSize)
				return(0);
			uint32 nCur = nSeq % nSize;
			if(m_pWindow[nCur].nValid)
				return(0);
			uint32 nDis1 = (nSeq - m_nHead + nRange)%nRange;
			uint32 nDis2 = (m_nTail - m_nHead + nRange)%nRange;
			if(nDis1 < nDis2)
			{
				uint32 nRet = m_pSequenceQueue->PutInVac(m_pWindow[nCur].nPos);
				if(((int32)nRet) < 0)
					return(-1);
				pContext->OnLostMsg((uint32)m_nWindowId, nSeq);
				m_pWindow[nCur].nValid = 1;
				m_pWindow[nCur].nAckTimes = 0;
				--m_nInvalid;
				if(nSeq == m_nHead)
					MoveRecvWindow();
			}
			return(0);
		}

		inline void Clear()
		{
			if(!m_bFirst)
			{
				TContext * pContext = m_pSequenceQueue->m_pContext;
				uint32 nSize = m_pSequenceQueue->m_nWindowSize;
				uint32 nRange = m_pSequenceQueue->m_nRange;
				for(; m_nHead!=m_nTail; )
				{
					uint32 nCur = m_nHead % nSize;
					if(!m_pWindow[nCur].nValid)
					{
						m_pSequenceQueue->PutInVac(m_pWindow[nCur].nPos);
						pContext->OnLostMsg((uint32)m_nWindowId, m_nHead);
					}
					m_nHead = (m_nHead+1)%nRange;
				}
				m_bFirst = true;
				m_bMove = false;
				pContext->OnClearRecvWindow((uint32)m_nWindowId);
			}
		}

		inline void MoveRecvWindow()
		{
			uint32 nMove = m_nHead;
			uint32 nSize = m_pSequenceQueue->m_nWindowSize;
			uint32 nRange = m_pSequenceQueue->m_nRange;
			for(; nMove!=m_nTail; )
			{
				if(!m_pWindow[nMove%nSize].nValid)
					break;
				nMove = (nMove+1)%nRange;
			}
			if(nMove != m_nHead)
			{
				m_nHead = nMove;
				--nMove;
				if(nMove == (uint32)(-1))
					nMove = nRange - 1;
				if(m_bMove)
				{
					if(((nMove - m_nMove + nRange) % nRange) > ADT_MIN_ACK_SIZE)
						m_pSequenceQueue->m_pContext->OnMoveRecvWindow((uint32)m_nWindowId, nMove);
					m_nMove = nMove;
				}
				else
				{
					m_nMove = nMove;
					m_bMove = true;
				}
			}
		}

		inline void RequireResend(uint32 nMaxCount, uint32 * pSeq)
		{
			uint32 nSeq, nCur, nCount;
			if(m_nInvalid)
			{
				nCount = 0;
				uint32 nSize = m_pSequenceQueue->m_nWindowSize;
				uint32 nRange = m_pSequenceQueue->m_nRange;
				TContext * pContext = m_pSequenceQueue->m_pContext;
				for(nSeq=m_nHead; nSeq!=m_nTail;)
				{
					nCur = nSeq % nSize;
					if(!m_pWindow[nCur].nValid)
					{
						if(m_pWindow[nCur].nAckTimes)
						{
							--m_pWindow[nCur].nAckTimes;
							pSeq[nCount] = nSeq;
							++nCount;
							if(nCount == nMaxCount)
							{
								pContext->OnRequestMsg((uint32)m_nWindowId, nCount, pSeq);
								nCount = 0;
							}
						}
						else
						{
							--m_nInvalid;
							m_pWindow[nCur].nValid = 1;
							m_pSequenceQueue->PutInVac(m_pWindow[nCur].nPos);
							pContext->OnLostMsg((uint32)m_nWindowId, nSeq);
						}
					}
					nSeq = (nSeq+1)%nRange;
				}
				if(nCount)
					pContext->OnRequestMsg((uint32)m_nWindowId, nCount, pSeq);
				MoveRecvWindow();
			}
			if(m_bMove)
				m_pSequenceQueue->m_pContext->OnMoveRecvWindow((uint32)m_nWindowId, m_nMove);
		}
	};

private:
	TMutex m_oMutex;
	uint32 m_nCapacity, m_nWindowSize, m_nRange, m_nMaxAckTimes;
	uint32 m_nHead, m_nTail;
	TElement* m_pQueue;
	uint8* m_pValid; //0=invalid, 1=valid msg, 2=valid null msg
	TContext * m_pContext;
	CRbMap<uint64, CRecvWindow*> m_oRecvWindows;
	CRecvWindow* m_pWindows;

public:
	inline CSequenceRecvQueue(TContext* pContext, uint32 nCapacity=50000, uint8 nMaxAckTimes=10)
	{
		if(!nCapacity)
			nCapacity = 50000;
		else if(nCapacity > 1000000)
			nCapacity = 1000000;
		if(!nMaxAckTimes)
			nMaxAckTimes = 10;
		m_nRange = ADT_MAX_SEQUENCE;
		m_pContext = pContext;
		m_nCapacity = nCapacity;
		m_nWindowSize = nCapacity / 10;
		m_nMaxAckTimes = nMaxAckTimes;
		m_nHead = 0;
		m_nTail = 0;
		m_pQueue = new TElement[m_nCapacity];
		m_pValid = new uint8[m_nCapacity];
		m_pWindows = NULL;
	}

	inline ~CSequenceRecvQueue()
	{
		delete[] m_pQueue;
		delete[] m_pValid;
		m_pQueue = NULL;
		m_pValid = NULL;
		while(m_pWindows)
		{
			CRecvWindow* pNext = m_pWindows->m_pNext;
			delete m_pWindows;
			m_pWindows = pNext;
		}
		CRbTreeNode* pEnd = m_oRecvWindows.End();
		CRbTreeNode* pIt = m_oRecvWindows.First();
		while(pIt != pEnd)
		{
			CRecvWindow *pWin = m_oRecvWindows.GetItem(pIt);
			delete pWin;
			pIt = m_oRecvWindows.GetNext(pIt);
		}
	}

	inline void CreateWindow(uint64 nWinId)
	{
		CRbTreeNode* pEnd = m_oRecvWindows.End();
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oRecvWindows.Find(nWinId);
		if(pIt == pEnd)
		{
			CRecvWindow* pWin = m_pWindows;
			if(m_pWindows)
			{
				m_pWindows = m_pWindows->m_pNext;
				m_pWindows->m_nWindowId = nWinId;
			}
			else
			{
				pWin = new CRecvWindow;
				pWin->Init(this, nWinId);
			}
			m_oRecvWindows[nWinId] = pWin;
		}
		m_oMutex.Leave();
	}

	inline void RemoveWindow(uint64 nWinId)
	{
		CRbTreeNode* pEnd = m_oRecvWindows.End();
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oRecvWindows.Find(nWinId);
		if(pIt != pEnd)
		{
			CRecvWindow *pWin = m_oRecvWindows.GetItem(pIt);
			pWin->m_pNext = m_pWindows;
			m_pWindows = pWin;
			m_oRecvWindows.Remove(pIt);
		}
		m_oMutex.Leave();
	}

	inline uint32 PutIn(TElement& oItem)
	{
		uint32 nRet = -1;
		uint64 nWinId = oItem.GetNode();
		m_oMutex.Enter();
		CRecvWindow* pWin = GetRecvWindow(nWinId);
		if(pWin)
			nRet = pWin->PutIn(oItem);
		m_oMutex.Leave();
		return nRet;
	}

	inline uint32 PutInNull(uint64 nWinId, uint32 nSeq)
	{
		uint32 nRet = -1;
		m_oMutex.Enter();
		CRecvWindow* pWin = GetRecvWindow(nWinId);
		if(pWin)
			nRet = pWin->PutInNull(nSeq);
		m_oMutex.Leave();
		return nRet;
	}

	inline uint32 PutInNack(TElement& oItem)
	{
		uint32 nRet = -1;
		uint64 nWinId = oItem.GetNode();
		m_oMutex.Enter();
		CRecvWindow* pWin = GetRecvWindow(nWinId);
		if(pWin)
			nRet = pWin->PutInNack(oItem);
		m_oMutex.Leave();
		return nRet;
	}

	inline uint32 PutInNack(uint64 nWinId, uint32 nSeq)
	{
		uint32 nRet = -1;
		m_oMutex.Enter();
		CRecvWindow* pWin = GetRecvWindow(nWinId);
		if(pWin)
			nRet = pWin->PutInNack(nSeq);
		m_oMutex.Leave();
		return nRet;
	}

	inline void Clear(uint64 nWinId)//注销时需要调用
	{
		m_oMutex.Enter();
		CRecvWindow* pWin = GetRecvWindow(nWinId);
		if(pWin)
			pWin->Clear();
		m_oMutex.Leave();
	}

	inline void RequireResendAll(uint32 nMaxCount, uint32 * pSeq)//定时检查
	{
		CRbTreeNode* pEnd = m_oRecvWindows.End();
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oRecvWindows.First();
		m_oMutex.Leave();
		while(pIt!=pEnd)
		{
			CRecvWindow* pWin = m_oRecvWindows.GetItem(pIt);
			m_oMutex.Enter();
			pWin->RequireResend(nMaxCount, pSeq);
			pIt = m_oRecvWindows.GetNext(pIt);
			m_oMutex.Leave();
		}
	}

	uint32 GetCapacity(uint32 &nSize)
	{
		m_oMutex.Enter();
		nSize = (m_nTail - m_nHead + m_nCapacity) % m_nCapacity;
		m_oMutex.Leave();
		return m_nCapacity;
	}

private:
	inline CRecvWindow* GetRecvWindow(uint64 nWinId)
	{
		CRecvWindow* pWin = NULL;
		CRbTreeNode* pEnd = m_oRecvWindows.End();
		CRbTreeNode* pIt = m_oRecvWindows.Find(nWinId);
		if(pIt != pEnd)
			pWin = m_oRecvWindows.GetItem(pIt);
		return pWin;
	}

	inline uint32 PutInVac()
	{
		uint32 nPos, nTail;
		nTail = (m_nTail+1)%m_nCapacity;
		if(nTail == m_nHead)
			DispatchMsg(true);
		if(nTail == m_nHead)
			return -1;
		nPos = m_nTail;
		m_pValid[m_nTail] = 0;
		m_nTail = nTail;
		return(nPos);
	}

	inline uint32 PutInNull()
	{
		uint32 nPos, nTail;
		nTail = (m_nTail+1)%m_nCapacity;
		if(nTail == m_nHead)
			DispatchMsg(true);
		if(nTail == m_nHead)
			return -1;
		nPos = m_nTail;
		m_pValid[m_nTail] = 2;
		m_nTail = nTail;
		return(nPos);
	}

	inline uint32 PutInVac(uint32 nPos)
	{
		uint32 nDis1, nDis2;
		nDis1 = (nPos+m_nCapacity-m_nHead)%m_nCapacity;
		nDis2 = (m_nTail+m_nCapacity-m_nHead)%m_nCapacity;
		if(nDis1<nDis2 && !m_pValid[nPos])
		{
			m_pValid[nPos] = 2;//VALID NULL
			DispatchMsg();
		}
		return(nPos);
	}

	inline uint32 PutInSQ(TElement& oItem)
	{
		uint32 nPos, nTail;
		nTail = (m_nTail+1)%m_nCapacity;
		if(nTail == m_nHead)
			DispatchMsg(true);
		if(nTail == m_nHead)
			return -1;
		nPos = m_nTail;
		m_pValid[m_nTail] = 1;
		m_pQueue[m_nTail].Swap(oItem);
		m_nTail = nTail;
		DispatchMsg();
		return(nPos);
	}

	inline uint32 PutInSQ(TElement& oItem, uint32 nPos)
	{
		uint32 nDis1, nDis2;
		nDis1 = (nPos+m_nCapacity-m_nHead)%m_nCapacity;
		nDis2 = (m_nTail+m_nCapacity-m_nHead)%m_nCapacity;
		if(nDis1<nDis2 && !m_pValid[nPos])
		{
			m_pValid[nPos] = 1;
			m_pQueue[nPos].Swap(oItem);
			DispatchMsg();
			return(nPos);
		}
		return(-1);
	}

	inline void DispatchMsg(bool bAll=false)
	{
		while(m_nHead!=m_nTail)
		{
			if(m_pValid[m_nHead])
			{
				if(m_pValid[m_nHead] == 1)
					m_pContext->OnMessage(m_pQueue[m_nHead]);
				m_pValid[m_nHead] = 0;
			}
			else if(!bAll)
				break;
			m_nHead = (m_nHead+1)%m_nCapacity;
		}
	}
};

template<typename TElement, typename TEvent, typename TMutex> class CSequenceSendQueue
{

	struct CSendWindow;

	friend struct CSequenceSendQueue<TElement,TEvent,TMutex>::CSendWindow;

	struct CSendWindow
	{
		TEvent m_oEvent;
		uint32 m_nHead, m_nTail;//sequence
		uint32* m_pWindow;//只需要记录消息在CSequenceSendQueue中的位置即可。
		CSequenceSendQueue<TElement,TEvent,TMutex>* m_pParent;
		CSendWindow* m_pNext;

		inline CSendWindow()
		{
			m_nHead = 0;
			m_nTail = 0;
			m_pWindow = NULL;
			m_pParent = NULL;
		}

		inline CSendWindow(const CSendWindow& oSrc)
		{
			m_nHead = oSrc.m_nHead;
			m_nTail = oSrc.m_nTail;
			m_pWindow = oSrc.m_pWindow;
			m_pParent = oSrc.m_pParent;
		}

		inline CSendWindow& operator=(const CSendWindow& oSrc)
		{
			m_nHead = oSrc.m_nHead;
			m_nTail = oSrc.m_nTail;
			m_pWindow = oSrc.m_pWindow;
			m_pParent = oSrc.m_pParent;
			return *this;
		}

		inline ~CSendWindow()
		{
			if(m_pWindow)
			{
				delete[] m_pWindow;
				m_pWindow = NULL;
			}
		}

		inline void Init(CSequenceSendQueue<TElement,TEvent,TMutex>* pParent)
		{
			m_pParent = pParent;
			m_pWindow = new uint32[m_pParent->m_nWindowSize];
		}

		inline void Wait(bool bForever)
		{
			if(bForever)
				m_oEvent.Wait(0xFFFFFFFF);
			else
				m_oEvent.Wait(1000);
		}

		inline void Reset()
		{
			uint32 nRange = m_pParent->m_nRange;
			if(((m_nTail - m_nHead + nRange) % nRange) >= m_pParent->m_nWindowSize - 1)//full
				return;
			m_oEvent.Reset();
		}

		inline void Set()
		{
			uint32 nRange = m_pParent->m_nRange;
			if(((m_nTail - m_nHead + nRange) % nRange) >= m_pParent->m_nWindowSize - 1)//full
				return;
			m_oEvent.Set();
		}

		inline uint32 Send(uint32 nPos)
		{
			uint32 nSize = m_pParent->m_nWindowSize;
			uint32 nRange = m_pParent->m_nRange;
			uint32 nCount = ((m_nTail - m_nHead + nRange) % nRange) + 1;
			if(nCount == nSize)
				return -1;//full
			uint32 nSeq = m_nTail;
			m_pWindow[nSeq % nSize] = nPos;
			m_nTail = (nSeq+1)%nRange;
			if(nCount == nSize - 1)
				m_oEvent.Reset();
			return nSeq;
		}

		inline uint32 Query(uint32 nSeq)
		{
			uint32 nSize = m_pParent->m_nWindowSize;
			uint32 nRange = m_pParent->m_nRange;
			uint32 nDis1 = (nSeq - m_nHead + nRange) % nRange;
			uint32 nDis2 = (m_nTail - m_nHead + nRange) % nRange;
			if(nDis1 >= nDis2)//Acked
				return -1;
			return m_pWindow[nSeq % nSize];
		}

		inline bool Ack(uint32 nSeq)
		{
			uint32 nSize = m_pParent->m_nWindowSize;
			uint32 nRange = m_pParent->m_nRange;
			uint32 nDis1 = (nSeq - m_nHead + nRange) % nRange;
			uint32 nDis2 = (m_nTail - m_nHead + nRange) % nRange;
			if(nDis1 < nDis2)
			{
				while(m_nHead!=nSeq)
				{
					m_pParent->m_pAcked[m_pWindow[m_nHead % nSize]] = 1;
					m_nHead = (m_nHead + 1) % nRange;
				}
				return true;
			}
			return false;
		}

		inline void Clear()
		{
			uint32 nSize = m_pParent->m_nWindowSize;
			uint32 nRange = m_pParent->m_nRange;
			while(m_nHead!=m_nTail)
			{
				m_pParent->m_pAcked[m_pWindow[m_nHead % nSize]] = 1;
				m_nHead = (m_nHead + 1) % nRange;
			}
		}
	};

private:
	TMutex m_oMutex;
	uint32 m_nCapacity, m_nWindowSize, m_nRange;
	CRbMap<uint64, CSendWindow*>* m_pSendWindows;
	CSendWindow* m_pWindows;
	uint32 m_nHead;//only for SendWindow, it's the subscript of m_pQueue
	uint32 m_nTail;//it's the subscript of m_pQueue for SendWindow, else it's sequence
	TElement* m_pQueue;
	uint8* m_pAcked;

public:
	inline CSequenceSendQueue(uint32 nCapacity=50000, bool bNeedAck=false)
	{
		if(!nCapacity)
			nCapacity = 50000;
		if(nCapacity > 1000000)
			nCapacity = 1000000;
		m_nHead = 0;
		m_nTail = 0;
		m_nCapacity = nCapacity;
		m_nWindowSize = nCapacity/10;
		m_nRange = ADT_MAX_SEQUENCE;//最大消息序号范围，必须和CSequenceRecvQueue.m_nRange相同
		m_pQueue = new TElement[m_nCapacity];
		if(bNeedAck)
		{
			m_pSendWindows = new CRbMap<uint64, CSendWindow*>;
			m_pAcked = new uint8[m_nCapacity];
		}
		else
		{
			m_pSendWindows = NULL;
			m_pAcked = NULL;
		}
		m_pWindows = NULL;
	}

	inline ~CSequenceSendQueue()
	{
		while(m_pWindows)
		{
			CSendWindow* pWin = m_pWindows->m_pNext;
			delete m_pWindows;
			m_pWindows = pWin;
		}
		if(m_pSendWindows)
		{
			CRbTreeNode* pEnd = m_pSendWindows->End();
			CRbTreeNode* pIt = m_pSendWindows->First();
			while(pIt != pEnd)
			{
				CSendWindow* pWin = m_pSendWindows->GetItem(pIt);
				delete pWin;
				pIt = m_pSendWindows->GetNext(pIt);
			}
			delete m_pSendWindows;
			m_pSendWindows = NULL;
		}
		if(m_pAcked)
		{
			delete[] m_pAcked;
			m_pAcked = NULL;
		}
		delete[] m_pQueue;
		m_pQueue = NULL;
	}

	inline bool NeedAck()
	{
		if(m_pSendWindows)
			return true;
		return false;
	}

	uint32 GetCapacity(uint32 &nSize)
	{
		if(m_pSendWindows == NULL)
			nSize = 0;
		else
		{
			m_oMutex.Enter();
			nSize = (m_nTail - m_nHead + m_nCapacity) % m_nCapacity;
			m_oMutex.Leave();
		}
		return m_nCapacity;
	}

	inline void CreateWindow(uint64 nWinId)
	{
		if(m_pSendWindows && nWinId!=(uint64)(-1))
		{
			CRbTreeNode* pEnd = m_pSendWindows->End();
			m_oMutex.Enter();
			CRbTreeNode* pIt = m_pSendWindows->Find(nWinId);
			if(pIt == pEnd)
			{
				CSendWindow* pWin = m_pWindows;
				if(m_pWindows)
					m_pWindows = m_pWindows->m_pNext;
				else
				{
					pWin = new CSendWindow;
					pWin->Init(this);
				}
				(*m_pSendWindows)[nWinId] = pWin;
			}
			m_oMutex.Leave();
		}
	}

	inline void RemoveWindow(uint32 nWinId)
	{
		CRbTreeNode* pEnd = m_pSendWindows->End();
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_pSendWindows->Find(nWinId);
		if(pIt != pEnd)
		{
			CSendWindow *pWin = m_pSendWindows->GetItem(pIt);
			pWin->m_pNext = m_pWindows;
			m_pWindows = pWin;
			m_pSendWindows->Remove(pIt);
		}
		m_oMutex.Leave();
	}

	inline bool Query(TElement &oItem, uint32 nSeq, uint64 nWinId=(uint64)(-1))
	{
		if(nSeq >= m_nRange)
			return false;
		if(m_pSendWindows)
		{
			CSendWindow* pWin = GetSendWindow(nWinId);
			if(pWin == NULL)
				return false;
			m_oMutex.Enter();
			uint32 nPos = pWin->Query(nSeq);
			if(nPos == (uint32)(-1))
			{
				m_oMutex.Leave();
				return false;
			}
			oItem = m_pQueue[nPos];
			m_oMutex.Leave();
			return true;
		}
		else
		{
			uint32 nPos = nSeq % m_nCapacity;
			m_oMutex.Enter();
			TElement& oItem2 = m_pQueue[nPos];
			if(oItem2.GetSequence() != nSeq)
			{
				m_oMutex.Leave();
				return false;
			}
			oItem = oItem2;
			m_oMutex.Leave();
			return true;
		}
	}

	inline bool Send(TElement &oItem, uint64 nWinId=(uint64)(-1), bool bForever=false)
	{
		if(m_pSendWindows)
		{
			CSendWindow* pWin = GetSendWindow(nWinId);
			if(pWin == NULL)
				return false;
			pWin->Wait(bForever);
			m_oMutex.Enter();
			uint32 nTail = (m_nTail + 1) % m_nCapacity;
			if(nTail >= m_nHead)//full
			{
				m_oMutex.Leave();
				return false;
			}
			uint32 nSeq = pWin->Send(m_nTail);
			if(nSeq == (uint32)(-1))//full
			{
				m_oMutex.Leave();
				return false;
			}
			oItem.SetSequence(nSeq);
			m_pQueue[m_nTail] = oItem;
			m_pAcked[m_nTail] = 0;
			m_nTail = nTail;
			nTail = (m_nTail + 1) % m_nCapacity;
			if(nTail >= m_nHead)//full
				ResetAll();
			m_oMutex.Leave();
			return true;
		}
		else
		{
			m_oMutex.Enter();
			uint32 nRet = m_nTail;
			uint32 nIdx = nRet % m_nCapacity;
			oItem.SetSequence(nRet);
			m_pQueue[nIdx] = oItem;
			m_nTail = (nRet+1)%m_nRange;
			m_oMutex.Leave();
			return true;
		}
	}

	inline void Ack(uint32 nSeq, uint64 nWinId)
	{
		if(nSeq >= m_nRange)
			return;
		if(m_pSendWindows == NULL)
			return;
		CSendWindow* pWin = GetSendWindow(nWinId);
		if(pWin == NULL)
			return;
		m_oMutex.Enter();
		if(pWin->Ack(nSeq))
		{
			uint32 nIdx = m_nHead;
			while(nIdx!=m_nTail)
			{
				if(m_pAcked[nIdx] == 0)
					break;
				nIdx = (nIdx + 1)%m_nCapacity;
			}
			if(m_nHead != nIdx)
			{
				SetAll();
				m_nHead = nIdx;
			}
		}
		m_oMutex.Leave();
	}

	inline void Clear(uint64 nWinId)
	{
		if(m_pSendWindows == NULL)
			return;
		CSendWindow* pWin = GetSendWindow(nWinId);
		if(pWin == NULL)
			return;
		m_oMutex.Enter();
		pWin->Clear();
		uint32 nIdx = m_nHead;
		while(nIdx!=m_nTail)
		{
			if(m_pAcked[nIdx] == 0)
				break;
			nIdx = (nIdx + 1)%m_nCapacity;
		}
		if(m_nHead != nIdx)
		{
			SetAll();
			m_nHead = nIdx;
		}
		m_oMutex.Leave();
	}

private:
	inline void ResetAll()
	{
		CRbTreeNode* pEnd = m_pSendWindows->End();
		CRbTreeNode* pIt = m_pSendWindows->First();
		for(; pIt!=pEnd; pIt=m_pSendWindows->GetNext(pIt))
			m_pSendWindows->GetItem(pIt)->Reset();
	}

	inline void SetAll()
	{
		CRbTreeNode* pEnd = m_pSendWindows->End();
		CRbTreeNode* pIt = m_pSendWindows->First();
		for(; pIt!=pEnd; pIt=m_pSendWindows->GetNext(pIt))
			m_pSendWindows->GetItem(pIt)->Set();
	}

	inline CSendWindow* GetSendWindow(uint64 nWinId)
	{
		CSendWindow* pWin = NULL;
		CRbTreeNode* pEnd = m_pSendWindows->End();
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_pSendWindows->Find(nWinId);
		if(pIt != pEnd)
			pWin = m_pSendWindows->GetItem(pIt);
		m_oMutex.Leave();
		return pWin;
	}
};

FOCP_END();

#endif //_ADT_QUEUE_HPP_
