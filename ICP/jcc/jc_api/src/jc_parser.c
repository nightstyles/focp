
#include "jc_parser.h"

static int TestOption(int argc, char* argv[], const char* sOpt);
static void ReportSynErr(CJcParser* parser, char* fname, int line, int col, int n);
static void ReportSemErr(CJcParser* parser, int bWarning, char* fname, int line, int col, const char *s);
static void ReportSemErr2(CJcParser* parser, int bWarning, const char *s);
static void SynErr(CJcParser* parser, int n);
static void Accept(CJcParser* parser);
static void Expect(CJcParser* parser, int n);
static int StartOf(CJcParser* parser, int s);
static void ExpectWeak(CJcParser* parser, int n, int follow);
static int WeakSeparator(CJcParser* parser, int n, int syFol, int repFol);

static void OnInitializeParser(CJcParser*parser,int argc, char* argv[]);
static void OnDestroyParser(CJcParser*parser);
static void OnGetToken(CJcParser*parser);
static void Main(CJcParser*parser);
static void Declaration(CJcParser*parser,jc_uint nHasType );
static void Statement(CJcParser*parser);
static void GotoStatement(CJcParser*parser);
static void IfStatement(CJcParser*parser);
static void SwitchStatement(CJcParser*parser);
static void WhileStatement(CJcParser*parser);
static void DoStatement(CJcParser*parser);
static void ForStatement(CJcParser*parser);
static void CompoundStatement(CJcParser*parser);
static void ReturnStatement(CJcParser*parser);
static void ContinueStatement(CJcParser*parser);
static void BreakStatement(CJcParser*parser);
static void ExpressionStatement(CJcParser*parser);
static void Expression(CJcParser*parser,CJcExpress* pExp, jc_bool bSupportComma);
static void CaseStatement(CJcParser*parser,jc_uint nSwitchExpressType);
static void CaseLabel(CJcParser*parser,jc_uint nSwitchExpressType);
static void ForInitializer(CJcParser*parser);
static void StorageSpecifier(CJcParser*parser,jc_uint* pStorage, jc_bool bAllowTypedef);
static void QualSpecifier(CJcParser*parser,jc_uint* pQual);
static void TypeSpecifier(CJcParser*parser,jc_uint* pQual, CJcTypeInfo * pType, jc_bool nAllowBody);
static void ObjectsSpecifier(CJcParser*parser,jc_uint nStorage, CJcTypeInfo* pType, jc_bool bAllowFunctionDefine);
static void ConstQualSpecifier(CJcParser*parser,jc_uint* pQual);
static void VolatileQualSpecifier(CJcParser*parser,jc_uint* pQual);
static void SimpleSpecifier(CJcParser*parser,jc_uint* pQual, CJcTypeInfo* pType);
static void StructSpecifier(CJcParser*parser,jc_uint* pQual, CJcTypeInfo* pType, jc_bool nAllowBody);
static void EnumSpecifier(CJcParser*parser,jc_uint* pQual, CJcTypeInfo* pType, jc_bool nAllowBody);
static void UserTypeSpecifier(CJcParser*parser,jc_uint* pQual, CJcTypeInfo* pType);
static void PointerTypeSpecifier(CJcParser*parser,CJcTypeInfo * pType);
static void IntegerSpecifier(CJcParser*parser,jc_uint bSigned, jc_uint* pType);
static void EnumBodySpecifier(CJcParser*parser,CJcEnumInfo* pEnumInfo, char* pEnumName);
static void EnumMemberSpecifier(CJcParser*parser,CJcEnumInfo* pEnumInfo, char* pEnumName);
static void StructBodySpecifier(CJcParser*parser,jc_uint nStructType, CJcStructInfo* pStructInfo);
static void StructMemberSpecifier(CJcParser*parser,jc_uint nStructType, CJcStructInfo* pStructInfo);
static void FieldsSpecifier(CJcParser*parser,CJcTypeInfo* pFieldType, CJcStructInfo* pStructInfo, jc_uint nStructType);
static void FieldsSpecifier2(CJcParser*parser,CJcTypeInfo* pFieldType, CJcStructInfo* pStructInfo, jc_uint nStructType, jc_bool* pMeetEnd, jc_bool bRemovePointer);
static void ObjectSpecifier(CJcParser*parser,jc_uint nStorage, CJcString* pName, CJcTypeInfo* pType, jc_bool bAllowFunctionDeclare);
static void ObjectsSpecifier2(CJcParser*parser,jc_uint nStorage, CJcTypeInfo* pType, jc_bool bAllowFunctionDefine, jc_bool* pMeetEnd, jc_bool bRemovePointer);
static void FunctionDefine2(CJcParser*parser,CJcSymbol* pSymbol);
static void InitializeVariable(CJcParser*parser,CJcSymbol* pSymbol, jc_bool* pMeetEnd);
static void FunctionDeclare(CJcParser*parser,CJcTypeInfo* pType);
static void ArrayDeclare(CJcParser*parser,CJcTypeInfo* pType);
static void ParaSpecifier(CJcParser*parser,CJcString* pName, CJcTypeInfo* pParaType);
static void InitializerExpr(CJcParser*parser,CJcVariableInfo* pVariable);
static void PrimaryExpr(CJcParser*parser,CJcExpress* pExp);
static void PostfixExpr(CJcParser*parser,CJcExpress* pExp);
static void AssignmentExpr(CJcParser*parser,CJcExpress* pExp);
static void UnaryExpr(CJcParser*parser,CJcExpress* pExp);
static void UnaryOperator(CJcParser*parser,jc_uint* pUnaryOperator);
static void CastExpr(CJcParser*parser,CJcExpress* pExp);
static void MultiplicativeExpr(CJcParser*parser,CJcExpress* pExp);
static void AdditiveExpr(CJcParser*parser,CJcExpress* pExp);
static void ShiftExpr(CJcParser*parser,CJcExpress* pExp);
static void RelationalExpr(CJcParser*parser,CJcExpress* pExp);
static void EqualityExpr(CJcParser*parser,CJcExpress* pExp);
static void AndExpr(CJcParser*parser,CJcExpress* pExp);
static void ExclusiveOrExpr(CJcParser*parser,CJcExpress* pExp);
static void InclusiveOrExpr(CJcParser*parser,CJcExpress* pExp);
static void LogicalAndExpr(CJcParser*parser,CJcExpress* pExp);
static void LogicalOrExpr(CJcParser*parser,CJcExpress* pExp);
static void ConditionalExpr(CJcParser*parser,CJcExpress* pExp);
static void InitializeExpressA(CJcParser*parser,CJcExpress* pExp);
static void InitializeStringExpress(CJcParser*parser,CJcExpress* pExp, CJcArrayInfo* pArray);
static void InitializeCompoundExpress(CJcParser*parser,CJcExpress* pExp, CJcTypeInfo* pType, jc_uint nTypeCode);
static void InitializeFieldListExpress(CJcParser*parser,jc_uint nTypeCode, CJcExpress* pStruct, CJcStructFieldInfo* pField);
static void InitializeArrayItemExpress(CJcParser*parser,CJcExpress* pExp, CJcArrayInfo* pArray, CJcTypeInfo* pItemType, jc_uint nDim, jc_uint nIdx);

static jc_bool TestOption(int argc, char* argv[], const char* sOpt)
{
	jc_int i;
	for(i=1; i<argc; ++i)
	{
		if(!StringCompare(argv[i], sOpt))
			return True;
	}
	return False;
}

static void Accept(CJcParser* parser){
	if(parser->la){
		parser->t = parser->la;
		parser->la = NULL;
	}
}

CJcToken* Get(CJcParser* parser){
	if(parser->la)
		return parser->la;
	for(;;){
		parser->la = ScanToken(parser->scanner);
		if(parser->la->kind == parser->maxT){
			SetError(parser, 0, "invalid token '%s'", parser->la->val);
			continue;
		}
		if (parser->la->kind < parser->maxT){
			++parser->errDist;
			break;
		}

	}
	OnGetToken(parser);
	return parser->la;
}
static void OnInitializeParser(CJcParser* parser,int argc, char* argv[]) {
		parser->bSupportHostSymbol = TestOption(argc, argv, "-H");
		parser->bSupportShareSymbol = TestOption(argc, argv, "-S");
		InitializeSymbolStack(&parser->oSymbolStack);
		InitializeGotoStack(&parser->oGotoStack);
		InitializeBreakStack(&parser->oBreakStack);
		InitializeContinueStack(&parser->oContinueStack);
		InitializeSwitchStack(&parser->oSwitchStack);
		parser->oSymbolStack.pParser = parser;
		
}

static void OnDestroyParser(CJcParser* parser) {
		ClearSwitchStack(&parser->oSwitchStack);
		ClearContinueStack(&parser->oContinueStack);
		ClearBreakStack(&parser->oBreakStack);
		ClearGotoStack(&parser->oGotoStack);
		ClearSymbolStack(&parser->oSymbolStack);
		
}

static void OnGetToken(CJcParser* parser) {
		if(parser->la->kind == parser->_host && !parser->bSupportHostSymbol)
		parser->la->kind = parser->_Identifier;
		else if(parser->la->kind == parser->_share && !parser->bSupportShareSymbol)
			parser->la->kind = parser->_Identifier;
		
}

static void Main(CJcParser* parser) {
		jc_uint bShouldBeDeclaration, nHasType;
		CJcGotoFillError oError;
		parser->pFunction = NULL;
		NewVariableTable(&parser->oSymbolStack.oDataSegment);
		NewVariableTable(&parser->oSymbolStack.oGlobalStack);
		NewSymbolTable(&parser->oSymbolStack);
		NewGotoTable(&parser->oGotoStack);
		
		while (StartOf(parser, 1)) {
			nHasType = 0;
			bShouldBeDeclaration = 0;
			if(IsTypeSpecifier(&parser->oSymbolStack, Get(parser)->kind, Get(parser)->val)){
				nHasType = 1;
				bShouldBeDeclaration = 1;
			}
			else{
				CJcSymbol oSymbol;
				CJcToken* pToken = Get(parser);
				if(pToken->kind == parser->_Identifier && FindSymbol(&parser->oSymbolStack, pToken->val, 1, &oSymbol)){
					pToken = PeekToken(parser->scanner);
					if(pToken->kind == parser->_lparentheses){
						pToken = PeekToken(parser->scanner);
						if(pToken->kind == parser->_rparentheses){
							pToken = PeekToken(parser->scanner);
							if(pToken->kind == parser->_lbrace)
								bShouldBeDeclaration = 1;
						}
						else if(IsTypeSpecifier(&parser->oSymbolStack, pToken->kind, pToken->val))
							bShouldBeDeclaration = 1;
						else if(pToken->kind == parser->_Identifier){
							pToken = PeekToken(parser->scanner);
							if(pToken->kind == parser->_Identifier)
								bShouldBeDeclaration = 1;
						}
					}
					ResetPeek(parser->scanner);
				}
			}
			
			if (bShouldBeDeclaration) {
				Declaration(parser,nHasType);
			} else {
				Statement(parser);
			}
		}
		Expect(parser, 0);
		while(BackFillGotoStack(&parser->oGotoStack, &parser->oSymbolStack.oCodeSegment, &oError))
		SetErrorEx(parser, 0, oError.sFileName, oError.nLine, oError.nCol, "missing label '%s'", oError.sLabel);
		PopGotoTable(&parser->oGotoStack);
		PopSymbolTable(&parser->oSymbolStack);
		PopVariableTable(&parser->oSymbolStack.oGlobalStack);
		PopVariableTable(&parser->oSymbolStack.oDataSegment);
		
}

static void Declaration(CJcParser* parser,jc_uint nHasType ) {
		CJcTypeInfo oType;
		jc_uint nQual = JC_NONE, nStorage = JC_NONE;
		jc_bool bAllowBody = True;
		InitializeType(&oType, JC_NONE, JC_SIGNED_INT, NULL, NULL, NULL);
		
		if (StartOf(parser, 2)) {
			StorageSpecifier(parser,&nStorage, True);
		}
		if (Get(parser)->kind == 10 || Get(parser)->kind == 38) {
			QualSpecifier(parser,&nQual);
		}
		if(nQual || (nStorage && nStorage!=JC_TYPEDEF))
		bAllowBody = False;
		
		if (nHasType) {
			TypeSpecifier(parser,&nQual, &oType, bAllowBody);
		}
		if (nQual || nStorage || !nHasType) {
			ObjectsSpecifier(parser,nStorage, &oType, True);
		} else if (Get(parser)->kind == 79) {
			Accept(parser);
		} else if (Get(parser)->kind == 1 || Get(parser)->kind == 67 || Get(parser)->kind == 68) {
			ObjectsSpecifier(parser,nStorage, &oType, True);
		} else SynErr(parser, 88);
		ClearType(&oType); 
}

