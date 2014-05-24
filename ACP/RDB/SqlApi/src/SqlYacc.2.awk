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
/SqlYacc\.2\.tab\.c/ {
	sub(/SqlYacc\.2\.tab\.c/, "./src/SqlYacc.cpp.2", $0);
	print $0;
	next;
}
/SqlYacc\.2\.y/ {
	sub(/SqlYacc\.2\.y/, "./sc/SqlYacc.2.y", $0);
	print $0;
	next;
}
{
	print $0;
}
' SqlYacc.2.tab.c > SqlYacc.cpp.2

gawk '
/SqlYacc\.2\.y/ {
	sub(/SqlYacc\.2\.y/, "./src/SqlYacc.2.y", $0);
	print $0;
	next;
}
/SqlYacc\.2\.tab\.h/ {
	sub(/SqlYacc\.2\.tab\.h/, "./det/SqlYacc.2.hpp", $0);
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
' SqlYacc.2.tab.h > ../det/SqlYacc.2.hpp
