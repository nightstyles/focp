
#include "../../04.AFS/AFS.hpp"

#ifndef _Afc_Functor_Hpp_
#define _Afc_Functor_Hpp_

#ifdef FUN_EXPORTS
#define FUN_API FOCP_EXPORT
#else
#define FUN_API FOCP_IMPORT
#endif

FOCP_BEGIN();

/**********************************************************************************
* 操作定义部分
**********************************************************************************/

FOCP_DETAIL_BEGIN();

struct CBaseFocpValue {};

template<typename TValue> struct CProtoType
{
private:
	enum
	{
		IsArray = CIsArray<TValue>::value,
		IsConst = CIsConst<TValue>::value,
		IsVolatile = CIsVolatile<TValue>::value,
		IsRefer = CIsRefer<TValue>::value,
		IsConst2 = CIsConst<typename CRemoveRefer<TValue>::type>::value,
		IsVolatile2 = CIsVolatile<typename CRemoveRefer<TValue>::type>::value,
	};

public:
	enum
	{
		value= IsArray?0:(IsRefer?(17+(IsVolatile2?2:0)+(IsConst2?4:0)):(1+(IsVolatile?2:0)+(IsConst?4:0)))
	};

private:
	template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperA
	{
		template<typename T2> struct CHelperB
		{
			typedef void type;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<1 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T2> struct CHelperB
		{
			typedef T2 type;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<3 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T2> struct CHelperB
		{
			typedef typename CRemoveVolatile<T2>::type type;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<5 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T2> struct CHelperB
		{
			typedef typename CRemoveConst<T2>::type type;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<7 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T2> struct CHelperB
		{
			typedef typename CRemoveConst<typename CRemoveVolatile<T2>::type>::type type;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<17 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T2> struct CHelperB
		{
			typedef typename CRemoveRefer<T2>::type type;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<19 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T2> struct CHelperB
		{
			typedef typename CRemoveVolatile<typename CRemoveRefer<T2>::type>::type type;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<21 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T2> struct CHelperB
		{
			typedef typename CRemoveConst<typename CRemoveRefer<T2>::type>::type type;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<23 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T2> struct CHelperB
		{
			typedef typename CRemoveConst<typename CRemoveVolatile<typename CRemoveRefer<T2>::type>::type>::type type;
		};
	};

public:
	typedef typename CHelperA<value>::template CHelperB<TValue>::type type;
};

FOCP_DETAIL_END();

template<typename TValue> struct CFocpValue:
	public FOCP_DETAIL_NAME::CBaseFocpValue
{
	typedef TValue TBaseType;
};

/********************************************************
 一元表达式结果类型推导
 ********************************************************/
enum
{
	FOCP_INC_OP,		// a++ 		n
	FOCP_DEC_OP,		// a--		n
	FOCP_SINC_OP,		// ++a		n&
	FOCP_SDEC_OP,		// --a		n&
	FOCP_NON_OP,		// ~a			i
	FOCP_NEG_OP,		// -a			n
	FOCP_LNON_OP,		// !a			n,p
	FOCP_POST_OP,		// +a			n
	FOCP_ADDR_OP,		// &a			a
	FOCP_INST_OP		// *a			p
};

template<typename A, uint32 nOp> struct CUnaryResult
{
	typedef void TResult;
};

#define DefineFocpUnaryResult(R,nOp,A) template<> struct CUnaryResult<A,nOp> { typedef R TResult; }

//template<typename A> struct CConvertResult
//{
//	typedef void TResult;
//};

FOCP_DETAIL_BEGIN();
template<typename A, uint32 nOp> struct CUnaryCustomConclude
{
	//CResult，根据匹配要求，返回匹配结果
	template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CResult;
	template<FOCP_FAKE_TYPE(S)> struct CResult<0 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelper
		{
			typedef void TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<1 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelper
		{
			typedef typename CUnaryResult<T,nOp>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<3 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelper
		{
			typedef typename CUnaryResult<T volatile,nOp>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<5 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelper
		{
			typedef typename CUnaryResult<T const,nOp>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<7 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelper
		{
			typedef typename CUnaryResult<T const volatile,nOp>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<17 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelper
		{
			typedef typename CUnaryResult<T&,nOp>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<19 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelper
		{
			typedef typename CUnaryResult<T volatile&,nOp>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<21 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelper
		{
			typedef typename CUnaryResult<T const&,nOp>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<23 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelper
		{
			typedef typename CUnaryResult<T const volatile&,nOp>::TResult TResult;
		};
	};

	//CSelectResult，根据匹配要求列表，返回第一个匹配的结果
	template<typename T, uint32 a1=0, uint32 a2=0, uint32 a3=0, uint32 a4=0, uint32 a5=0, uint32 a6=0, uint32 a7=0, uint32 a8=0> struct CSelectResult
	{
		//CArg，提取指定的匹配要求
		template<uint32 nIdx FOCP_FAKE_DEFAULT_TYPE(S)> struct CArg;
		template<FOCP_FAKE_TYPE(S)> struct CArg<1 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a1};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<2 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a2};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<3 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a3};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<4 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a4};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<5 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a5};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<6 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a6};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<7 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a7};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<8 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a8};
		};

		//CHelper，循环扫描匹配要求，进行匹配测试
		template<uint32 nIdx FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			typedef typename CResult<CArg<nIdx>::value>::template CHelper<T>::TResult TTmpResult;
			template<bool FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperB
			{
				typedef TTmpResult TResult;
			};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperB<true FOCP_FAKE_TYPE_ARG(SS)>
			{
				typedef typename CHelper<nIdx+1>::TResult TResult;
			};
			typedef typename CHelperB<CIsSameType<TTmpResult, void>::value>::TResult TResult;
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<9 FOCP_FAKE_TYPE_ARG(S)>//定义无效扫描
		{
			typedef void TResult;
		};

		typedef typename CHelper<1>::TResult TResult;
	};

	//CResultTest，匹配性测试，根据是否为常量，而用相应扫描要求列表，进行匹配扫描
	template<typename T, bool bConst> struct CResultTest
	{
		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			typedef typename CSelectResult<T,21,23,1,3,5,7,0,0>::TResult TResult;
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
		{
			typedef typename CSelectResult<T,17,19,21,23,1,3,5,7>::TResult TResult;
		};
		typedef typename CHelper<bConst>::TResult TResult;
	};

	//CInheritTest继承性测试
	template<typename T, bool bConst> struct CInheritTest
	{
		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			template<typename T2> struct CHelperA
			{
				typedef void TResult;
			};
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>
		{
			template<typename T2> struct CHelperA
			{
				typedef typename T2::TBaseType TBaseType;
				typedef typename CResultTest<TBaseType,bConst>::TResult TBaseResult;
				template<bool FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperB
				{
					typedef TBaseResult TResult;
				};
				template<FOCP_FAKE_TYPE(SS)> struct CHelperB<true FOCP_FAKE_TYPE_ARG(SS)>
				{
					typedef typename CHelperA<TBaseType>::TResult TResult;
				};
				enum {Cond = CIsSameType<TBaseResult,void>::value && !CIsSameType<T2, TBaseType>::value};
				typedef typename CHelperB<Cond>::TResult TResult;
			};
		};
		typedef typename CHelper<CIsInherited<T, CBaseFocpValue>::value>::template CHelperA<T>::TResult TResult;
	};

	//CColligateTest，综合测试，
	template<bool bDirect FOCP_FAKE_DEFAULT_TYPE(S)> struct CColligateTest
	{//如果没有注册CUnaryResult<A,nOp>
		enum{ bConst = !CIsWritable<A>::value };
		typedef typename CProtoType<A>::type TProtoType;
		typedef typename CResultTest<TProtoType,bConst>::TResult TProtoResult;
		template<bool bProtoInvalid FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelper
		{
			typedef TProtoResult TResult;
		};
		template<FOCP_FAKE_TYPE(SS)> struct CHelper<true FOCP_FAKE_TYPE_ARG(SS)>
		{
			typedef typename CInheritTest<TProtoType,bConst>::TResult TResult;
		};
		typedef typename CHelper<CIsSameType<TProtoResult,void>::value>::TResult TResult;
	};

	template<FOCP_FAKE_TYPE(S)> struct CColligateTest<false FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef typename CUnaryResult<A,nOp>::TResult TResult;
	};

	typedef typename CColligateTest<CIsSameType<typename CUnaryResult<A,nOp>::TResult,void>::value>::TResult TResult;
};

template<typename A, uint32 nOp> struct CUnaryDefaultConclude
{
	//系统默认推导方式1，支持++,--,~,-,+操作
	template<typename T, uint32 nTypeLimit, bool bWritableLimit, bool bRemoveRefer> struct CHelperA
	{
		template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CTypeCond
		{
			enum {cond=false};
		};
		template<FOCP_FAKE_TYPE(S)> struct CTypeCond<0 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {cond=CIsIntegerType<T>::value};
		};
		template<FOCP_FAKE_TYPE(S)> struct CTypeCond<1 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {cond=CIsNumberType<T>::value};
		};
		template<FOCP_FAKE_TYPE(S)> struct CTypeCond<2 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {cond=CIsPointer<T>::value};
		};
		template<FOCP_FAKE_TYPE(S)> struct CTypeCond<3 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {cond=CIsPointer<T>::value || CIsNumberType<T>::value};
		};
		template<FOCP_FAKE_TYPE(S)> struct CTypeCond<4 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {cond=true};
		};

		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CWritableCond
		{
			enum {cond=true};
		};
		template<FOCP_FAKE_TYPE(S)> struct CWritableCond<true FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {cond=CIsWritable<T>::value};
		};

		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CResultType
		{
			typedef T TResult;
		};
		template<FOCP_FAKE_TYPE(S)> struct CResultType<true FOCP_FAKE_TYPE_ARG(S)>
		{
			typedef typename CRemoveRefer<T>::type TResult;
		};
		typedef typename CResultType<bRemoveRefer>::TResult TTmpResult;

		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			typedef TTmpResult TResult;
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
		{
			typedef void TResult;
		};
		enum {cond = CTypeCond<nTypeLimit>::cond && CWritableCond<bWritableLimit>::cond};

		typedef typename CHelper<cond>::TResult TResult;
	};

	//系统默认推导方式2，支持!,&,*操作
	template<typename T, uint32 nTypeLimit, uint32 nConvertType> struct CHelperB
	{
		typedef typename CHelperA<T,nTypeLimit,false,true>::TResult TTmpResult;
		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			template<typename X>struct CHelperA
			{
				typedef void TResult;
			};
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
		{
			template<typename X>struct CHelperA
			{
				template<uint32 FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperX
				{
					typedef void TResult;
				};
				template<FOCP_FAKE_TYPE(SS)> struct CHelperX<0 FOCP_FAKE_TYPE_ARG(SS)>
				{
					typedef bool TResult;
				};
				template<FOCP_FAKE_TYPE(SS)> struct CHelperX<1 FOCP_FAKE_TYPE_ARG(SS)>
				{
					typedef X* TResult;
				};
				template<FOCP_FAKE_TYPE(SS)> struct CHelperX<2 FOCP_FAKE_TYPE_ARG(SS)>
				{
					typedef typename CRemovePointer<X>::type & TResult;
				};
				typedef typename CHelperX<nConvertType>::TResult TResult;
			};
		};
		typedef typename CHelper<CIsSameType<TTmpResult,void>::value>::template CHelperA<TTmpResult>::TResult TResult;
	};

	//定义系统默认推导原型
	template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
		template<typename T> struct CHelperX
		{
			typedef void TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_INC_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperA<T,1,true,true>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_DEC_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperA<T,1,true,true>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_SINC_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperA<T,1,true,false>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_SDEC_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperA<T,1,true,false>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_NON_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperA<T,0,false,true>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_NEG_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperA<T,1,false,true>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_POST_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperA<T,1,false,true>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_LNON_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperB<T,3,0>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_ADDR_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperB<T,4,1>::TResult TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_INST_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename T> struct CHelperX
		{
			typedef typename CHelperB<T,2,2>::TResult TResult;
		};
	};

	//获得自动推导结果
	typedef typename CHelper<nOp>::template CHelperX<A>::TResult TResult;
};

FOCP_DETAIL_END();

template<typename A, uint32 nOp> struct CUnaryQuery
{
private:
	typedef typename FOCP_DETAIL_NAME::CUnaryCustomConclude<A,nOp>::TResult TCustomResult;

	template<bool bCustomInvalid FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
		typedef TCustomResult TResult;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef typename FOCP_DETAIL_NAME::CUnaryDefaultConclude<A,nOp>::TResult TResult;
	};

public:
	typedef typename CHelper<CIsSameType<TCustomResult,void>::value>::TResult TResult;
};

/********************************************************
 二元表达式结果类型推导
 ********************************************************/
enum
{
	// binary											base-type			refer
	FOCP_ADD_OP,		// a + b			i,f,p+i				f
	FOCP_SUB_OP,		// a - b			i,f,p-i,p-p		f
	FOCP_MUL_OP,		// a * b			i,f						f
	FOCP_DIV_OP,		// a / b			i,f						f
	FOCP_MOD_OP,		// a % b			i							f
	FOCP_AND_OP,		// a & b			i,						f
	FOCP_OR_OP, 		// a | b			i,						f
	FOCP_XOR_OP,		// a ^ b			i,						f

	FOCP_LAND_OP,		// a && b			a							f
	FOCP_LOR_OP, 		// a || b			a							f
	FOCP_LT_OP, 		// a < b			a							f
	FOCP_GT_OP, 		// a > b			a							f
	FOCP_LE_OP, 		// a <= b			a							f
	FOCP_GE_OP, 		// a >= b			a							f
	FOCP_EQ_OP, 		// a == b			a							f
	FOCP_NE_OP, 		// a != b			a							f

	FOCP_LSH_OP,		// a << b			i							f
	FOCP_RSH_OP,		// a >> b			i 						f

	FOCP_ASIGN_OP, 	// a = b			a							t
	FOCP_ADDEQ_OP, 	// a += b			a							t
	FOCP_SUBEQ_OP, 	// a -= b			a							t
	FOCP_MULEQ_OP, 	// a *= b			a							t
	FOCP_DIVEQ_OP, 	// a /= b			a							t
	FOCP_MODEQ_OP,	// a %= b			a							t
	FOCP_LSHEQ_OP, 	// a <<= b		a							t
	FOCP_RSHEQ_OP, 	// a >>= b		a							t
	FOCP_ANDEQ_OP, 	// a &= b			a							t
	FOCP_OREQ_OP,  	// a |= b			a							t
	FOCP_XOREQ_OP, 	// a ^= b			a							t

