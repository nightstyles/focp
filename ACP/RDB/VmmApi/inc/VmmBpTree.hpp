/************************************************************************
*input restrictions:   MIN_LEAF_SLOTS must be less or equal  MAX_LEAF_SLOTS*1.0/2 
*				  MIN_INNER_SLOTS must be less or equal  MAX_INNER_SLOTS*1.0/2 
*				  MAX_INNER_SLOTS must be greater or equal 4
*				  MAX_LEAF_SLOTS must be greater or equal 4
*************************************************************************/

#include "VmmVao.hpp"

#ifndef _VMM_BTREE_HPP_
#define _VMM_BTREE_HPP_

FOCP_BEGIN();

struct CBaseBTreeInfo
{
	uint32 m_bUnique;
	uint32 m_nItemCount; 		    /*Number of items in the B+ tree*/
	uint32 m_nLeaveCount;           /*Number of leaves in the B+ tree*/
	uint32 m_nInnerCount;        	/*Number of inner nodes in the B+ tree*/    	
	uint64 m_nEnd;			        /*point to end node. addr should be valid for ever*/
};

template<class TKey, uint32 MAX_INNER_SLOTS> struct CBTreeBasicInnerNode
{
	int32 nLevel; 			/*must not be 0*/
	int32 nSlotUse;
	uint64 nFather;

	uint32 m_nDirty;
	uint32 m_nCounter;
	
	uint64 pSlotChild[MAX_INNER_SLOTS];
	TKey pSlotKey[MAX_INNER_SLOTS];
};

template<class TKey,class TData, uint32 MAX_LEAF_SLOTS> struct CBTreeBasicLeafNode
{	
	int32 nLevel; 			/*must be 0*/
	int32 nSlotUse;	       /*if end node, should be 0*/
	uint64 nFather;	   

	uint32 m_nDirty;
	uint32 m_nCounter;

	uint64 nPrevLeaf;
	uint64 nNextLeaf;

	TKey pSlotKey[MAX_LEAF_SLOTS];
	TData pSlotData[MAX_LEAF_SLOTS];
};

struct CNodeBasicInfo
{
	int32 nLevel;
	int32 nSlotUse;
	uint64 nFather;

	uint32 m_nDirty;
	uint32 m_nCounter;
};

template
<
	class TKey,
	class TData,
	int32 nMaxLeafSlots,
	int32 nMinLeafSlots,
	int32 nMaxInnerSlots,
	int32 nMinInnerSlots
> 
class CBTree
{ 
public:
	enum
	{
		MAX_LEAF_SLOTS = (nMaxLeafSlots&0xfffffffe),
		MIN_LEAF_SLOTS = (nMinLeafSlots),
		MAX_INNER_SLOTS = (nMaxInnerSlots&0xfffffffe),
		MIN_INNER_SLOTS = (nMinInnerSlots)
	};

	typedef CBTreeBasicInnerNode<TKey,MAX_INNER_SLOTS> TBaseInnerNode;
	typedef CBTreeBasicLeafNode<TKey,TData,MAX_LEAF_SLOTS> TBaseLeafNode;
	
	typedef CBTree<TKey,TData,MAX_LEAF_SLOTS,MIN_LEAF_SLOTS, MAX_INNER_SLOTS,MIN_INNER_SLOTS> TBTree;
	
	typedef CPersistObjectPool<TBaseInnerNode> TInnerNodePool;
	
	typedef CRbTree<uint64,uint64> INNER_CONTARINER_T;
	typedef CRbTreeNode* INNER_CONTARINER_IT_T;
	
	enum
	{
		VMM_OBJECT_SIZE = sizeof(CBaseBTreeInfo), 
		VMM_INNER_NODE_SIZE=sizeof(TBaseInnerNode),
		VMM_LEAF_NODE_SIZE=sizeof(TBaseLeafNode)
	};
	
private:
	class CBTreeInnerNode
	{
	public:
		TBTree* m_pRbTree;
		TBaseInnerNode* m_pBaseInner;
		uint64 m_nThis;
		
	public:
		CBTreeInnerNode(TBTree* pBTree = NULL)
			:m_pRbTree(pBTree),
			m_pBaseInner(NULL),
			m_nThis(0)
		{
		}
		
		CBTreeInnerNode(TBTree* pBTree,uint64 nAddr,bool bFirst=false)
			:m_pRbTree(pBTree),m_nThis(nAddr)
		{
			if( m_pRbTree->m_bMemory )
				m_pBaseInner = GET_MEMORY_OBJECT(TBaseInnerNode, m_nThis);
			else
				m_pBaseInner = m_pRbTree->AllocateInnerNode(m_nThis, bFirst);			
		}
		
		~CBTreeInnerNode()
		{
			if( !m_pRbTree->m_bMemory && m_pBaseInner )	
				m_pRbTree->DeAllocateInnerNode(m_nThis, m_pBaseInner);
		}
		
		CBTreeInnerNode& operator=(const CBTreeInnerNode& oSrc)
		{
			if(this == & oSrc )
				return *this;
			
			if( m_pBaseInner && !m_pRbTree->m_bMemory )
				m_pRbTree->DeAllocateInnerNode(m_nThis, m_pBaseInner);
			
			m_pRbTree = oSrc.m_pRbTree;
			m_pBaseInner = oSrc.m_pBaseInner;
			m_nThis = oSrc.m_nThis;
			
			if( m_pBaseInner && !m_pRbTree->m_bMemory )
				m_pRbTree->AddInnerRef(m_pBaseInner);
			
			return *this;
		}
		
		void Initialize(uint32 nLevel,uint64 nFather)
		{
			m_pBaseInner->nLevel = nLevel;
			m_pBaseInner->nSlotUse = 0;
			m_pBaseInner->nFather = nFather;
		}
		
		CBTreeInnerNode& SetThis(uint64 nThis, bool bFirst=false)
		{
			if( m_nThis == nThis )
				return *this;
			
			if( m_pBaseInner && !m_pRbTree->m_bMemory )
			{
				m_pRbTree->DeAllocateInnerNode(m_nThis, m_pBaseInner);
				m_pBaseInner = NULL;
			}
			
			m_nThis = nThis;

			if( 0 == nThis )
				return  *this;
			
			if( m_pRbTree->m_bMemory )
				m_pBaseInner = GET_MEMORY_OBJECT(TBaseInnerNode, nThis);
			else
				m_pBaseInner =  m_pRbTree->AllocateInnerNode(nThis, bFirst);
			
			return *this;
		}		
		
		bool IsInnerNode()
		{
			return (m_pBaseInner->nLevel != 0);
		}		
		bool IsFull() const
		{
			return (m_pBaseInner->nSlotUse == m_pRbTree->MAX_INNER_SLOTS);
		}
		bool IsUnderFlow()const
		{
			return (m_pBaseInner->nSlotUse < m_pRbTree->MIN_INNER_SLOTS);
		}
		bool IsFew()const
		{
			return (m_pBaseInner->nSlotUse <= m_pRbTree->MIN_INNER_SLOTS);
		}
	};	
	
	class CBTreeLeafNode
	{
	public:
		TBTree* m_pRbTree;
		TBaseLeafNode* m_pBaseLeaf;
		uint64 m_nThis;
		
		~CBTreeLeafNode()
		{
			if( m_pBaseLeaf && !m_pRbTree->m_bMemory )			
				m_pRbTree->DeAllocateInnerNode(m_nThis, (TBaseInnerNode*)m_pBaseLeaf);
		}
		
		CBTreeLeafNode(TBTree* pRbTree=NULL)
			:m_pRbTree(pRbTree),
			m_pBaseLeaf(NULL),
			m_nThis(0)
		{		
		}
		
		CBTreeLeafNode(TBTree* pBTree,uint64 nAddr,bool bFirst=false)
			:m_pRbTree(pBTree),
			m_nThis(nAddr)
		{
			if(m_pRbTree->m_bMemory )
				m_pBaseLeaf = GET_MEMORY_OBJECT(TBaseLeafNode, m_nThis);
			else
				m_pBaseLeaf = (TBaseLeafNode*) m_pRbTree->AllocateInnerNode(m_nThis, bFirst);
		}
		
		CBTreeLeafNode(CBTreeLeafNode& oSrc)
			:m_pRbTree(oSrc.m_pRbTree),
			m_pBaseLeaf(oSrc.m_pBaseLeaf),
			m_nThis(oSrc.m_nThis)
		{
			if( m_pBaseLeaf && !m_pRbTree->m_bMemory )
				m_pRbTree->AddInnerRef((TBaseInnerNode*)m_pBaseLeaf);
		}
		