static void Statement(CJcParser* parser) {
		CJcString oLabelName;
		ResetPeek(parser->scanner); 
		
		while (Get(parser)->kind==parser->_Identifier && PeekToken(parser->scanner)->kind == parser->_colon) {
			InitializeString(&oLabelName);
			AppendString(&oLabelName, Get(parser)->val);
			
			Expect(parser, 1);
			Expect(parser, 78);
			if(NewGotoLabel(&parser->oGotoStack, oLabelName.pStr, GetPosOfSegment(&parser->oSymbolStack.oCodeSegment)))
			SetError(parser, 0, "redefine label '%s'", oLabelName.pStr);
			
		}
		switch (Get(parser)->kind) {
		case 20: {
			GotoStatement(parser);
			break;
		}
		case 22: {
			IfStatement(parser);
			break;
		}
		case 33: {
			SwitchStatement(parser);
			break;
		}
		case 39: {
			WhileStatement(parser);
			break;
		}
		case 13: {
			DoStatement(parser);
			break;
		}
		case 19: {
			ForStatement(parser);
			break;
		}
		case 73: {
			CompoundStatement(parser);
			break;
		}
		case 26: {
			ReturnStatement(parser);
			break;
		}
		case 11: {
			ContinueStatement(parser);
			break;
		}
		case 7: {
			BreakStatement(parser);
			break;
		}
		case 1: case 2: case 3: case 4: case 5: case 30: case 60: case 61: case 62: case 63: case 66: case 67: case 68: case 70: case 71: case 79: case 86: {
			ExpressionStatement(parser);
			break;
		}
		default: SynErr(parser, 89); break;
		}
}

static void GotoStatement(CJcParser* parser) {
		Expect(parser, 20);
		Expect(parser, 1);
		CreateGoto(&parser->oGotoStack, GetPosOfSegment(&parser->oSymbolStack.oCodeSegment),
		parser->t->fname, parser->t->val, parser->t->fline, parser->t->fcol);
		Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, 0, JC_DEFAULT_INSADDR);
		
		Expect(parser, 79);
}

static void IfStatement(CJcParser* parser) {
		CJcExpress oExp;
		jc_bool bNormalCond, bForce=False, bTrue;
		jc_uint nIfInsAddr, nIfEndInsAddr;
		jc_uint nElseSentAddr = 0, nIfEndSentAddr = 0;
		InitializeExpress(&parser->oSymbolStack, &oExp);
		
		Expect(parser, 22);
		Expect(parser, 68);
		Expression(parser,&oExp, True);
		bNormalCond = CheckCondExpress(&oExp); 
		Expect(parser, 69);
		if(bNormalCond && oExp.nOpt == JC_IS){
		CJcVal oVal;
		GetConstValue(&oExp, &oVal, GetTypeCode(GetOrigType(oExp.pType)), 1);
		bForce = True;
		bTrue = (oVal.ul?True:False);
		if(!bTrue){
			nIfInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nElseSentAddr, JC_DEFAULT_INSADDR);
		}
		}
		else{
			nIfInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			EmitCondJmp(&oExp, False, nElseSentAddr, JC_DEFAULT_INSADDR);
		}
		
		Statement(parser);
		nElseSentAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		if(!bForce)
			EmitCondJmp(&oExp, False, nElseSentAddr, nIfInsAddr);
		else if(!bTrue)
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nElseSentAddr, nIfInsAddr);
		DestroyExpress(&oExp);
		
		if (Get(parser)->kind == 15) {
			Accept(parser);
			if(!bForce || bTrue){
			nIfEndInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nIfEndSentAddr, JC_DEFAULT_INSADDR);
			}
			
			Statement(parser);
			if(!bForce || bTrue){
			nIfEndSentAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nIfEndSentAddr, nIfEndInsAddr);
			}
			
		}
}

static void SwitchStatement(CJcParser* parser) {
		CJcExpress oExp;
		jc_uint nSwitchInsAddr, nSwitchTableAddr = 0, nSwitchEndAddr, nSwitchExpressType;
		InitializeExpress(&parser->oSymbolStack, &oExp);
		
		Expect(parser, 33);
		Expect(parser, 68);
		Expression(parser,&oExp, True);
		CheckSwitchExpress(&oExp); 
		Expect(parser, 69);
		nSwitchInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		EmitSwitchJmp(&oExp, nSwitchTableAddr, JC_DEFAULT_INSADDR);
		NewBreakTable(&parser->oBreakStack);
		nSwitchExpressType = GetTypeCode(GetOrigType(oExp.pType));
		NewSwitchTable(&parser->oSwitchStack, nSwitchExpressType);
		
		Expect(parser, 73);
		CaseStatement(parser,nSwitchExpressType);
		while (Get(parser)->kind == 8 || Get(parser)->kind == 12) {
			CaseStatement(parser,nSwitchExpressType);
		}
		Expect(parser, 74);
		nSwitchEndAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		WriteDefault(&parser->oSwitchStack, nSwitchEndAddr, 1);
		nSwitchTableAddr = BackFillSwitchStack(&parser->oSwitchStack, &parser->oSymbolStack.oConstSegment, &parser->oSymbolStack.oConstTable);
		EmitSwitchJmp(&oExp, nSwitchTableAddr, nSwitchInsAddr);
		BackFillBreakStack(&parser->oBreakStack, &parser->oSymbolStack.oCodeSegment, nSwitchEndAddr);
		PopSwitchTable(&parser->oSwitchStack);
		PopBreakTable(&parser->oBreakStack);
		DestroyExpress(&oExp);
		
}

static void WhileStatement(CJcParser* parser) {
		CJcExpress oExp;
		jc_uint nWhileInsAddr;
		jc_uint nEndAddr = 0;
		jc_bool bNormalCond, bForce=False, bTrue;
		jc_uint nLoopAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		InitializeExpress(&parser->oSymbolStack, &oExp);
		
		Expect(parser, 39);
		Expect(parser, 68);
		Expression(parser,&oExp, True);
		bNormalCond = CheckCondExpress(&oExp); 
		Expect(parser, 69);
		if(bNormalCond && oExp.nOpt == JC_IS){
		CJcVal oVal;
		GetConstValue(&oExp, &oVal, GetTypeCode(GetOrigType(oExp.pType)), 1);
		bForce = True;
		bTrue = (oVal.ul?True:False);
		if(!bTrue){
			nWhileInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nEndAddr, JC_DEFAULT_INSADDR);
		}
		}
		else{
			nWhileInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			EmitCondJmp(&oExp, False, nEndAddr, JC_DEFAULT_INSADDR);
		}
		NewBreakTable(&parser->oBreakStack);
		NewContinueTable(&parser->oContinueStack);
		
		Statement(parser);
		if(!bForce || bTrue)
		Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nLoopAddr, JC_DEFAULT_INSADDR);
		nEndAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		if(!bForce)
			EmitCondJmp(&oExp, False, nEndAddr, nWhileInsAddr);
		else if(!bTrue)
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nEndAddr, nWhileInsAddr);
		BackFillBreakStack(&parser->oBreakStack, &parser->oSymbolStack.oCodeSegment, nEndAddr);
		BackFillContinueStack(&parser->oContinueStack, &parser->oSymbolStack.oCodeSegment, nLoopAddr);
		PopBreakTable(&parser->oBreakStack);
		PopContinueTable(&parser->oContinueStack);
		DestroyExpress(&oExp);
		
}

static void DoStatement(CJcParser* parser) {
		CJcExpress oExp;
		jc_uint nDoAddr, nLoopAddr, nEndAddr = 0;
		jc_bool bNormalCond;
		InitializeExpress(&parser->oSymbolStack, &oExp);
		
		Expect(parser, 13);
		nDoAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		NewBreakTable(&parser->oBreakStack);
		NewContinueTable(&parser->oContinueStack);
		
		Statement(parser);
		nLoopAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment); 
		Expect(parser, 39);
		Expect(parser, 68);
		Expression(parser,&oExp, True);
		bNormalCond = CheckCondExpress(&oExp); 
		Expect(parser, 69);
		Expect(parser, 79);
		if(bNormalCond && oExp.nOpt == JC_IS){
		CJcVal oVal;
		GetConstValue(&oExp, &oVal, GetTypeCode(GetOrigType(oExp.pType)), 1);
		if(oVal.ul)
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nDoAddr, JC_DEFAULT_INSADDR);
		}
		else
			EmitCondJmp(&oExp, True, nDoAddr, JC_DEFAULT_INSADDR);
		nEndAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		BackFillBreakStack(&parser->oBreakStack, &parser->oSymbolStack.oCodeSegment, nEndAddr);
		BackFillContinueStack(&parser->oContinueStack, &parser->oSymbolStack.oCodeSegment, nLoopAddr);
		PopBreakTable(&parser->oBreakStack);
		PopContinueTable(&parser->oContinueStack);
		DestroyExpress(&oExp);
		
}

static void ForStatement(CJcParser* parser) {
		CJcToken* pToken;
		CJcExpress oExp, oSubExp;
		jc_uint nTestAddr, nEndAddr=0;
		jc_uint nLoopAddr, nSentAddr = 0;
		jc_uint nIfInsAddr, nElseInsAddr;
		jc_bool bForce = False, bTrue, bNormalCond, bIsType = False;
		InitializeExpress(&parser->oSymbolStack, &oExp);
		InitializeExpress(&parser->oSymbolStack, &oSubExp);
		
		Expect(parser, 19);
		NewSymbolTable(&parser->oSymbolStack);
		if(parser->oSymbolStack.nLevel == 2)
			NewGotoTable(&parser->oGotoStack);
		if(parser->oSymbolStack.pInFunction)
			NewVariableTable(&parser->oSymbolStack.oLocalStack);
		else
			NewVariableTable(&parser->oSymbolStack.oGlobalStack);
		
		Expect(parser, 68);
		pToken = Get(parser);
		bIsType = IsTypeSpecifier(&parser->oSymbolStack, pToken->kind, pToken->val);
		
		if (bIsType) {
			ForInitializer(parser);
		} else if (StartOf(parser, 3)) {
			if (StartOf(parser, 4)) {
				Expression(parser,&oExp, True);
			}
		} else SynErr(parser, 90);
		Expect(parser, 79);
		nTestAddr = nLoopAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment); 
		if (StartOf(parser, 4)) {
			Expression(parser,&oExp, True);
			bNormalCond = CheckCondExpress(&oExp);
			nIfInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			if(bNormalCond && oExp.nOpt == JC_IS){
				CJcVal oVal;
				GetConstValue(&oExp, &oVal, GetTypeCode(GetOrigType(oExp.pType)), 1);
				bTrue = (oVal.ul?True:False);
				bForce = True;
			}
			else
				EmitCondJmp(&oExp, True, nSentAddr, JC_DEFAULT_INSADDR);
			nElseInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nEndAddr, JC_DEFAULT_INSADDR);
			nLoopAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			
		}
		Expect(parser, 79);
		if (StartOf(parser, 4)) {
			Expression(parser,&oSubExp, True);
			if(nTestAddr != nLoopAddr)
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nTestAddr, JC_DEFAULT_INSADDR);
			
		}
		Expect(parser, 69);
		nSentAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		if(nTestAddr != nLoopAddr){
			if(bForce){
				if(nSentAddr == nLoopAddr)
					RollBackSegment(&parser->oSymbolStack.oCodeSegment, 8);
				else
					Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nSentAddr, nElseInsAddr);
			}
			else
				EmitCondJmp(&oExp, True, nSentAddr, nIfInsAddr);
		}
		NewBreakTable(&parser->oBreakStack);
		NewContinueTable(&parser->oContinueStack);
		
		Statement(parser);
		Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nLoopAddr, JC_DEFAULT_INSADDR);
		nEndAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		if(nTestAddr != nLoopAddr && (!bForce || !bTrue))
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nEndAddr, nElseInsAddr);
		BackFillBreakStack(&parser->oBreakStack, &parser->oSymbolStack.oCodeSegment, nEndAddr);
		BackFillContinueStack(&parser->oContinueStack, &parser->oSymbolStack.oCodeSegment, nLoopAddr);
		PopBreakTable(&parser->oBreakStack);
		PopContinueTable(&parser->oContinueStack);
		if(parser->oSymbolStack.nLevel == 2)
		{
			CJcGotoFillError oError;
			while(BackFillGotoStack(&parser->oGotoStack, &parser->oSymbolStack.oCodeSegment, &oError))
				SetErrorEx(parser, 0, oError.sFileName, oError.nLine, oError.nCol, "missing label '%s'", oError.sLabel);
			PopGotoTable(&parser->oGotoStack);
		}
		if(parser->oSymbolStack.pInFunction)
			PopVariableTable(&parser->oSymbolStack.oLocalStack);
		else
			PopVariableTable(&parser->oSymbolStack.oGlobalStack);
		PopSymbolTable(&parser->oSymbolStack);
		DestroyExpress(&oExp);
		DestroyExpress(&oSubExp);
		
}