	FOCP_COMMA_OP,	// a,b				a
	FOCP_INDEX_OP		// a[b]				p[i]
};

template<typename A, uint32 nOp> struct CBinaryResult
{
	template<uint32 nArgIdx> struct CResultPair
	{
		typedef void B;
		typedef void TResult;
	};
	enum {Argc=0};
};

template<typename A1, typename A2> struct CBinaryResultPair
{
	typedef A1 B;
	typedef A2 TResult;
};

#define FocpBinaryResultMacro0(nArgc,PairList,ItemMacro)
#define FocpBinaryResultMacro1(nArgc,PairList,ItemMacro)  ItemMacro(nArgc,PairList,1)
#define FocpBinaryResultMacro2(nArgc,PairList,ItemMacro)  FocpBinaryResultMacro1(nArgc,PairList,ItemMacro);ItemMacro(nArgc,PairList,2)
#define FocpBinaryResultMacro3(nArgc,PairList,ItemMacro)  FocpBinaryResultMacro2(nArgc,PairList,ItemMacro);ItemMacro(nArgc,PairList,3)
#define FocpBinaryResultMacro4(nArgc,PairList,ItemMacro)  FocpBinaryResultMacro3(nArgc,PairList,ItemMacro);ItemMacro(nArgc,PairList,4)
#define FocpBinaryResultMacro5(nArgc,PairList,ItemMacro)  FocpBinaryResultMacro4(nArgc,PairList,ItemMacro);ItemMacro(nArgc,PairList,5)
#define FocpBinaryResultMacro6(nArgc,PairList,ItemMacro)  FocpBinaryResultMacro5(nArgc,PairList,ItemMacro);ItemMacro(nArgc,PairList,6)
#define FocpBinaryResultMacro7(nArgc,PairList,ItemMacro)  FocpBinaryResultMacro6(nArgc,PairList,ItemMacro);ItemMacro(nArgc,PairList,7)
#define FocpBinaryResultMacro8(nArgc,PairList,ItemMacro)  FocpBinaryResultMacro7(nArgc,PairList,ItemMacro);ItemMacro(nArgc,PairList,8)
#define FocpBinaryResultMacro9(nArgc,PairList,ItemMacro)  FocpBinaryResultMacro8(nArgc,PairList,ItemMacro);ItemMacro(nArgc,PairList,9)

#define FocpBinaryResultHelp2(x) FOCP_ARGLIST2 x
#define FocpBinaryResultHelp(nArgc,PairList,nIdx) \
template<> struct CResultPair<nIdx>: public CBinaryResultPair< FocpBinaryResultHelp2(FOCP_ARGLIST##nArgc##_##nIdx PairList) > { }

#define DefineFocpBinaryResultEx(A,nOp,nArgc,PairList) \
template<> struct CBinaryResult<A,nOp> \
{ \
	template<uint32 nArgIdx> struct CResultPair \
	{ \
		typedef void B; \
		typedef void TResult; \
	}; \
	enum{Argc=nArgc}; \
	FocpBinaryResultMacro##nArgc(nArgc,PairList,FocpBinaryResultHelp); \
}

#define DefineFocpBinaryResult(R,A,nOp,B) DefineFocpBinaryResultEx(A,nOp,1,((B,R)))

FOCP_DETAIL_BEGIN();

template<typename A, uint32 nOp, uint32 nArgIdx> struct CBinaryResultPair
{
	typedef typename CBinaryResult<A,nOp>::template CResultPair<nArgIdx>::B B;
	typedef typename CBinaryResult<A,nOp>::template CResultPair<nArgIdx>::TResult TResult;
};

template<typename A, uint32 nOp, typename B> struct CQueryBinaryResult
{
	template<typename X, typename Y, uint32 nTestMode> struct CSingleTest
	{
	private:
		typedef typename CTryRemoveConst<typename CTryRemoveVolatile<typename CRemoveRefer<X>::type>::type>::type X1;
		typedef typename CTryRemoveConst<typename CTryRemoveVolatile<typename CRemoveRefer<Y>::type>::type>::type Y1;
	public:
		enum
		{
			value=(nTestMode==0?(CIsSameType<X1,Y1>::value):
				   (nTestMode==1?(CConvertible<X1,Y1>::value):
					(nTestMode==2?(CExplicitConvertible<X1,Y1>::value):
					 (CImplicitConvertible<X1,Y1>::value))))
		};
	};

	//1:如果X是常量，那么Y必须是常量，删除引用、常量、变量，进行相同性测试
	//2:如果X是常量，那么Y必须是常量，删除引用、常量、变量，进行继承性测试
	//3:如果X是常量，那么Y必须是常量，删除引用、常量、变量，进行拷贝构造测试
	//4:如果X是常量，那么Y必须是常量，删除引用、常量、变量，进行拷贝测试

	//5:如果X是变量，并且Y是变量，删除引用、常量、变量，进行相同性测试
	//6:如果X是变量，并且Y是变量，删除引用、常量、变量，进行继承性测试

	//7:如果X是变量，并且Y是常量，删除引用、常量、变量，进行相同性测试
	//8:如果X是变量，并且Y是常量，删除引用、常量、变量，进行继承性测试
	//9:如果X是变量，并且Y是常量，删除引用、常量、变量，进行拷贝构造测试
	//10:如果X是变量，并且Y是常量，删除引用、常量、变量，进行拷贝测试
	template<typename X, typename Y, uint32 nTestIndex> struct CTestCond
	{
	private:
		enum {AttrY=CProtoType<Y>::value};
		enum {bConstY = (AttrY != 17 && AttrY != 19)};
		enum {bReferX=CIsRefer<X>::value};
		enum {bReferY=CIsRefer<Y>::value};
		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			enum {value=CIsSameType<typename CRemoveRefer<X>::type, typename CRemoveRefer<Y>::type>::value};
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=CIsSameType<X, Y>::value};
		};
	public:
		enum
		{
			value=(nTestIndex==1?(bConstY && CSingleTest<X,Y,0>::value):
				   (nTestIndex==2?(bConstY && CSingleTest<X,Y,1>::value):
					(nTestIndex==3?(bConstY && CSingleTest<X,Y,2>::value):
					 (nTestIndex==4?(bConstY && CSingleTest<X,Y,3>::value):
					  (nTestIndex==5?((!bConstY) && CSingleTest<X,Y,0>::value):
					   (nTestIndex==6?((!bConstY) && CSingleTest<X,Y,1>::value):
						(nTestIndex==7?(bConstY && CSingleTest<X,Y,0>::value):
						 (nTestIndex==8?(bConstY && CSingleTest<X,Y,1>::value):
						  (nTestIndex==9?(bConstY && CSingleTest<X,Y,2>::value):
						   (nTestIndex==10?(bConstY && CSingleTest<X,Y,3>::value):
							(bReferX==bReferY && CHelper<bReferX>::value)))))))))))
		};
	};

	template<typename X, typename Y, uint32 nTestIndex> struct CLoopTester
	{
		template<uint32 nArgIdx FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelper
		{
			typedef typename CBinaryResultPair<X,nOp,nArgIdx>::B TmpB;
			template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperX
			{
				typedef TmpB TArgB;
				typedef typename CBinaryResultPair<X,nOp,nArgIdx>::TResult TResult;
			};
			template<FOCP_FAKE_TYPE(S)> struct CHelperX<false FOCP_FAKE_TYPE_ARG(S)>
			{
				typedef typename CHelper<nArgIdx-1>::TArgB TArgB;
				typedef typename CHelper<nArgIdx-1>::TResult TResult;
			};
			enum {bConvert=CTestCond<Y,TmpB,nTestIndex>::value};
			typedef typename CHelperX<bConvert>::TArgB TArgB;
			typedef typename CHelperX<bConvert>::TResult TResult;
		};
		template<FOCP_FAKE_TYPE(SS)> struct CHelper<0 FOCP_FAKE_TYPE_ARG(SS)>
		{
			typedef void TArgB;
			typedef void TResult;
		};
		typedef typename CHelper<CBinaryResult<X, nOp>::Argc>::TResult TResult;
		typedef typename CHelper<CBinaryResult<X, nOp>::Argc>::TArgB TArgB;
		enum {value=CIsSameType<TResult,void>::value};
	};

	enum
	{
		TestIndex = ((CLoopTester<A,B,0>::value)?0:
					 ((CIsWritable<B>::value)?(
						  (CLoopTester<A,B,5>::value)?5:
						  (CLoopTester<A,B,6>::value)?6:
						  (CLoopTester<A,B,7>::value)?7:
						  (CLoopTester<A,B,8>::value)?8:
						  (CLoopTester<A,B,9>::value)?9:
						  (CLoopTester<A,B,10>::value)?10:11):
					  ((CLoopTester<A,B,1>::value)?1:
					   (CLoopTester<A,B,2>::value)?2:
					   (CLoopTester<A,B,3>::value)?3:
					   (CLoopTester<A,B,4>::value)?4:11))),
		bCond = (TestIndex < 11)
	};

	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CTester
	{
		typedef void TArgB;
		typedef void TResult;
	};
	template<FOCP_FAKE_TYPE(S)> struct CTester<true FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef typename CLoopTester<A,B,TestIndex>::TResult TResult;
		typedef typename CLoopTester<A,B,TestIndex>::TArgB TArgB;
	};
	typedef typename CTester<bCond>::TResult TResult;
	typedef typename CTester<bCond>::TArgB TArgB;
};

template<typename A, uint32 nOp, typename B> struct CBinaryCustomConclude
{
	//CResult，根据匹配要求，返回匹配结果
	template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CResult;
	template<FOCP_FAKE_TYPE(S)> struct CResult<0 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelper
		{
			typedef void TArgB;
			typedef void TResult;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<1 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelper
		{
			typedef CQueryBinaryResult<X,nOp,B> XResult;
			typedef typename XResult::TResult TResult;
			typedef typename XResult::TArgB TArgB;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<3 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelper
		{
			typedef CQueryBinaryResult<X volatile,nOp,B> XResult;
			typedef typename XResult::TResult TResult;
			typedef typename XResult::TArgB TArgB;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<5 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelper
		{
			typedef CQueryBinaryResult<X const,nOp,B> XResult;
			typedef typename XResult::TResult TResult;
			typedef typename XResult::TArgB TArgB;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<7 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelper
		{
			typedef CQueryBinaryResult<X const volatile,nOp,B> XResult;
			typedef typename XResult::TResult TResult;
			typedef typename XResult::TArgB TArgB;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<17 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelper
		{
			typedef CQueryBinaryResult<X&,nOp,B> XResult;
			typedef typename XResult::TResult TResult;
			typedef typename XResult::TArgB TArgB;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<19 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelper
		{
			typedef CQueryBinaryResult<X volatile&,nOp,B> XResult;
			typedef typename XResult::TResult TResult;
			typedef typename XResult::TArgB TArgB;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<21 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelper
		{
			typedef CQueryBinaryResult<X const&,nOp,B> XResult;
			typedef typename XResult::TResult TResult;
			typedef typename XResult::TArgB TArgB;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CResult<23 FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelper
		{
			typedef CQueryBinaryResult<X const volatile&,nOp,B> XResult;
			typedef typename XResult::TResult TResult;
			typedef typename XResult::TArgB TArgB;
		};
	};

	//CSelectResult，根据匹配要求列表，返回第一个匹配的结果
	template<typename X, typename Y, uint32 a1=0, uint32 a2=0, uint32 a3=0, uint32 a4=0, uint32 a5=0, uint32 a6=0, uint32 a7=0, uint32 a8=0> struct CSelectResult
	{
		//CArg，提取指定的匹配要求
		template<uint32 nIdx FOCP_FAKE_DEFAULT_TYPE(S)> struct CArg;
		template<FOCP_FAKE_TYPE(S)> struct CArg<1 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a1};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<2 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a2};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<3 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a3};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<4 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a4};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<5 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a5};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<6 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a6};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<7 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a7};
		};
		template<FOCP_FAKE_TYPE(S)> struct CArg<8 FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=a8};
		};

		//CHelper，循环扫描匹配要求，进行匹配测试
		template<uint32 nIdx FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelper
		{
			typedef typename CResult<CArg<nIdx>::value>::template CHelper<X,Y>::TResult TTmpResult;
			template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperB
			{
				typedef TTmpResult TResult;
				typedef typename CResult<CArg<nIdx>::value>::template CHelper<X,Y>::TArgB TArgB;
			};
			template<FOCP_FAKE_TYPE(S)> struct CHelperB<true FOCP_FAKE_TYPE_ARG(S)>
			{
				typedef typename CHelper<nIdx+1>::TResult TResult;
				typedef typename CHelper<nIdx+1>::TArgB TArgB;
			};
			typedef typename CHelperB<CIsSameType<TTmpResult, void>::value>::TResult TResult;
			typedef typename CHelperB<CIsSameType<TTmpResult, void>::value>::TArgB TArgB;
		};
		template<FOCP_FAKE_TYPE(SS)> struct CHelper<9 FOCP_FAKE_TYPE_ARG(SS)>//定义无效扫描
		{
			typedef void TArgB;
			typedef void TResult;
		};

		typedef typename CHelper<1>::TResult TResult;
		typedef typename CHelper<1>::TArgB TArgB;
	};

	//CResultTest，匹配性测试，根据是否为常量，而用相应扫描要求列表，进行匹配扫描
	template<typename X, typename Y, bool bConst> struct CResultTest
	{
		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			typedef typename CSelectResult<X,Y,21,23,1,3,5,7,0,0>::TResult TResult;
			typedef typename CSelectResult<X,Y,21,23,1,3,5,7,0,0>::TArgB TArgB;
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
		{
			typedef typename CSelectResult<X,Y,17,19,21,23,1,3,5,7>::TResult TResult;
			typedef typename CSelectResult<X,Y,17,19,21,23,1,3,5,7>::TArgB TArgB;
		};
		typedef typename CHelper<bConst>::TResult TResult;
		typedef typename CHelper<bConst>::TArgB TArgB;
	};

	//CInheritTest继承性测试
	template<typename X, typename Y, bool bConst> struct CInheritTest
	{
		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			template<typename X2, typename Y2> struct CHelperA
			{
				typedef void TResult;
				typedef void TArgB;
			};
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>
		{
			template<typename X2, typename Y2> struct CHelperA
			{
				typedef typename X2::TBaseType TBaseType;
				typedef typename CResultTest<TBaseType,Y2,bConst>::TResult TBaseResult;
				template<bool FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperB
				{
					typedef TBaseResult TResult;
					typedef typename CResultTest<TBaseType,Y2,bConst>::TArgB TArgB;
				};
				template<FOCP_FAKE_TYPE(SS)> struct CHelperB<true FOCP_FAKE_TYPE_ARG(SS)>
				{
					typedef typename CHelperA<TBaseType,Y2>::TResult TResult;
					typedef typename CHelperA<TBaseType,Y2>::TArgB TArgB;
				};
				enum {Cond = CIsSameType<TBaseResult,void>::value && !CIsSameType<X2, TBaseType>::value};
				typedef typename CHelperB<Cond>::TResult TResult;
				typedef typename CHelperB<Cond>::TArgB TArgB;
			};
		};
		typedef typename CHelper<CIsInherited<X, CBaseFocpValue>::value>::template CHelperA<X,Y>::TResult TResult;
		typedef typename CHelper<CIsInherited<X, CBaseFocpValue>::value>::template CHelperA<X,Y>::TArgB TArgB;
	};

	//CColligateTest，综合测试，
	template<bool bDirect FOCP_FAKE_DEFAULT_TYPE(SS)> struct CColligateTest
	{//如果没有注册CUnaryResult<A,nOp>
		enum{ bConst = !CIsWritable<A>::value };
		typedef typename CProtoType<A>::type TProtoType;
		typedef typename CResultTest<TProtoType,B,bConst>::TResult TProtoResult;
		template<bool bProtoInvalid FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			typedef TProtoResult TResult;
			typedef typename CResultTest<TProtoType,B,bConst>::TArgB TArgB;
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>
		{
			typedef typename CInheritTest<TProtoType,B,bConst>::TResult TResult;
			typedef typename CInheritTest<TProtoType,B,bConst>::TArgB TArgB;
		};
		typedef typename CHelper<CIsSameType<TProtoResult,void>::value>::TResult TResult;
		typedef typename CHelper<CIsSameType<TProtoResult,void>::value>::TArgB TArgB;
	};

	template<FOCP_FAKE_TYPE(SS)> struct CColligateTest<false FOCP_FAKE_TYPE_ARG(SS)>
	{
		typedef typename CQueryBinaryResult<A,nOp,B>::TResult TResult;
		typedef typename CQueryBinaryResult<A,nOp,B>::TArgB TArgB;
	};

	typedef typename CColligateTest<CIsSameType<typename CQueryBinaryResult<A,nOp,B>::TResult,void>::value>::TResult TResult;
	typedef typename CColligateTest<CIsSameType<typename CQueryBinaryResult<A,nOp,B>::TResult,void>::value>::TArgB TArgB;
};