		CBTreeLeafNode& operator=(CBTreeLeafNode& oSrc)
		{
			if( this == &oSrc )
				return *this;
			
			if( m_pBaseLeaf && !m_pRbTree->m_bMemory )
				m_pRbTree->DeAllocateInnerNode(m_nThis, (TBaseInnerNode*)m_pBaseLeaf);
			
			m_pRbTree = oSrc.m_pRbTree;
			m_pBaseLeaf = oSrc.m_pBaseLeaf;
			m_nThis = oSrc.m_nThis;
			
			if( m_pBaseLeaf && !m_pRbTree->m_bMemory )
				m_pRbTree->AddInnerRef((TBaseInnerNode* )m_pBaseLeaf);
			
			return *this;
		}
		
		void Initialize(uint64 nFather)
		{
			m_pBaseLeaf->nLevel = 0;
			m_pBaseLeaf->nSlotUse = 0;
			m_pBaseLeaf->nFather = nFather;
			m_pBaseLeaf->nPrevLeaf = 0;
			m_pBaseLeaf->nNextLeaf = 0;
		}
		
		uint64 GetThis()
		{
			return m_nThis;
		}
		
		CBTreeLeafNode& SetThis(uint64 nThis, bool bFirst=false)
		{
			if( m_nThis == nThis )
				return *this;
			
			if( m_pBaseLeaf && !m_pRbTree->m_bMemory )
			{
				m_pRbTree->DeAllocateInnerNode(m_nThis, (TBaseInnerNode* )m_pBaseLeaf);
				m_pBaseLeaf = NULL;
			}
			
			m_nThis = nThis;
			if( 0 == nThis )
				return  *this;
			
			if( m_pRbTree->m_bMemory )
				m_pBaseLeaf = GET_MEMORY_OBJECT(TBaseLeafNode, nThis);
			else
				m_pBaseLeaf = (TBaseLeafNode*) m_pRbTree->AllocateInnerNode(nThis, bFirst);
			
			return *this;
		}
		
		bool IsEndNode()
		{
			return (m_nThis == m_pRbTree->m_pObjectInfo->m_nEnd) ? true:false;
		}
		
		bool IsBeginNode()
		{
			return ( m_pBaseLeaf->nPrevLeaf   == m_pRbTree->m_pObjectInfo->m_nEnd) ? true:false;
		}
		
		TData& GetValue(int32 s)
		{
			return m_pBaseLeaf->pSlotData[s];
		}

		TKey& GetKey(int32 s)
		{
			return m_pBaseLeaf->pSlotKey[s];
		}
		
		void SetValue(TData& v,int32 s)
		{
			m_pBaseLeaf->pSlotData[s] = v;
			m_pBaseLeaf->m_nDirty = 1;
		}		
		
		uint64 GetParent()
		{
			return m_pBaseLeaf->nFather;
		}		
		bool IsFull() const
		{
			return (m_pBaseLeaf->nSlotUse == m_pRbTree->MAX_LEAF_SLOTS);
		}
		bool IsUnderFlow()const
		{
			return (m_pBaseLeaf->nSlotUse < m_pRbTree->MIN_LEAF_SLOTS);
		}
		bool IsFew()const
		{
			return (m_pBaseLeaf->nSlotUse <=m_pRbTree->MIN_LEAF_SLOTS);		
		}
	};
	
public:
	class CIterator
	{
	private:
		CBTreeLeafNode m_oLeaf;
		int32 m_nCurrSlot;
		uint64 m_nEnd;			/*end node addr*/		
		
	public:
		~CIterator()
		{
		}
		
		CIterator(CBTree* pRbTree = NULL)
			:m_oLeaf(pRbTree),
			m_nCurrSlot(0),
			m_nEnd(0)
		{
		}
		
		CIterator(CBTreeLeafNode& node,int32 slot,uint64 end)
			:m_oLeaf(node),
			m_nCurrSlot(slot),
			m_nEnd(end)
		{
			if( !m_oLeaf.GetThis() && m_nEnd )
				m_oLeaf.SetThis(m_nEnd);       /*end node*/
		}
		
		CIterator(CIterator& oSrc)
		{
			if(this != &oSrc)
			{
				m_oLeaf = oSrc.m_oLeaf;
				m_nCurrSlot = oSrc.m_nCurrSlot;
				m_nEnd = oSrc.m_nEnd;
			}
		}
		
		CIterator& operator++()
		{
			if( m_oLeaf.IsEndNode() )
				return *this;
			
			if( m_nCurrSlot < m_oLeaf.m_pBaseLeaf->nSlotUse - 1 )
				m_nCurrSlot++;
			else
			{
				m_nCurrSlot = 0;
				m_oLeaf.SetThis( m_oLeaf.m_pBaseLeaf->nNextLeaf );
			}
			return *this;
		}
		
		CIterator& operator--()
		{
			if( m_oLeaf.IsBeginNode() )
				return *this;		
			
			if(m_nCurrSlot != 0)
				m_nCurrSlot--; 
			else
			{
				m_oLeaf.SetThis( m_oLeaf.m_pBaseLeaf->nPrevLeaf );	
				m_nCurrSlot = m_oLeaf.m_pBaseLeaf->nSlotUse - 1;
			}
			
			return *this;
		}
		
		CIterator operator++ (int32)
		{
			CIterator it(*this);
			operator++();
			return it;
		}
		
		CIterator operator-- (int32)
		{
			CIterator it(*this);
			operator--();
			return it;
		}
		
		bool operator == (CIterator &oSrc)
		{
			return GetThis() == oSrc.GetThis();
		}
		
		bool operator != (CIterator &oSrc)
		{
			return GetThis() != oSrc.GetThis();
		}
		
		uint64 GetThis()
		{
			uint64 nRet = m_oLeaf.GetThis();
			return ( nRet==m_nEnd  ? 0 : nRet );
		}
		
		CIterator& SetThis(uint64 nThis,int32 nSlot,uint64 nEnd)
		{
			m_oLeaf.SetThis(nThis);
			m_nCurrSlot = nSlot;      
			m_nEnd = nEnd;
			return *this;
		}
		
		TData& GetValue()
		{
			return m_oLeaf.GetValue(m_nCurrSlot);
		}
		
		void SetValue(TData& value)
		{
			m_oLeaf.SetValue(value,m_nCurrSlot);
		}
		
		TKey& GetKey()
		{
			return m_oLeaf.GetKey(m_nCurrSlot);
		}

		CBTreeLeafNode& GetNode()
		{
			return m_oLeaf;
		}
		
		uint64 GetHead()
		{
			return m_nEnd;
		}
		
		int32 GetSlot()
		{
			return m_nCurrSlot;
		}
		
		void SetSlot(int32 slot)
		{
			m_nCurrSlot = slot;
		}	
	};	
private:
	struct CAuxStack
	{
		CAuxStack():
			pSlotStack(NULL),pStackTop(-1)
		{
		}
		
		~CAuxStack()
		{
			if(pSlotStack)
				delete []  (char*)pSlotStack;
		}
		
		int32 Initialize(int32 nLevel)
		{
			const int32 BUFF_SIZE = (sizeof(int32)) * nLevel + 1;
			char* p = new(std::nothrow) char[BUFF_SIZE];
			if( !p )
				return -1;
			
			CBinary::MemorySet(p,0,BUFF_SIZE);
			
			pSlotStack = (int32*)p;
			return 0;
		}
		
		int32* pSlotStack;		/*note slot in each nLevel*/
		int32 pStackTop;
	};
	
	private:
		uint64 m_nThis;
		CBaseBTreeInfo * m_pObjectInfo;
		
		CVirtualAccess* m_pAccess;
		CVirtualAllocator* m_pAllocator;
		CVirtualAllocator* m_pTopAllocator;
		
		//leaf node and inner node both use this pool
		TInnerNodePool m_oInnerPool;
		
		CVirtualCompare<TKey>* m_pCompare;
		
		INNER_CONTARINER_T m_nInnerContariner;
		uint64 m_nLeafContainer;
		
	public:
		uint32 m_bMemory;				/*0: disk  other:memory*/
		
	private:
		CBTree(CBTree& oSrc);					/*prohibit copy construction*/
		CBTree& operator=(CBTree & oSrc);		/*prohibit operator=()*/
		
		void InitEnd()
		{
			CBTreeLeafNode oEnd(this);
			oEnd.SetThis(m_pObjectInfo->m_nEnd, true);
			oEnd.m_pBaseLeaf->nFather = 0;
			oEnd.m_pBaseLeaf->nPrevLeaf = m_pObjectInfo->m_nEnd;		
			oEnd.m_pBaseLeaf->nNextLeaf= m_pObjectInfo->m_nEnd;		
		}
		
	private:
		inline bool KeyLess(TKey &a,TKey& b) const
		{
			return m_pCompare->Compare(a,b);
		}
		
		inline bool KeyLessEqual(TKey &a, TKey& b) const
		{
			return !m_pCompare->Compare(b, a);
		}
		
		inline bool KeyGreater(TKey &a, TKey &b) const
		{
			return m_pCompare->Compare(b, a);
		}
		
		inline bool KeyGreaterEqual(TKey &a, TKey& b) const
		{
			return !m_pCompare->Compare(a, b);
		}
		
