gawk '
/^int yychar;/ {
	print "CSqlEnv* pSqlEnv = ((CSqlEnv*)YYPARSE_PARAM);";
	print "CSqlDataBase * pDb = pSqlEnv->GetDataBase();";
	print $0;
	next;
}
/^# define YYLEX yylex \(\&yylval\)/ {
	print "#define YYLEX ((yyFlexLexer*)YYPARSE_PARAM)->yylex(&yylval)";
	next;
}
/SqlYacc\.1\.tab\.c/ {
	sub(/SqlYacc\.1\.tab\.c/, "./src/SqlYacc.cpp.1", $0);
	print $0;
	next;
}
/SqlYacc\.1\.y/ {
	sub(/SqlYacc\.1\.y/, "./sc/SqlYacc.1.y", $0);
	print $0;
	next;
}
{
	print $0;
}
' SqlYacc.1.tab.c > SqlYacc.cpp.1

gawk '
/SqlYacc\.1\.y/ {
	sub(/SqlYacc\.1\.y/, "./src/SqlYacc.1.y", $0);
	print $0;
	next;
}
/SqlYacc\.1\.tab\.h/ {
	sub(/SqlYacc\.1\.tab\.h/, "./det/SqlYacc.1.hpp", $0);
	print $0;
	next;
}
/^#ifndef YYTOKENTYPE/ {
	print "#include \"RdbApi.hpp\"";
	print $0;
	next;
}
{
	print $0;
}
' SqlYacc.1.tab.h > ../det/SqlYacc.1.hpp
