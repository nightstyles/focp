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
/SqlYacc\.tab\.c/ {
	sub(/SqlYacc\.tab\.c/, "./src/SqlYacc.cpp", $0);
	print $0;
	next;
}
/SqlYacc\.y/ {
	sub(/SqlYacc\.y/, "./sc/SqlYacc.y", $0);
	print $0;
	next;
}
{
	print $0;
}
' SqlYacc.tab.c > SqlYacc.cpp

gawk '
/SqlYacc\.y/ {
	sub(/SqlYacc\.y/, "./src/SqlYacc.y", $0);
	print $0;
	next;
}
/SqlYacc\.tab\.h/ {
	sub(/SqlYacc\.tab\.h/, "./det/SqlYacc.hpp", $0);
	print $0;
	next;
}
/^#ifndef YYTOKENTYPE/ {
	print "#include \"MdbApi.hpp\"";
	print $0;
	next;
}
{
	print $0;
}
' SqlYacc.tab.h > ../det/SqlYacc.hpp