		inline bool KeyEqual(TKey &a,TKey &b) const
		{
			return !m_pCompare->Compare(a, b) && !m_pCompare->Compare(b, a);
		}
		
		/// find first slot greater or equal  key	
		/// return: 0---n
		inline int32 FindLower(TKey* buff,const int32 n, TKey& key)
		{
			int32 lo = 0;
			int32 hi = n - 1;
			
			while(lo < hi)
			{
				int32 mid = (lo + hi) >> 1;
				
				if (KeyLessEqual(key, buff[mid])) 
					hi = mid - 1;
				else 
					lo = mid + 1;
			}
			
			if (hi < 0 || KeyLess(buff[hi], key))
				hi++;
			
			// verify result using simple linear search
			return hi;
		}
		
		/// find first slot greater than key
		/// return: 0---n
		inline int32 FindUpper(TKey* buff,const int32 n,TKey& key)
		{
			int32 lo = 0;
			int32 hi = n - 1;
			
			while(lo < hi)
			{
				int32 mid = (lo + hi) >> 1;
				
				if ( KeyLess(key,buff[mid]) ) 
					hi = mid - 1;
				else 
					lo = mid + 1;
			}
			
			if ( hi < 0 || KeyLessEqual(buff[hi], key) )
				hi++;
			
			return hi;
		}
		
		///return value: 0 --- n-1
		inline int32 FindInsertSlotInner(TKey*  buff, int32 n, TKey& key)
		{
			int32 lo = 0;
			int32 hi = n - 1;
			
			while(lo < hi)
			{
				int32 mid = (lo + hi) >> 1;
				
				if ( KeyLess(key,buff[mid]) ) 
					hi = mid - 1;
				else 
					lo = mid + 1;
				
			}
			if( hi > 0 && KeyGreater(buff[hi], key) )
				hi--;
			else if( hi < 0 )
				hi = 0;	
			return hi;
		}
		
		void AddIntoList(CBTreeLeafNode& oNew,CBTreeLeafNode& oPrev)
		{
			oNew.m_pBaseLeaf->nPrevLeaf = oPrev.m_nThis;
			oNew.m_pBaseLeaf->nNextLeaf= oPrev.m_pBaseLeaf->nNextLeaf;
			
			CBTreeLeafNode oNext(this,oPrev.m_pBaseLeaf->nNextLeaf);
			oNext.m_pBaseLeaf->nPrevLeaf =  oNew.m_nThis;
			
			oPrev.m_pBaseLeaf->nNextLeaf = oNew.m_nThis;
			
			oNew.m_pBaseLeaf->m_nDirty = 1;
			oPrev.m_pBaseLeaf->m_nDirty = 1;	
			oNext.m_pBaseLeaf->m_nDirty = 1;
		}
		
		CIterator InsertNullTree(TKey& key,TData& data)
		{
			uint64 nNode = GetNewLeaf();
			
			CBTreeLeafNode oNewLeaf(this,nNode,true);
			oNewLeaf.Initialize(0);
			
			oNewLeaf.m_pBaseLeaf->pSlotKey[0] = key;
			oNewLeaf.m_pBaseLeaf->pSlotData[0] = data;
			oNewLeaf.m_pBaseLeaf->nSlotUse = 1;
			
			//set end node value
			CBTreeLeafNode oEnd(this,m_pObjectInfo->m_nEnd);
			AddIntoList(oNewLeaf,oEnd);
			
			oEnd.m_pBaseLeaf->nFather = oNewLeaf.m_nThis;
			oEnd.m_pBaseLeaf->m_nDirty = 1;
			
			m_pObjectInfo->m_nItemCount = 1;
			if( !m_bMemory )
				vflush(m_pObjectInfo);
			
			CIterator it(oNewLeaf,0,m_pObjectInfo->m_nEnd);
			return it;		
		}
		
		int32  BuildStack(CBTreeInnerNode& oInner,CAuxStack& oInfo,TKey& key)
		{
			while( oInner.IsInnerNode() )
			{
				const int32 n = oInner.m_pBaseInner->nSlotUse;
				TKey* const k = oInner.m_pBaseInner->pSlotKey;
				uint64* const c = oInner.m_pBaseInner->pSlotChild;
				
				int32 pos = FindLower(k,n,key);
				if( pos == 0 && !KeyEqual(key, k[0]) )
					return -1;
				else if(pos==n || !KeyEqual(key,k[pos])  )
					pos--;
				
				oInner.SetThis( c[pos] );
				
				oInfo.pSlotStack[++oInfo.pStackTop] = pos;
			}
			return 0;
		}	
		
		void  UpdateIndex(uint64 nFatherAddr,CAuxStack* pAuxBuff,TKey* pKey)
		{
			for( ; nFatherAddr != 0; pAuxBuff->pStackTop-- )
			{
				CBTreeInnerNode oInner(this,nFatherAddr);
				
				oInner.m_pBaseInner->pSlotKey[ pAuxBuff->pSlotStack[pAuxBuff->pStackTop] ] = *pKey;
				oInner.m_pBaseInner->m_nDirty = 1;
				
				if( pAuxBuff->pSlotStack[pAuxBuff->pStackTop] != 0)
					return;
				
				pKey = &oInner.m_pBaseInner->pSlotKey[0];
				nFatherAddr = oInner.m_pBaseInner->nFather;			
			}
		}
		
		void  UpdateIndex(uint64 fa_addr,uint64 curr_addr,TKey* k)
		{
			while( fa_addr != 0 )
			{
				CBTreeInnerNode oFatherInner(this,fa_addr);
				TKey* fa_k= oFatherInner.m_pBaseInner->pSlotKey;
				uint64* fa_c = oFatherInner.m_pBaseInner->pSlotChild;
				int32& fa_s = oFatherInner.m_pBaseInner->nSlotUse;
				
				int32 ii = 0;
				for( ; ii < fa_s && fa_c[ii] != curr_addr; ii++ ) ; 
				
				fa_k[ii] = *k;
				
				oFatherInner.m_pBaseInner->m_nDirty = 1;
				
				if( ii != 0 )
					break;
				
				curr_addr = oFatherInner.m_nThis;
				fa_addr = oFatherInner.m_pBaseInner->nFather;
				
				//no need udpate k????
			}
		}
		