template<typename A, uint32 nOp, typename B> struct CBinaryDefaultConclude
{
	template<typename C> struct CRemoveAll
	{
		typedef typename CTryRemoveConst<typename CTryRemoveVolatile<typename CRemoveRefer<C>::type>::type>::type type;
	};

	struct CInvalidResult
	{
		typedef void TResult;
	};

	template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperA
	{
		template<typename X, typename Y> struct CHelperB
		{
			typedef void TResult;
		};
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_ADD_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelperB
		{
			typedef typename CRemoveAll<X>::type X1;
			typedef typename CRemoveAll<Y>::type Y1;
		template<uint32 FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperC:public CInvalidResult {};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperC<0 FOCP_FAKE_TYPE_ARG(SS)>//i,f
			{
				typedef typename CHighPriorityType<X1,Y1>::type TResult;
			};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperC<1 FOCP_FAKE_TYPE_ARG(SS)>//p+i
			{
				typedef X1 TResult;
			};
			enum
			{
				mode = ((CIsNumberType<X1>::value && CIsNumberType<Y1>::value)?0:
						((CIsPointer<X1>::value && CIsIntegerType<Y1>::value)?1:2))
			};
			typedef typename CHelperC<mode>::TResult TResult;
		};
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_SUB_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelperB
		{
			typedef typename CRemoveAll<X>::type X1;
			typedef typename CRemoveAll<Y>::type Y1;
		template<uint32 FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperC:public CInvalidResult {};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperC<0 FOCP_FAKE_TYPE_ARG(SS)>//i,f
			{
				typedef typename CHighPriorityType<X1,Y1>::type TResult;
			};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperC<1 FOCP_FAKE_TYPE_ARG(SS)>//p-i
			{
				typedef X1 TResult;
			};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperC<2 FOCP_FAKE_TYPE_ARG(SS)>//p-p
			{
				typedef uint32 TResult;
			};
			enum
			{
				mode = ((CIsNumberType<X1>::value && CIsNumberType<Y1>::value)?0:
						((CIsPointer<X1>::value && CIsIntegerType<Y1>::value)?1:
						 ((CIsPointer<X1>::value && CIsIntegerType<Y1>::value)?2:3)))
			};
			typedef typename CHelperC<mode>::TResult TResult;
		};
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_MUL_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelperB
		{
			typedef typename CRemoveAll<X>::type X1;
			typedef typename CRemoveAll<Y>::type Y1;
		template<uint32 FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperC:public CInvalidResult {};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperC<0 FOCP_FAKE_TYPE_ARG(SS)>//i,f
			{
				typedef typename CHighPriorityType<X1,Y1>::type TResult;
			};
			enum
			{
				mode = ((CIsNumberType<X1>::value && CIsNumberType<Y1>::value)?0:1)
			};
			typedef typename CHelperC<mode>::TResult TResult;
		};
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_DIV_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_MUL_OP>::template CHelperB<X,Y> { };
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_MOD_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelperB
		{
		private:
			typedef typename CRemoveAll<X>::type X1;
			typedef typename CRemoveAll<Y>::type Y1;
		template<uint32 FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperC:public CInvalidResult {};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperC<0 FOCP_FAKE_TYPE_ARG(SS)>//i
			{
				typedef typename CHighPriorityType<X1,Y1>::type TResult;
			};
			enum
			{
				mode = ((CIsIntegerType<X1>::value && CIsIntegerType<Y1>::value)?0:1)
			};
			public:
			typedef typename CHelperC<mode>::TResult TResult;
		};
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_AND_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_MOD_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_OR_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_MOD_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_XOR_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_MOD_OP>::template CHelperB<X,Y> { };
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_LAND_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelperB
		{
			typedef bool TResult;
		};
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_LOR_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_LAND_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_LT_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_LAND_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_GT_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_LAND_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_LE_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_LAND_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_GE_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_LAND_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_EQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_LAND_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_NE_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_LAND_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_LSH_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_MOD_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_RSH_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_MOD_OP>::template CHelperB<X,Y> { };
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_ASIGN_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelperB
		{
		template<uint32 FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperC:public CInvalidResult {};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperC<0 FOCP_FAKE_TYPE_ARG(SS)>
			{
				typedef X TResult;
			};
			enum {mode = CIsWritable<X>::value?0:1};
			typedef typename CHelperC<mode>::TResult TResult;
		};
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_ADDEQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_SUBEQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_MULEQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_DIVEQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_MODEQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_LSHEQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_RSHEQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_ANDEQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_OREQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_XOREQ_OP FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename X, typename Y> struct CHelperB: public CHelperA<FOCP_ASIGN_OP>::template CHelperB<X,Y> { };
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_COMMA_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelperB
		{
			typedef Y TResult;
		};
	};

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<FOCP_INDEX_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X, typename Y> struct CHelperB
		{
			typedef typename CRemoveAll<X>::type X1;
			typedef typename CRemoveAll<Y>::type Y1;
		template<uint32 FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperC:public CInvalidResult {};
			template<FOCP_FAKE_TYPE(SS)> struct CHelperC<0 FOCP_FAKE_TYPE_ARG(SS)>//i
			{
				typedef typename CRemovePointer<X1>::type& TResult;
			};
			enum
			{
				mode = ((CIsPointer<X1>::value && CIsIntegerType<Y1>::value)?0:1)
			};
			public:
			typedef typename CHelperC<mode>::TResult TResult;
		};
	};

	typedef typename CHelperA<nOp>::template CHelperB<A,B>::TResult TResult;
};

FOCP_DETAIL_END();
template<typename A, uint32 nOp, typename B> struct CBinaryQuery
{
private:
	typedef typename FOCP_DETAIL_NAME::CBinaryCustomConclude<A,nOp,B>::TResult TCustomResult;

	template<bool bCustomInvalid FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
		typedef TCustomResult TResult;
		typedef typename FOCP_DETAIL_NAME::CBinaryCustomConclude<A,nOp,B>::TArgB TArgB;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef typename FOCP_DETAIL_NAME::CBinaryDefaultConclude<A,nOp,B>::TResult TResult;
		typedef B TArgB;
	};

public:
	typedef typename CHelper<CIsSameType<TCustomResult,void>::value>::TResult TResult;
	typedef typename CHelper<CIsSameType<TCustomResult,void>::value>::TArgB TArgB;
};

template<typename TValue> class CValueExpression;
template<typename A, typename R, uint32 nOp> class CUnaryExpression;

FOCP_DETAIL_BEGIN();

#define MacroHelp(oActName,nOp,oMethod) \
template<typename A> struct oActName \
{ \
private: \
	typedef typename CUnaryQuery<A,nOp>::TResult TResult; \
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CAction{}; \
	template<FOCP_FAKE_TYPE(S)> struct CAction<false FOCP_FAKE_TYPE_ARG(S)> \
	{ \
		CUnaryExpression<A,TResult,nOp> oMethod { return CUnaryExpression<A,TResult,nOp>(*(CValueExpression<A>*)this); }\
	}; \
public: \
	typedef CAction<CIsSameType<TResult,void>::value> TAction; \
}
MacroHelp(CUnaryIncOp, FOCP_INC_OP, operator++(int) );
MacroHelp(CUnaryDecOp, FOCP_DEC_OP, operator--(int) );
MacroHelp(CUnarySIncOp, FOCP_SINC_OP, operator++() );
MacroHelp(CUnarySDecOp, FOCP_SDEC_OP, operator--() );
MacroHelp(CUnaryNonOp, FOCP_NON_OP, operator~() );
MacroHelp(CUnaryNegOp, FOCP_NEG_OP, operator-() );
MacroHelp(CUnaryLNonOp, FOCP_LNON_OP, operator!() );
MacroHelp(CUnaryPostOp, FOCP_POST_OP, operator+() );
MacroHelp(CUnaryAddrOp, FOCP_ADDR_OP, operator&() );
MacroHelp(CUnaryInstOp, FOCP_INST_OP, operator*() );
#undef MacroHelp

FOCP_DETAIL_END();

template<typename A> struct CUnaryOperator:
	public FOCP_DETAIL_NAME::CUnaryIncOp<A>::TAction,
	public FOCP_DETAIL_NAME::CUnaryDecOp<A>::TAction,
	public FOCP_DETAIL_NAME::CUnarySIncOp<A>::TAction,
	public FOCP_DETAIL_NAME::CUnarySDecOp<A>::TAction,
	public FOCP_DETAIL_NAME::CUnaryNonOp<A>::TAction,
	public FOCP_DETAIL_NAME::CUnaryNegOp<A>::TAction,
	public FOCP_DETAIL_NAME::CUnaryLNonOp<A>::TAction,
	public FOCP_DETAIL_NAME::CUnaryPostOp<A>::TAction,
	public FOCP_DETAIL_NAME::CUnaryAddrOp<A>::TAction,
	public FOCP_DETAIL_NAME::CUnaryInstOp<A>::TAction
{
};

/**********************************************************************************
* 表达式定义部分
**********************************************************************************/

/*********************************************************
 * CVoidExpression，无值表达式
 *********************************************************/
class FUN_API CVoidExpression
{
public:
	virtual ~CVoidExpression();
	CVoidExpression();

	virtual CVoidExpression* Clone() const;

	virtual void Compute();

private:
	//阻止赋值操作
	CVoidExpression& operator=(const CVoidExpression& oSrc);
};

/*********************************************************
 * CValueExpression<TValue>，有值表达式
 *	CCondExpression，条件表达式
 *	CIntegerExpression，整数表达式
 *	CFloatExpression，浮点表达式
 *	CBaseExpression<nMode>::CBaseClass<TValue>有值表达式基类
 *********************************************************/
class FUN_API CCondExpression: public CVoidExpression
{
public:
	virtual bool GetCond();
};

class FUN_API CIntegerExpression: public CCondExpression
{
public:
	virtual uint32 GetInteger();
};

class FUN_API CFloatExpression: public CCondExpression
{
public:
	virtual double GetFloat();
};

FOCP_DETAIL_BEGIN();

template<uint32> struct CBaseExpression;

template<> struct CBaseExpression<0>
{
	template<typename TValue> class CBaseClass: public CVoidExpression
	{
	};
};

template<> struct CBaseExpression<1>
{
	template<typename TValue> class CBaseClass: public CIntegerExpression
	{
		inline virtual uint32 GetInteger()
		{
			CValueExpression<TValue>* pExp = (CValueExpression<TValue>*)this;
			return pExp->GetValue();
		}
		inline virtual bool GetCond()
		{
			CValueExpression<TValue>* pExp = (CValueExpression<TValue>*)this;
			return pExp->GetValue();
		}
	};
};

template<> struct CBaseExpression<2>
{
	template<typename TValue> class CBaseClass: public CIntegerExpression
	{
		inline virtual double GetFloat()
		{
			CValueExpression<TValue>* pExp = (CValueExpression<TValue>*)this;
			return pExp->GetValue();
		}
		inline virtual bool GetCond()
		{
			CValueExpression<TValue>* pExp = (CValueExpression<TValue>*)this;
			return pExp->GetValue();
		}
	};
};

template<typename TValue> struct CExpressionMode
{
	enum
	{
		value = ((CIsSameType<TValue,void>::value || CIsArray<TValue>::value || CIsConst<TValue>::value || CIsVolatile<TValue>::value)?3:
				 (CAsFloatType<TValue>::value?2:(CAsIntegerType<TValue>::value?1:0)))
	};
};