static void CompoundStatement(CJcParser* parser) {
		jc_uint nStackInsAddr, nStackSize, bShouldBeDeclaration, nHasType; 
		Expect(parser, 73);
		NewSymbolTable(&parser->oSymbolStack);
		if(parser->pFunction){
			SetFunction(&parser->oSymbolStack, parser->pFunction);
			parser->pFunction = NULL;
		}
		if(parser->oSymbolStack.nLevel == 2){
			NewGotoTable(&parser->oGotoStack);
			nStackInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_newstk, (jc_uchar)JC_UN, 0, JC_DEFAULT_INSADDR);
		}
		if(parser->oSymbolStack.pInFunction)
			NewVariableTable(&parser->oSymbolStack.oLocalStack);
		else
			NewVariableTable(&parser->oSymbolStack.oGlobalStack);
		
		while (StartOf(parser, 1)) {
			nHasType = 0;
			bShouldBeDeclaration = 0;
			if(IsTypeSpecifier(&parser->oSymbolStack, Get(parser)->kind, Get(parser)->val)){
				nHasType = 1;
				bShouldBeDeclaration = 1;
			}
			else{
				CJcSymbol oSymbol;
				CJcToken* pToken = Get(parser);
				if(pToken->kind == parser->_Identifier && FindSymbol(&parser->oSymbolStack, pToken->val, 1, &oSymbol)){
					pToken = PeekToken(parser->scanner);
					if(pToken->kind == parser->_lparentheses){
						pToken = PeekToken(parser->scanner);
						if(IsTypeSpecifier(&parser->oSymbolStack, pToken->kind, pToken->val))
							bShouldBeDeclaration = 1;
						else if(pToken->kind == parser->_Identifier){
							pToken = PeekToken(parser->scanner);
							if(pToken->kind == parser->_Identifier)
								bShouldBeDeclaration = 1;
						}
					}
					ResetPeek(parser->scanner);
				}
			}
			
			if (bShouldBeDeclaration) {
				Declaration(parser,nHasType);
			} else {
				Statement(parser);
			}
		}
		Expect(parser, 74);
		if(parser->oSymbolStack.nLevel == 2){
		CJcGotoFillError oError;
		if(parser->oSymbolStack.pInFunction)
			NewGotoLabel(&parser->oGotoStack, "$Ret", GetPosOfSegment(&parser->oSymbolStack.oCodeSegment));
		while(BackFillGotoStack(&parser->oGotoStack, &parser->oSymbolStack.oCodeSegment, &oError))
			SetErrorEx(parser, 0, oError.sFileName, oError.nLine, oError.nCol, "missing label '%s'", oError.sLabel);
		PopGotoTable(&parser->oGotoStack);
		if(parser->oSymbolStack.pInFunction)
			nStackSize = GetSizeOfStack(&parser->oSymbolStack.oLocalStack);
		else
			nStackSize = GetSizeOfStack(&parser->oSymbolStack.oGlobalStack);
		Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_newstk, (jc_uchar)JC_UN, nStackSize, nStackInsAddr);
		Emit0(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_delstk, JC_DEFAULT_INSADDR);
		if(parser->oSymbolStack.pInFunction)
			Emit0(&parser->oSymbolStack.oCodeSegment, (jc_short)jc_ret, JC_DEFAULT_INSADDR);
		}
		if(parser->oSymbolStack.pInFunction)
			PopVariableTable(&parser->oSymbolStack.oLocalStack);
		else
			PopVariableTable(&parser->oSymbolStack.oGlobalStack);
		PopSymbolTable(&parser->oSymbolStack);
		
}

static void ReturnStatement(CJcParser* parser) {
		CJcExpress oExp;
		jc_bool nHasReturnValue = False;
		InitializeExpress(&parser->oSymbolStack, &oExp);
		
		Expect(parser, 26);
		if (StartOf(parser, 4)) {
			Expression(parser,&oExp, True);
			nHasReturnValue = True; 
		}
		Expect(parser, 79);
		if(!parser->oSymbolStack.pInFunction)
		SetError(parser, 0, "invalid return statement");
		else if(NeedReturnValue(&parser->oSymbolStack) != nHasReturnValue){
			if(nHasReturnValue)
				SetError(parser, 0, "return express is unexpected", Get(parser)->val);
			else
				SetError(parser, 0, "return express is expected", Get(parser)->val);
		}
		else{
			if(nHasReturnValue)
				ReturnExpress(&oExp);
			CreateGoto(&parser->oGotoStack, GetPosOfSegment(&parser->oSymbolStack.oCodeSegment),
				parser->t->fname, "$Ret", parser->t->fline, parser->t->fcol);
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, 0, JC_DEFAULT_INSADDR);
		}
		DestroyExpress(&oExp);
		
}

static void ContinueStatement(CJcParser* parser) {
		Expect(parser, 11);
		Expect(parser, 79);
		if(CreateContinue(&parser->oContinueStack, GetPosOfSegment(&parser->oSymbolStack.oCodeSegment)))
		SetError(parser, 0, "invalid continue statement");
		else
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, 0, JC_DEFAULT_INSADDR);
		
}

static void BreakStatement(CJcParser* parser) {
		Expect(parser, 7);
		Expect(parser, 79);
		if(CreateBreak(&parser->oBreakStack, GetPosOfSegment(&parser->oSymbolStack.oCodeSegment)))
		SetError(parser, 0, "invalid break statement");
		else
			Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, 0, JC_DEFAULT_INSADDR);
		
}

static void ExpressionStatement(CJcParser* parser) {
		CJcExpress oExp;
		InitializeExpress(&parser->oSymbolStack, &oExp);
		
		if (StartOf(parser, 4)) {
			Expression(parser,&oExp, True);
			DestroyExpress(&oExp); 
		}
		Expect(parser, 79);
}

static void Expression(CJcParser* parser,CJcExpress* pExp, jc_bool bSupportComma) {
		AssignmentExpr(parser,pExp);
		while (Get(parser)->kind == 82) {
			if (bSupportComma && Get(parser)->kind == parser->_comma) {
				Expect(parser, 82);
				AssignmentExpr(parser,pExp);
			} else {
				break; 
			}
		}
}

static void CaseStatement(CJcParser* parser,jc_uint nSwitchExpressType) {
		CaseLabel(parser,nSwitchExpressType);
		while (Get(parser)->kind == 8 || Get(parser)->kind == 12) {
			CaseLabel(parser,nSwitchExpressType);
		}
		Statement(parser);
		while (StartOf(parser, 5)) {
			Statement(parser);
		}
}

static void CaseLabel(CJcParser* parser,jc_uint nSwitchExpressType) {
		CJcExpress oExp;
		jc_uint nError, nLabelAddr;
		nLabelAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		InitializeExpress(&parser->oSymbolStack, &oExp);
		
		if (Get(parser)->kind == 12) {
			Accept(parser);
			nError = WriteDefault(&parser->oSwitchStack, nLabelAddr, False); 
		} else if (Get(parser)->kind == 8) {
			Accept(parser);
			Expression(parser,&oExp, False);
			CheckCaseExpress(&oExp);
			nError = WriteCase(&parser->oSwitchStack, &parser->oSymbolStack.oConstSegment,
				&parser->oSymbolStack.oConstTable, nLabelAddr, &oExp, nSwitchExpressType);
			
		} else SynErr(parser, 91);
		if(nError)
		SetError(parser, 0, "redefine case label");
		DestroyExpress(&oExp);
		
		Expect(parser, 78);
}

static void ForInitializer(CJcParser* parser) {
		CJcTypeInfo oType;
		jc_uint nQual = JC_NONE, nStorage = JC_NONE;
		InitializeType0(&oType);
		
		if (StartOf(parser, 2)) {
			StorageSpecifier(parser,&nStorage, False);
		}
		if (Get(parser)->kind == 10 || Get(parser)->kind == 38) {
			QualSpecifier(parser,&nQual);
		}
		TypeSpecifier(parser,&nQual, &oType, False);
		ObjectsSpecifier(parser,nStorage, &oType, False);
		ClearType(&oType); 
}

static void StorageSpecifier(CJcParser* parser,jc_uint* pStorage, jc_bool bAllowTypedef) {
		switch (Get(parser)->kind) {
		case 27: {
			Accept(parser);
			if(parser->oSymbolStack.nLevel == 1)
			pStorage[0] = JC_SHARE;
			else
				SetError(parser, 0, "'share' can be only used in the global region");
			
			break;
		}
		case 34: {
			Accept(parser);
			if(bAllowTypedef)
			pStorage[0] = JC_TYPEDEF;
			else
				SetError(parser, 0, "here doesn't allow 'typedef'");
			
			break;
		}
		case 6: {
			Accept(parser);
			if(parser->oSymbolStack.nLevel>1)
			pStorage[0] = JC_AUTO;
			else
				SetError(parser, 0, "'auto' can be only used in the local region");
			
			break;
		}
		case 25: {
			Accept(parser);
			if(parser->oSymbolStack.nLevel>1)
			pStorage[0] = JC_REGISTER;
			else
				SetError(parser, 0, "'register' can be only used in the local region");
			
			break;
		}
		case 31: {
			Accept(parser);
			pStorage[0] = JC_STATIC; 
			break;
		}
		case 17: {
			Accept(parser);
			pStorage[0] = JC_EXTERN; 
			if (Get(parser)->kind == 21 || Get(parser)->kind == 27) {
				if (Get(parser)->kind == 21) {
					Accept(parser);
					pStorage[0] |= JC_HOST; 
				} else {
					Accept(parser);
					pStorage[0] |= JC_SHARE; 
				}
			}
			break;
		}
		default: SynErr(parser, 92); break;
		}
}

static void QualSpecifier(CJcParser* parser,jc_uint* pQual) {
		if (Get(parser)->kind == 10) {
			ConstQualSpecifier(parser,pQual);
			if (Get(parser)->kind == 38) {
				VolatileQualSpecifier(parser,pQual);
			}
		} else if (Get(parser)->kind == 38) {
			VolatileQualSpecifier(parser,pQual);
			if (Get(parser)->kind == 10) {
				ConstQualSpecifier(parser,pQual);
			}
		} else SynErr(parser, 93);
}

static void TypeSpecifier(CJcParser* parser,jc_uint* pQual, CJcTypeInfo * pType, jc_bool nAllowBody) {
		*pQual = JC_NONE; 
		if (StartOf(parser, 6)) {
			SimpleSpecifier(parser,pQual, pType);
		} else if (Get(parser)->kind == 32 || Get(parser)->kind == 35) {
			StructSpecifier(parser,pQual, pType, nAllowBody);
		} else if (Get(parser)->kind == 16) {
			EnumSpecifier(parser,pQual, pType, nAllowBody);
		} else if (Get(parser)->kind == 1) {
			UserTypeSpecifier(parser,pQual, pType);
		} else SynErr(parser, 94);
		while (Get(parser)->kind == 67) {
			PointerTypeSpecifier(parser,pType);
		}
}

static void ObjectsSpecifier(CJcParser* parser,jc_uint nStorage, CJcTypeInfo* pType, jc_bool bAllowFunctionDefine) {
		jc_bool bMeetEnd=False; 
		ObjectsSpecifier2(parser,nStorage, pType, bAllowFunctionDefine, &bMeetEnd, False);
		while (!bMeetEnd) {
			ObjectsSpecifier2(parser,nStorage, pType, bAllowFunctionDefine, &bMeetEnd, True);
		}
}

static void ConstQualSpecifier(CJcParser* parser,jc_uint* pQual) {
		Expect(parser, 10);
		if(pQual[0] & JC_CONST)
		SetError(parser, 0, "repeated const keyword");
		else
			pQual[0] |= JC_CONST;
		
}

static void VolatileQualSpecifier(CJcParser* parser,jc_uint* pQual) {
		Expect(parser, 38);
		if(pQual[0] & JC_VOLATILE)
		SetError(parser, 0, "repeated volatile keyword");
		else
			pQual[0] |= JC_VOLATILE;
		
}

static void SimpleSpecifier(CJcParser* parser,jc_uint* pQual, CJcTypeInfo* pType) {
		jc_uint nType=JC_SIGNED_INT, bSigned = 1; 
		switch (Get(parser)->kind) {
		case 18: {
			Accept(parser);
			nType = JC_FLOAT; 
			break;
		}
		case 14: {
			Accept(parser);
			nType = JC_DOUBLE; 
			break;
		}
		case 37: {
			Accept(parser);
			nType = JC_VOID; 
			break;
		}
		case 24: {
			Accept(parser);
			nType = JC_SIGNED_LONG; 
			if (Get(parser)->kind == 14 || Get(parser)->kind == 23 || Get(parser)->kind == 24) {
				if (Get(parser)->kind == 14) {
					Accept(parser);
					nType = JC_DOUBLE; 
				} else if (Get(parser)->kind == 23) {
					Accept(parser);
				} else {
					Accept(parser);
					if (Get(parser)->kind == 23) {
						Accept(parser);
					}
				}
			}
			break;
		}
		case 29: case 36: {
			if (Get(parser)->kind == 29) {
				Accept(parser);
				bSigned = 1; nType = JC_SIGNED_INT; 
			} else {
				Accept(parser);
				bSigned = 0; nType = JC_UNSIGNED_INT; 
			}
			if (Get(parser)->kind == 24) {
				Accept(parser);
				if (Get(parser)->kind == 23 || Get(parser)->kind == 24) {
					if (Get(parser)->kind == 23) {
						Accept(parser);
					} else {
						Accept(parser);
						if (Get(parser)->kind == 23) {
							Accept(parser);
						}
					}
				}
				nType = (bSigned)?JC_SIGNED_LONG:JC_UNSIGNED_LONG; 
			} else if (Get(parser)->kind == 9 || Get(parser)->kind == 23 || Get(parser)->kind == 28) {
				IntegerSpecifier(parser,bSigned, &nType);
			} else SynErr(parser, 95);
			break;
		}
		case 9: case 23: case 28: {
			IntegerSpecifier(parser,bSigned, &nType);
			break;
		}
		default: SynErr(parser, 96); break;
		}
		if (Get(parser)->kind == 10 || Get(parser)->kind == 38) {
			QualSpecifier(parser,pQual);
		}
		ClearType(pType);
		InitializeType(pType, *pQual, nType, NULL, NULL, NULL);
		
}