		bool EraseOne(TKey& key)
		{
			CBTreeLeafNode oEnd(this,m_pObjectInfo->m_nEnd);
			const uint64 root = oEnd.m_pBaseLeaf->nFather;
			
			//NULL tree
			if( root == 0 )	
				return false;
			
			CBTreeInnerNode oInner(this,root);
			const int32 ROOT_LEVEL = oInner.m_pBaseInner->nLevel;
			
			CAuxStack oAuxStack;
			int32 nRst = oAuxStack.Initialize(oInner.m_pBaseInner->nLevel);
			if( nRst != 0 )
				return false;
			
			nRst = BuildStack(oInner,oAuxStack,key);
			if( nRst != 0 )
				return false;/*not exist*/
			
			CBTreeLeafNode oCurrLeaf(this,oInner.m_nThis);
			int32& curr_s = oCurrLeaf.m_pBaseLeaf->nSlotUse;
			TKey* const curr_k= oCurrLeaf.m_pBaseLeaf->pSlotKey;
			TData* const curr_d = oCurrLeaf.m_pBaseLeaf->pSlotData; 
			
			const int32 POS = FindLower(curr_k,curr_s,key);
			if( POS == curr_s && !KeyEqual(key,curr_k[POS]) )
				return false;	/*not exist*/
			
			//erase it
			int32 ii = POS;
			for( ; ii < curr_s - 1; ii++)
			{
				curr_k[ii] = curr_k[ii+1];
				curr_d[ii] = curr_d[ii+1];			
			}
			curr_s--;
			m_pObjectInfo->m_nItemCount--;
			if( !m_bMemory )
				vflush(m_pObjectInfo);
			
			if( curr_s == 0 )	/*become null tree*/
			{
				oEnd.m_pBaseLeaf->nNextLeaf = oEnd.m_nThis;
				oEnd.m_pBaseLeaf->nPrevLeaf = oEnd.m_nThis;
				
				oEnd.m_pBaseLeaf->nFather = NULL;
				oEnd.m_pBaseLeaf->m_nDirty = 1;
				
				FreeLeaf(oCurrLeaf.m_nThis);
				return true;
			}
			else if( root == oCurrLeaf.m_nThis || ( !oCurrLeaf.IsUnderFlow() && POS != 0) )
				return true;
			else if( !oCurrLeaf.IsUnderFlow() && POS == 0)	/*update index*/
			{
				UpdateIndex(oCurrLeaf.m_pBaseLeaf->nFather,&oAuxStack,&(curr_k[0]));
				return true;
			}
			
			//process leaf node
			oInner.SetThis(oCurrLeaf.m_pBaseLeaf->nFather);
			int32 slot = oAuxStack.pSlotStack[oAuxStack.pStackTop--];
			
			uint64 leftleaf = 0;
			uint64 rightleaf = 0;
			if( slot > 0 )
				leftleaf = oInner.m_pBaseInner->pSlotChild[slot-1];
			if( slot < oInner.m_pBaseInner->nSlotUse - 1)
				rightleaf = oInner.m_pBaseInner->pSlotChild[slot+1];
			
			do
			{
				if( leftleaf )
				{
					CBTreeLeafNode oLeftLeaf(this,leftleaf);
					if( !oLeftLeaf.IsFew() )
					{
						ShiftLeftLeaf(&oLeftLeaf,&oCurrLeaf,&oInner,slot);
						break;
					}
				}
				else
				{
					CBTreeLeafNode oRightLeaf(this,rightleaf);
					if( !oRightLeaf.IsFew() )
					{
						ShiftRightLeaf(&oRightLeaf,&oCurrLeaf,&oInner,slot); 
						break;
					}		
				}
				
				if( leftleaf )
				{
					CBTreeLeafNode oLeftLeaf(this,leftleaf);
					MergeLeaves(&oLeftLeaf,&oCurrLeaf,&oInner,slot-1);
				}
				else
				{
					CBTreeLeafNode oRightLeaf(this,rightleaf);
					MergeLeaves(&oCurrLeaf,&oRightLeaf,&oInner,slot);
				}
			}while(0);
			
			//process inner node
			while( oInner.m_nThis != root )
			{
				if( !oInner.IsUnderFlow() )
				{
					//update index
					while( oInner.m_nThis != root && slot == 0 && POS == 0 )
					{
						TKey new_index = oInner.m_pBaseInner->pSlotKey[0];
						oInner.SetThis( oInner.m_pBaseInner->nFather );
						slot = oAuxStack.pSlotStack[oAuxStack.pStackTop--];
						
						oInner.m_pBaseInner->pSlotKey[slot] = new_index;
						oInner.m_pBaseInner->m_nDirty = 1;
					}
					return true;
				}
				slot = oAuxStack.pSlotStack[oAuxStack.pStackTop--];
				
				CBTreeInnerNode oParentInner(this,oInner.m_pBaseInner->nFather);
				uint64 leftinner = 0;
				uint64 rightinner = 0;
				if( slot > 0 )
					leftinner = oParentInner.m_pBaseInner->pSlotChild[slot-1];
				if( slot < oParentInner.m_pBaseInner->nSlotUse - 1)
					rightinner = oParentInner.m_pBaseInner->pSlotChild[slot+1];
	              	
				do
				{
					if(leftinner)
					{
						CBTreeInnerNode oLeftInner(this,leftinner);
						if( !oLeftInner.IsFew() )
						{
							ShiftLeftInner(&oLeftInner,&oInner,&oParentInner,slot);
							break;
						}
					}
					else
					{
						CBTreeInnerNode oRightInner(this,rightinner);
						if( !oRightInner.IsFew() )
						{
							ShiftRightInner(&oInner,&oRightInner,&oParentInner,slot);
							break;
						}					
					}
					
					if(leftinner)
					{
						CBTreeInnerNode oLeftInner(this,leftinner);
						MergeInner(&oLeftInner,&oInner,&oParentInner,slot-1,ROOT_LEVEL);
					}
					else
					{
						CBTreeInnerNode oRightInner(this,rightinner);
						MergeInner(&oInner,&oRightInner,&oParentInner,slot,ROOT_LEVEL);	
					}
				}while(0);
				
				oInner = oParentInner;
			}
			
			if(oInner.m_pBaseInner->nSlotUse == 1)
			{
				CBTreeInnerNode oNewRoot(this,oInner.m_pBaseInner->pSlotChild[0]);
				oNewRoot.m_pBaseInner->nFather = 0;
				oNewRoot.m_pBaseInner->m_nDirty = 1;
				
				oEnd.m_pBaseLeaf->nFather = oNewRoot.m_nThis;
				oEnd.m_pBaseLeaf->m_nDirty = 1;
				
				FreeInner(oInner.m_nThis,ROOT_LEVEL);
			}
			
			return true;
	}
	
	bool ShiftLeftInner(CBTreeInnerNode* left,CBTreeInnerNode* curr,CBTreeInnerNode* nFather,int32 curr_slot)
	{
		TKey* left_k = left->m_pBaseInner->pSlotKey;
		uint64* left_c = left->m_pBaseInner->pSlotChild;
		int32& left_s = left->m_pBaseInner->nSlotUse;
		
		TKey* curr_k = curr->m_pBaseInner->pSlotKey;
		uint64* curr_c = curr->m_pBaseInner->pSlotChild;
		int32& curr_s = curr->m_pBaseInner->nSlotUse;
		
		int32 num = (left_s - MIN_INNER_SLOTS)>>1;
		num = (num ==0)?1:num ;		
		
		int32 i = 0;
		for( i = curr_s-1; i >= 0; i--)
		{
			curr_k[i+num] = curr_k[i];
			curr_c[i+num] = curr_c[i];
		}
		
		CBTreeInnerNode oChild( this);
		for( i = 0; i < num; i++)
		{
			curr_k[i] = left_k[ left_s - num + i ];
			curr_c[i] = left_c[ left_s - num + i ];
			
			//update child nFather pointer
			oChild.SetThis( curr_c[i] );
			oChild.m_pBaseInner->nFather = curr->m_nThis;
			oChild.m_pBaseInner->m_nDirty = 1;
		}
		
		curr_s += num;
		left_s -= num;
		
		//update nFather index
		nFather->m_pBaseInner->pSlotKey[curr_slot] = curr_k[0];
		nFather->m_pBaseInner->pSlotKey[curr_slot-1] = left_k[0];
		
		left->m_pBaseInner->m_nDirty = 1;
		curr->m_pBaseInner->m_nDirty = 1;
		nFather->m_pBaseInner->m_nDirty = 1;
		
		return true;
	}
	
	bool ShiftRightInner(CBTreeInnerNode* curr,CBTreeInnerNode* right,CBTreeInnerNode* nFather,int32 curr_slot)
	{
		TKey* right_k = right->m_pBaseInner->pSlotKey;
		uint64* right_c = right->m_pBaseInner->pSlotChild;
		int32& right_s = right->m_pBaseInner->nSlotUse;
		
		TKey* curr_k = curr->m_pBaseInner->pSlotKey;
		uint64* curr_c = curr->m_pBaseInner->pSlotChild;
		int32& curr_s = curr->m_pBaseInner->nSlotUse;
		
		int32 num = (right_s - MIN_INNER_SLOTS)>>1;
		num = (num ==0)?1:num;	
		
		int32 i= 0;
		CBTreeInnerNode oChild( this);		
		for( i = 0; i < num; i++)
		{
			curr_k[curr_s+i] = right_k[i];
			curr_c[curr_s+i] = right_c[i];
			
			//update child node nFather pointer
			oChild.SetThis(curr_c[curr_s+i]);
			oChild.m_pBaseInner->nFather = curr->m_nThis;
			oChild.m_pBaseInner->m_nDirty = 1;
		}     		
		curr_s += num;
		
		for( i = num ; i < right_s; i++)
		{
			right_k[ i - num  ] = right_k[ i ];
			right_c[ i - num  ] = right_c[ i ];
		}
		right_s -= num ;
		
		//update index
		nFather->m_pBaseInner->pSlotKey[curr_slot+1] =  right_k[0];
		nFather->m_pBaseInner->pSlotKey[curr_slot] =  curr_k[0];
		
		curr->m_pBaseInner->m_nDirty = 1;
		right->m_pBaseInner->m_nDirty = 1;		
		nFather->m_pBaseInner->m_nDirty = 1;
		
		return true;
	}
	
	bool MergeInner(CBTreeInnerNode* left,CBTreeInnerNode* right,CBTreeInnerNode* nFather,int32 left_slot,int32 root_level)
	{
		TKey* left_k = left->m_pBaseInner->pSlotKey;
		uint64* left_c = left->m_pBaseInner->pSlotChild;
		int32& left_s = left->m_pBaseInner->nSlotUse;
		
		TKey* right_k = right->m_pBaseInner->pSlotKey;
		uint64* right_c = right->m_pBaseInner->pSlotChild;
		int32& right_s = right->m_pBaseInner->nSlotUse;
		
		CBTreeInnerNode oChild(this);
		int32 i = 0;
		for( ; i < right_s; i++ )
		{
			left_k[left_s+i] = right_k[i];
			left_c[left_s+i] = right_c[i];
			
			//update child nFather pointer
			oChild.SetThis( right_c[i] );
			oChild.m_pBaseInner->nFather = left->m_nThis;
			oChild.m_pBaseInner->m_nDirty = 1;
		}
		left_s += right_s;
		
		TKey* father_k = nFather->m_pBaseInner->pSlotKey;
		uint64* father_c = nFather->m_pBaseInner->pSlotChild;
		int32& father_s = nFather->m_pBaseInner->nSlotUse;
		
		for( i = left_slot + 1;i < father_s - 1; i++)
		{
			father_k[i] = father_k[i+1];
			father_c[i] = father_c[i+1];
		}
		father_s--;
		
		//update index
		father_k[left_slot] = left_k[0];
		
		FreeInner(right->m_nThis,root_level);
		
		nFather->m_pBaseInner->m_nDirty = 1;
		left->m_pBaseInner->m_nDirty = 1;
		
		return true;
	}
	