FOCP_DETAIL_END();

template<typename A, typename B, typename R, uint32 nOp> class CBinaryExpression;

template<typename A> class CCopyExpression;

template<typename TValue> class CValueExpression:
	public FOCP_DETAIL_NAME::CBaseExpression<FOCP_DETAIL_NAME::CExpressionMode<TValue>::value>::template CBaseClass<TValue>,
	   public CUnaryOperator<TValue>
	   {
		   template<typename TOther,uint32 nOp> struct CBinExpAlia
		   {
			   typedef CBinaryExpression<TValue, TOther, typename CBinaryQuery<TValue, nOp, TOther>::TResult, nOp> type;
		   };

		   template<typename TOther,uint32 nOp> struct CBinExpCopy
		   {
			   typedef CBinaryExpression<TValue, TOther&, typename CBinaryQuery<TValue, nOp, TOther&>::TResult, nOp> type;
		   };

		   public:
		   typedef TValue TValueType;

		   inline virtual ~CValueExpression()
{
}

inline CValueExpression()
{
}

inline CValueExpression(const CValueExpression<TValue>& oSrc)
{
}

inline virtual void Compute()
{
	GetValue();
}

inline virtual CVoidExpression* Clone() const
{
	return new CValueExpression<TValue>(*this);
}

inline virtual TValueType GetValue()
{
	return *(typename CRemoveRefer<TValue>::type*)0;
}

#define MacroHeper(nOp, oMethod) \
		template<typename TOther> typename CBinExpAlia<TOther,nOp>::type oMethod(const CValueExpression<TOther>& oRight) {	return typename  CBinExpAlia<TOther,nOp>::type(*this, *oRight);	} \
		template<typename TOther> typename CBinExpCopy<TOther,nOp>::type oMethod(const TOther& oRight)	{	return typename CBinExpCopy<TOther,nOp>::type(*this, CCopyExpression<TOther>(oRight)); }

MacroHeper(FOCP_ADD_OP,operator+)
MacroHeper(FOCP_SUB_OP,operator-);
MacroHeper(FOCP_MUL_OP,operator*);
MacroHeper(FOCP_MOD_OP,operator/);
MacroHeper(FOCP_MOD_OP,operator%);
MacroHeper(FOCP_AND_OP,operator&);
MacroHeper(FOCP_OR_OP,operator|);
MacroHeper(FOCP_XOR_OP,operator^);

MacroHeper(FOCP_LAND_OP,operator&&);
MacroHeper(FOCP_LOR_OP,operator||);
MacroHeper(FOCP_LT_OP,operator<);
MacroHeper(FOCP_GT_OP,operator>);
MacroHeper(FOCP_LE_OP,operator<=);
MacroHeper(FOCP_GE_OP,operator>=);
MacroHeper(FOCP_EQ_OP,operator==);
MacroHeper(FOCP_NE_OP,operator!=);
MacroHeper(FOCP_LSH_OP,operator<<);
MacroHeper(FOCP_RSH_OP,operator>>);

MacroHeper(FOCP_ASIGN_OP,operator=);
MacroHeper(FOCP_ADDEQ_OP,operator+=);
MacroHeper(FOCP_SUBEQ_OP,operator-=);
MacroHeper(FOCP_MULEQ_OP,operator*=);
MacroHeper(FOCP_DIVEQ_OP,operator/=);
MacroHeper(FOCP_MODEQ_OP,operator%=);
MacroHeper(FOCP_LSHEQ_OP,operator<<=);
MacroHeper(FOCP_RSHEQ_OP,operator>>=);
MacroHeper(FOCP_ANDEQ_OP,operator&=);
MacroHeper(FOCP_OREQ_OP,operator|=);
MacroHeper(FOCP_XOREQ_OP,operator^=);
MacroHeper(FOCP_INDEX_OP,operator[]);

template<typename TOther> typename CBinExpAlia<TOther,FOCP_COMMA_OP>::type operator,(const CValueExpression<TOther>& oRight)
{
	return typename CBinExpAlia<TOther,FOCP_COMMA_OP>::type(*this, *oRight);
}

template<typename TOther> typename CBinExpCopy<TOther,FOCP_COMMA_OP>::type operator,(const TOther& oRight)
{
	return typename CBinExpCopy<TOther,FOCP_COMMA_OP>::type(*this, CCopyExpression<TOther>(oRight));
}

#undef MacroHeper

	   };

/*********************************************************
 * CUnaryExpression<A,B,nOp>，一元表达式
 *********************************************************/
template<typename A, typename B, uint32 nOp> class CUnaryExpression:
	public CValueExpression<B>
{
private:
	template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper;

#define MacroHelper(nOpCode, nOpMethod) template<FOCP_FAKE_TYPE(S)> struct CHelper<nOpCode FOCP_FAKE_TYPE_ARG(S)> \
		{\
			template<typename A1, typename B1> struct CHelperA \
			{ \
				inline B1 Compute(A1 a){return (B1)(nOpMethod a);} \
			}; \
		}

	MacroHelper(FOCP_SINC_OP,++);
	MacroHelper(FOCP_SDEC_OP,--);
	MacroHelper(FOCP_NON_OP,~);
	MacroHelper(FOCP_NEG_OP,-);
	MacroHelper(FOCP_LNON_OP,!);
	MacroHelper(FOCP_POST_OP,+);
	MacroHelper(FOCP_ADDR_OP,&);
	MacroHelper(FOCP_INST_OP,*);

	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_INC_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename A1, typename B1> struct CHelperA
		{
			inline B1 Compute(A1 a)
	{
		return (B1)(a++);
	}
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_DEC_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename A1, typename B1> struct CHelperA
		{
			inline B1 Compute(A1 a)
	{
		return (B1)(a--);
	}
		};
	};

#undef MacroHelper

protected:
	CValueExpression<A>* m_pLeft;

public:
	inline virtual ~CUnaryExpression()
	{
		delete m_pLeft;
	}

	inline CUnaryExpression(const CValueExpression<A>& oLeft)
	{
		m_pLeft = (CValueExpression<A>*)oLeft.Clone();
	}

	inline virtual CVoidExpression* Clone() const
	{
		return new CUnaryExpression(*this);
	}

	inline virtual B GetValue()
	{
		typename CHelper<nOp>::template CHelperA<A,B> oFun;
		return oFun.Compute(m_pLeft->GetValue());
	}

	inline CUnaryExpression(const CUnaryExpression<A,B,nOp>& oExp)
	{
		m_pLeft = (CValueExpression<A>*)oExp.m_pLeft->Clone();
	}
};

/*********************************************************
 * CBinaryExpression<A,B,C,nOp>，二元表达式
 *********************************************************/
template<typename A, typename B, typename C, uint32 nOp> class CBinaryExpression: public CValueExpression<C>
{
private:
	template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper;

#define MacroHelper(nOpCode, nOpMethod) template<FOCP_FAKE_TYPE(S)> struct CHelper<nOpCode FOCP_FAKE_TYPE_ARG(S)> \
	{ \
		template<typename A1, typename B1, typename C1> struct CHelperA \
		{ \
			inline C1 Compute(A1 a, B1 b){return (C1)(a nOpMethod b);} \
		}; \
	}

	MacroHelper(FOCP_ADD_OP,+);
	MacroHelper(FOCP_SUB_OP,-);
	MacroHelper(FOCP_MUL_OP,*);
	MacroHelper(FOCP_DIV_OP,/);
	MacroHelper(FOCP_MOD_OP,%);
	MacroHelper(FOCP_AND_OP,&);
	MacroHelper(FOCP_OR_OP,|);
	MacroHelper(FOCP_XOR_OP,^);
	MacroHelper(FOCP_LAND_OP,&&);
	MacroHelper(FOCP_LOR_OP,||);
	MacroHelper(FOCP_LT_OP,<);
	MacroHelper(FOCP_GT_OP,>);
	MacroHelper(FOCP_LE_OP,<=);
	MacroHelper(FOCP_GE_OP,>=);
	MacroHelper(FOCP_EQ_OP,==);
	MacroHelper(FOCP_NE_OP,!=);
	MacroHelper(FOCP_LSH_OP,<<);
	MacroHelper(FOCP_RSH_OP,>>);
	MacroHelper(FOCP_ASIGN_OP,=);
	MacroHelper(FOCP_ADDEQ_OP,+=);
	MacroHelper(FOCP_SUBEQ_OP,-=);
	MacroHelper(FOCP_MULEQ_OP,*=);
	MacroHelper(FOCP_DIVEQ_OP,/=);
	MacroHelper(FOCP_MODEQ_OP,%=);
	MacroHelper(FOCP_LSHEQ_OP,<<=);
	MacroHelper(FOCP_RSHEQ_OP,>>=);
	MacroHelper(FOCP_ANDEQ_OP,&=);
	MacroHelper(FOCP_OREQ_OP,|=);
	MacroHelper(FOCP_XOREQ_OP,^=);

#undef MacroHelper

	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_COMMA_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename A1, typename B1, typename C1> struct CHelperA
		{
			inline C1 Compute(A1 a, B1 b)
	{
		return (C1)((a,b));
	}
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<FOCP_INDEX_OP FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename A1, typename B1, typename C1> struct CHelperA
		{
			inline C1 Compute(A1 a, B1 b)
	{
		return (C1)(a[b]);
	}
		};
	};

protected:
	CValueExpression<A>* m_pLeft;
	CValueExpression<B>* m_pRight;

public:
	inline virtual ~CBinaryExpression()
	{
		delete m_pLeft;
		delete m_pRight;
	}

	inline CBinaryExpression(const CValueExpression<A>& oLeft, const CValueExpression<B>& oRight)
	{
		m_pLeft = (CValueExpression<A>*)oLeft.Clone();
		m_pRight = (CValueExpression<B>*)oRight.Clone();
	}

	inline virtual CVoidExpression* Clone() const
	{
		return new CBinaryExpression(*this);
	}

	inline virtual C GetValue()
	{
		typename CHelper<nOp>::template CHelperA<A,B,C> oFun;
		return oFun.Compute(m_pLeft->GetValue(), m_pRight->GetValue());
	}

	inline CBinaryExpression(const CBinaryExpression<A,B,C,nOp>& oExp)
	{
		m_pLeft = (CValueExpression<A>*)oExp.m_pLeft->Clone();
		m_pRight = (CValueExpression<B>*)oExp.m_pRight->Clone();
	}
};

/*********************************************************
 * CExpressionConvert<TValue>，拷贝表达式
 *********************************************************/
FOCP_DETAIL_BEGIN();
template<typename TValue> struct CBaseExpressionConvert
{
	inline virtual ~CBaseExpressionConvert()
	{
	}
	inline virtual TValue GetValue()
	{
		return *(typename CRemoveRefer<TValue>::type*)0;
	};
	inline virtual CBaseExpressionConvert<TValue>* Clone()const
	{
		return NULL;
	}
};

template<typename TValue, typename TOther> struct CExpressionConvert: public CBaseExpressionConvert<TValue>
{
private:
	CAny m_oAny;

public:
	inline CExpressionConvert(const CValueExpression<TOther>& oSrc):m_oAny(oSrc.Clone())
	{
	}

	inline virtual ~CExpressionConvert()
	{
		CValueExpression<TOther>* pExp = m_oAny;
		delete pExp;
	}

	inline virtual TValue GetValue()
	{
		CValueExpression<TOther>* pExp = m_oAny;
		return pExp->GetValue();
	}

	inline virtual CBaseExpressionConvert<TValue>* Clone()const
	{
		CValueExpression<TOther>* pExp = m_oAny;
		return new CExpressionConvert<TValue,TOther>(*pExp);
	}
};

FOCP_DETAIL_END();

/*********************************************************
 * CCopyExpression<TValue>，拷贝表达式
 *********************************************************/
template<typename TValue> class CCopyExpression:
	public CValueExpression<TValue&>
{
private:
	TValue* m_pValue;
	FOCP_DETAIL_NAME::CBaseExpressionConvert<TValue&>* m_pOther;

public:
	inline virtual ~CCopyExpression()
	{
		if(m_pOther)
			delete m_pOther;
		if(m_pValue)
			delete m_pValue;
	}

	inline CCopyExpression()
	{
		m_pOther = NULL;
		m_pValue = new TValue();
	}

	template<typename TOther> inline CCopyExpression(const TOther& oValue)
	{
		m_pOther = NULL;
		m_pValue = new TValue(oValue);
	}

	template<typename TOther> inline CCopyExpression(const CValueExpression<TOther> &oExp)
	{
		m_pValue = NULL;
		m_pOther = new FOCP_DETAIL_NAME::CExpressionConvert<TValue&,TOther>(oExp);
	}

	inline CCopyExpression(const CCopyExpression<TValue>& oExp)
	{
		if(oExp.m_pValue)
		{
			m_pValue = new TValue(*oExp.m_pValue);
			m_pOther = NULL;
		}
		else
		{
			m_pValue = NULL;
			m_pOther = oExp.m_pOther->Clone();
		}
	}

	inline virtual CVoidExpression* Clone() const
	{
		return new CCopyExpression<TValue>(*this);
	}

	inline virtual TValue& GetValue()
	{
		if(m_pOther)
			return m_pOther->GetValue();
		return *m_pValue;
	}

};

/*********************************************************
 * CRefExpression<TValue>，引用表达式
 *********************************************************/
template<typename TValue> class CRefExpression:
	public CValueExpression<TValue&>
{
private:
	TValue* m_pValue;
	FOCP_DETAIL_NAME::CBaseExpressionConvert<TValue&>* m_pOther;

public:
	inline virtual ~CRefExpression()
	{
		if(m_pOther)
			delete m_pOther;
		if(m_pValue)
			delete m_pValue;
	}

	inline CRefExpression(TValue& oValue)
	{
		m_pOther = NULL;
		m_pValue = (TValue*)&(char&)oValue;
	}

	template<typename TOther> inline CRefExpression(const CValueExpression<TOther> &oExp)
	{
		m_pValue = NULL;
		m_pOther = new FOCP_DETAIL_NAME::CExpressionConvert<TValue&,TOther>(oExp);
	}

	inline CRefExpression(const CRefExpression<TValue>& oExp)
	{
		if(oExp.m_pValue)
		{
			m_pValue = oExp.m_pValue;
			m_pOther = NULL;
		}
		else
		{
			m_pValue = NULL;
			m_pOther = oExp.m_pOther->Clone();
		}
	}

	inline virtual CVoidExpression* Clone() const
	{
		return new CRefExpression<TValue>(*this);
	}

	inline virtual TValue& GetValue()
	{
		if(m_pOther)
			return m_pOther->GetValue();
		return *m_pValue;
	}

};