static void StructSpecifier(CJcParser* parser,jc_uint* pQual, CJcTypeInfo* pType, jc_bool nAllowBody) {
		jc_uint nError, nRet;
		CJcSymbol oSymbol;
		CJcStructInfo oStructInfo;
		jc_uint nStructType;
		CJcString oFullName, oTypeName, oStructName;
		jc_bool bDefined = False, bHasName = False, bHasBody = False;
		InitializeString(&oFullName);
		InitializeString(&oTypeName);
		InitializeString(&oStructName);
		InitializeStructInfo(&oStructInfo);
		
		if (Get(parser)->kind == 32) {
			Accept(parser);
			nStructType = JC_STRUCT; 
		} else if (Get(parser)->kind == 35) {
			Accept(parser);
			nStructType = JC_UNION; 
		} else SynErr(parser, 97);
		CoverString(&oStructName, parser->t->val); 
		if (!nAllowBody) {
			Expect(parser, 1);
			bHasName = True; 
		} else if (StartOf(parser, 7)) {
			if (Get(parser)->kind == 1) {
				Accept(parser);
				bHasName = True; 
			}
		} else SynErr(parser, 98);
		if(bHasName){
		CoverString(&oTypeName, parser->t->val);
		AppendString(&oFullName, oStructName.pStr);
		AppendString(&oFullName, (jc_char*)" ");
		AppendString(&oFullName, oTypeName.pStr);
		}
		if(!nAllowBody)
			goto nobody;
		
		if (!bHasName) {
			StructBodySpecifier(parser,nStructType, &oStructInfo);
			bHasBody = True; 
		} else if (StartOf(parser, 7)) {
			if (Get(parser)->kind == 73) {
				StructBodySpecifier(parser,nStructType, &oStructInfo);
				bHasBody = True; 
			}
		} else SynErr(parser, 99);
		if(bHasBody){
		if(bHasName){
			if(!FindSymbol(&parser->oSymbolStack, oFullName.pStr, 0, &oSymbol)){
				CJcTypeInfo* pTmpType = oSymbol.info.pType;
				if(StructImplemented(GetStructInfo(pTmpType)))
					SetError(parser, 0, "redefine %s symbol '%s'", oStructName.pStr, oTypeName.pStr);
				else{
					CJcStructInfo* pDstInfo = GetStructInfo(pTmpType);
					ClearStructInfo(pDstInfo);
					*pDstInfo = oStructInfo;
				}
				ClearType(pType);
				InitializeType(pType, *pQual, JC_ALIAS, oFullName.pStr, pTmpType, NULL);
				bDefined = True;
			}
		}
		if(!bDefined){
			CJcTypeInfo * pTmpType;
			CJcStructInfo* pStructInfo = New(CJcStructInfo);
			*pStructInfo = oStructInfo;
			pTmpType = CreateType(*pQual, nStructType, oTypeName.pStr, NULL, pStructInfo);
			if(oTypeName.pStr){
				if(!CreateTypeSymbol(&parser->oSymbolStack, oFullName.pStr, pTmpType))
					SetError(parser, 0, "redefine %s symbol '%s'", oStructName.pStr, oTypeName.pStr);
				else{
					pTmpType = CreateType(*pQual, JC_ALIAS, oFullName.pStr, pTmpType, NULL);
					if(CreateTypeSymbol(&parser->oSymbolStack, oTypeName.pStr, pTmpType))
						pTmpType = CloneType(pTmpType);
				}
			}
			ClearType(pType);
			*pType = *pTmpType;
			g_oInterface.Free(pTmpType);
		}
		}
		else{
			nobody:
			nError = 0;
			nRet = FindSymbol(&parser->oSymbolStack, oFullName.pStr, 0, &oSymbol);
			if(nRet){
				nRet = FindSymbol(&parser->oSymbolStack, oTypeName.pStr, 0, &oSymbol);
				if(!nRet){
					nError = 1;
					SetError(parser, 0, "redefine symbol '%s'", oTypeName.pStr);
				}
				else
					nRet = FindSymbol(&parser->oSymbolStack, oFullName.pStr, 1, &oSymbol);
			}
			if(!nRet){
				ClearType(pType);
				InitializeType(pType, *pQual, JC_ALIAS, oFullName.pStr, oSymbol.info.pType, NULL);
			}
			else if(!nError){
				CJcTypeInfo* pTmpType = CreateType(JC_NONE, nStructType, oTypeName.pStr, NULL, CreateStructInfo());
				CreateTypeSymbol(&parser->oSymbolStack, oFullName.pStr, pTmpType);
				pTmpType = CreateType(*pQual, JC_ALIAS, oFullName.pStr, pTmpType, NULL);
				CreateTypeSymbol(&parser->oSymbolStack, oTypeName.pStr, pTmpType);
				CoverType(pType, pTmpType);
			}
			else{
				ClearType(pType);
				InitializeType(pType, JC_NONE, nStructType, oTypeName.pStr, NULL, CreateStructInfo());
			}
		}
		ClearString(&oFullName);
		ClearString(&oTypeName);
		ClearString(&oStructName);
		
}

static void EnumSpecifier(CJcParser* parser,jc_uint* pQual, CJcTypeInfo* pType, jc_bool nAllowBody) {
		jc_uint nError, nRet;
		CJcString oFullName, oTypeName;
		CJcEnumInfo oEnumInfo;
		CJcSymbol oSymbol;
		jc_bool bDefined = False, bHasName = False, bHasBody = False;
		InitializeString(&oFullName);
		InitializeString(&oTypeName);
		oEnumInfo.nImplemented = 0;
		oEnumInfo.nNextValue = 0;
		
		Expect(parser, 16);
		if (!nAllowBody) {
			Expect(parser, 1);
			bHasName = True; 
		} else if (StartOf(parser, 7)) {
			if (Get(parser)->kind == 1) {
				Accept(parser);
				bHasName = True; 
			}
		} else SynErr(parser, 100);
		if(bHasName){
		CoverString(&oTypeName, parser->t->val);
		AppendString(&oFullName, "enum ");
		AppendString(&oFullName, oTypeName.pStr);
		}
		if(!nAllowBody)
			goto nobody;
		
		if (!bHasName) {
			EnumBodySpecifier(parser,&oEnumInfo, oTypeName.pStr);
			bHasBody = True; 
		} else if (StartOf(parser, 7)) {
			if (Get(parser)->kind == 73) {
				EnumBodySpecifier(parser,&oEnumInfo, oTypeName.pStr);
				bHasBody = True; 
			}
		} else SynErr(parser, 101);
		if(bHasBody){
		if(bHasName){
			if(!FindSymbol(&parser->oSymbolStack, oFullName.pStr, 0, &oSymbol)){
				CJcTypeInfo* pTmpType = oSymbol.info.pType;
				if(EnumImplemented(GetEnumInfo(pTmpType)))
					SetError(parser, 0, "redefine enum symbol '%s'", oTypeName.pStr);
				else
					*GetEnumInfo(pTmpType) = oEnumInfo;
				ClearType(pType);
				InitializeType(pType, *pQual, JC_ALIAS, oFullName.pStr, pTmpType, NULL);
				bDefined = True;
			}
		}
		if(!bDefined){
			CJcTypeInfo * pTmpType = CreateType(*pQual, JC_ENUM, oTypeName.pStr, NULL, CloneEnumInfo(&oEnumInfo));
			if(oTypeName.pStr){
				if(!CreateTypeSymbol(&parser->oSymbolStack, oFullName.pStr, pTmpType))
					SetError(parser, 0, "redefine enum symbol '%s'", oTypeName.pStr);
				else{
					pTmpType = CreateType(*pQual, JC_ALIAS, oFullName.pStr, pTmpType, NULL);
					if(CreateTypeSymbol(&parser->oSymbolStack, oTypeName.pStr, pTmpType))
						pTmpType = CloneType(pTmpType);
				}
			}
			ClearType(pType);
			*pType = *pTmpType;
			g_oInterface.Free(pTmpType);
		}
		}
		else{
			nobody:
			nError = 0;
			nRet = FindSymbol(&parser->oSymbolStack, oFullName.pStr, 0, &oSymbol);
			if(nRet){
				nRet = FindSymbol(&parser->oSymbolStack, oTypeName.pStr, 0, &oSymbol);
				if(!nRet){
					nError = 1;
					SetError(parser, 0, "redefine symbol '%s'", oTypeName.pStr);
				}
				else
					nRet = FindSymbol(&parser->oSymbolStack, oFullName.pStr, 1, &oSymbol);
			}
			if(!nRet){
				ClearType(pType);
				InitializeType(pType, *pQual, JC_ALIAS, oFullName.pStr, oSymbol.info.pType, NULL);
			}
			else if(!nError){
				CJcTypeInfo* pTmpType = CreateType(JC_NONE, JC_ENUM, oTypeName.pStr, NULL, CreateEnumInfo());
				CreateTypeSymbol(&parser->oSymbolStack, oFullName.pStr, pTmpType);
				pTmpType = CreateType(*pQual, JC_ALIAS, oFullName.pStr, pTmpType, NULL);
				CreateTypeSymbol(&parser->oSymbolStack, oTypeName.pStr, pTmpType);
				ClearType(pType);
				*pType = *pTmpType;
			}
			else{
				ClearType(pType);
				InitializeType(pType, JC_NONE, JC_ENUM, oTypeName.pStr, NULL, CreateEnumInfo());
			}
		}
		ClearString(&oFullName);
		ClearString(&oTypeName);
		
}

static void UserTypeSpecifier(CJcParser* parser,jc_uint* pQual, CJcTypeInfo* pType) {
		CJcSymbol oSymbol;
		CJcTypeInfo* pOldType = NULL;
		
		Expect(parser, 1);
		if(!FindSymbol(&parser->oSymbolStack, parser->t->val, 1, &oSymbol)){
		if(oSymbol.nSymbol == JC_TYPE_SYMBOL)
			pOldType = oSymbol.info.pType;
		}
		if(!pOldType){
			SetError(parser, 0, "invalid type symbol '%s'", parser->t->val);
			return;
		}
		
		if (Get(parser)->kind == 10 || Get(parser)->kind == 38) {
			QualSpecifier(parser,pQual);
		}
		pQual[0] |= GetQualCode(pOldType);
		if(GetTypeCode(pOldType) == JC_ALIAS){
			CoverType(pType, pOldType);
			SetQualCode(pType, *pQual);
		}
		else{
			ClearType(pType);
			InitializeType(pType, pQual[0], JC_ALIAS, NULL, pOldType, NULL);
		}
		
}

static void PointerTypeSpecifier(CJcParser* parser,CJcTypeInfo * pType) {
		jc_uint nQual = JC_NONE;
		CJcTypeInfo oTmpType;
		
		Expect(parser, 67);
		if (Get(parser)->kind == 10 || Get(parser)->kind == 38) {
			QualSpecifier(parser,&nQual);
		}
		InitializeType(&oTmpType, nQual, JC_POINTER, NULL, CloneType(pType), NULL);
		ClearType(pType);
		*pType = oTmpType;
		
}

static void IntegerSpecifier(CJcParser* parser,jc_uint bSigned, jc_uint* pType) {
		if (Get(parser)->kind == 9) {
			Accept(parser);
			pType[0] = (bSigned)?JC_SIGNED_CHAR:JC_UNSIGNED_CHAR; 
		} else if (Get(parser)->kind == 28) {
			Accept(parser);
			if (Get(parser)->kind == 23) {
				Accept(parser);
			}
			pType[0] = (bSigned)?JC_SIGNED_SHORT:JC_UNSIGNED_SHORT; 
		} else if (Get(parser)->kind == 23) {
			Accept(parser);
			pType[0] = (bSigned)?JC_SIGNED_INT:JC_UNSIGNED_INT; 
		} else SynErr(parser, 102);
}

static void EnumBodySpecifier(CJcParser* parser,CJcEnumInfo* pEnumInfo, char* pEnumName) {
		jc_bool bEnd = False; 
		Expect(parser, 73);
		if (Get(parser)->kind == 1) {
			EnumMemberSpecifier(parser,pEnumInfo, pEnumName);
			while (Get(parser)->kind == 82) {
				Accept(parser);
				if (Get(parser)->kind == 1) {
					EnumMemberSpecifier(parser,pEnumInfo, pEnumName);
				} else if (Get(parser)->kind == 74) {
					Accept(parser);
					bEnd = True; 
				} else SynErr(parser, 103);
			}
		}
		if (bEnd) {
			; 
		} else if (Get(parser)->kind == 74) {
			Accept(parser);
		} else SynErr(parser, 104);
		FinishEnumDefine(pEnumInfo); 
}

static void EnumMemberSpecifier(CJcParser* parser,CJcEnumInfo* pEnumInfo, char* pEnumName) {
		CJcExpress oExp;
		CJcString oConstName;
		jc_int nLine, nCol, nNextValue;
		jc_bool bHasError = False;
		InitializeString(&oConstName);
		InitializeExpress(&parser->oSymbolStack, &oExp);
		nNextValue = GetNextEnumValue(pEnumInfo);
		
		Expect(parser, 1);
		CoverString(&oConstName, parser->t->val);
		nLine = parser->t->line;
		nCol = parser->t->col;
		
		if (Get(parser)->kind == 72) {
			Accept(parser);
			Expression(parser,&oExp, False);
			if(GetEnumValue(&oExp, &nNextValue)){
			bHasError = True;
			SetError(parser, 0, "invalid enum const");
			nNextValue = GetNextEnumValue(pEnumInfo);
			}
			
		}
		UseEnumValue(pEnumInfo, nNextValue);
		if(!bHasError){
			CJcSymbol* pSymbol;
			CJcTypeInfo oEnumType;
			InitializeType(&oEnumType, JC_NONE, JC_ENUM, pEnumName, NULL, CloneEnumInfo(pEnumInfo));
			pSymbol = CreateEnumSymbol(&parser->oSymbolStack, oConstName.pStr, &oEnumType, nNextValue);
			if(!pSymbol)
				SetError(parser, 0, "redefine symbol '%s'", oConstName.pStr);
			ClearType(&oEnumType);
		}
		DestroyExpress(&oExp);
		ClearString(&oConstName);
		
}