	bool ShiftLeftLeaf(CBTreeLeafNode* left,CBTreeLeafNode* curr,CBTreeInnerNode* nFather,int32 curr_slot,CIterator* it=NULL)
	{	
		TKey* left_k = left->m_pBaseLeaf->pSlotKey;
		TData* left_d = left->m_pBaseLeaf->pSlotData;
		int32& left_s = left->m_pBaseLeaf->nSlotUse;
		
		TKey* curr_k = curr->m_pBaseLeaf->pSlotKey;
		TData* curr_d = curr->m_pBaseLeaf->pSlotData;
		int32& curr_s = curr->m_pBaseLeaf->nSlotUse;
		
		int32 num = (left_s - MIN_LEAF_SLOTS)>>1;
		num = (num==0) ? 1 : num;
		
		int32 it_slot = 0;
		if( it && (it_slot = it->GetSlot()) == curr_s)
			it->SetThis(curr->m_pBaseLeaf->nNextLeaf,0,m_pObjectInfo->m_nEnd);
		else if( it )
			it->SetSlot( it_slot + num );
		
		int32 i;
		for( i = curr_s -1; i >= 0; i--)
		{
			curr_k[i+num] = curr_k[i];
			curr_d[i+num] = curr_d[i];
		}
		for( i = 0; i < num; i++)
		{
			curr_k[ i ] = left_k[ left_s - num + i ];
			curr_d[ i ] = left_d[ left_s - num + i ];
		}		
		curr_s += num;
		left_s -= num;
		
		//update index
		nFather->m_pBaseInner->pSlotKey[curr_slot] =  curr_k[0];
		
		left->m_pBaseLeaf->m_nDirty = 1;
		curr->m_pBaseLeaf->m_nDirty = 1;	        
		nFather->m_pBaseInner->m_nDirty = 1;
		
		return true;
	}
	
	bool ShiftRightLeaf(CBTreeLeafNode* right,CBTreeLeafNode* curr,CBTreeInnerNode* nFather,int32 curr_slot)
	{	
		TKey* right_k = right->m_pBaseLeaf->pSlotKey;
		TData* right_d = right->m_pBaseLeaf->pSlotData;
		int32& right_s = right->m_pBaseLeaf->nSlotUse;
		
		TKey* curr_k = curr->m_pBaseLeaf->pSlotKey;
		TData* curr_d = curr->m_pBaseLeaf->pSlotData;
		int32& curr_s = curr->m_pBaseLeaf->nSlotUse;
		
		int32 num = (right_s - MIN_LEAF_SLOTS)>>1;
		num = (num==0) ? 1 : num;
		
		int32 i = 0;
		for( ; i < num; i++)
		{
			curr_k[curr_s+i] = right_k[i];
			curr_d[curr_s+i] = right_d[i];
		}
		curr_s += num;
		for(i = num; i < right_s; i++)
		{
			right_k[i - num] = right_k[i];
			right_d[i - num] = right_d[i];
		}
		right_s -= num;
		
		//update index
		nFather->m_pBaseInner->pSlotKey[curr_slot+1] =  right_k[0];
		nFather->m_pBaseInner->pSlotKey[curr_slot] =  curr_k[0];		 
		
		right->m_pBaseLeaf->m_nDirty = 1;
		curr->m_pBaseLeaf->m_nDirty = 1;	        
		
		return true;
	}
	
	bool MergeLeaves(CBTreeLeafNode* left,CBTreeLeafNode* right,CBTreeInnerNode* nFather,int32 left_slot,CIterator* it=NULL)
	{
		TKey* left_k = left->m_pBaseLeaf->pSlotKey;
		TData* left_d = left->m_pBaseLeaf->pSlotData;
		int32& left_s = left->m_pBaseLeaf->nSlotUse;
		
		TKey* right_k = right->m_pBaseLeaf->pSlotKey;
		TData* right_d = right->m_pBaseLeaf->pSlotData;
		int32& right_s = right->m_pBaseLeaf->nSlotUse;
		
		int32 it_slot = 0;
		if( it  && (it_slot = it->GetSlot()) == right_s)
			it->SetThis(right->m_pBaseLeaf->nNextLeaf,0, m_pObjectInfo->m_nEnd);
		else if( it )
			it->SetThis(left->m_nThis,left_s+it_slot+1,m_pObjectInfo->m_nEnd);
		
		int32 ii;
		for(ii = 0; ii < right_s; ii++)
		{
			left_k[left_s+ii] = right_k[ii];
			left_d[left_s+ii] = right_d[ii];
		}
		left_s += right_s;
		
		//updae index
		TKey* father_k = nFather->m_pBaseInner->pSlotKey;
		uint64* father_c = nFather->m_pBaseInner->pSlotChild;
		int32& father_s = nFather->m_pBaseInner->nSlotUse;
		
		for( ii = left_slot+1; ii < father_s-1; ii++)
		{
			father_k[ii] = father_k[ii+1];
			father_c[ii] = father_c[ii+1];
		}
		father_s--;	        
		
		father_k[left_slot] = left_k[0];
		
		left->m_pBaseLeaf->m_nDirty = 1;
		nFather->m_pBaseInner->m_nDirty = 1;
		
		//detach right node from list
		CBTreeLeafNode oRRight(this,right->m_pBaseLeaf->nNextLeaf);
		oRRight.m_pBaseLeaf->nPrevLeaf = left->m_nThis;
		oRRight.m_pBaseLeaf->m_nDirty = 1;
		
		left->m_pBaseLeaf->nNextLeaf = right->m_pBaseLeaf->nNextLeaf;
		
		FreeLeaf(right->m_nThis);
		return true;
	}
	
	void FreeInner(uint64 addr,const int32 root_level)
	{
		if( m_nInnerContariner.GetSize() < root_level )
			m_nInnerContariner.Insert(addr);
		else
			m_pAllocator->DeAllocate(addr);
		
		m_pObjectInfo->m_nInnerCount--;
		if( !m_bMemory )
			vflush(m_pObjectInfo);
	}
	
	void FreeLeaf(uint64 addr)
	{
		if( m_nLeafContainer == 0 )
			m_nLeafContainer = addr;
		else
			m_pAllocator->DeAllocate(addr);
		
		m_pObjectInfo->m_nLeaveCount--;
		if( !m_bMemory )
			vflush(m_pObjectInfo);
	}	
	
	uint64 GetNewInner()
	{
		INNER_CONTARINER_IT_T b = m_nInnerContariner.First();
		uint64 addr = m_nInnerContariner.GetItem(b);
		m_nInnerContariner.Remove( b );
		
		m_pObjectInfo->m_nInnerCount++;
		if( !m_bMemory )
			vflush(m_pObjectInfo);
		return addr;
	}
	
	uint64 GetNewLeaf()
	{
		uint64 addr = m_nLeafContainer;
		m_nLeafContainer = 0;
		
		m_pObjectInfo->m_nLeaveCount++;
		if( !m_bMemory )
			vflush(m_pObjectInfo);
		return addr;
	}	
	
	bool AllocAllSpace(int32 root_level)
	{
		if( m_nLeafContainer == 0 )
		{
			m_nLeafContainer = m_pAllocator->Allocate( (uint32)VMM_LEAF_NODE_SIZE,m_bMemory );
			if( m_nLeafContainer == 0)
				return false;
		}
		
		INNER_CONTARINER_IT_T oIt;
		const int32 NUM = m_nInnerContariner.GetSize() - (root_level + 1);
		int32 ii = NUM;
		for( ; ii > 0; ii--)
		{
			oIt = m_nInnerContariner.First();
			m_pAllocator->DeAllocate(m_nInnerContariner.GetItem(oIt));
			m_nInnerContariner.Remove(oIt);
		}
		
		for( ii = NUM; ii < 0; ii++)
		{
			uint64 addr = m_pAllocator->Allocate( (uint32)VMM_INNER_NODE_SIZE,m_bMemory );
			if(addr == 0)
				return false;
			
			m_nInnerContariner.Insert(addr);
		}
		return true;
	}
	
	CIterator& Erase(CIterator& oFirst, CIterator &oEnd)
	{
		while(oFirst != oEnd)
			Erase(oFirst);
		return oEnd;
	}

public:
	void* GetObjectInfo()
	{
		return m_pObjectInfo;
	}
	