/*********************************************************
 * CUseExpression<TValue>，代理表达式
 *********************************************************/
FOCP_DETAIL_BEGIN();

template<bool> struct CBaseUseExpression;

template<> struct CBaseUseExpression<true>
{
	template<typename TValue> class CHelper: public CVoidExpression
	{
	public:
		CVoidExpression* m_pExp;

	public:
		typedef CVoidExpression TBase;

		inline virtual void Compute()
		{
			m_pExp->Compute();
		}
	};
};

template<> struct CBaseUseExpression<false>
{
	template<typename TValue> class CHelper: public CValueExpression<TValue>
	{
	public:
		CValueExpression<TValue>* m_pExp;

	public:
		typedef CValueExpression<TValue> TBase;

		inline virtual TValue GetValue()
		{
			return m_pExp->GetValue();
		}
	};
};

FOCP_DETAIL_END();

template<typename TValue> class CUseExpression
		:public FOCP_DETAIL_NAME::CBaseUseExpression<CIsSameType<TValue,void>::value>::template CHelper<TValue>
{
	typedef typename FOCP_DETAIL_NAME::CBaseUseExpression<CIsSameType<TValue,void>::value>::template CHelper<TValue> TBaseClass;
	typedef typename TBaseClass::TBase TBase;
	public:
	inline virtual ~CUseExpression()
{
}

inline CUseExpression(const TBase& oExp)
{
	TBaseClass::m_pExp = (TBase*)&(char&)oExp;
}

inline CUseExpression(const CUseExpression<TValue>& oExp)
{
	TBaseClass::m_pExp = oExp.m_pExp;
}

inline virtual CVoidExpression* Clone() const
{
	return new CUseExpression(*this);
}
};

/*********************************************************
 * CMemberExpression<TValue>，数据成员表达式
 *********************************************************/
template<typename TObject, typename TMember> class CMemberExpression: public CValueExpression<TMember&>
{
private:
	TMember (TObject::*m_pMember);
	uint32 m_nObjExpType;
	union CObjExp
	{
		CValueExpression<TObject&>* pExp1;
		CValueExpression<TObject*>* pExp2;
		TObject* pObject;
	} m_oObjExp;

public:
	inline virtual ~CMemberExpression()
	{
		if(m_nObjExpType == 1)
			delete m_oObjExp.pExp1;
		else if(m_nObjExpType == 2)
			delete m_oObjExp.pExp2;
	}

	inline CMemberExpression(TMember (TObject::*pMember), TObject& oObj)
	{
		m_pMember = pMember;
		m_nObjExpType = 3;
		m_oObjExp.pObject = (TObject*)(char&)oObj;
	}

	inline CMemberExpression(TMember (TObject::*pMember), TObject* pObj)
	{
		m_pMember = pMember;
		m_nObjExpType = 3;
		m_oObjExp.pObject = pObj;
	}

	inline CMemberExpression(TMember (TObject::*pMember), const CValueExpression<TObject&>& oExp)
	{
		m_pMember = pMember;
		m_nObjExpType = 1;
		m_oObjExp.pExp1 = oExp.Clone();
	}

	/*
		inline CMemberExpression(TMember (TObject::*pMember), const CValueExpression<TObject*>& oExp)
		{
			m_pMember = pMember;
			m_nObjExpType = 2;
			m_oObjExp.pExp2 = oExp.Clone();
		}
	*/

	inline CMemberExpression(const CMemberExpression<TObject,TMember>& oExp)
	{
		m_pMember = oExp.m_pMember;
		m_nObjExpType = oExp.m_nObjExpType;
		switch(m_nObjExpType)
		{
		case 1:
			m_oObjExp.pExp1 = oExp.m_oObjExp.pExp1->Clone();
			break;
		case 2:
			m_oObjExp.pExp2 = oExp.m_oObjExp.pExp2->Clone();
			break;
		case 3:
			m_oObjExp.pObject = oExp.m_oObjExp.pObject;
		}
	}

	inline virtual CVoidExpression* Clone() const
	{
		return new CMemberExpression<TObject,TMember>(*this);
	}

	virtual TMember& GetValue()
	{
		TObject* pObject;
		switch(m_nObjExpType)
		{
		case 1:
			pObject = (TObject*)&(char&)m_oObjExp.pExp1->GetValue();
			break;
		case 2:
			pObject = m_oObjExp.pExp2->GetValue();
			break;
		case 3:
			pObject = m_oObjExp.pObject;
		}
		return (pObject->*m_pMember);
	}
};

template<typename TValue> CCopyExpression<TValue> _copy(const TValue& oValue)
{
	return CCopyExpression<TValue>(oValue);
}
template<typename TValue> CRefExpression<TValue> _refer(TValue& oValue)
{
	return CRefExpression<TValue>(oValue);
}
template<typename TValue> CUseExpression<TValue> _use(const CValueExpression<TValue>& oExp)
{
	return CUseExpression<TValue>(oExp);
}
template<typename TValue> CUseExpression<TValue>& _use(CUseExpression<TValue>& oExp)
{
	return oExp;
}
extern FUN_API CUseExpression<void> _use(const CVoidExpression& oExp);
template<typename TObject, typename TMember> CMemberExpression<TObject,TMember> _member(TMember (TObject::*pMember), TObject& oObj)
{
	return CMemberExpression<TObject,TMember>(pMember, oObj);
}
template<typename TObject, typename TMember> CMemberExpression<TObject,TMember> _member(TMember (TObject::*pMember), TObject* pObj)
{
	return CMemberExpression<TObject,TMember>(pMember, pObj);
}
template<typename TObject, typename TMember> CMemberExpression<TObject,TMember> _member(TMember (TObject::*pMember), const CValueExpression<TObject&>& oExp)
{
	return CMemberExpression<TObject,TMember>(pMember, oExp);
}
//template<typename TObject, typename TMember> _member(TMember (TObject::*pMember), const CValueExpression<TObject*>& oExp) { return CMemberExpression<TObject,TMember>(pMember, oExp); }

/**********************************************************************************
* 调用表达式定义部分
**********************************************************************************/

#include "../../02.ADT/inc/RepeatMacroDef.hpp"
/**************************************************************
参数列表定义
***************************************************************/
#define TypeMacro(n) A##n
#define TypeList(n) ListMacro##n(TypeMacro)
#define xTypeList(n) xListMacro##n(TypeMacro)
#define PreTypeList(Pre,n) PreListMacro##n(Pre,TypeMacro)
#define xPreTypeList(Pre,n) xPreListMacro##n(Pre,TypeMacro)

#define TypeNameMacro(n) typename A##n=CVoid
#define TypeNameList(n) ListMacro##n(TypeNameMacro)

#define FunTypeNameMacro(n) typename A##n
#define FunTypeNameList(n) ListMacro##n(FunTypeNameMacro)

#define ParaMacro(n) typename FOCP_DETAIL_NAME::CArgExpClass<A##n>::type e##n
#define PreParaList(Pre,n) PreListMacro##n(Pre(n),ParaMacro)

#define InsideParaMacro(n) E##n e##n
#define InsideParaList(n) ListMacro##n(InsideParaMacro)

#define ArgMacro(n) e##n
#define ArgList(n) ListMacro##n(ArgMacro)

#define CreateArgMacro(n) m_e##n(e##n)
#define CreateArgList(n) PreListMacro##n(:TBaseFunctor::m_oFunctor(CAny()),CreateArgMacro)

#define CopyArgMacro(n) m_e##n(oSrc.m_e##n)
#define CopyArgList(n) PreListMacro##n(:TBaseFunctor::m_oFunctor(oSrc.m_oFunctor),CopyArgMacro)

#define GetArgMacro(n) m_e##n.GetValue()
#define GetArgList(n) ListMacro##n(GetArgMacro)

struct CVoid {};

FOCP_DETAIL_BEGIN();
/**************************************************************
CExpCover表达式包装器
***************************************************************/
template<typename R> class CExpCover: public CValueExpression<R>
{
private:
	CValueExpression<R>* m_pExp;

public:
	inline virtual ~CExpCover()
	{
		if(m_pExp)
			delete m_pExp;
	}

	inline CExpCover()
	{
		m_pExp = NULL;
	}

	inline CExpCover(const CValueExpression<R> &oExp)
	{
		m_pExp = oExp.Clone();
	}

	inline CExpCover(const CExpCover<R>& oSrc)
	{
		if(oSrc.m_pExp)
			m_pExp = oSrc.m_pExp.Clone();
		else
			m_pExp = NULL;
	}

	inline CExpCover<R>& operator=(const CExpCover<R>& oSrc)
	{
		if(this != &oSrc)
		{
			if(m_pExp)
				delete m_pExp;
			if(oSrc.m_pExp)
				m_pExp = oSrc.m_pExp.Clone();
			else
				m_pExp = NULL;
		}
		return *this;
	}

	inline virtual CVoidExpression* Clone() const
	{
		return new CExpCover<R>(*this);
	}

	virtual R GetValue()
	{
		return m_pExp->GetValue();
	}
};

template<> class CExpCover<void>: public CVoidExpression
{
private:
	CVoidExpression* m_pExp;

public:
	inline virtual ~CExpCover()
	{
		delete m_pExp;
	}

	inline CExpCover(const CVoidExpression &oExp)
	{
		m_pExp = oExp.Clone();
	}

	inline virtual CVoidExpression* Clone() const
	{
		return new CExpCover<void>(*this);
	}

	virtual void Compute()
	{
		m_pExp->Compute();
	}
};

/**************************************************************
CTypeListAttr，类型列表属性；Mask【掩码】, Argc【参数个数】
***************************************************************/
template<TypeNameList(15)> struct CTypeListAttr
{
private:
	template<uint32 nIdx, typename A> struct CArgMask
	{
		enum {Mask = nIdx>15?0:((CIsSameType<A, CVoid>::value||CIsSameType<A, void>::value)?0:(1<<(15 - nIdx)))};
	};

public:
	enum
	{
		Mask =
			CArgMask<1, A1>::Mask | CArgMask<2, A2>::Mask | CArgMask<3, A3>::Mask |	CArgMask<4, A4>::Mask | CArgMask<5, A5>::Mask |
		CArgMask<6, A6>::Mask | CArgMask<7, A7>::Mask |	CArgMask<8, A8>::Mask | CArgMask<9, A9>::Mask |	CArgMask<10, A10>::Mask |
		CArgMask<11, A11>::Mask | CArgMask<12, A12>::Mask | CArgMask<13, A13>::Mask |	CArgMask<14, A14>::Mask | CArgMask<15, A15>::Mask
	};

private:
	template<uint32 nArgc=15 FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
	private:
		enum {Valid = ((Mask >> (nArgc - 1))?true:false)};
		template<bool FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperA
		{
			enum {value=CHelper<nArgc-1>::value+1};
		};
		template<FOCP_FAKE_TYPE(SS)> struct CHelperA<false FOCP_FAKE_TYPE_ARG(SS)>
		{
			enum {value=nArgc-1};
		};
	public:
		enum {value=CHelperA<Valid>::value};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<0 FOCP_FAKE_TYPE_ARG(S)>
	{
		enum {value=0};
	};

public:
	enum {Argc = CHelper<>::value};
};

/**************************************************************
CFunTypeHelper，函数类型信息
***************************************************************/
template<uint32 nArgc=0> struct CFunTypeHelperA
{
	template<typename R, TypeNameList(15)> struct CHelper
	{
		typedef R TResult;
		typedef R (*TFun)();
	};
};

#define PreMacro(n) typedef A##n TArg##n
#define	MacroHelper(n) template<> struct CFunTypeHelperA<n> \
{ \
	template<typename R, TypeNameList(15)> struct CHelper \
	{ \
		typedef R TResult; \
		xSentenceMacro##n(PreMacro); \
		typedef R (*TFun)(TypeList(n)); \
	}; \
}
MakeMacroInstance(15,MacroHelper);
#undef MacroHelper
#undef PreMacro

template<typename R, TypeNameList(15)> struct CFunTypeHelper
{
private:
	enum {Argc = CTypeListAttr<TypeList(15)>::Argc};
public:
	typedef typename CFunTypeHelperA<Argc>::template CHelper<R,TypeList(15)> CInformation;
};

/**************************************************************
CMemberFunctionHelper，成员函数类型信息
***************************************************************/
template<typename R, typename TObject, TypeNameList(15)> struct CMemberFunctionHelper
{
private:
	template<uint32 nArgc FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper;
#define MacroHelper(n) template<FOCP_FAKE_TYPE(S)> struct CHelper<n FOCP_FAKE_TYPE_ARG(S)>{ typedef R (TObject::*TFun)(TypeList(n)); }
	MakeMacroInstance0(15,MacroHelper);
#undef MacroHelper

public:
	typedef CHelper<CTypeListAttr<TypeList(15)>::Argc> CInformation;
};

FOCP_DETAIL_END();

/**************************************************************
CMemberFunction，成员函数类型定义
***************************************************************/
template<typename R, typename TObject, TypeNameList(15)> struct CMemberFunction:
	public FOCP_DETAIL_NAME::CMemberFunctionHelper<R,TObject,TypeList(15)>::CInformation
{
	//typedef R (*TObject::TFun)(...);
};

/**************************************************************
CFunType，函数类型定义
***************************************************************/
template<typename R, TypeNameList(15)> struct CFunType:
	public FOCP_DETAIL_NAME::CFunTypeHelper<R,TypeList(15)>::CInformation,
	   public FOCP_DETAIL_NAME::CTypeListAttr<TypeList(15)>
	   {
		   //typedef A##n TArg##n;
		   //typedef R (*TFun)(...);
		   //Mask
		   //Argc
	   };

FOCP_DETAIL_BEGIN();

template<typename R> struct CBaseFunExp
{
	enum {ExpType = 1};
	typedef CValueExpression<R> type;
};

template<> struct CBaseFunExp<void>
{
	enum {ExpType=0};
	typedef CVoidExpression type;
};