static void StructBodySpecifier(CJcParser* parser,jc_uint nStructType, CJcStructInfo* pStructInfo) {
		Expect(parser, 73);
		while (StartOf(parser, 8)) {
			StructMemberSpecifier(parser,nStructType, pStructInfo);
		}
		Expect(parser, 74);
		FinishStructDefine(pStructInfo); 
}

static void StructMemberSpecifier(CJcParser* parser,jc_uint nStructType, CJcStructInfo* pStructInfo) {
		jc_uint nQual = JC_NONE;
		CJcTypeInfo oFieldType;
		InitializeType0(&oFieldType);
		
		TypeSpecifier(parser,&nQual, &oFieldType, True);
		FieldsSpecifier(parser,&oFieldType, pStructInfo, nStructType);
}

static void FieldsSpecifier(CJcParser* parser,CJcTypeInfo* pFieldType, CJcStructInfo* pStructInfo, jc_uint nStructType) {
		jc_bool bMeetEnd = False; 
		FieldsSpecifier2(parser,pFieldType, pStructInfo, nStructType, &bMeetEnd, False);
		while (!bMeetEnd) {
			FieldsSpecifier2(parser,pFieldType, pStructInfo, nStructType, &bMeetEnd, True);
		}
}

static void FieldsSpecifier2(CJcParser* parser,CJcTypeInfo* pFieldType, CJcStructInfo* pStructInfo, jc_uint nStructType, jc_bool* pMeetEnd, jc_bool bRemovePointer) {
		CJcString oName;
		jc_int nRet, nBitCount = 0;
		CJcExpress oExp;
		CJcTypeInfo *pNewType = CloneType(pFieldType);
		InitializeString(&oName);
		InitializeExpress(&parser->oSymbolStack, &oExp);
		if(bRemovePointer)
			RemovePointer(pNewType);
		
		ObjectSpecifier(parser,JC_NONE, &oName, pNewType, False);
		if (Get(parser)->kind == 78) {
			Accept(parser);
			Expression(parser,&oExp, False);
			if(GetEnumValue(&oExp, &nBitCount))
			SetError(parser, 0, "invalid bit field express");
			
		}
		if (Get(parser)->kind == 82) {
			Accept(parser);
		} else if (Get(parser)->kind == 79) {
			Accept(parser);
			*pMeetEnd = True; 
		} else SynErr(parser, 105);
		nRet = AddStructField(pStructInfo, nStructType, oName.pStr, pNewType, (jc_uint)nBitCount);
		if(nRet == 1)
			SetError(parser, 0, "invalid field '%s' definition", oName.pStr);
		else if(nRet == 2)
			SetError(parser, 0, "redefine field '%s'", oName.pStr);
		ClearString(&oName);
		DestroyExpress(&oExp);
		
}

static void ObjectSpecifier(CJcParser* parser,jc_uint nStorage, CJcString* pName, CJcTypeInfo* pType, jc_bool bAllowFunctionDeclare) {
		jc_bool bMeetFunction = False;
		CJcTypeInfo* pOldType, oTmpType, *pTmp, *pPrev, *pPara;
		
		while (Get(parser)->kind == 67) {
			PointerTypeSpecifier(parser,pType);
		}
		if (Get(parser)->kind == 68) {
			Accept(parser);
			pOldType = CloneType(pType);
			InitializeType(&oTmpType, JC_NONE, JC_ALIAS, NULL, pOldType, NULL);
			
			PointerTypeSpecifier(parser,&oTmpType);
			ObjectSpecifier(parser,nStorage, pName, &oTmpType, bAllowFunctionDeclare);
			pPara = NULL;
			pTmp = &oTmpType;
			while(pTmp){
				if(pTmp->pNext == pOldType){
					pPrev->pNext = pOldType;
					g_oInterface.Free(pTmp);
					break;
				}
				if(pTmp->nType == JC_FUNCTION){
					CJcFunctionInfo* pFunction = GetFunctionInfo(pTmp);
					pTmp = GetFunctionType(pFunction);
					if(GetTypeCode(GetOrigType(pTmp)) != JC_VOID)
						pPara = GetFunctionPara(pFunction, 0)->pType;
				}
				else{
					pPrev = pTmp;
					pTmp = pTmp->pNext;
				}
			}
			while(pPara){
				if(pPara->pNext == pOldType){
					g_oInterface.Free(pPara);
					pPara = pPrev;
					break;
				}
				pPrev = pPara;
				pPara = pPara->pNext;
			}
			ClearType(pType);
			*pType = oTmpType;
			
			Expect(parser, 69);
			if (Get(parser)->kind == 68) {
				FunctionDeclare(parser,pOldType);
			} else if (Get(parser)->kind == 75) {
				ArrayDeclare(parser,pOldType);
			} else SynErr(parser, 106);
			if(pPara)
			pPara->pNext = CloneType(pOldType);
			
		} else if (Get(parser)->kind == 1) {
			Accept(parser);
			CoverString(pName, parser->t->val);
			if(bAllowFunctionDeclare && !(nStorage&(JC_AUTO|JC_REGISTER)) ){
				if(Get(parser)->kind == parser->_lparentheses)
					bMeetFunction = True;
			}
			
			if (bMeetFunction) {
				FunctionDeclare(parser,pType);
			} else if (StartOf(parser, 9)) {
				if (Get(parser)->kind == 75) {
					ArrayDeclare(parser,pType);
				}
			} else SynErr(parser, 107);
		} else SynErr(parser, 108);
}

static void ObjectsSpecifier2(CJcParser* parser,jc_uint nStorage, CJcTypeInfo* pType, jc_bool bAllowFunctionDefine, jc_bool* pMeetEnd, jc_bool bRemovePointer) {
		CJcString oName;
		CJcSymbol* pSymbol = NULL;
		jc_bool bMustCommaOrScolon = False;
		CJcTypeInfo *pTmpType, *pNewType = CloneType(pType);
		InitializeString(&oName);
		if(bRemovePointer)
			RemovePointer(pNewType);
		
		ObjectSpecifier(parser,nStorage, &oName, pNewType, True);
		pTmpType = CloneType(pNewType);
		pSymbol = CreateSymbol(&parser->oSymbolStack, nStorage, oName.pStr, pTmpType);
		if(!pSymbol){
			DestroyType(pTmpType);
		}
		if(nStorage&JC_TYPEDEF)
			bMustCommaOrScolon = True;
		else if(GetTypeCode(pNewType)==JC_FUNCTION){
			if(!bAllowFunctionDefine)
				bMustCommaOrScolon = True;
		}
		else if(nStorage&JC_EXTERN)
			bMustCommaOrScolon = True;
		
		if (bMustCommaOrScolon) {
			if (Get(parser)->kind == 82) {
				Accept(parser);
			} else if (Get(parser)->kind == 79) {
				Accept(parser);
				*pMeetEnd = True; 
			} else SynErr(parser, 109);
		} else if (Get(parser)->kind == 82) {
			Accept(parser);
		} else if (Get(parser)->kind == 79) {
			Accept(parser);
			*pMeetEnd = True; 
		} else if (GetTypeCode(pNewType)==JC_FUNCTION) {
			FunctionDefine2(parser,pSymbol);
			*pMeetEnd = True; 
		} else if (Get(parser)->kind == 72) {
			InitializeVariable(parser,pSymbol, pMeetEnd);
		} else SynErr(parser, 110);
		ClearString(&oName);
		DestroyType(pNewType);
		
}

static void FunctionDefine2(CJcParser* parser,CJcSymbol* pSymbol) {
		jc_uint nEndAddr=0, nInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		parser->pFunction = pSymbol->info.pFunction;
		Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nEndAddr, JC_DEFAULT_INSADDR);
		
		CompoundStatement(parser);
		nEndAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmp, (jc_uchar)JC_UN, nEndAddr, nInsAddr);
		FunctionDefine(&parser->oSymbolStack, pSymbol, nInsAddr+8);
		
}

static void InitializeVariable(CJcParser* parser,CJcSymbol* pSymbol, jc_bool* pMeetEnd) {
		CJcVariableInfo* pVariable = pSymbol->info.pVariable; 
		Expect(parser, 72);
		InitializerExpr(parser,pVariable);
		if (Get(parser)->kind == 82) {
			Accept(parser);
		} else if (Get(parser)->kind == 79) {
			Accept(parser);
			*pMeetEnd = True; 
		} else SynErr(parser, 111);
}

static void FunctionDeclare(CJcParser* parser,CJcTypeInfo* pType) {
		jc_uint i=0, nQual;
		jc_bool bEnd = False, bAddPara;
		CJcString oParaName;
		CJcTypeInfo oParaType;
		CJcFunctionInfo* pFunction = CreateFunctionInfo(CloneType(pType));
		ClearType(pType);
		InitializeType(pType, JC_NONE, JC_FUNCTION, NULL, NULL, pFunction);
		InitializeType0(&oParaType);
		InitializeString(&oParaName);
		
		Expect(parser, 68);
		while (!bEnd) {
			nQual = JC_NONE;
			bAddPara = True;
			
			if (Get(parser)->kind == 82) {
				Accept(parser);
				bAddPara=False; continue; 
			} else if (Get(parser)->kind == 69) {
				Accept(parser);
				bAddPara=False; bEnd=True; continue; 
			} else if (Get(parser)->kind == 40) {
				Accept(parser);
				Expect(parser, 69);
				CoverString(&oParaName, "..."); bEnd=True; 
			} else if (StartOf(parser, 10)) {
				if (Get(parser)->kind == 25) {
					Accept(parser);
				}
				if (Get(parser)->kind == 10 || Get(parser)->kind == 38) {
					QualSpecifier(parser,&nQual);
				}
				TypeSpecifier(parser,&nQual, &oParaType, False);
				if(!i && nQual == JC_NONE &&
				GetTypeCode(GetOrigType(&oParaType)) == JC_VOID &&
				Get(parser)->kind == parser->_rparentheses){
				Accept(parser);
				ClearType(&oParaType);
				break;
				}
				
				if (Get(parser)->kind == 1 || Get(parser)->kind == 67 || Get(parser)->kind == 68) {
					ParaSpecifier(parser,&oParaName, &oParaType);
				}
			} else SynErr(parser, 112);
			if(bAddPara){
			CJcTypeInfo* pParaType = CloneType(&oParaType);
			jc_uint nRet = AddFunctionPara(pFunction, oParaName.pStr, pParaType);
			if(nRet){
				if(nRet == 1)
					SetError(parser, 0, "invalid parameter '%s' definition", oParaName.pStr);
				else
					SetError(parser, 0, "redefine parameter '%s'", oParaName.pStr);
				DestroyType(pParaType);
			}
			}
			ClearType(&oParaType);
			ClearString(&oParaName);
			++i;
			
		}
}

static void ArrayDeclare(CJcParser* parser,CJcTypeInfo* pType) {
		jc_int i, nDim = 0, nCount=0;
		CJcExpress oExp;
		CJcTypeInfo oNewType;
		InitializeExpress(&parser->oSymbolStack, &oExp);
		
		Expect(parser, 75);
		if (StartOf(parser, 4)) {
			Expression(parser,&oExp, False);
			if(GetEnumValue(&oExp, &nDim))
			SetError(parser, 0, "invalid array subscript express");
			DestroyExpress(&oExp);
			
		}
		Expect(parser, 76);
		InitializeType(&oNewType, JC_NONE, JC_ARRAY, NULL, CloneType(pType), CreateArrayInfo((jc_uint)nDim));
		ClearType(pType);
		*pType = oNewType;
		++nCount;
		
		while (Get(parser)->kind == 75) {
			Accept(parser);
			Expression(parser,&oExp, False);
			if(GetEnumValue(&oExp, &nDim) || !nDim)
			SetError(parser, 0, "invalid array subscript express");
			DestroyExpress(&oExp);
			
			Expect(parser, 76);
			InitializeType(&oNewType, JC_NONE, JC_ARRAY, NULL, CloneType(pType), CreateArrayInfo((jc_uint)nDim));
			ClearType(pType);
			*pType = oNewType;
			++nCount;
			
		}
		if(nCount > 1){
		CJcTypeInfo* pTmp, **pArray, *pLast;
		pTmp = CloneType(pType);
		ClearType(pType);
		pArray = New2(CJcTypeInfo*, nCount);
		for(i=0; i<nCount; ++i){
			pArray[i] = pTmp;
			pTmp = pTmp->pNext;
		}
		pTmp = pArray[nCount-1];
		pLast = pTmp->pNext;
		for(i=nCount-1; i>0; --i){
			pArray[i]->pNext = pArray[i-1];
		}
		pArray[i]->pNext = pLast;
		*pType = *pTmp;
		g_oInterface.Free(pTmp);
		g_oInterface.Free(pArray);
		}
		
}

static void ParaSpecifier(CJcParser* parser,CJcString* pName, CJcTypeInfo* pParaType) {
		ObjectSpecifier(parser,JC_NONE, pName, pParaType, False);
}