	~CBTree()
	{
		//free leaf
		if( m_nLeafContainer != 0 )
		{
			m_pAllocator->DeAllocate(m_nLeafContainer);
			m_nLeafContainer = 0;
		}
		
		//free inner
		INNER_CONTARINER_IT_T oIT = m_nInnerContariner.First();
		INNER_CONTARINER_IT_T oEndIT = m_nInnerContariner.End();
		for(;oIT != oEndIT; oIT++)
			m_pAllocator->DeAllocate(m_nInnerContariner.GetItem(oIT));
		
		if(m_pObjectInfo && !m_bMemory )
			unvmap(m_pObjectInfo);
	}
	
	CBTree(CVirtualAccess* pAccess,
		CVirtualCompare<TKey>* pCompare,
		CVirtualAllocator* pAllocator, 
		CVirtualAllocator* pTopAllocator=NULL)
	{
		m_nThis = 0;	
		m_bMemory = 0;
		m_pObjectInfo = NULL;
		
		m_pAccess = pAccess;
		m_pAllocator = pAllocator;
		m_pTopAllocator = pTopAllocator;
		
		m_pCompare = pCompare;
		
		if(MIN_LEAF_SLOTS >MAX_LEAF_SLOTS*1.0/2 || MIN_INNER_SLOTS > MAX_INNER_SLOTS*1.0/2 
			|| MAX_INNER_SLOTS < 4 ||MAX_LEAF_SLOTS < 4 )
			Abort();
		
		//Initialize container
		m_nLeafContainer = 0;
	}
	
	TBaseInnerNode* AllocateInnerNode(uint64 nThis, bool bFirst=false)
	{
		return m_oInnerPool.QueryObject(nThis, bFirst);
	}
	
	void DeAllocateInnerNode(uint64 nThis, TBaseInnerNode* pNode)
	{
		m_oInnerPool.ReleaseObject(nThis, pNode);
	}
	
	void AddInnerRef(TBaseInnerNode* pNode)
	{
		m_oInnerPool.AddRef(pNode);
	}
	
	uint64 GetThis()
	{
		return m_nThis;
	}
	
	CBTree& SetThis(uint64 nThis)
	{
		if(nThis == m_nThis)
			return *this;
		
		if( m_pObjectInfo && !m_bMemory )
		{
			unvmap(m_pObjectInfo);
			m_pObjectInfo = NULL;
		}
		
		m_nThis = nThis;
		if(nThis)
		{
			m_bMemory = IS_MEMORY_ADDR(nThis);
			if(m_bMemory)
				m_pObjectInfo = GET_MEMORY_OBJECT(CBaseBTreeInfo, nThis);
			else
				m_pObjectInfo = (CBaseBTreeInfo*)vmap(nThis);
		}
		return *this;
	}
	
	bool CreateObject(uint32 bUnique, int32 bInMemory)
	{
		DestroyObject();
		m_bMemory = bInMemory;
		if(m_pTopAllocator)
			m_nThis = m_pTopAllocator->Allocate((uint32)VMM_OBJECT_SIZE + VMM_LEAF_NODE_SIZE, bInMemory);
		else
			m_nThis = vmalloc((uint32)VMM_OBJECT_SIZE + VMM_LEAF_NODE_SIZE, bInMemory);
		
		if(!m_nThis)
			return false;
		
		if(m_bMemory)
			m_pObjectInfo = GET_MEMORY_OBJECT(CBaseBTreeInfo, m_nThis);
		else
			m_pObjectInfo = (CBaseBTreeInfo*)vmap(m_nThis);
		
		m_pObjectInfo->m_bUnique = bUnique;
		m_pObjectInfo->m_nItemCount = 0;
		m_pObjectInfo->m_nLeaveCount = 0;
		m_pObjectInfo->m_nInnerCount = 0;
		m_pObjectInfo->m_nEnd = m_nThis + VMM_OBJECT_SIZE;	
		
		if(!m_bMemory)
			vflush(m_pObjectInfo);
		
		InitEnd();
		return true;
	}
	
	void DestroyObject()
	{
		if(m_nThis)
		{
			Clear();
			if(!m_bMemory)
				unvmap(m_pObjectInfo);
			m_pObjectInfo = NULL;
			if(m_pTopAllocator)
				m_pTopAllocator->DeAllocate(m_nThis);
			else
				vfree(m_nThis);
			m_nThis = 0;
		}
	}
	
	void Truncate()
	{
		if(m_nThis && m_pObjectInfo->m_nItemCount)
		{
			InitEnd();
			m_pObjectInfo->m_nItemCount = 0;
			m_pObjectInfo->m_nLeaveCount = 0;
			m_pObjectInfo->m_nInnerCount = 0;
			if( !m_bMemory )
				vflush(m_pObjectInfo);
		}
	}
	
	void Clear()
	{
		CBTreeLeafNode oEnd(this,m_pObjectInfo->m_nEnd);
		uint64 nParent = oEnd.GetParent();
		if(nParent)
			Clear(nParent);
	}
	
	CIterator Begin()
	{
		CBTreeLeafNode oEnd(this,m_pObjectInfo->m_nEnd);
		CBTreeLeafNode oFirtstLeaf(this,oEnd.m_pBaseLeaf->nNextLeaf);
		return CIterator(oFirtstLeaf,0,m_pObjectInfo->m_nEnd);
	}
	
	CIterator End()
	{
		uint64 nEnd = m_pObjectInfo->m_nEnd;
		return CIterator(CBTreeLeafNode(this,nEnd),0,nEnd);
	}
	
	CIterator LowerBound(TKey& oKey)
	{
		CBTreeLeafNode oEnd(this,m_pObjectInfo->m_nEnd);
		if( oEnd.GetParent() == 0 )
			return End();
		
		CBTreeInnerNode oInner(this,oEnd.GetParent());
		while( oInner.IsInnerNode() )
		{
			register const int32 n = oInner.m_pBaseInner->nSlotUse;
			register TKey* k = oInner.m_pBaseInner->pSlotKey;
			
			int32 pos =  FindLower( k,n,oKey );
			if( pos == 0 && !KeyEqual(oKey,k[0]))
				return End();
          		else if( pos == n  || !KeyEqual(oKey,k[pos]) )
					pos--;
				
				oInner.SetThis(oInner.m_pBaseInner->pSlotChild[pos]);
		}
		
		CBTreeLeafNode oCurrLeaf(this,oInner.m_nThis);
		register const int32 n = oCurrLeaf.m_pBaseLeaf->nSlotUse;
		register TKey* k = oCurrLeaf.m_pBaseLeaf->pSlotKey;		
		int32 slot = FindLower( k,n,oKey );
		
		return ( slot < n && KeyEqual(oKey,k[slot]) ) ? CIterator(oCurrLeaf,slot,m_pObjectInfo->m_nEnd) : End();	
	}
	
	CIterator UpperBound(TKey& oKey)
	{
		CBTreeLeafNode oEnd(this,m_pObjectInfo->m_nEnd);
		if( oEnd.GetParent() == 0 )
			return End();
		
		CBTreeInnerNode oInner(this,oEnd.GetParent());
		while( oInner.IsInnerNode() )
		{
			register const int32 n = oInner.m_pBaseInner->nSlotUse;
			register TKey* k = oInner.m_pBaseInner->pSlotKey;
			
			int32 pos =  FindUpper( k,n,oKey ); 
			if( pos == 0 )
				return End();
          		else
					pos--;
				
				oInner.SetThis(oInner.m_pBaseInner->pSlotChild[pos]);
		}
		
		CBTreeLeafNode oCurrLeaf(this,oInner.m_nThis);
		register const int32 n = oCurrLeaf.m_pBaseLeaf->nSlotUse;
		register TKey* k = oCurrLeaf.m_pBaseLeaf->pSlotKey;
		register int32 slot = FindUpper( k,n,oKey );
		
		return ( slot != 0 && KeyEqual(oKey,k[slot-1]) ? CIterator(oCurrLeaf,slot-1,m_pObjectInfo->m_nEnd) : End());
	}
	
	CIterator Find(TKey& oKey)
	{
		return LowerBound(oKey);
	}
	