template<typename A> struct CArgExpClass
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
		template<typename X> struct CHelperB
		{
			typedef CRefExpression<typename CRemoveRefer<X>::type> type;
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename X> struct CHelperB
		{
			typedef CCopyExpression<X> type;
		};
	};
	enum {cond = CIsRefer<A>::value};
public:
	typedef typename CHelper<cond>::template CHelperB<A>::type type;
};

template<typename R, TypeNameList(15)> struct CBaseFunctor
{
	CAny m_oFunctor;

	inline void SetFunction(typename CFunType<R,TypeList(15)>::TFun f)
	{
		m_oFunctor = f;
		m_oFunctor.SetType(1);
	}

	inline void SetFunction(const CValueExpression<typename CFunType<R,TypeList(15)>::TFun>& oExp)
	{
		m_oFunctor = oExp;
		m_oFunctor.SetType(2);
	}

	template<typename TObject> inline void SetObjectFunction(const TObject& oObject, typename CMemberFunction<R,TObject,TypeList(15)>::TFun f)
	{
		struct CFunctorInfo
		{
			TObject* pObject;
			typename CMemberFunction<R,TObject,TypeList(15)>::TFun f;
		} oFunctor = {(TObject*)(void*)&(char&)oObject, f};
		m_oFunctor = oFunctor;
		m_oFunctor.SetType(3);
	}

	template<typename TObject> inline void SetExpression(const CValueExpression<TObject&>& oExp, typename CMemberFunction<R,TObject,TypeList(15)>::TFun f)
	{
		struct CFunctorInfo
		{
			CExpCover<TObject&> oExp;
			typename CMemberFunction<R,TObject,TypeList(15)>::TFun f;
		} oFunctor = {oExp, f};
		m_oFunctor = oFunctor;
		m_oFunctor.SetType(4);
	}

	template<typename TObject> inline void SetExpression(const CValueExpression<TObject&>& oObj, const CValueExpression<typename CMemberFunction<R,TObject,TypeList(15)>::TFun>& oFun)
	{
		struct CFunctorInfo
		{
			CExpCover<TObject&> oObjExp;
			CExpCover<typename CMemberFunction<R,TObject,TypeList(15)>::TFun> oFunExp;
		} oFunctor = {oObj, oFun};
		m_oFunctor = oFunctor;
		m_oFunctor.SetType(5);
	}
};

#define MemberTypeMacro(n) typedef typename CArgExpClass<A##n>::type E##n
#define MemberVariableMacro(n) E##n m_e##n

#define MacroHelper1(n) \
template<FOCP_FAKE_TYPE(S)> struct CHelperA<n FOCP_FAKE_TYPE_ARG(S)> { \
	template<typename R, TypeNameList(15)> class CHelperB: \
		public CBaseFunctor<R,TypeList(15)>, public CBaseFunExp<R>::type \
	{ \
	public: \
		typedef CBaseFunctor<R,TypeList(15)> TBaseFunctor;

#define MacroHelper2(n) xSentenceMacro##n(MemberTypeMacro);xSentenceMacro##n(MemberVariableMacro)

#define MacroHelper3(n) \
	inline virtual ~CHelperB(){} \
	inline CHelperB(InsideParaList(n)) CreateArgList(n){} \
	inline CHelperB(const CHelperB<R,TypeList(15)>& oSrc) CopyArgList(n){} \
	inline virtual CVoidExpression* Clone() const{return new CHelperB<R,TypeList(15)>(*this);}

#define MacroHelper4(n) \
switch(TBaseFunctor::m_oFunctor.GetType()) \
{ \
case 1: \
	{ \
		typedef R (*TFun)(TypeList(n)); \
		TFun f = TBaseFunctor::m_oFunctor; \
		ReturnKey f(GetArgList(n)); \
	} \
	break; \
case 2: \
	{ \
		typedef R (*TFun)(TypeList(n)); \
		CValueExpression<TFun>& oExp = TBaseFunctor::m_oFunctor; \
		ReturnKey oExp.GetValue()(GetArgList(n)); \
	} \
	break; \
case 3: \
	{ \
		struct CFunctorInfo; \
		struct CFunctorInfo{CFunctorInfo* pObject; R(CFunctorInfo::*f)(TypeList(n));}; \
		CFunctorInfo &f = TBaseFunctor::m_oFunctor; \
		ReturnKey (f.pObject->*(f.f))(GetArgList(n)); \
	} \
	break; \
case 4: \
	{ \
		struct CFunctorInfo; \
		struct CFunctorInfo{CExpCover<CFunctorInfo&> exp; R(CFunctorInfo::*f)(TypeList(n));}; \
		CFunctorInfo &f = TBaseFunctor::m_oFunctor; \
		ReturnKey (f.exp.GetValue().*(f.f))(GetArgList(n)); \
	} \
	break; \
case 5: \
	{ \
		struct CFunctorInfo; \
		typedef R(CFunctorInfo::*TFun)(TypeList(n)); \
		struct CFunctorInfo{CExpCover<CFunctorInfo&> oObjExp; CExpCover<TFun> oFunExp;}; \
		CFunctorInfo &f = TBaseFunctor::m_oFunctor; \
		ReturnKey (f.oObjExp.GetValue().*(f.oFunExp.GetValue()))(GetArgList(n)); \
	} \
	break; \
}

template<uint32 nExpType=0> struct CFunExpHelper
{//CVoidExpression
	template<uint32 nArgc FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperA;
#define ReturnKey
#define MacroHelper(n) MacroHelper1(n) MacroHelper2(n); MacroHelper3(n) inline virtual void Compute(){MacroHelper4(n)} }; }
	MakeMacroInstance(15,MacroHelper);
#undef MacroHelper
	MacroHelper1(0) MacroHelper3(0) inline virtual void Compute()
	{
		MacroHelper4(0);
	}
};
};
#undef ReturnKey
};

template<> struct CFunExpHelper<1>
{//CValueExpression
	template<uint32 nArgc FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperA;
#define ReturnKey return
#define MacroHelper(n) MacroHelper1(n) MacroHelper2(n); MacroHelper3(n) virtual R GetValue(){MacroHelper4(n) ReturnKey *(typename CRemoveRefer<R>::type*)0;} }; }
	MakeMacroInstance(15,MacroHelper);
#undef MacroHelper
	MacroHelper1(0) MacroHelper3(0) virtual R GetValue()
	{
		MacroHelper4(0) ReturnKey *(typename CRemoveRefer<R>::type*)0;
	}
};
};
#undef ReturnKey
};

#undef MacroHelper1
#undef MacroHelper2
#undef MacroHelper3
#undef MacroHelper4
#undef MacroHelper5
#undef MemberTypeMacro
#undef MemberVariableMacro

FOCP_DETAIL_END();

template<typename R, TypeNameList(15)> struct CFunExp
{
private:
	enum {ExpType=FOCP_DETAIL_NAME::CBaseFunExp<R>::ExpType};
	enum {Argc=FOCP_DETAIL_NAME::CTypeListAttr<TypeList(15)>::Argc};

public:
	typedef typename FOCP_DETAIL_NAME::CFunExpHelper<ExpType>::template CHelperA<Argc>::template CHelperB<R,TypeList(15)> TFunExp;
};

FOCP_DETAIL_BEGIN();
template<typename R, TypeNameList(15)> struct CFunctorHelp
{
private:
	enum {Argc = CTypeListAttr<TypeList(15)>::Argc};
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)/*VoidReturn*/> struct CHelperA
	{
	private:
		template<uint32 nArgc FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperB;
#define MacroHelper(n) template<FOCP_FAKE_TYPE(SS)> struct CHelperB<n FOCP_FAKE_TYPE_ARG(SS)> { inline virtual void operator()(TypeList(n)){} }
		MakeMacroInstance0(15,MacroHelper);
#undef MacroHelper
	public:
		typedef CHelperB<Argc> CHelperC;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<false FOCP_FAKE_TYPE_ARG(S)>
	{
	private:
		template<uint32 nArgc FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperB;
#define MacroHelper(n) template<FOCP_FAKE_TYPE(SS)> struct CHelperB<n FOCP_FAKE_TYPE_ARG(SS)> { inline virtual R operator()(TypeList(n)){return *(typename CRemoveRefer<R>::type*)0;} }
		MakeMacroInstance0(15,MacroHelper);
#undef MacroHelper
	public:
		typedef CHelperB<Argc> CHelperC;
	};
public:
	typedef typename CHelperA<CIsSameType<R,void>::value>::CHelperC CHelper;
};
FOCP_DETAIL_END();

template<typename R, TypeNameList(15)> struct CFunctor: public FOCP_DETAIL_NAME::CFunctorHelp<R,TypeList(15)>::CHelper
{
};

FOCP_DETAIL_BEGIN();

#define PreMacro1(n) R(*f)(xTypeList(n))
#define PreMacro3(n) R(TObject::*f)(xTypeList(n))
#define PreMacro5(n) const TFunctor& oFunctor
#define MacroHelper(n) \
	template<FOCP_FAKE_TYPE(S)> struct CHelper<n FOCP_FAKE_TYPE_ARG(S)> \
	{ \
	private: \
		typedef CFunctor<xPreTypeList(R,n)> TFunctor; \
	public: \
		inline TFunExp operator()(PreParaList(PreMacro1,n)) \
		{ \
			TFunExp oRet(ArgList(n)); \
			oRet.SetFunction(f); \
			return oRet; \
		} \
		inline TFunExp operator()(PreParaList(PreMacro5,n)) \
		{ \
			TFunExp oRet(ArgList(n)); \
			oRet.SetObjectFunction(oFunctor, &TFunctor::operator()); \
			return oRet; \
		} \
		template<typename TObject> inline TFunExp operator()(const TObject& oObject, PreParaList(PreMacro3,n)) \
		{ \
			TFunExp oRet(ArgList(n)); \
			oRet.SetObjectFunction(oObject,f); \
			return oRet; \
		} \
		template<typename TObject> inline TFunExp operator()(const TObject* pObject, PreParaList(PreMacro3,n)) \
		{ \
			TFunExp oRet(ArgList(n)); \
			oRet.SetObjectFunction(*pObject,f); \
			return oRet; \
		} \
		template<typename TObject> inline TFunExp operator()(const CValueExpression<TObject&>& oExp, PreParaList(PreMacro3,n)) \
		{ \
			TFunExp oRet(ArgList(n)); \
			oRet.SetExpression(oExp,f); \
			return oRet; \
		} \
	}

#define MacroHelper0() \
	template<FOCP_FAKE_TYPE(S)> struct CHelper<0 FOCP_FAKE_TYPE_ARG(S)> \
	{ \
	private: \
		typedef CFunctor<xPreTypeList(R,0)> TFunctor; \
	public: \
		inline TFunExp operator()(PreParaList(PreMacro1,0)) \
		{ \
			TFunExp oRet; \
			oRet.SetFunction(f); \
			return oRet; \
		} \
		inline TFunExp operator()(PreParaList(PreMacro5,0)) \
		{ \
			TFunExp oRet; \
			oRet.SetObjectFunction(oFunctor, &TFunctor::operator()); \
			return oRet; \
		} \
		template<typename TObject> inline TFunExp operator()(const TObject& oObject, PreParaList(PreMacro3,0)) \
		{ \
			TFunExp oRet; \
			oRet.SetObjectFunction(oObject,f); \
			return oRet; \
		} \
		template<typename TObject> inline TFunExp operator()(const TObject* pObject, PreParaList(PreMacro3,0)) \
		{ \
			TFunExp oRet; \
			oRet.SetObjectFunction(*pObject,f); \
			return oRet; \
		} \
		template<typename TObject> inline TFunExp operator()(const CValueExpression<TObject&>& oExp, PreParaList(PreMacro3,0)) \
		{ \
			TFunExp oRet; \
			oRet.SetExpression(oExp,f); \
			return oRet; \
		} \
	}

template<typename R, TypeNameList(15)> struct CCallHelp
{
private:
	typedef typename CFunExp<R,TypeList(15)>::TFunExp TFunExp;
	template<uint32 nArgc FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper;
	MakeMacroInstance(15,MacroHelper);
	MacroHelper0();
public:
	typedef CHelper<CFunType<R,TypeList(15)>::Argc> CCaller;
};
#undef MacroHelper
#undef PreMacro1
#undef PreMacro3
#undef PreMacro5

FOCP_DETAIL_END();

template<typename R, TypeNameList(15)> struct _call: public FOCP_DETAIL_NAME::CCallHelp<R,TypeList(15)>::CCaller
{
};

//删除所有不再使用的宏定义
#undef TypeMacro
#undef TypeList
#undef xTypeList
#undef PreTypeList
#undef xPreTypeList
#undef TypeNameMacro
#undef TypeNameList
#undef FunTypeNameMacro
#undef FunTypeNameList
#undef ParaMacro
#undef PreParaList
#undef InsideParaMacro
#undef InsideParaList
#undef ArgMacro
#undef ArgList
#undef CreateArgMacro
#undef CreateArgList
#undef CopyArgMacro
#undef CopyArgList
#undef GetArgMacro
#undef GetArgList
#include "../../02.ADT/inc/RepeatMacroUnDef.hpp"

/**********************************************************************************
* 语句定义部分
**********************************************************************************/

FOCP_DETAIL_BEGIN();
enum
{
	FOCP_INVALID_SENTENCE,
	FOCP_COMPLEX_SENTENCE,
	FOCP_EXPRESSION_SENTENCE,
	FOCP_IF_SENTENCE,
	FOCP_DOWHILE_SENTENCE,
	FOCP_WHILE_SENTENCE,
	FOCP_FOR_SENTENCE,
	FOCP_SWITCH_SENTENCE,
	FOCP_CASE_SENTENCE,
	FOCP_DEFAULT_SENTENCE,
	FOCP_BREAK_SENTENCE,
	FOCP_CONTINUE_SENTENCE,
	FOCP_RETURN_SENTENCE,
};
FOCP_DETAIL_END();

class CSentence;
class CComplexSentence;
class CExpressionSentence;
class CIfSentence;
class CDoWhileSentence;
class CWhileSentence;
class CForSentence;
class CSwitchSentence;
class CCaseSentence;
class CDefaultSentence;
class CBreakSentence;
class CContinueSenctence;

enum
{
	FOCP_SENTENCE_FINISH,
	FOCP_SENTENCE_BREAK,
	FOCP_SENTENCE_CONTINUE,
	FOCP_SENTENCE_RETURN
};