static void InitializerExpr(CJcParser* parser,CJcVariableInfo* pVariable) {
		CJcExpress oExp;
		jc_uint nControlVar;
		jc_uint nControlPos, nControlEnd = 0;
		jc_uint nOpt = pVariable->nOpt;
		jc_uint nArg = pVariable->nArg;
		CJcTypeInfo* pType = pVariable->pType;
		InitializeExpress(&parser->oSymbolStack, &oExp);
		oExp.pType = pVariable->pType;
		oExp.nOpt = pVariable->nOpt;
		oExp.nArg = pVariable->nArg;
		oExp.nLeftValue = 1;
		if(pVariable->nOpt == JC_DS && parser->oSymbolStack.pInFunction){
			CJcExpress oLExp = BuildCharConstantExpress(&parser->oSymbolStack, "'1'");
			CJcExpress oRExp = BuildCharConstantExpress(&parser->oSymbolStack, "'1'");
			nControlVar = AllocVarInStack(&parser->oSymbolStack.oDataSegment, 1, 1, NULL);
			nControlPos = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
			Emit2(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmptc, (jc_uchar)JC_DS, nControlVar, (jc_uchar)JC_UN, nControlEnd, JC_DEFAULT_INSADDR);
			oLExp.nOpt = JC_DS;
			oLExp.nArg = nControlVar;
			oLExp.nLeftValue = 1;
			SaveExpress(&oLExp, &oRExp);
			DestroyExpress(&oLExp);
			DestroyExpress(&oRExp);
		}
		
		InitializeExpressA(parser,&oExp);
		if(pVariable->nOpt == JC_DS && parser->oSymbolStack.pInFunction){
		nControlEnd = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		Emit2(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_jmptc, (jc_uchar)JC_DS, nControlVar, (jc_uchar)JC_UN, nControlEnd, nControlPos);
		}
		
}

static void PrimaryExpr(CJcParser* parser,CJcExpress* pExp) {
		CJcSymbol oSymbol; 
		switch (Get(parser)->kind) {
		case 68: {
			Accept(parser);
			Expression(parser,pExp, True);
			Expect(parser, 69);
			break;
		}
		case 1: {
			Accept(parser);
			DestroyExpress(pExp);
			if(FindSymbol(&parser->oSymbolStack, parser->t->val, 1, &oSymbol)){
				SetError(parser, 0, "undefine symbol '%s'", parser->t->val);
				*pExp = BuildIntegerConstantExpressA(&parser->oSymbolStack, 0, False);
				return;
			}
			if(oSymbol.nSymbol == JC_TYPE_SYMBOL){
				SetError(parser, 0, "the symbol '%s' is type, not express", parser->t->val);
				*pExp = BuildIntegerConstantExpressA(&parser->oSymbolStack, 0, False);
				return;
			}
			*pExp = BuildSymbolExpress(&parser->oSymbolStack, &oSymbol);
			
			break;
		}
		case 2: {
			Accept(parser);
			DestroyExpress(pExp);
			*pExp = BuildCharConstantExpress(&parser->oSymbolStack, parser->t->val);
			
			break;
		}
		case 3: {
			Accept(parser);
			DestroyExpress(pExp);
			*pExp = BuildStringConstantExpress(&parser->oSymbolStack, parser->t->val);
			
			break;
		}
		case 4: {
			Accept(parser);
			DestroyExpress(pExp);
			*pExp = BuildIntegerConstantExpress(&parser->oSymbolStack, parser->t->val);
			
			break;
		}
		case 5: {
			Accept(parser);
			DestroyExpress(pExp);
			*pExp = BuildFloatConstantExpress(&parser->oSymbolStack, parser->t->val);
			
			break;
		}
		default: SynErr(parser, 113); break;
		}
}

static void PostfixExpr(CJcParser* parser,CJcExpress* pExp) {
		jc_uint i = 1, nArgInsAddr, nArgSize;
		CJcExpress oArg;
		InitializeExpress(&parser->oSymbolStack, &oArg);
		
		PrimaryExpr(parser,pExp);
		while (StartOf(parser, 11)) {
			switch (Get(parser)->kind) {
			case 75: {
				Accept(parser);
				Expression(parser,&oArg, True);
				Expect(parser, 76);
				BuildArrayExpress(pExp, &oArg); DestroyExpress(&oArg); 
				break;
			}
			case 68: {
				Accept(parser);
				nArgInsAddr = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
				Emit1(&parser->oSymbolStack.oCodeSegment, (jc_ushort)jc_newarg, (jc_uchar)JC_UN, 0, JC_DEFAULT_INSADDR);
				nArgSize = 0;
				
				if (StartOf(parser, 4)) {
					AssignmentExpr(parser,&oArg);
					BuildArgumentExpress(pExp, &oArg, i, &nArgSize);
					DestroyExpress(&oArg);
					++i;
					
					while (Get(parser)->kind == 82) {
						Accept(parser);
						AssignmentExpr(parser,&oArg);
						BuildArgumentExpress(pExp, &oArg, i, &nArgSize);
						DestroyExpress(&oArg);
						++i;
						
					}
				}
				Expect(parser, 69);
				BuildCallExpress(pExp, nArgSize, nArgInsAddr); 
				break;
			}
			case 83: {
				Accept(parser);
				Expect(parser, 1);
				BuildFieldExpress(pExp, parser->t->val, False); 
				break;
			}
			case 59: {
				Accept(parser);
				Expect(parser, 1);
				BuildFieldExpress(pExp, parser->t->val, True); 
				break;
			}
			case 60: {
				Accept(parser);
				BuildIncreaseExpress(pExp, False, True); 
				break;
			}
			case 61: {
				Accept(parser);
				BuildIncreaseExpress(pExp, False, False); 
				break;
			}
			}
		}
}

static void AssignmentExpr(CJcParser* parser,CJcExpress* pExp) {
		jc_uint nOperator;
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		ConditionalExpr(parser,pExp);
		while (StartOf(parser, 12)) {
			switch (Get(parser)->kind) {
			case 72: {
				Accept(parser);
				nOperator = JC_ASSIGN_EQUAL; 
				break;
			}
			case 47: {
				Accept(parser);
				nOperator = JC_ASSIGN_MUL_EQ; 
				break;
			}
			case 51: {
				Accept(parser);
				nOperator = JC_ASSIGN_DIV_EQ; 
				break;
			}
			case 44: {
				Accept(parser);
				nOperator = JC_ASSIGN_MOD_EQ; 
				break;
			}
			case 49: {
				Accept(parser);
				nOperator = JC_ASSIGN_ADD_EQ; 
				break;
			}
			case 48: {
				Accept(parser);
				nOperator = JC_ASSIGN_SUB_EQ; 
				break;
			}
			case 42: {
				Accept(parser);
				nOperator = JC_ASSIGN_LSH_EQ; 
				break;
			}
			case 41: {
				Accept(parser);
				nOperator = JC_ASSIGN_RSH_EQ; 
				break;
			}
			case 46: {
				Accept(parser);
				nOperator = JC_ASSIGN_AND_EQ; 
				break;
			}
			case 45: {
				Accept(parser);
				nOperator = JC_ASSIGN_OR_EQ; 
				break;
			}
			case 50: {
				Accept(parser);
				nOperator = JC_ASSIGN_XOR_EQ; 
				break;
			}
			}
			AssignmentExpr(parser,&oSecondExp);
			BuildAssignmentExpress(pExp, &oSecondExp, nOperator, 1);
			DestroyExpress(&oSecondExp);
			
		}
}

static void UnaryExpr(CJcParser* parser,CJcExpress* pExp) {
		jc_uint nUnaryOperator;
		jc_uint nQual;
		CJcTypeInfo oType, *pType;
		CJcExpress oNewExp;
		CJcToken* pToken;
		jc_uint bSizeOf;
		jc_uint nPos = GetPosOfSegment(&parser->oSymbolStack.oCodeSegment);
		InitializeType0(&oType);
		
		if (StartOf(parser, 13)) {
			PostfixExpr(parser,pExp);
		} else if (Get(parser)->kind == 60) {
			Accept(parser);
			UnaryExpr(parser,pExp);
			BuildIncreaseExpress(pExp, True, True); 
		} else if (Get(parser)->kind == 61) {
			Accept(parser);
			UnaryExpr(parser,pExp);
			BuildIncreaseExpress(pExp, True, False); 
		} else if (StartOf(parser, 14)) {
			UnaryOperator(parser,&nUnaryOperator);
			CastExpr(parser,pExp);
			BuildUnaryExpress(pExp, nUnaryOperator); 
		} else if (Get(parser)->kind == 30 || Get(parser)->kind == 86) {
			if (Get(parser)->kind == 30) {
				Accept(parser);
				bSizeOf=1; 
			} else {
				Accept(parser);
				bSizeOf=0; 
			}
			ResetPeek(parser->scanner); 
			if (parser->_lparentheses == Get(parser)->kind && (pToken=PeekToken(parser->scanner)) &&
IsTypeSpecifier(&parser->oSymbolStack, pToken->kind, pToken->val)) {
				Expect(parser, 68);
				TypeSpecifier(parser,&nQual, &oType, False);
				Expect(parser, 69);
				pType = &oType; 
			} else if (StartOf(parser, 4)) {
				UnaryExpr(parser,pExp);
				pType = pExp->pType;
				SetPosOfSegment(&parser->oSymbolStack.oCodeSegment, nPos);
				CompactSegment(&parser->oSymbolStack.oCodeSegment);
				
			} else SynErr(parser, 114);
			oNewExp = BuildSizeExpress(&parser->oSymbolStack, pType, bSizeOf);
			DestroyExpress(pExp);
			*pExp = oNewExp;
			
		} else SynErr(parser, 115);
		ClearType(&oType);
		
}

static void UnaryOperator(CJcParser* parser,jc_uint* pUnaryOperator) {
		switch (Get(parser)->kind) {
		case 66: {
			Accept(parser);
			break;
		}
		case 67: {
			Accept(parser);
			break;
		}
		case 71: {
			Accept(parser);
			break;
		}
		case 70: {
			Accept(parser);
			break;
		}
		case 62: {
			Accept(parser);
			break;
		}
		case 63: {
			Accept(parser);
			break;
		}
		default: SynErr(parser, 116); break;
		}
		*pUnaryOperator = parser->t->val[0]; 
}

static void CastExpr(CJcParser* parser,CJcExpress* pExp) {
		jc_uint nQual;
		CJcToken* pToken;
		CJcTypeInfo oType;
		InitializeType0(&oType);
		nQual = JC_NONE;
		
		if (parser->_lparentheses == Get(parser)->kind &&
(pToken=PeekToken(parser->scanner)) &&
IsTypeSpecifier(&parser->oSymbolStack, pToken->kind, pToken->val)
 ) {
			Expect(parser, 68);
			if (Get(parser)->kind == 10 || Get(parser)->kind == 38) {
				QualSpecifier(parser,&nQual);
			}
			TypeSpecifier(parser,&nQual, &oType, False);
			Expect(parser, 69);
			Expression(parser,pExp, False);
			BuildCastExpress(pExp, &oType); 
		} else if (StartOf(parser, 4)) {
			UnaryExpr(parser,pExp);
		} else SynErr(parser, 117);
		ClearType(&oType); 
}

static void MultiplicativeExpr(CJcParser* parser,CJcExpress* pExp) {
		jc_uint nBinaryOp;
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		CastExpr(parser,pExp);
		while (Get(parser)->kind == 64 || Get(parser)->kind == 67 || Get(parser)->kind == 85) {
			if (Get(parser)->kind == 67) {
				Accept(parser);
				nBinaryOp = JC_BINARY_MUL; 
			} else if (Get(parser)->kind == 85) {
				Accept(parser);
				nBinaryOp = JC_BINARY_DIV; 
			} else {
				Accept(parser);
				nBinaryOp = JC_BINARY_MOD; 
			}
			CastExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, nBinaryOp);
			DestroyExpress(&oSecondExp);
			
		}
}

static void AdditiveExpr(CJcParser* parser,CJcExpress* pExp) {
		jc_uint nBinaryOp;
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		MultiplicativeExpr(parser,pExp);
		while (Get(parser)->kind == 70 || Get(parser)->kind == 71) {
			if (Get(parser)->kind == 71) {
				Accept(parser);
				nBinaryOp = JC_BINARY_ADD; 
			} else {
				Accept(parser);
				nBinaryOp = JC_BINARY_SUB; 
			}
			MultiplicativeExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, nBinaryOp);
			DestroyExpress(&oSecondExp);
			
		}
}

static void ShiftExpr(CJcParser* parser,CJcExpress* pExp) {
		jc_uint nBinaryOp;
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		AdditiveExpr(parser,pExp);
		while (Get(parser)->kind == 55 || Get(parser)->kind == 56) {
			if (Get(parser)->kind == 55) {
				Accept(parser);
				nBinaryOp = JC_BINARY_LSHIFT; 
			} else {
				Accept(parser);
				nBinaryOp = JC_BINARY_RSHIFT; 
			}
			AdditiveExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, nBinaryOp);
			DestroyExpress(&oSecondExp);
			
		}
}

static void RelationalExpr(CJcParser* parser,CJcExpress* pExp) {
		jc_uint nBinaryOp;
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		ShiftExpr(parser,pExp);
		while (StartOf(parser, 15)) {
			if (Get(parser)->kind == 80) {
				Accept(parser);
				nBinaryOp = JC_BINARY_LT; 
			} else if (Get(parser)->kind == 81) {
				Accept(parser);
				nBinaryOp = JC_BINARY_MT; 
			} else if (Get(parser)->kind == 53) {
				Accept(parser);
				nBinaryOp = JC_BINARY_LE; 
			} else {
				Accept(parser);
				nBinaryOp = JC_BINARY_ME; 
			}
			ShiftExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, nBinaryOp);
			DestroyExpress(&oSecondExp);
			
		}
}

static void EqualityExpr(CJcParser* parser,CJcExpress* pExp) {
		jc_uint nBinaryOp;
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		RelationalExpr(parser,pExp);
		while (Get(parser)->kind == 43 || Get(parser)->kind == 54) {
			if (Get(parser)->kind == 54) {
				Accept(parser);
				nBinaryOp = JC_BINARY_EQ; 
			} else {
				Accept(parser);
				nBinaryOp = JC_BINARY_NE; 
			}
			RelationalExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, nBinaryOp);
			DestroyExpress(&oSecondExp);
			
		}
}