	CIterator Insert(TKey& key,TData& data,bool* pLackMemory=NULL)
	{
		CBTreeLeafNode oEnd(this,m_pObjectInfo->m_nEnd);
		const uint64 root = oEnd.m_pBaseLeaf->nFather;
		
		if( pLackMemory ) pLackMemory[0] = false;
		
		//alloc all space,may be larger than needed
		CBTreeInnerNode oInner(this);
		int32 root_level = 0;
		if( root != 0 )
		{
			oInner.SetThis(root);
			root_level = oInner.m_pBaseInner->nLevel;
		}
		if( !AllocAllSpace(root_level) )
		{
			if( pLackMemory ) pLackMemory[0] = true;
			return End();
		}
		
		if( root == 0 )	
			return InsertNullTree(key,data);
		
		int32 father_slot = -1;
		CBTreeInnerNode oFather(this);
		
		//locate destination leaf
		while( oInner.IsInnerNode() )
		{
			TKey* const old_k = oInner.m_pBaseInner->pSlotKey;
			uint64* const old_c = oInner.m_pBaseInner->pSlotChild;
			int32& old_s = oInner.m_pBaseInner->nSlotUse;
			
			int32 pos = FindInsertSlotInner(old_k,old_s,key);
			if( m_pObjectInfo->m_bUnique && KeyEqual(key,old_k[pos]) )
				return End();
			
			//update index
			if( pos == 0 && KeyLess(key,old_k[0]) ) 
			{
				old_k[0] = key;
				oInner.m_pBaseInner->m_nDirty = 1;
			}
			
			if( oInner.IsFull() )
			{
				uint64 addr = GetNewInner();
				CBTreeInnerNode oNewInner(this,addr,true);
				oNewInner.Initialize( oInner.m_pBaseInner->nLevel, oInner.m_pBaseInner->nFather );
				TKey* const new_k = oNewInner.m_pBaseInner->pSlotKey;
				uint64* const new_c = oNewInner.m_pBaseInner->pSlotChild;
				int32& new_s = oNewInner.m_pBaseInner->nSlotUse;
				
				const int32 NUM = old_s>>1;
				for( int32 ii = NUM; ii < old_s; ii++ )
				{
					new_k[ii - NUM] = old_k[ii];
					new_c[ii - NUM] = old_c[ii];
					
					CBTreeInnerNode oTmpInner(this,old_c[ii]);
					oTmpInner.m_pBaseInner->nFather = oNewInner.m_nThis;
					oTmpInner.m_pBaseInner->m_nDirty = 1;					
				}
				new_s = old_s - NUM;
				oNewInner.m_pBaseInner->m_nDirty = 1;
				
				old_s = NUM;
				oInner.m_pBaseInner->m_nDirty = 1;
				
				if( father_slot != -1 )
				{
					TKey* const father_k = oFather.m_pBaseInner->pSlotKey;
					uint64* const father_c = oFather.m_pBaseInner->pSlotChild;
					int32& father_s = oFather.m_pBaseInner->nSlotUse;
					
					for(int32 jj = father_s - 1; jj > father_slot; jj--)
					{
						father_k[jj+1] = father_k[jj];
						father_c[jj+1] = father_c[jj];
					}
					father_s++;
					
					father_k[father_slot+1] = new_k[0];
					father_c[father_slot+1] = oNewInner.m_nThis;
					
					oFather.m_pBaseInner->m_nDirty = 1;
				}
				else	/*oInner is root node*/
				{
					uint64 new_root = GetNewInner();
					CBTreeInnerNode oNewRoot(this,new_root,true);
					oNewRoot.Initialize( oInner.m_pBaseInner->nLevel+1, 0 );
					TKey* const root_k = oNewRoot.m_pBaseInner->pSlotKey;
					uint64* const root_c = oNewRoot.m_pBaseInner->pSlotChild;
					
					root_k[0] = old_k[0];
					root_c[0] = oInner.m_nThis;
					
					root_k[1] = new_k[0];
					root_c[1] = oNewInner.m_nThis;
					
					oNewRoot.m_pBaseInner->nSlotUse = 2;
					
					oInner.m_pBaseInner->nFather = new_root;
					oNewInner.m_pBaseInner->nFather = new_root;					
					oEnd.m_pBaseLeaf->nFather = new_root;
					
					//oNewInner.m_pBaseLeaf->m_nDirty = 1;
					//oInner.m_pBaseInner->m_nDirty = 1;
					oEnd.m_pBaseLeaf->m_nDirty = 1;
				}
				
				if( pos >= NUM )
				{
					oInner = oNewInner;
					pos -= NUM;
				}
			}
			
			oFather = oInner;
			father_slot = pos;
			oInner.SetThis( oInner.m_pBaseInner->pSlotChild[pos] );
		}
		
		CBTreeLeafNode oOldLeaf(this,oInner.m_nThis);
		TKey* const oldk= oOldLeaf.m_pBaseLeaf->pSlotKey;
		TData* const oldd = oOldLeaf.m_pBaseLeaf->pSlotData; 
		int32& olds = oOldLeaf.m_pBaseLeaf->nSlotUse;
		
		//duplicate&& not permit duplication
		const int32 INSERT_SLOT = FindUpper(oldk,olds,key);
		if( m_pObjectInfo->m_bUnique && INSERT_SLOT > 0 && KeyEqual(key,oldk[INSERT_SLOT-1]) )
			return End();
		
		m_pObjectInfo->m_nItemCount++;
		if( !m_bMemory )
			vflush(m_pObjectInfo);
		
		if( !oOldLeaf.IsFull() )
		{
			for( int32 ii = olds - 1; ii >= INSERT_SLOT; ii-- )
			{
				oldk[ii+1] = oldk[ii];
				oldd[ii+1] = oldd[ii];
			}
			oldk[INSERT_SLOT] = key;
			oldd[INSERT_SLOT] = data;
			olds++;
			oOldLeaf.m_pBaseLeaf->m_nDirty = 1;
			
			return CIterator(oOldLeaf,INSERT_SLOT,m_pObjectInfo->m_nEnd);
		}
		
		uint64 new_leaf = GetNewLeaf();
		CBTreeLeafNode oNewLeaf(this,new_leaf,true);
		oNewLeaf.Initialize( oOldLeaf.m_pBaseLeaf->nFather );
		TKey* const newk= oNewLeaf.m_pBaseLeaf->pSlotKey;
		TData* const newd = oNewLeaf.m_pBaseLeaf->pSlotData; 
		int32& news = oNewLeaf.m_pBaseLeaf->nSlotUse;
		
		AddIntoList(oNewLeaf,oOldLeaf);
		
	       const int32 HALF = olds>>1;
		   CIterator oIt(this);
		   if( INSERT_SLOT  <= HALF )
		   {
			   int32 jj = HALF;
			   for( ; jj < olds; jj++)
			   {
				   newk[jj - HALF] = oldk[jj];
				   newd[jj - HALF] = oldd[jj];				
			   }
	              news = olds - HALF;
				  for( jj = HALF -1; jj >= INSERT_SLOT; jj-- )
				  {
					  oldk[jj+1] = oldk[jj];
					  oldd[jj+1] = oldd[jj];
				  }
				  oldk[INSERT_SLOT] = key;
				  oldd[INSERT_SLOT] = data;
				  olds = HALF + 1;
				  
				  oIt.SetThis(oOldLeaf.m_nThis, INSERT_SLOT, m_pObjectInfo->m_nEnd);
		   }
		   else
		   {
			   int32 ii = HALF;
			   for(;ii<INSERT_SLOT;ii++)
			   {
				   newk[ii-HALF] = oldk[ii];
				   newd[ii-HALF] = oldd[ii];
			   }	
			   newk[ii-HALF] = key;
			   newd[ii-HALF] = data;		
			   
			   oIt.SetThis(oNewLeaf.m_nThis, ii-HALF, m_pObjectInfo->m_nEnd);
			   for(; ii < olds; ii++)
			   {
				   newk[ii-HALF+1] = oldk[ii];
				   newd[ii-HALF+1] = oldd[ii];                
			   }			
			   news = olds - HALF + 1;
			   olds = HALF;
		   }
		   
		   //add new leaf into nFather node
		   if( root == oOldLeaf.m_nThis )
		   {
			   uint64 new_root = GetNewInner();
			   CBTreeInnerNode oNewRoot(this,new_root,true);
			   oNewRoot.Initialize( 1, 0 );
			   TKey* const root_k = oNewRoot.m_pBaseInner->pSlotKey;
			   uint64* const root_c = oNewRoot.m_pBaseInner->pSlotChild;
			   
			   root_k[0] = oldk[0];
			   root_c[0] = oOldLeaf.m_nThis;
			   
			   root_k[1] = newk[0];
			   root_c[1] = oNewLeaf.m_nThis;
			   
			   oNewRoot.m_pBaseInner->nSlotUse = 2;
			   
			   oOldLeaf.m_pBaseLeaf->nFather = new_root;
			   oNewLeaf.m_pBaseLeaf->nFather = new_root;
			   oEnd.m_pBaseLeaf->nFather = new_root;
			   
			   //oNewRoot.m_pBaseLeaf->m_nDirty = 1;
			   //oOldLeaf.m_pBaseLeaf->m_nDirty = 1;
			   oEnd.m_pBaseLeaf->m_nDirty = 1;
			   
			   return oIt;
		   }
		   
		   TKey* const father_k = oFather.m_pBaseInner->pSlotKey;
		   uint64* const father_c = oFather.m_pBaseInner->pSlotChild;
		   int32& father_s = oFather.m_pBaseInner->nSlotUse;		
		   for(int32 kk = father_s - 1; kk > father_slot; kk--)
		   {
			   father_k[kk+1] = father_k[kk];
			   father_c[kk+1] = father_c[kk];			
		   }
		   father_k[father_slot+1] = newk[0];
		   father_c[father_slot+1] = oNewLeaf.m_nThis;
		   father_s++;
		   oFather.m_pBaseInner->m_nDirty = 1;
		   
		   return oIt;
	}	
	
