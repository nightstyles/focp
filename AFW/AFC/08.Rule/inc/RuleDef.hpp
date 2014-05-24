
#ifndef _RULE_DEF_HPP_
#define _RULE_DEF_HPP_

#include "../../04.AFS/AFS.hpp"

#ifdef RULE_EXPORTS
#define RULE_API FOCP_EXPORT
#else
#define RULE_API FOCP_IMPORT
#endif

FOCP_BEGIN();

class CRuleSystem;
class CRuleModule;
class CRuleProc;
class CRuleFunc;

//规则类型编码
enum
{
	ARF_RULE_ATOM = 0,
	ARF_RULE_CALL,
	ARF_RULE_SEQUENCE,
	ARF_RULE_OR,
	ARF_RULE_LOOP,
	ARF_RULE_BREAK
};

//规则语句类型
enum
{
	ARF_SENTENCE_INVALID,
	ARF_SENTENCE_RETURN,
	ARF_SENTENCE_EXPRESS,
	ARF_SENTENCE_COND,
	ARF_SENTENCE_WHILE,
	ARF_SENTENCE_FOR,
	ARF_SENTENCE_SWITCH,
	ARF_SENTENCE_BREAK,
	ARF_SENTENCE_CONTINUE,
	ARF_SENTENCE_COMPLEX
};

//规则数据类型编码
enum
{
	ARF_BOOL, ARF_CHAR,
	ARF_INT8, ARF_UINT8,
	ARF_INT16, ARF_UINT16,
	ARF_INT32, ARF_UINT32,
	ARF_INT64, ARF_UINT64,
	ARF_FLOAT, ARF_DOUBLE,
	ARF_STRING, ARF_STRUCT, ARF_VECTOR, ARF_OBJECT,
};

//规则操作符定义
enum
{
	//unary operator
	ARF_INC_OP,		// a++ 		n
	ARF_DEC_OP,		// a--		n
	ARF_SINC_OP,	// ++a		n&
	ARF_SDEC_OP,	// --a		n&
	ARF_NON_OP,		// ~a		i
	ARF_NEG_OP,		// -a		n
	ARF_LNON_OP,	// !a		n
	ARF_POST_OP,	// +a		n
	//binary operator
	ARF_ADD_OP,		// n = n1 + n2
	ARF_SUB_OP,		// n = n1 - n2
	ARF_MUL_OP,		// n = n1 * n2
	ARF_DIV_OP,		// n = n1 / n2
	ARF_MOD_OP,		// n = n1 % n2
	ARF_AND_OP,		// i = i1 & i2
	ARF_OR_OP, 		// i = i1 | i2
	ARF_XOR_OP,		// i = i1 ^ i2
	ARF_LAND_OP,	// b = (b1 or n1) && (b2 or n2)
	ARF_LOR_OP, 	// b = (b1 or n1) || (b2 or n2)
	ARF_LT_OP, 		// b = (n1 or s1) < (n2 or s2)
	ARF_GT_OP, 		// b = (n1 or s1) > (n2 or s2)
	ARF_LE_OP, 		// b = (n1 or s1) <= (n2 or s2)
	ARF_GE_OP, 		// b = (n1 or s1) >= (n2 or s2)
	ARF_EQ_OP, 		// b = (n1 or s1 or b1) == (n2 or s2 or b2)
	ARF_NE_OP, 		// b = (n1 or s1 or b1) != (n2 or s2 or b2)
	ARF_LSH_OP,		// i = i1 << i2
	ARF_RSH_OP,		// i = i1 >> i2
	ARF_ASIGN_OP, 	// a& = a1
	ARF_ADDEQ_OP,	// n& += n1
	ARF_SUBEQ_OP, 	// n& -= n1
	ARF_MULEQ_OP, 	// n& *= n1
	ARF_DIVEQ_OP, 	// n& /= n1
	ARF_MODEQ_OP,	// n& %= n1
	ARF_LSHEQ_OP, 	// i& <<= i1
	ARF_RSHEQ_OP, 	// i& >>= i1
	ARF_ANDEQ_OP, 	// i& &= i1
	ARF_OREQ_OP,  	// i& |= i1
	ARF_XOREQ_OP, 	// i& ^= i1
	ARF_COMMA_OP,	// a1,b2
};

struct CRuleStack
{
	CRuleSystem* pSystem;
	CRuleModule* pModule;
	CRuleProc* pProc;
	CRuleFunc* pFunc;
	void* pData;			//管局数据区
	void* pArgv;			//参数变量区
	void* pVarv;			//局部变量区
	uint32 nArgOff;
};

typedef void (*FRuleHost)(CRuleStack& oArgv);//bRet传入时已初始化为true

struct CRuleArgv
{
	void* pArgv;
};

RULE_API void InitRuleArgv(CRuleArgv& oArgv, CRuleStack &oStack);
RULE_API bool& GetRuleArgBool(CRuleArgv& oArgv);
RULE_API char& GetRuleArgChar(CRuleArgv& oArgv);
RULE_API int16& GetRuleArgShort(CRuleArgv& oArgv);
RULE_API uint16& GetRuleArgUShort(CRuleArgv& oArgv);
RULE_API int32& GetRuleArgInt(CRuleArgv& oArgv);
RULE_API uint32& GetRuleArgUInt(CRuleArgv& oArgv);
RULE_API int64& GetRuleArgLong(CRuleArgv& oArgv);
RULE_API uint64& GetRuleArgULong(CRuleArgv& oArgv);
RULE_API float& GetRuleArgFloat(CRuleArgv& oArgv);
RULE_API double& GetRuleArgDouble(CRuleArgv& oArgv);
RULE_API CString& GetRuleArgString(CRuleArgv& oArgv);

template<typename TObject> TObject& GetRuleArgObject(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(TObject*);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	TObject* &nRet = *(TObject**)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	if(nRet == NULL)
		nRet = new TObject;
	return *nRet;
}

struct CRuleStr
{
	uint32 len;
	char* s;
};

union CRuleVal
{
	int8 i8;
	int16 i16;
	int32 i32;
	int64 i64;
	float f32;
	double f64;
	bool b;
	CRuleStr s;
};

//----------------------------------------------------
// CRuleFileInfo
//----------------------------------------------------
class RULE_API CRuleFileInfo
{
public:
	const char* m_sFileName;
	uint32 m_nLine;
	uint32 m_nCol;

	CRuleFileInfo();
	virtual ~CRuleFileInfo();

	void SetFile(const char* sFileName, uint32 nLine, uint32 nCol=0);
	void SetFile(const CRuleFileInfo& oSrc);
};

// ---------------------------------------------------
// CRuleChecker
// ---------------------------------------------------
class RULE_API CRuleChecker
{
public:
	CRuleChecker();
	virtual ~CRuleChecker();

	virtual void OnError(const CRuleFileInfo& oFileInfo, const char* sFormat, ...);
};

FOCP_END();

#endif