static void AndExpr(CJcParser* parser,CJcExpress* pExp) {
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		EqualityExpr(parser,pExp);
		while (Get(parser)->kind == 66) {
			Accept(parser);
			EqualityExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, JC_BINARY_BIT_AND);
			DestroyExpress(&oSecondExp);
			
		}
}

static void ExclusiveOrExpr(CJcParser* parser,CJcExpress* pExp) {
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		AndExpr(parser,pExp);
		while (Get(parser)->kind == 65) {
			Accept(parser);
			AndExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, JC_BINARY_BIT_XOR);
			DestroyExpress(&oSecondExp);
			
		}
}

static void InclusiveOrExpr(CJcParser* parser,CJcExpress* pExp) {
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		ExclusiveOrExpr(parser,pExp);
		while (Get(parser)->kind == 77) {
			Accept(parser);
			ExclusiveOrExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, JC_BINARY_BIT_OR);
			DestroyExpress(&oSecondExp);
			
		}
}

static void LogicalAndExpr(CJcParser* parser,CJcExpress* pExp) {
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		InclusiveOrExpr(parser,pExp);
		while (Get(parser)->kind == 57) {
			Accept(parser);
			InclusiveOrExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, JC_BINARY_LOGIC_AND);
			DestroyExpress(&oSecondExp);
			
		}
}

static void LogicalOrExpr(CJcParser* parser,CJcExpress* pExp) {
		CJcExpress oSecondExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		
		LogicalAndExpr(parser,pExp);
		while (Get(parser)->kind == 58) {
			Accept(parser);
			LogicalAndExpr(parser,&oSecondExp);
			BuildBinaryExpress(pExp, &oSecondExp, JC_BINARY_LOGIC_OR);
			DestroyExpress(&oSecondExp);
			
		}
}

static void ConditionalExpr(CJcParser* parser,CJcExpress* pExp) {
		CJcExpress oSecondExp, oThirdExp;
		InitializeExpress(&parser->oSymbolStack, &oSecondExp);
		InitializeExpress(&parser->oSymbolStack, &oThirdExp);
		
		LogicalOrExpr(parser,pExp);
		if (Get(parser)->kind == 84) {
			Accept(parser);
			LogicalOrExpr(parser,&oSecondExp);
			Expect(parser, 78);
			ConditionalExpr(parser,&oThirdExp);
			BuildConditionalExpress(pExp, &oSecondExp, &oThirdExp);
			DestroyExpress(&oSecondExp);
			DestroyExpress(&oThirdExp);
			
		}
}

static void InitializeExpressA(CJcParser* parser,CJcExpress* pExp) {
		jc_uint nTypeCode;
		CJcTypeInfo* pType;
		CJcExpress oExp;
		jc_uint nStringExp = 0;
		pType = GetOrigType(pExp->pType);
		nTypeCode = GetTypeCode(pType);
		if(nTypeCode == JC_ARRAY && Get(parser)->kind == parser->_StringConst &&
			(JC_CHAR & GetTypeCode(GetOrigType(GetNextType(pType)))))
			nStringExp = 1;
		
		if (nStringExp) {
			InitializeStringExpress(parser,pExp, GetArrayInfo(pType));
		} else if (Get(parser)->kind == 73) {
			Accept(parser);
			InitializeCompoundExpress(parser,pExp, pType, nTypeCode);
			Expect(parser, 74);
		} else if (StartOf(parser, 4)) {
			InitializeExpress(&parser->oSymbolStack, &oExp); 
			AssignmentExpr(parser,&oExp);
			BuildAssignmentExpress(pExp, &oExp, JC_ASSIGN_EQUAL, 0);
			DestroyExpress(&oExp);
			
		} else SynErr(parser, 118);
}

static void InitializeStringExpress(CJcParser* parser,CJcExpress* pExp, CJcArrayInfo* pArray) {
		CJcStructFieldInfo* pField;
		CJcStructInfo * pStruct;
		CJcTypeInfo* pOldType;
		CJcExpress oItemExp;
		jc_uint nItemSize;
		InitializeExpress(&parser->oSymbolStack, &oItemExp);
		
		AssignmentExpr(parser,&oItemExp);
		nItemSize = GetTypeSize(oItemExp.pType);
		if(!pArray->nDim){
			CJcLocalStack* pStack;
			if(pExp->nOpt==JC_DS || pExp->nOpt == JC_SS)
				pStack = &parser->oSymbolStack.oDataSegment;
			else if(!parser->oSymbolStack.pInFunction)
				pStack = &parser->oSymbolStack.oGlobalStack;
			else
				pStack = &parser->oSymbolStack.oLocalStack;
			AllocVarInStack(pStack, nItemSize, 1, NULL);
			pArray->nDim = nItemSize;
		}
		else if(nItemSize > pArray->nDim){
			nItemSize = pArray->nDim;
			SetError(parser, 1, "initialization string too long");
		}
		pField = (CJcStructFieldInfo*)g_oInterface.Malloc(sizeof(CJcStructFieldInfo));
		pField->pType = oItemExp.pType;
		InitializeString(&pField->oName);
		CoverString(&pField->oName, "???");
		pField->nOffset = 0;
		pField->nBitCount = 0;
		pField->nBitOffset = 0;
		pField->pNext = NULL;
		pStruct = (CJcStructInfo*)g_oInterface.Malloc(sizeof(CJcStructInfo));
		pStruct->nImplemented = 1;
		pStruct->nSize = nItemSize;
		pStruct->nAlign = 1;
		pStruct->nFieldCount = 1;
		pStruct->pHead = pStruct->pTail;
		oItemExp.pType = CreateType(JC_NONE, JC_STRUCT, NULL, NULL, pStruct);
		pOldType = pExp->pType;
		pExp->pType = oItemExp.pType;
		SaveExpress(pExp, &oItemExp);
		pExp->pType = pOldType;
		DestroyExpress(&oItemExp);
		
}

static void InitializeCompoundExpress(CJcParser* parser,CJcExpress* pExp, CJcTypeInfo* pType, jc_uint nTypeCode) {
		CJcExpress oExp;
		CJcStructInfo* pStructInfo;
		CJcStructFieldInfo* pField;
		CJcArrayInfo* pArray;
		CJcTypeInfo* pNext;
		jc_uint nCode, nSize;
		if(nTypeCode == JC_STRUCT || nTypeCode == JC_UNION){
			pStructInfo = GetStructInfo(pType);
			pField = pStructInfo->pHead;
		}
		if(nTypeCode == JC_ARRAY){
			pArray = GetArrayInfo(pType);
			pNext = GetOrigType(pType->pNext);
			nCode = GetTypeCode(pNext);
			nSize = pArray->nDim;
		}
		
		if (nTypeCode == JC_STRUCT || nTypeCode == JC_UNION) {
			InitializeFieldListExpress(parser,nTypeCode, pExp, pField);
		} else if (nTypeCode == JC_ARRAY) {
			InitializeArrayItemExpress(parser,pExp, pArray, pNext, nSize, 0);
		} else if (StartOf(parser, 4)) {
			InitializeExpress(&parser->oSymbolStack, &oExp); 
			AssignmentExpr(parser,&oExp);
			BuildAssignmentExpress(pExp, &oExp, JC_ASSIGN_EQUAL, 0);
			DestroyExpress(&oExp);
			
		} else SynErr(parser, 119);
}

static void InitializeFieldListExpress(CJcParser* parser,jc_uint nTypeCode, CJcExpress* pStruct, CJcStructFieldInfo* pField) {
		CJcExpress oFieldExp;
		jc_char* sName = pField->oName.pStr;
		oFieldExp = *pStruct;
		oFieldExp.pType = CloneType(oFieldExp.pType);
		BuildFieldExpress(&oFieldExp, sName, 0);
		if(!GetTypeSize(oFieldExp.pType))
			SetError(parser, 0, "the zero length field '%s' cann't be initialized", sName);
		pField = pField->pNext;
		
		InitializeExpressA(parser,&oFieldExp);
		DestroyExpress(&oFieldExp);
		if(nTypeCode != JC_STRUCT)
			return;
		
		if (Get(parser)->kind == 82) {
			Accept(parser);
			if(!pField){
			SetError(parser, 0, "too many initialization item");
			return;
			}
			
			InitializeFieldListExpress(parser,nTypeCode, pStruct, pField);
		}
}

static void InitializeArrayItemExpress(CJcParser* parser,CJcExpress* pExp, CJcArrayInfo* pArray, CJcTypeInfo* pItemType, jc_uint nDim, jc_uint nIdx) {
		CJcExpress oItemExp;
		CJcExpress oIdxExp;
		InitializeExpress(&parser->oSymbolStack, &oItemExp);
		InitializeExpress(&parser->oSymbolStack, &oIdxExp);
		if(!nDim){
			jc_uint nAlign = GetTypeAlign(pItemType);
			jc_uint nSize = GetTypeSize(pItemType);
			CJcLocalStack* pStack;
			if(pExp->nOpt == JC_DS || pExp->nOpt == JC_SS)
				pStack = &parser->oSymbolStack.oDataSegment;
			else if(!parser->oSymbolStack.pInFunction)
				pStack = &parser->oSymbolStack.oGlobalStack;
			else
				pStack = &parser->oSymbolStack.oLocalStack;
			AllocVarInStack(pStack, nSize, nAlign, NULL);
			++pArray->nDim;
		}
		oItemExp = *pExp;
		oItemExp.pType = CloneType(oItemExp.pType);
		oIdxExp = BuildIntegerConstantExpressA(&parser->oSymbolStack, nIdx, False);
		BuildArrayExpress(&oItemExp, &oIdxExp);
		DestroyExpress(&oIdxExp);
		
		InitializeExpressA(parser,&oItemExp);
		DestroyExpress(&oItemExp); 
		if (Get(parser)->kind == 82) {
			Accept(parser);
			InitializeArrayItemExpress(parser,pExp, pArray, pItemType, nDim, nIdx+1);
		}
}


void Parse(CJcParser* parser){
	BeginScan(parser->scanner);
	parser->t = NULL;
	parser->la = NULL;
	Main(parser);

	Expect(parser, 0);
}

void InitializeParser(CJcParser* parser, CJcScanner *scanner, CJcErrorSystem *pErrorSystem, int argc, char* argv[]){
	parser->_EOF = 0;
	parser->_Identifier = 1;
	parser->_CharConst = 2;
	parser->_StringConst = 3;
	parser->_IntegerConst = 4;
	parser->_FloatConst = 5;
	parser->_auto = 6;
	parser->_break = 7;
	parser->_case = 8;
	parser->_char = 9;
	parser->_const = 10;
	parser->_continue = 11;
	parser->_default = 12;
	parser->_do = 13;
	parser->_double = 14;
	parser->_else = 15;
	parser->_enum = 16;
	parser->_extern = 17;
	parser->_float = 18;
	parser->_for = 19;
	parser->_goto = 20;
	parser->_host = 21;
	parser->_if = 22;
	parser->_int = 23;
	parser->_long = 24;
	parser->_register = 25;
	parser->_return = 26;
	parser->_share = 27;
	parser->_short = 28;
	parser->_signed = 29;
	parser->_sizeof = 30;
	parser->_static = 31;
	parser->_struct = 32;
	parser->_switch = 33;
	parser->_typedef = 34;
	parser->_union = 35;
	parser->_unsigned = 36;
	parser->_void = 37;
	parser->_volatile = 38;
	parser->_while = 39;
	parser->_ellipsis = 40;
	parser->_rshifteq = 41;
	parser->_lshifteq = 42;
	parser->_noteq = 43;
	parser->_modeq = 44;
	parser->_xoreq = 45;
	parser->_andeq = 46;
	parser->_muleq = 47;
	parser->_subeq = 48;
	parser->_addeq = 49;
	parser->_oreq = 50;
	parser->_diveq = 51;
	parser->_mteq = 52;
	parser->_lteq = 53;
	parser->_equal = 54;
	parser->_lshift = 55;
	parser->_rshift = 56;
	parser->_land = 57;
	parser->_lor = 58;
	parser->_pointer = 59;
	parser->_inc = 60;
	parser->_dec = 61;
	parser->_tilde = 62;
	parser->_not = 63;
	parser->_mod = 64;
	parser->_xor = 65;
	parser->_and = 66;
	parser->_mul = 67;
	parser->_lparentheses = 68;
	parser->_rparentheses = 69;
	parser->_sub = 70;
	parser->_add = 71;
	parser->_assign = 72;
	parser->_lbrace = 73;
	parser->_rbrace = 74;
	parser->_lbrack = 75;
	parser->_rbrack = 76;
	parser->_or = 77;
	parser->_colon = 78;
	parser->_scolon = 79;
	parser->_lt = 80;
	parser->_mt = 81;
	parser->_comma = 82;
	parser->_dot = 83;
	parser->_quest = 84;
	parser->_div = 85;
	parser->maxT = 87;

	parser->pErrorSystem = pErrorSystem;
	parser->errDist = parser->minErrDist = 2;
	parser->scanner = scanner;
	OnInitializeParser(parser, argc, argv);
}