	///return: the number of elements that have been removed
	int32 Erase(TKey& key)
	{
		int32 nCount = 0;
		
		while( EraseOne(key) )
		{
			++nCount;
			if ( m_pObjectInfo->m_bUnique ) 
				break;
		}
		
		return nCount;
	}
	
	CIterator& Erase(CIterator& it)
	{
		CIterator end_it = End();
		if( it == end_it )
			return it;
		
		CBTreeLeafNode oCurrLeaf = it.GetNode();
		const int32 POS = it.GetSlot();
		if( !oCurrLeaf.m_pBaseLeaf || POS < 0 || POS >= oCurrLeaf.m_pBaseLeaf->nSlotUse)
		{
			it = end_it;
			return it;
		}
		
		int32& curr_s = oCurrLeaf.m_pBaseLeaf->nSlotUse;
		TKey* const curr_k= oCurrLeaf.m_pBaseLeaf->pSlotKey;
		TData* const curr_d = oCurrLeaf.m_pBaseLeaf->pSlotData; 
		
		//erase it
		TKey key = curr_k[POS];
		
		int32 ii = POS;
		for( ; ii < curr_s - 1; ii++)
		{
			curr_k[ii] = curr_k[ii+1];
			curr_d[ii] = curr_d[ii+1];			
		}
		curr_s--;
		m_pObjectInfo->m_nItemCount--;
		if( !m_bMemory )
			vflush(m_pObjectInfo);
		
		CBTreeLeafNode oEndNode(this,m_pObjectInfo->m_nEnd);
		const uint64 root = oEndNode.m_pBaseLeaf->nFather;
		
		if( curr_s == 0 )	/*become null tree*/
		{
			oEndNode.m_pBaseLeaf->nNextLeaf = oEndNode.m_nThis;
			oEndNode.m_pBaseLeaf->nPrevLeaf = oEndNode.m_nThis;
			
			oEndNode.m_pBaseLeaf->nFather = NULL;
			oEndNode.m_pBaseLeaf->m_nDirty = 1;
			
			FreeLeaf(oCurrLeaf.m_nThis);
			
			it = end_it;
			return it;			
		}
		else if( root == oCurrLeaf.m_nThis || ( !oCurrLeaf.IsUnderFlow() && (POS != 0 || KeyEqual(curr_k[0], key) ) ) )
		{
			if( POS == curr_s )
				it.SetThis(oCurrLeaf.m_pBaseLeaf->nNextLeaf,0,m_pObjectInfo->m_nEnd);		
			return it;
		}
		else if( !oCurrLeaf.IsUnderFlow() )
		{
			UpdateIndex(oCurrLeaf.m_pBaseLeaf->nFather,oCurrLeaf.m_nThis,&curr_k[0]);
			
			if( POS == curr_s )
				it.SetThis(oCurrLeaf.m_pBaseLeaf->nNextLeaf,0,m_pObjectInfo->m_nEnd);					
			return it;
		}
		
		//process leaf node
		CBTreeInnerNode oInner(this,oCurrLeaf.m_pBaseLeaf->nFather);
		TKey* fa_k= oInner.m_pBaseInner->pSlotKey;
		uint64* fa_c = oInner.m_pBaseInner->pSlotChild;
		int32& fa_s = oInner.m_pBaseInner->nSlotUse;
		
		for( ii = 0; ii < fa_s && fa_c[ii] !=oCurrLeaf.m_nThis; ii++ ) ;
		int32 slot = ii;
		
		fa_k[slot] = curr_k[0];
		
		uint64 leftleaf = 0;
		uint64 rightleaf = 0;
		if( slot  > 0 )
			leftleaf = oInner.m_pBaseInner->pSlotChild[slot  - 1];
		if( slot  < oInner.m_pBaseInner->nSlotUse - 1)
			rightleaf = oInner.m_pBaseInner->pSlotChild[slot  + 1];
		
		do
		{
			if( leftleaf )
			{
				CBTreeLeafNode oLeftLeaf(this,leftleaf);
				if( !oLeftLeaf.IsFew() )
				{
					ShiftLeftLeaf(&oLeftLeaf,&oCurrLeaf,&oInner,slot, &it );
					break;
				}
			}
			else
			{
				CBTreeLeafNode oRightLeaf(this,rightleaf);
				if( !oRightLeaf.IsFew() )
				{
					ShiftRightLeaf(&oRightLeaf,&oCurrLeaf,&oInner,slot); 
					break;
				}		
			}
			
			if( leftleaf )
			{
				CBTreeLeafNode oLeftLeaf(this,leftleaf);
				MergeLeaves(&oLeftLeaf,&oCurrLeaf,&oInner,slot -1, &it);
			}
			else
			{
				CBTreeLeafNode oRightLeaf(this,rightleaf);
				MergeLeaves(&oCurrLeaf,&oRightLeaf,&oInner,slot);
			}
		}while(0);
		
		CBTreeInnerNode oRootNode(this,root);
		const int32 ROOT_LEVEL = oRootNode.m_pBaseInner->nLevel;
		
		while( oInner.m_nThis != root )
		{
			if( !oInner.IsUnderFlow() )
			{
				//update index
				while( oInner.m_nThis != root && slot == 0 && POS == 0 )
				{
					TKey& new_index = oInner.m_pBaseInner->pSlotKey[0];
					
					CBTreeInnerNode oParentInner(this,oInner.m_pBaseInner->nFather);
					TKey* pa_k = oParentInner.m_pBaseInner->pSlotKey;
					uint64* pa_c =  oParentInner.m_pBaseInner->pSlotChild;
					int32& pa_s = oParentInner.m_pBaseInner->nSlotUse;
					
					int32 kk = 0;
					for(; kk < pa_s && pa_c[kk] != oInner.m_nThis; kk++);
					slot = kk;
					
					oInner.SetThis( oInner.m_pBaseInner->nFather );
					
					oInner.m_pBaseInner->pSlotKey[slot] = new_index;
					oInner.m_pBaseInner->m_nDirty = 1;
				}				
				return it;
			}
			
			CBTreeInnerNode oParentInner(this,oInner.m_pBaseInner->nFather);
			TKey* pa_k = oParentInner.m_pBaseInner->pSlotKey;
			uint64* pa_c =  oParentInner.m_pBaseInner->pSlotChild;
			int32& pa_s = oParentInner.m_pBaseInner->nSlotUse;
			
			int32 kk = 0;
			for(; kk < pa_s && pa_c[kk] != oInner.m_nThis; kk++);
			slot = kk;
			
			uint64 leftinner = 0;
			uint64 rightinner = 0;
			if( slot > 0 )
				leftinner = oParentInner.m_pBaseInner->pSlotChild[slot-1];
			if( slot < oParentInner.m_pBaseInner->nSlotUse - 1)
				rightinner = oParentInner.m_pBaseInner->pSlotChild[slot+1];			
			
			do
			{
				if(leftinner)
				{
					CBTreeInnerNode oLeftInner(this,leftinner);
					if( !oLeftInner.IsFew() )
					{
						ShiftLeftInner(&oLeftInner,&oInner,&oParentInner,slot);
						break;
					}
				}
				else
				{
					CBTreeInnerNode oRightInner(this,rightinner);
					if( !oRightInner.IsFew() )
					{
						ShiftRightInner(&oInner,&oRightInner,&oParentInner,slot);
						break;
					}					
				}
				
				if(leftinner)
				{
					CBTreeInnerNode oLeftInner(this,leftinner);
					MergeInner(&oLeftInner,&oInner,&oParentInner,slot-1,ROOT_LEVEL);
				}
				else
				{
					CBTreeInnerNode oRightInner(this,rightinner);
					MergeInner(&oInner,&oRightInner,&oParentInner,slot,ROOT_LEVEL);	
				}
			}while(0);
			
			oInner = oParentInner;
		}
		
		if(oInner.m_pBaseInner->nSlotUse == 1)
		{
			CBTreeInnerNode oNewRoot(this,oInner.m_pBaseInner->pSlotChild[0]);
			oNewRoot.m_pBaseInner->nFather = 0;
			oNewRoot.m_pBaseInner->m_nDirty = 1;
			
			oEndNode.m_pBaseLeaf->nFather = oNewRoot.m_nThis;
			oEndNode.m_pBaseLeaf->m_nDirty = 1;
			
			FreeInner(oInner.m_nThis,ROOT_LEVEL);
		}
		
		return it;
	}

private:
	void Clear(uint64 nThis)
	{
		CBTreeInnerNode oInner(this,nThis);
		while( oInner.IsInnerNode() )
		{
			for(int32 i=0; i<oInner.nSlotUse; ++i)
			{
				if(oInner.pSlotChild[i])
					Clear(oInner.pSlotChild[i]);
			}
			
		}
	}
};

FOCP_END();

#endif