class FUN_API CSentence
{
	friend class CSentenceExpression;
public:
	CSentence* m_pNext;
	CSentence* m_pParent;

	virtual ~CSentence();
	CSentence();
	CSentence(const CSentence& oSrc);

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	virtual uint32 GetSentenceType() const ;
};

class FUN_API CComplexSentence: public CSentence
{
private:
	CSentence *m_pHead, *m_pTail;

public:
	virtual ~CComplexSentence();
	CComplexSentence();
	CComplexSentence(const CComplexSentence& oSrc);

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	CComplexSentence& AddSentence(const CSentence& oSentence);

	CSentence* Begin();

	virtual uint32 GetSentenceType() const;
};

template<typename TSentence> class CSentenceBuilder
{
private:
	TSentence* m_pSentence;
	CComplexSentence m_oBody;

public:
	inline virtual ~CSentenceBuilder()
	{
	}

	inline CSentenceBuilder(TSentence* pSentence)
	{
		m_pSentence = pSentence;
		m_oBody.m_pParent = pSentence;
	}

	inline TSentence& operator[](const CSentence& oRight)
	{
		m_oBody.AddSentence(oRight);
		return *m_pSentence;
	}

	inline CComplexSentence* operator->()
	{
		return &m_oBody;
	}

	inline const CComplexSentence& body() const
	{
		return m_oBody;
	}
};

class FUN_API CExpressionSentence: public CSentence
{
private:
	CVoidExpression* m_pExp;

public:
	CExpressionSentence(const CVoidExpression& oExp);
	virtual ~CExpressionSentence();

	virtual CSentence* Clone() const;
	virtual uint32 Call();

	virtual uint32 GetSentenceType() const;
};

class FUN_API CIfSentence: public CSentence
{
private:
	CCondExpression* m_pCond;
	CSentenceBuilder<CIfSentence> _then;

public:
	CIfSentence(const CCondExpression& oExp);
	CIfSentence(const CIfSentence& oSrc);
	virtual ~CIfSentence();

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	CSentenceBuilder<CIfSentence> _else;

	CIfSentence& operator[](const CSentence& oSentence);

	virtual uint32 GetSentenceType() const;
};

FOCP_DETAIL_BEGIN();
class CWhileExpression
{
private:
	CDoWhileSentence* m_pSentence;
	CCondExpression* m_pCond;

public:
	virtual ~CWhileExpression();
	CWhileExpression(CDoWhileSentence* pSentence);
	template<typename TValue> inline CDoWhileSentence& operator()(const TValue& nCond)
	{
		if(m_pCond)
			delete m_pCond;
		m_pCond = new CCopyExpression<TValue>(nCond);
		return *this;
	}
	CDoWhileSentence& operator()(const CCondExpression& oExp);
	const CCondExpression* GetCond() const;
	operator bool();
};
FOCP_DETAIL_END();

class FUN_API CDoWhileSentence: public CSentence
{
private:
	CSentenceBuilder<CDoWhileSentence> _body;

public:
	CDoWhileSentence();
	CDoWhileSentence(const CDoWhileSentence& oSrc);
	virtual ~CDoWhileSentence();

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	FOCP_DETAIL_NAME::CWhileExpression _while;

	CDoWhileSentence& operator[](const CSentence& oSentence);

	virtual uint32 GetSentenceType() const;
};

class FUN_API CWhileSentence: public CSentence
{
private:
	CCondExpression* m_pCond;
	CSentenceBuilder<CWhileSentence> _body;

public:
	CWhileSentence(const CCondExpression& oExp);
	CWhileSentence(const CWhileSentence& oSrc);
	virtual ~CWhileSentence();

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	CWhileSentence& operator[](const CSentence& oSentence);

	virtual uint32 GetSentenceType() const;
};

FOCP_DETAIL_BEGIN();
class CForExpression
{
private:
	CForSentence* m_pSentence;
	CVoidExpression* m_pExp;

public:
	~CForExpression();
	CForExpression(CForSentence* pSentence);
	CForSentence& operator()(const CVoidExpression& oExp);
	const CVoidExpression* GetExp() const;
	void Compute();
};
FOCP_DETAIL_END();

class FUN_API CForSentence: public CSentence
{
private:
	CSentenceBuilder<CForSentence> _body;
	CCondExpression* m_pCond;

public:
	virtual ~CForSentence();
	CForSentence(const CCondExpression &oCond);
	CForSentence(const CForSentence &oSrc);

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	FOCP_DETAIL_NAME::CForExpression _init, _loop;

	CForSentence& operator[](const CSentence& oSentence);

	virtual uint32 GetSentenceType() const;
};

class FUN_API CSwitchSentence: public CSentence
{
private:
	CIntegerExpression * m_pExp;
	CDefaultSentence* m_pDefault;
	CSentenceBuilder<CSwitchSentence> _body;

public:
	virtual ~CSwitchSentence();
	CSwitchSentence(const CIntegerExpression& oExp);
	CSwitchSentence(const CSwitchSentence& oSrc);

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	CSwitchSentence& operator[](const CSentence& oSentence);

	virtual uint32 GetSentenceType() const;
};

class FUN_API CDefaultSentence: public CSentence
{
private:
	CSentenceBuilder<CDefaultSentence> _body;

public:
	CDefaultSentence();
	CDefaultSentence(const CDefaultSentence& oSrc);
	virtual ~CDefaultSentence();

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	CDefaultSentence& operator[](const CSentence& oSentence);

	virtual uint32 GetSentenceType() const;
};

class FUN_API CCaseSentence: public CSentence
{
	friend class CSwitchSentence;
private:
	uint32 m_nValue;
	CSentenceBuilder<CCaseSentence> _body;

public:
	virtual ~CCaseSentence();
	CCaseSentence(uint32 nValue);
	CCaseSentence(const CCaseSentence& oSrc);

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	CCaseSentence& operator[](const CSentence& oSentence);

	virtual uint32 GetSentenceType() const;
};

class FUN_API CBreakSentence: public CSentence
{
public:
	virtual ~CBreakSentence();
	CBreakSentence();
	CBreakSentence(const CBreakSentence& oSrc);

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	virtual uint32 GetSentenceType() const;
};

class FUN_API CContinueSentence: public CSentence
{
public:
	virtual ~CContinueSentence();
	CContinueSentence();
	CContinueSentence(const CContinueSentence& oSrc);

	virtual CSentence* Clone() const;
	virtual uint32 Call();
	virtual void Check();

	virtual uint32 GetSentenceType() const;
};

extern FUN_API CComplexSentence operator,(const CSentence& oLeft, const CSentence& oRight);
extern FUN_API CComplexSentence operator,(const CSentence& oLeft, const CVoidExpression& oRight);
extern FUN_API CComplexSentence& operator,(CComplexSentence& oLeft, const CSentence& oRight);
extern FUN_API CComplexSentence& operator,(CComplexSentence& oLeft, const CVoidExpression& oRight);
extern FUN_API CExpressionSentence _exp(CVoidExpression& oExp);
template<typename TValue> CIfSentence _if(const TValue & nCond)
{
	return CIfSentence(CCopyExpression<TValue>(nCond));
}
extern FUN_API CIfSentence _if(const CCondExpression& nCond);
extern FUN_API CDoWhileSentence _do();
template<typename TValue> CWhileSentence _while(const TValue &nCond)
{
	return CWhileSentence(nCond);
}
extern FUN_API CWhileSentence _while(const CCondExpression& oExp);
template<typename TValue> inline CForSentence _for(const TValue &nCond)
{
	return CForSentence(CCopyExpression<TValue>(nCond));
}
extern FUN_API CForSentence _for(const CCondExpression &oCond);
template<typename TValue> inline CSwitchSentence _switch(const TValue &nCond)
{
	return CSwitchSentence(CCopyExpression<TValue>(nCond));
}
extern FUN_API CSwitchSentence _switch(const CIntegerExpression& oExp);
extern FUN_API CDefaultSentence _default();
extern FUN_API CCaseSentence _case(uint32 nValue);
extern FUN_API CBreakSentence _break();
extern FUN_API CContinueSentence _continue();

/**********************************************************************************
* 函件定义部分
**********************************************************************************/

#include "../../02.ADT/inc/RepeatMacroDef.hpp"
/**************************************************************
参数列表定义
***************************************************************/
#define TypeMacro(n) A##n
#define TypeList(n) ListMacro##n(TypeMacro)

#define TypeNameMacro(n) typename A##n=CVoid
#define TypeNameList(n) ListMacro##n(TypeNameMacro)

#define ParaMacro(n) A##n a##n
#define ParaList(n) ListMacro##n(ParaMacro)

#define LocalTypeMacro(n) L##n
#define LocalTypeList(n) ListMacro##n(LocalTypeMacro)

#define LocalTypeNameMacro(n) typename L##n=CVoid
#define LocalTypeNameList(n) ListMacro##n(LocalTypeNameMacro)

FOCP_DETAIL_BEGIN();

template<typename A> struct CArgExpression
{
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CExpression;

	template<FOCP_FAKE_TYPE(S)> struct CExpression<true FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename B> struct CHelper: public CValueExpression<B>//可写
		{
			CCopyExpression<typename CRemoveRefer<B>::type*> m_oExp;

			inline virtual ~CHelper()
	{
	}

	inline CHelper()
	{
		m_oExp.GetValue() = NULL;
	}

	inline void SetValue(B a)
	{
		m_oExp.GetValue() = (typename CRemoveRefer<B>::type*)&(char&)a;
	}

	inline CHelper(const CHelper<B> &oExp):m_oExp(oExp.m_oExp)
	{
	}

	inline virtual CVoidExpression* Clone() const
	{
		return new CHelper<B>(*this);
	}

	inline virtual B GetValue()
	{
		return *m_oExp.GetValue();
	}
		};
	};

	template<FOCP_FAKE_TYPE(S)> struct CExpression<false FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename B> struct CHelper: public CValueExpression<B>//不可写
		{
			CCopyExpression<typename CRemoveRefer<B>::type*> m_oExp;

			inline virtual ~CHelper()
	{
		typename CRemoveRefer<B>::type* pVal = m_oExp.GetValue();
		if(pVal)
			delete pVal;
	}

	inline CHelper()
	{
		m_oExp.GetValue() = NULL;
	}

	inline void SetValue(B a)
	{
		typename CRemoveRefer<B>::type* &pVal = m_oExp.GetValue();
		if(pVal)
			delete pVal;
		pVal = new typename CRemoveRefer<B>::type(a);
	}

	inline CHelper(const CHelper<B> &oExp)
	{
		m_oExp.GetValue() = new typename CRemoveRefer<B>::type(oExp.GetValue());
	}

	inline virtual CVoidExpression* Clone() const
	{
		return new CHelper<B>(*this);
	}

	inline virtual B GetValue()
	{
		return *m_oExp.GetValue();
	}
		};
	};
	typedef typename CExpression<CIsWritable<A>::value>::template CHelper<A> TExp;
};

template<LocalTypeNameList(32)> struct CFunctionLocalHelpA
{
private:
	template<uint32 nIdx, typename A> struct CMask
	{
		enum {Mask = nIdx>32?0:((CIsSameType<A, CVoid>::value||CIsSameType<A, void>::value)?0:(1<<(32 - nIdx)))};
	};

public:
	enum
	{
		Mask =
			CMask<1, L1>::Mask | CMask<2, L2>::Mask | CMask<3, L3>::Mask |	CMask<4, L4>::Mask | CMask<5, L5>::Mask |
		CMask<6, L6>::Mask | CMask<7, L7>::Mask |	CMask<8, L8>::Mask | CMask<9, L9>::Mask |	CMask<10, L10>::Mask |
		CMask<11, L11>::Mask | CMask<12, L12>::Mask | CMask<13, L13>::Mask |	CMask<14, L14>::Mask | CMask<15, L15>::Mask |
		CMask<16, L16>::Mask | CMask<17, L17>::Mask |	CMask<18, L18>::Mask | CMask<19, L19>::Mask |	CMask<20, L20>::Mask |
		CMask<21, L21>::Mask | CMask<22, L22>::Mask | CMask<23, L23>::Mask |	CMask<24, L24>::Mask | CMask<25, L25>::Mask |
		CMask<26, L26>::Mask | CMask<27, L27>::Mask |	CMask<28, L28>::Mask | CMask<29, L29>::Mask |	CMask<30, L30>::Mask |
		CMask<31, L31>::Mask | CMask<32, L32>::Mask
	};

private:
	template<uint32 nArgc FOCP_FAKE_DEFAULT_TYPE(SS)> struct CArg
	{
	private:
		enum {Valid = ((Mask >> (nArgc - 1))?true:false)};
		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			enum {value=CArg<nArgc-1>::value+1};
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=nArgc-1};
		};
	public:
		enum {value=CHelper<Valid>::value};
	};
	template<FOCP_FAKE_TYPE(SS)> struct CArg<0 FOCP_FAKE_TYPE_ARG(SS)>
	{
		enum {value=0};
	};

public:
	enum {Argc = CArg<32>::value};
};

template<uint32 nArgc=0> struct CFunctionLocalHelpB
{
	template<LocalTypeNameList(32)> struct CHelper
	{
		inline CHelper() {}
		inline virtual ~CHelper() {}
	};
};

#define LocalVariableDefine(n) L##n m_nVar##n
#define LocalExpressionDefine(n) CRefExpression<L##n> m_oVarExp##n
#define BindVariable(n) m_nVar##n(L##n())
#define BindExpression(n) m_oVarExp##n(m_nVar##n)
#define MacroHelper(n) \
template<> struct CFunctionLocalHelpB<n> \
{ \
	template<LocalTypeNameList(32)> struct CHelper \
	{ \
	private: \
		xSentenceMacro##n(LocalVariableDefine); \
	protected: \
		xSentenceMacro##n(LocalExpressionDefine); \
	public: \
		inline virtual ~CHelper(){} \
		inline CHelper():ListMacro##n(BindVariable),ListMacro##n(BindExpression){} \
	}; \
}
MakeMacroInstance(32,MacroHelper);
#undef MacroHelper
#undef LocalVariableDefine
#undef LocalExpressionDefine
#undef BindVariable
#undef BindExpression

template<LocalTypeNameList(32)> struct CFunctionLocal
{
	typedef typename CFunctionLocalHelpB<CFunctionLocalHelpA<LocalTypeList(32)>::Argc>::template CHelper<LocalTypeList(32)> TDefine;
};