static int StartOf(CJcParser* parser, int s){
#define T 1
#define x 0
	static int set[16][89] = {
		{T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,T,T,T, T,T,T,T, x,T,T,T, x,T,T,x, T,T,T,T, T,x,T,T, T,T,T,T, T,T,T,T, T,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, x,x,T,T, T,x,T,T, x,T,x,x, x,x,x,T, x,x,x,x, x,x,T,x, x},
		{x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, x,T,x,T, x,x,x,T, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, x,x,T,T, T,x,T,T, x,x,x,x, x,x,x,T, x,x,x,x, x,x,T,x, x},
		{x,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, x,x,T,T, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x},
		{x,T,T,T, T,T,x,T, x,x,x,T, x,T,x,x, x,x,x,T, T,x,T,x, x,x,T,x, x,x,T,x, x,T,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, x,x,T,T, T,x,T,T, x,T,x,x, x,x,x,T, x,x,x,x, x,x,T,x, x},
		{x,x,x,x, x,x,x,x, x,T,x,x, x,x,T,x, x,x,T,x, x,x,x,T, T,x,x,x, T,T,x,x, x,x,x,x, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,T,x,x, x,x,x,x, x,T,T,x, x,x,T,x, T,x,T,x, x,x,x,T, T,T,x,x, T,T,x,x, T,x,x,T, T,T,T,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,x,x, T,T,x,x, x,x,T,T, x,x,T,x, x,x,x,x, x},
		{x,T,x,x, x,x,x,x, x,T,x,x, x,x,T,x, T,x,T,x, x,x,x,T, T,x,x,x, T,T,x,x, T,x,x,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,T,x,x, x,x,x,x, x,T,T,x, x,x,T,x, T,x,T,x, x,x,x,T, T,T,x,x, T,T,x,x, T,x,x,T, T,T,T,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, T,T,x,T, x,x,T,T, x,x,T,x, x,x,x,x, x},
		{x,T,x,x, x,x,x,x, x,T,T,x, x,x,T,x, T,x,T,x, x,x,x,T, T,T,x,x, T,T,x,x, T,x,x,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,x,x, x,x,x,x, T,x,x,x, x,x,x,T, x,x,x,x, x,x,x,T, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,T,x, T,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, x,x,T,T, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,x,x, x,x,x,x, x}
	};


#undef T
#undef x
	return set[s][Get(parser)->kind];
}

static void ReportSynErr(CJcParser* parser, char* fname, int line, int col, int n){
	char* s;
	switch(n){
			case 0: s = (char*)("EOF expected"); break;
			case 1: s = (char*)("Identifier expected"); break;
			case 2: s = (char*)("CharConst expected"); break;
			case 3: s = (char*)("StringConst expected"); break;
			case 4: s = (char*)("IntegerConst expected"); break;
			case 5: s = (char*)("FloatConst expected"); break;
			case 6: s = (char*)("auto expected"); break;
			case 7: s = (char*)("break expected"); break;
			case 8: s = (char*)("case expected"); break;
			case 9: s = (char*)("char expected"); break;
			case 10: s = (char*)("const expected"); break;
			case 11: s = (char*)("continue expected"); break;
			case 12: s = (char*)("default expected"); break;
			case 13: s = (char*)("do expected"); break;
			case 14: s = (char*)("double expected"); break;
			case 15: s = (char*)("else expected"); break;
			case 16: s = (char*)("enum expected"); break;
			case 17: s = (char*)("extern expected"); break;
			case 18: s = (char*)("float expected"); break;
			case 19: s = (char*)("for expected"); break;
			case 20: s = (char*)("goto expected"); break;
			case 21: s = (char*)("host expected"); break;
			case 22: s = (char*)("if expected"); break;
			case 23: s = (char*)("int expected"); break;
			case 24: s = (char*)("long expected"); break;
			case 25: s = (char*)("register expected"); break;
			case 26: s = (char*)("return expected"); break;
			case 27: s = (char*)("share expected"); break;
			case 28: s = (char*)("short expected"); break;
			case 29: s = (char*)("signed expected"); break;
			case 30: s = (char*)("sizeof expected"); break;
			case 31: s = (char*)("static expected"); break;
			case 32: s = (char*)("struct expected"); break;
			case 33: s = (char*)("switch expected"); break;
			case 34: s = (char*)("typedef expected"); break;
			case 35: s = (char*)("union expected"); break;
			case 36: s = (char*)("unsigned expected"); break;
			case 37: s = (char*)("void expected"); break;
			case 38: s = (char*)("volatile expected"); break;
			case 39: s = (char*)("while expected"); break;
			case 40: s = (char*)("ellipsis expected"); break;
			case 41: s = (char*)("rshifteq expected"); break;
			case 42: s = (char*)("lshifteq expected"); break;
			case 43: s = (char*)("noteq expected"); break;
			case 44: s = (char*)("modeq expected"); break;
			case 45: s = (char*)("xoreq expected"); break;
			case 46: s = (char*)("andeq expected"); break;
			case 47: s = (char*)("muleq expected"); break;
			case 48: s = (char*)("subeq expected"); break;
			case 49: s = (char*)("addeq expected"); break;
			case 50: s = (char*)("oreq expected"); break;
			case 51: s = (char*)("diveq expected"); break;
			case 52: s = (char*)("mteq expected"); break;
			case 53: s = (char*)("lteq expected"); break;
			case 54: s = (char*)("equal expected"); break;
			case 55: s = (char*)("lshift expected"); break;
			case 56: s = (char*)("rshift expected"); break;
			case 57: s = (char*)("land expected"); break;
			case 58: s = (char*)("lor expected"); break;
			case 59: s = (char*)("pointer expected"); break;
			case 60: s = (char*)("inc expected"); break;
			case 61: s = (char*)("dec expected"); break;
			case 62: s = (char*)("tilde expected"); break;
			case 63: s = (char*)("not expected"); break;
			case 64: s = (char*)("mod expected"); break;
			case 65: s = (char*)("xor expected"); break;
			case 66: s = (char*)("and expected"); break;
			case 67: s = (char*)("mul expected"); break;
			case 68: s = (char*)("lparentheses expected"); break;
			case 69: s = (char*)("rparentheses expected"); break;
			case 70: s = (char*)("sub expected"); break;
			case 71: s = (char*)("add expected"); break;
			case 72: s = (char*)("assign expected"); break;
			case 73: s = (char*)("lbrace expected"); break;
			case 74: s = (char*)("rbrace expected"); break;
			case 75: s = (char*)("lbrack expected"); break;
			case 76: s = (char*)("rbrack expected"); break;
			case 77: s = (char*)("or expected"); break;
			case 78: s = (char*)("colon expected"); break;
			case 79: s = (char*)("scolon expected"); break;
			case 80: s = (char*)("lt expected"); break;
			case 81: s = (char*)("mt expected"); break;
			case 82: s = (char*)("comma expected"); break;
			case 83: s = (char*)("dot expected"); break;
			case 84: s = (char*)("quest expected"); break;
			case 85: s = (char*)("div expected"); break;
			case 86: s = (char*)("\"alignof\" expected"); break;
			case 87: s = (char*)("??? expected"); break;
			case 88: s = (char*)("invalid Declaration"); break;
			case 89: s = (char*)("invalid Statement"); break;
			case 90: s = (char*)("invalid ForStatement"); break;
			case 91: s = (char*)("invalid CaseLabel"); break;
			case 92: s = (char*)("invalid StorageSpecifier"); break;
			case 93: s = (char*)("invalid QualSpecifier"); break;
			case 94: s = (char*)("invalid TypeSpecifier"); break;
			case 95: s = (char*)("invalid SimpleSpecifier"); break;
			case 96: s = (char*)("invalid SimpleSpecifier"); break;
			case 97: s = (char*)("invalid StructSpecifier"); break;
			case 98: s = (char*)("invalid StructSpecifier"); break;
			case 99: s = (char*)("invalid StructSpecifier"); break;
			case 100: s = (char*)("invalid EnumSpecifier"); break;
			case 101: s = (char*)("invalid EnumSpecifier"); break;
			case 102: s = (char*)("invalid IntegerSpecifier"); break;
			case 103: s = (char*)("invalid EnumBodySpecifier"); break;
			case 104: s = (char*)("invalid EnumBodySpecifier"); break;
			case 105: s = (char*)("invalid FieldsSpecifier2"); break;
			case 106: s = (char*)("invalid ObjectSpecifier"); break;
			case 107: s = (char*)("invalid ObjectSpecifier"); break;
			case 108: s = (char*)("invalid ObjectSpecifier"); break;
			case 109: s = (char*)("invalid ObjectsSpecifier2"); break;
			case 110: s = (char*)("invalid ObjectsSpecifier2"); break;
			case 111: s = (char*)("invalid InitializeVariable"); break;
			case 112: s = (char*)("invalid FunctionDeclare"); break;
			case 113: s = (char*)("invalid PrimaryExpr"); break;
			case 114: s = (char*)("invalid UnaryExpr"); break;
			case 115: s = (char*)("invalid UnaryExpr"); break;
			case 116: s = (char*)("invalid UnaryOperator"); break;
			case 117: s = (char*)("invalid CastExpr"); break;
			case 118: s = (char*)("invalid InitializeExpressA"); break;
			case 119: s = (char*)("invalid InitializeCompoundExpress"); break;

		default:{
			char format[20];
			g_oInterface.FormatPrint(format, "error %d", n);
			s = format;
		}
		break;
	}
	ReportSemErr(parser, 0, fname, line, col, s);
}

static void ReportSemErr(CJcParser* parser, int bWarning, char* fname, int line, int col, const char *s){
	CompileError(parser->pErrorSystem, bWarning, "%s[%d:%d]: %s\n", fname, line, col, s);
}

static void ReportSemErr2(CJcParser* parser, int bWarning, const char *s){
	CompileError(parser->pErrorSystem, bWarning, "%s[%d:%d]: %s\n", parser->t->fname, parser->t->fline, parser->t->fcol, s);
}

static void SynErr(CJcParser* parser, int n){
	if(parser->errDist >= parser->minErrDist){
		ReportSynErr(parser, Get(parser)->fname, Get(parser)->fline, Get(parser)->fcol, n);
	}
	parser->errDist = 0;
}

static void Expect(CJcParser* parser, int n){
	if(Get(parser)->kind == n)
		Accept(parser);
	else
		SynErr(parser, n);
}

static void ExpectWeak(CJcParser* parser, int n, int follow){
	if(Get(parser)->kind == n)
		Accept(parser);
	else{
		SynErr(parser, n);
		while(!StartOf(parser, follow))
			Accept(parser);
	}
}

static int WeakSeparator(CJcParser* parser, int n, int syFol, int repFol){
	if(Get(parser)->kind == n)	{
		Accept(parser);
		return 1;
	}
	else if(StartOf(parser, repFol))
		return 0;
	SynErr(parser, n);
	while(!(StartOf(parser, syFol) || StartOf(parser, repFol) || StartOf(parser, 0)))
		Accept(parser);
	return StartOf(parser, syFol);
}

void DestroyParser(CJcParser* parser){
	OnDestroyParser(parser);
}

void SetError(CJcParser* parser, int bWarning, const char* msg, ...)
{
	va_list argptr;
	jc_char sError[1024];
#ifdef UNIX
	va_start(argptr);
#else
	va_start(argptr, msg);
#endif
	g_oInterface.FormatPrintV(sError, msg, argptr);
	va_end(argptr);

	if(bWarning)
		ReportSemErr2(parser, bWarning, sError);
	else{
		if(parser->errDist >= parser->minErrDist)
			ReportSemErr2(parser, bWarning, sError);
		parser->errDist = 0;
	}
}

void SetErrorEx(CJcParser* parser, int bWarning, char* fname, int line, int col, const char *msg, ...)
{
	va_list argptr;
	jc_char sError[1024];
#ifdef UNIX
	va_start(argptr);
#else
	va_start(argptr, msg);
#endif
	g_oInterface.FormatPrintV(sError, msg, argptr);
	va_end(argptr);

	if(bWarning){
		ReportSemErr(parser, bWarning, fname, line, col, sError);
	}
	else{
		if(parser->errDist >= parser->minErrDist)
			ReportSemErr(parser, bWarning, fname, line, col, sError);
		parser->errDist = 0;
	}
}

jc_bool IsTypeSpecifier(CJcSymbolStack* pStack, jc_int kind, jc_char* v)
{
	CJcSymbol oSymbol;
	if(kind == pStack->pParser->_Identifier && !FindSymbol(pStack, v, 1, &oSymbol) && oSymbol.nSymbol == JC_TYPE_SYMBOL)
		return True;
	if(kind == pStack->pParser->_void)
		return True;
	if(kind == pStack->pParser->_char)
		return True;
	if(kind == pStack->pParser->_short)
		return True;
	if(kind == pStack->pParser->_int)
		return True;
	if(kind == pStack->pParser->_long)
		return True;
	if(kind == pStack->pParser->_signed)
		return True;
	if(kind == pStack->pParser->_unsigned)
		return True;
	if(kind == pStack->pParser->_float)
		return True;
	if(kind == pStack->pParser->_double)
		return True;
	if(kind == pStack->pParser->_struct)
		return True;
	if(kind == pStack->pParser->_union)
		return True;
	if(kind == pStack->pParser->_enum)
		return True;
	if(kind == pStack->pParser->_const)
		return True;
	if(kind == pStack->pParser->_volatile)
		return True;
	if(kind == pStack->pParser->_extern)
		return True;
	if(kind == pStack->pParser->_share)
		return True;
	if(kind == pStack->pParser->_typedef)
		return True;
	if(kind == pStack->pParser->_register)
		return True;
	if(kind == pStack->pParser->_static)
		return True;
	return False;
}

jc_bool NeedReturnValue(CJcSymbolStack* pStack)
{
	return (GetTypeCode(GetOrigType(GetFunctionType(pStack->pInFunction))) != JC_VOID);
}

