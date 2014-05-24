
#include "../../01.Base/inc/AfcBase.hpp"

#ifndef _ADT_DEFINE_HPP_
#define _ADT_DEFINE_HPP_

#ifdef ADT_EXPORTS
#define ADT_API FOCP_EXPORT
#else
#define ADT_API FOCP_IMPORT
#endif

FOCP_BEGIN();

#define FOCP_FORBID_COPY(CName) \
private: \
	CName(const CName& oSrc); \
	CName& operator=(const CName& oSrc)


struct CStackAttr
{
	bool bCallDown;
	bool bParaUp;
	bool bLocalDown;
};

ADT_API void GetStackAttr(CStackAttr& oStackAttr);
ADT_API bool IsRecursive(void* pData);
ADT_API void UnRecursive(void* pData);
ADT_API void SystemLock();
ADT_API void SystemUnLock();
ADT_API void Abort();
ADT_API uint32 GetPid();
ADT_API bool IsSmallEndian();

ADT_API int32 Random();
ADT_API void RandomSeed(uint32 nSeed);

struct CSystemLock
{
	inline CSystemLock()
	{
		SystemLock();
	}
	inline ~CSystemLock()
	{
		SystemUnLock();
	}
};

template<typename TData> struct CSingleInstance
{
	inline static TData* GetInstance()
	{
		static TData* pData = NULL;
		if(pData == NULL)
		{
			SystemLock();
			if(pData == NULL)
			{
				static uint32 g_oInstance2;
				if(IsRecursive(&g_oInstance2))//如果发现递归现象，应该改用CSingleInstanceEx模式，并把构造函数分解到InitializeInstance中，以解除递归。
				{
					SystemUnLock();
					return NULL;
				}
				static TData g_oInstance;
				pData = &g_oInstance;
				UnRecursive(&g_oInstance2);
			}
			SystemUnLock();
		}
		return pData;
	}
};

template<typename TData> struct CSingleInstanceEx
{
	inline static TData* GetInstance()
	{
		static TData* pData = NULL;

		if(pData == NULL)
		{
			SystemLock();
			if(pData == NULL)
			{
				static TData g_oInstance;
				static bool g_bInstanceInitial(false);
				if(g_bInstanceInitial == false)
				{
					g_bInstanceInitial = true;
					g_oInstance.InitializeInstance();
				}
				pData = &g_oInstance;
			}
			SystemUnLock();
		}
		return pData;
	}
};


#define DefineStaticInstance(TData, oName) struct CStaticInstance_##TData##oName\
{\
	inline static TData* GetInstance()\
	{\
		static TData* pData = NULL;\
		if(pData == NULL)\
		{\
			SystemLock();\
			if(pData == NULL)\
			{\
				static uint32 g_oInstance2; \
				if(IsRecursive(&g_oInstance2)) \
				{\
					SystemUnLock();\
					return NULL;\
				}\
				static TData g_oInstance;\
				pData = &g_oInstance;\
				UnRecursive(&g_oInstance2);\
			}\
			SystemUnLock();\
		}\
		return pData;\
	}\
}

#define DefineStaticInstanceEx(TData, oName)  struct CStaticInstanceEx_##TData##oName\
{\
	inline static TData* GetInstance()\
	{\
		static TData* pData = NULL;\
		if(pData == NULL)\
		{\
			SystemLock();\
			if(pData == NULL)\
			{\
				static TData g_oInstance;\
				static bool g_bInstanceInitial(false);\
				if(g_bInstanceInitial == false)\
				{\
					g_bInstanceInitial = true;\
					g_oInstance.InitializeInstance();\
				}\
				pData = &g_oInstance;\
			}\
			SystemUnLock();\
		}\
		return pData;\
	}\
}

#define GetStaticInstance(TData, oName) (*CStaticInstance_##TData##oName::GetInstance())
#define GetStaticInstanceEx(TData, oName) (*CStaticInstanceEx_##TData##oName::GetInstance())

struct CMd5Context
{
	uint32 state[4];        /* state (ABCD) */
	uint32 count[2];        /* number of bits, modulo 2^64 (lsb first) */
	uchar buffer[64];       /* input buffer */
};

struct CIdeaContext
{
	uint16 key_schedule[52];
};

struct CShaContext
{
	uint32 h[5], count[2];
	int32 index;
	uchar X[64];
};

typedef uchar CDesBlock[8];
struct CDesKey
{
	union _
	{
		CDesBlock cblock;
		uint32 deslong[2];
	} ks[16];
};

//CRC32
ADT_API void InitCrc32(uint32* pCrc32);
ADT_API void ComputeCrc32(uint32* pCrc32, const uint8 *pBuf, uint32 nCount, uint32 bCaseSentive);
ADT_API void EndCrc32(uint32* pCrc32);
ADT_API uint32 GetCrc32(const uint8 *pBuf, uint32 nCount, uint32 bCaseSentive);

//MD5
ADT_API void InitMd5(CMd5Context* pContext);
ADT_API void UpdateMd5(CMd5Context* pContext, const uint8 *sInput, uint32 nInLen);
ADT_API void EndMd5(CMd5Context* pContext, uint8 sOutput[16]);
ADT_API void GetMd5(uint8 sOutput[16], const uint8 *sInput, uint32 nInLen);

//SHA
ADT_API void InitSha(CShaContext *pContext);
ADT_API void UpdateSha(CShaContext *pContext, const uchar *buf, uint32 lenBuf);
ADT_API void EndSha(CShaContext *pContext, uchar digest[20]);
ADT_API void GetSha(uchar sOutput[20], const uchar *sInput, uint32 nInLen);

//Base64
ADT_API char* Base64Decode(const char *sBuf, uint32 nLen, uint32 *pNewLen);//返回指针需要用CMalloc::Free释放
ADT_API char* Base64Encode(const char *sBuf, uint32 nLen, uint32 *pNewLen);//返回指针需要用CMalloc::Free释放

//IDEA
ADT_API void SetIdeaKey(CIdeaContext* pContext, const uchar pKey[16]);
ADT_API void MakeIdea(CIdeaContext* pContext, uchar iv[8], uchar* dest, const uchar* src, uint32 len, bool bEncode);

//DES
ADT_API int32 SetDesKey(CDesKey* pDstKey, const uchar pSrcKey[8], bool bCheck=false);
ADT_API void DesCbc(const uchar* pIn, uchar* pOut, int32 nLen, CDesKey* pKey, uchar iv[8], bool bEncode);
ADT_API void DesEde3Cbc(const uchar* pIn, uchar* pOut, int32 nLen, CDesKey pKey[3], uchar iv[8], bool bEncode);

FOCP_END();

#define FocpAbort(sLogInfo) do{FocpError(sLogInfo);FOCP_NAME::Abort();}while(0)
#define FocpAbortV(sLogInfo, argptr) FocpAbort((sLogInfo, argptr))
#define FocpAbortS(sLogInfo) FocpAbort(("%s", sLogInfo))

#endif
