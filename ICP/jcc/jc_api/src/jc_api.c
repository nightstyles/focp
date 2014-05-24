
#include "jc_parser.h"
#include "jc_pp.h"
#include "jc_link.h"

/*
	"-H" support host symbol
	"-S" bSupport ShareSymbol
	"-p" support preprocess output;
	"-cpara" C File Name
	"-opara" object/library/executable FileName and object file list
	"-epara" entry symbol name
	"-Ipara" include path
	"-Dpara" macro define
	"-Lpara" library path
	"-llib" library file
	"-hlib" host library file
*/

static jc_char* GetOption(jc_int argc, jc_char* argv[], const jc_char* sOpt)
{
	jc_int i, nlen = StringLength(sOpt);
	jc_char* sRet = NULL;
	for(i=1; i<argc; ++i)
	{
		if(!MemoryCompare(argv[i], (jc_void*)sOpt, nlen))
		{
			sRet = argv[i] + nlen;
			if(!sRet[0])
			{
				++i;
				if(i < argc)
					sRet = argv[i];
			}
			break;
		}
	}
	return sRet;
}

static const jc_char* JC_CALL GetLineFromPreProc(void* pSource, jc_int * pLine, jc_char** sFileName)
{
	jc_void* fp;
	jc_int nBufLen;

	jc_char* s, *p, *s2;
	CJcPreProc* pPreProc = (CJcPreProc*)pSource;
	CJcCodeLine* pCode = GetProcLine(pPreProc);
	if(pLine)
		pLine[0] = pCode->nFileLineNo;
	if(sFileName)
		sFileName[0] = pCode->sFileName;
	s2 = s = New2(jc_char, StringLength(pCode->sLine)+1);
	p = pCode->sLine;
	while(p[0])
	{
		s[0] = '\0';
		if(p[0] == '?' && p[1] == '?')
		{
			switch(p[2])
			{
			case '(': s[0] = '['; break;
			case ')': s[0] = ']'; break;
			case '<': s[0] = '{'; break;
			case '>': s[0] = '}'; break;
			case '/': s[0] = '\\'; break;
			case '!': s[0] = '|'; break;
			case '\'': s[0] = '^'; break;
			case '-': s[0] = '~'; break;
			case '=': s[0] = '#'; break;
			}
		}
		if(s[0])
			p += 2;
		else
			s[0] = p[0];
		++s;
		++p;
	}
	s[0] = '\0';
	StringCopy(pCode->sLine, s2);
	g_oInterface.Free(s2);

	if(pPreProc->bSupportOutput)
	{
		jc_char sFileName2[JC_MAX_PATH];
		g_oInterface.FormatPrint(sFileName2, "%s.p", pCode->sFileName);
		nBufLen = StringLength(pCode->sLine);
		fp = g_oInterface.OpenFile(sFileName2, "a+b");
		g_oInterface.WriteFile(fp, pCode->sLine, nBufLen);
		g_oInterface.CloseFile(fp);
	}

	return pCode->sLine;
}

JC_API jc_int JC_CALL JcCompile(jc_int argc, jc_char* argv[])
{
	jc_int nRet, nLen;
	jc_char* sObjFile;
	CJcErrorSystem oErrorSystem = {0, 0};

	sObjFile = GetOption(argc, argv, "-o");
	if(!sObjFile)
	{
		CompileError(&oErrorSystem, 0, "missing object file");
		AllError(&oErrorSystem);
		return -1;
	}
	nLen = StringLength(sObjFile);
	if(nLen>2 && !StringCompare(sObjFile+nLen-2, ".o"))
	{
		CJcScanner oScanner;
		CJcParser oParser;
		CJcPreProc oPreProcessor;
		jc_char* sFileName;
		CJcSegment* pObjectSegment;

		sFileName = GetOption(argc, argv, "-c");
		if(!sFileName[0])
			sFileName = NULL;
		if(!sFileName)
		{
			CompileError(&oErrorSystem, 0, "missing source file");
			AllError(&oErrorSystem);
			return -1;
		}

		if(InitializePreProcessor(&oPreProcessor, &oErrorSystem, sFileName))
		{
			CompileError(&oErrorSystem, 0, "can not open source file '%s'", sFileName);
			AllError(&oErrorSystem);
			return -1;
		}

		if(GetOption(argc, argv, "-p"))
			oPreProcessor.bSupportOutput = True;
		else
			oPreProcessor.bSupportOutput = False;

		if(oPreProcessor.bSupportOutput)
		{
			jc_void* fp;
			jc_char sFileName2[JC_MAX_PATH];
			g_oInterface.FormatPrint(sFileName2, "%s.p", oPreProcessor.oFileTable.pFileTable->sFileName);
			fp = g_oInterface.OpenFile(sFileName2, "wb");
			g_oInterface.CloseFile(fp);
		}

		InitializePreProcessorCmdLine(&oPreProcessor, argc, argv);
		InitializeScanner(&oScanner, &oPreProcessor, GetLineFromPreProc);
		InitializeParser(&oParser, &oScanner, &oErrorSystem, argc, argv);

		Parse(&oParser);

		nRet = 0;

		if(!oErrorSystem.nErrorCount)
		{
			jc_void* fp;
			jc_char* sEntry = GetOption(argc, argv, "-e");
			if(sEntry && !sEntry[0])
				sEntry = NULL;
			if(!sEntry)
				sEntry = "___000___cmain___000___";

			fp = g_oInterface.OpenFile(sObjFile, "wb");
			if(!fp)
			{
				CompileError(&oErrorSystem, 0, "can not open object file '%s'", sObjFile);
				nRet = -1;
			}
			else
			{
				pObjectSegment = CreateObjectFile(&oParser.oSymbolStack, sEntry);
				g_oInterface.WriteFile(fp, GetDataOfSegment(pObjectSegment), GetSizeOfSegment(pObjectSegment));
				ClearSegment(pObjectSegment);
				g_oInterface.Free(pObjectSegment);
				g_oInterface.CloseFile(fp);
			}
		}

		ClearPreProcessor(&oPreProcessor);
		DestroyParser(&oParser);
		DestroyScanner(&oScanner);
	}
	else
	{
		CJcLinker oLinker;
		jc_char sCurPath[256];
		jc_uint nFileType = 0;

		nRet = 0;
		if(!nFileType)
		{
			if(nLen>2 && !StringCompare(sObjFile+nLen-2, ".b"))
				nFileType = JC_LIB_FILE;
		}
		if(!nFileType)
		{
			if(nLen>2 && !StringCompare(sObjFile+nLen-2, ".d"))
				nFileType = JC_DLL_FILE;
		}
		if(!nFileType)
		{
			if(nLen>2 && !StringCompare(sObjFile+nLen-2, ".e"))
				nFileType = JC_EXE_FILE;
		}
		if(!nFileType)
		{
			CompileError(&oErrorSystem, 0, "invalid object filename postfix");
			AllError(&oErrorSystem);
			return -1;
		}

		InitializeLinker(&oLinker, &oErrorSystem, g_oInterface.GetCurrentPath(sCurPath, 256), nFileType, argc, argv);
		LinkJcFile(&oLinker, nFileType);
		if(!oErrorSystem.nErrorCount)
			nRet = -1;
		DestroyLinker(&oLinker);
	}

	AllError(&oErrorSystem);
	return nRet;
}