template<typename TOwner> struct CFunctorSentence
{
	TOwner* m_pOwner;

	inline CFunctorSentence(TOwner* pOwner):m_pOwner(pOwner) {};
	inline TOwner& operator[](CSentence& oSentence)
	{
		m_pOwner->SetSentence(oSentence);
	}
};

template<typename R> struct CReturnAction;

template<typename R> struct CReturnSentence
{
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperA;

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<true FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename R1> class CHelperB: public CSentence
		{
		public:
			inline virtual ~CHelperB()
	{
	}

	inline CHelperB(typename CReturnAction<R>::TReturnAction*)
	{
	}

	inline CHelperB(const CHelperB<R1>& oSrc)
	{
	}

	inline virtual CSentence* Clone() const
	{
		return new CHelperB<R1>;
	}

	inline virtual uint32 Call()
	{
		return FOCP_SENTENCE_RETURN;
	}

	inline virtual uint32 GetSentenceType() const
	{
		return FOCP_RETURN_SENTENCE;
	}
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<false FOCP_FAKE_TYPE_ARG(S)>
	{
	template<typename R1> class CHelperB: public CSentence
		{
		private:
			CValueExpression<R1>* m_pExp;
			typename CReturnAction<R1>::TReturnAction* m_pOwner;

		public:
			inline virtual ~CHelperB()
	{
		delete m_pExp;
	}

	inline CHelperB(typename CReturnAction<R1>::TReturnAction* pOwner, const CValueExpression<R1>& oExp)
	{
		m_pOwner = pOwner;
		m_pExp = oExp.Clone();
	}

	inline CHelperB(const CHelperB<R1> &oSentence)
	{
		if(oSentence.m_pExp)
			m_pExp = oSentence.m_pExp->Clone();
		else
			m_pExp = NULL;
		m_pOwner = oSentence.m_pOwner;
	}

	inline virtual CSentence* Clone() const
	{
		return new CHelperB<R1>(*this);
	}

	inline virtual uint32 Call()
	{
		m_pOwner->SetReturnSentence(this);
		return FOCP_SENTENCE_RETURN;
	}

	inline R1 GetResult()
	{
		return m_pExp->GetValue();
	}

	inline virtual uint32 GetSentenceType() const
	{
		return FOCP_RETURN_SENTENCE;
	}
		};
	};

	typedef typename CHelperA<CIsSameType<R,void>::value>::template CHelperB<R> TReturnSentence;
};

template<typename R> struct CReturnAction
{
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperA;

	template<FOCP_FAKE_TYPE(S)> struct CHelperA<true FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename R1> struct CHelperB
		{
			inline typename CReturnSentence<R1>::TReturnSentence _return()
	{
		return typename CReturnSentence<R1>::TReturnSentence(this);
	}
		};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<false FOCP_FAKE_TYPE_ARG(S)>
	{
		template<typename R1> struct CHelperB
		{
			typename CReturnSentence<R1>::TReturnSentence* m_pRetSentence;
			inline CHelperB()
	{
		m_pRetSentence = NULL;
	}

	inline typename CReturnSentence<R1>::TReturnSentence _return(const CValueExpression<R>& oExp)
	{
		return typename CReturnSentence<R1>::TReturnSentence(this, oExp);
	}

	inline void SetReturnSentence(typename CReturnSentence<R1>::TReturnSentence* pSentence)
	{
		m_pRetSentence = pSentence;
	}
		};
	};
	typedef typename CHelperA<CIsSameType<R,void>::value>::template CHelperB<R> TReturnAction;
};

template<uint32> struct CFunctionHelpA;

template<> struct CFunctionHelpA<0>
{
	template<typename R, TypeNameList(15)> struct CHelper: public CReturnAction<R>::TReturnAction, public CFunctor<R,TypeList(15)>
	{
		CSentence* m_pSentence;
	};
};

#define MemberTypeMacro(n) typedef typename CArgExpression<A##n>::TExp TArgExp##n
#define MemberVariableMacro(n) typename TArgExp##n::type m_oArgExp##n
#define MacroHelper(n) \
template<> struct CFunctionHelpA<n> \
{ \
	template<typename R, TypeNameList(15)> struct CHelper: public CFunctionHelpA<0>::CHelper<R,TypeList(15)> \
	{ \
		xSentenceMacro##n(MemberTypeMacro); \
		xSentenceMacro##n(MemberVariableMacro); \
	}; \
}
MakeMacroInstance(15, MacroHelper);
#undef MacroHelper
#undef MemberTypeMacro
#undef MemberVariableMacro

template<bool> struct CFunctionHelpB;

template<> struct CFunctionHelpB<true>
{
	template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperA;
#define MacroHelper1(n) TFunctionHelpA::m_oArgExp##n.SetValue(a##n)
#define MacroHelper2(n) \
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<n FOCP_FAKE_TYPE_ARG(S)> \
	{ \
		template<typename R, TypeNameList(15)> struct CHelperB: public CFunctionHelpA<n>::template CHelper<R,TypeList(15)> \
		{ \
			typedef CFunctionHelpA<n>::template CHelper<R,TypeList(15)> TFunctionHelpA; \
			using TFunctionHelpA::m_pSentence; \
			inline virtual R operator()(ParaList(n)) \
			{ \
				xSentenceMacro##n(MacroHelper1); \
				if(m_pSentence) \
					m_pSentence->Call(); \
			} \
		}; \
	}
	MakeMacroInstance0(15,MacroHelper2);
#undef MacroHelper1
#undef MacroHelper2
};

template<> struct CFunctionHelpB<false>
{
	template<uint32 FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperA;
#define MacroHelper1(n) TFunctionHelpA::m_oArgExp##n.SetValue(a##n)
#define MacroHelper2(n) \
	template<FOCP_FAKE_TYPE(S)> struct CHelperA<n FOCP_FAKE_TYPE_ARG(S)> \
	{ \
		template<typename R, TypeNameList(15)> struct CHelperB: public CFunctionHelpA<n>::template CHelper<R,TypeList(15)> \
		{ \
			typedef CFunctionHelpA<n>::template CHelper<R,TypeList(15)> TFunctionHelpA; \
			using TFunctionHelpA::m_pSentence; \
			inline virtual R operator()(ParaList(n)) \
			{ \
				xSentenceMacro##n(MacroHelper1); \
				m_pSentence->Call(); \
				return m_pSentence->GetResult(); \
			} \
		}; \
	}
	MakeMacroInstance0(15,MacroHelper2);
#undef MacroHelper1
#undef MacroHelper2
};

template<LocalTypeNameList(47)> struct CFunctionHelpC
{
	template<uint32 nArgc, uint32 nMask> struct CArgcHelp
	{
	private:
		enum {Valid = ((nMask >> (nArgc - 1))?true:false)};
		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
		{
			enum {value=CArgcHelp<nArgc-1, nMask>::value+1};
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
		{
			enum {value=nArgc-1};
		};
	public:
		enum {value=nArgc?CHelper<Valid>::value:0};
	};

	template<uint32 nIdx FOCP_FAKE_DEFAULT_TYPE(S)> struct CReName;
#define MacroHelper(n) template<FOCP_FAKE_TYPE(S)> struct CReName<n FOCP_FAKE_TYPE_ARG(S)>{typedef L##n type;}
	MakeMacroInstance(47,MacroHelper);
#undef MacroHelper

	template<uint32 nIdx> struct CArgMask
	{
		typedef typename CReName<nIdx>::type CArgType;
		enum {Mask = (CIsSameType<CArgType, CVoid>::value||CIsSameType<CArgType, void>::value)?0:(1<<(15 - nIdx))};
	};

	enum
	{
		ArgMask =
			CArgMask<1>::Mask | CArgMask<2>::Mask | CArgMask<3>::Mask |	CArgMask<4>::Mask | CArgMask<5>::Mask |
			CArgMask<6>::Mask | CArgMask<7>::Mask |	CArgMask<8>::Mask | CArgMask<9>::Mask |	CArgMask<10>::Mask |
			CArgMask<11>::Mask | CArgMask<12>::Mask | CArgMask<13>::Mask |	CArgMask<14>::Mask | CArgMask<15>::Mask,
		Argc = CArgcHelp<15,ArgMask>::value
	};

	enum {StartVarIdx=(Argc<15)?(Argc+1):Argc};

	template<uint32 nIdx> struct CVarMask
	{
		typedef typename CReName<nIdx+StartVarIdx>::type CVarType;
		enum {Mask = (CIsSameType<CVarType, CVoid>::value||CIsSameType<CVarType, void>::value)?0:(1<<(32 - nIdx))};
	};

	enum
	{
		VarMask =
			CVarMask<1>::Mask | CVarMask<2>::Mask | CVarMask<3>::Mask |	CVarMask<4>::Mask | CVarMask<5>::Mask |
			CVarMask<6>::Mask | CVarMask<7>::Mask |	CVarMask<8>::Mask | CVarMask<9>::Mask |	CVarMask<10>::Mask |
			CVarMask<11>::Mask | CVarMask<12>::Mask | CVarMask<13>::Mask |	CVarMask<14>::Mask | CVarMask<15>::Mask |
			CVarMask<16>::Mask | CVarMask<17>::Mask |	CVarMask<18>::Mask | CVarMask<19>::Mask |	CVarMask<20>::Mask |
			CVarMask<21>::Mask | CVarMask<22>::Mask | CVarMask<23>::Mask |	CVarMask<24>::Mask | CVarMask<25>::Mask |
			CVarMask<26>::Mask | CVarMask<27>::Mask |	CVarMask<28>::Mask | CVarMask<29>::Mask |	CVarMask<30>::Mask |
			CVarMask<31>::Mask | CVarMask<32>::Mask,
		Varc = CArgcHelp<32,VarMask>::value
	};
};

template<uint32 nArgc> struct CFunctionHelpD;

#define ItemMacro2(n) L##n
#define ItemMacro(n) CFunctionHelpC<xListMacro47(ItemMacro2)>::template CReName<n>::type
#define MacroHelper(n) template<> struct CFunctionHelpD<n> \
{ \
	template<typename R, LocalTypeNameList(47)> struct CHelper: \
		public CFunctionHelpB<CIsSameType<R,void>::value>::template CHelperA<n>::template CHelperB<PreListMacro##n(R,ItemMacro)> \
	{ \
		typedef typename CFunctionHelpB<CIsSameType<R,void>::value>::template CHelperA<n>::template CHelperB<PreListMacro##n(R,ItemMacro)> TSentenceOwner; \
		using TSentenceOwner::m_pSentence; \
	}; \
}
MakeMacroInstance0(15,MacroHelper);
#undef ItemMacro
#undef MacroHelper

template<uint32 nVarc> struct CFunctionHelpE;

#define TReTypeMacro(n) TReType
#define MacroHelper(n) template<> struct CFunctionHelpE<n> \
{ \
	template<typename R, LocalTypeNameList(47)> struct CHelperX \
	{ \
		typedef CFunctionHelpC<ListMacro47(ItemMacro2)> TFunctionHelpC; \
		enum{VarIdx=n+TFunctionHelpC::StartVarIdx}; \
		typedef typename TFunctionHelpC::template CReName<VarIdx>::type TReType; \
		typedef typename CFunctionLocal<ListMacro##n(TReTypeMacro)>::TDefine TDefine; \
	}; \
	template<typename R, LocalTypeNameList(47)> struct CHelper: \
		public CHelperX<R,ListMacro47(ItemMacro2)>::TDefine \
	{ \
	}; \
}
MakeMacroInstance0(32,MacroHelper);
#undef ItemMacro
#undef ItemMacro2
#undef MacroHelper

FOCP_DETAIL_END();

template<typename R, LocalTypeNameList(47)> struct CFunction:
	public FOCP_DETAIL_NAME::CFunctionHelpD<FOCP_DETAIL_NAME::CFunctionHelpC<LocalTypeList(47)>::Argc>::template CHelper<R,LocalTypeList(47)>,
	   public FOCP_DETAIL_NAME::CFunctionHelpE<FOCP_DETAIL_NAME::CFunctionHelpC<LocalTypeList(47)>::Varc>::template CHelper<R,LocalTypeList(47)>
	   {
		   typedef CFunction<R,LocalTypeList(47)> TFunction;
		   typedef typename FOCP_DETAIL_NAME::CFunctionHelpD<FOCP_DETAIL_NAME::CFunctionHelpC<LocalTypeList(47)>::Argc>::template CHelper<R,LocalTypeList(47)> TSentenceOwner;
		   using TSentenceOwner::m_pSentence;
		   FOCP_DETAIL_NAME::CFunctorSentence<TFunction> Build;

		   inline virtual ~CFunction()
{
	if(m_pSentence)
		delete m_pSentence;
}

inline CFunction():Build(this)
{
	m_pSentence = NULL;
}

inline CFunction(const CFunction& oSrc)
{
	if(oSrc.m_pSentence)
		m_pSentence = oSrc.m_pSentence->Clone();
	else
		m_pSentence = NULL;
}

inline void SetSentence(const CSentence& oRight)
{
	if(m_pSentence)
		delete m_pSentence;
	m_pSentence = oRight.Clone();
	return *m_pSentence;
}
	   };

#define FocpFunctorDecl(oFunName,RetType,TypeCount,TypeList) \
struct C##oFunName: public CFunction<RetType,FOCP_ARGLIST##TypeCount TypeList>; \
extern FUN_API C##oFunName oFunName

#define FocpFunctorBegin(oFunName,RetType,TypeCount,TypeList) \
struct C##oFunName: public CFunction<RetType,FOCP_ARGLIST##TypeCount TypeList> \
{ \
	C##oFunName() \
	{ \
		Build

#define FocpFunctorEnd(oFunName) ;\
	} \
}; \
FUN_API C##oFunName oFunName

#undef TypeMacro
#undef TypeList

#undef TypeNameMacro
#undef TypeNameList

#undef ParaMacro
#undef ParaList

#undef LocalTypeMacro
#undef LocalTypeList

#undef LocalTypeNameMacro
#undef LocalTypeNameList

/****************************************************
FocpFunctorBegin(F1, int, 3, (int, void, int))
[
	书写语句表达式
]
FocpFunctorEnd(F1);
*****************************************************/
#include "../../02.ADT/inc/RepeatMacroUnDef.hpp"

FOCP_END();

#endif
