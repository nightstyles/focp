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
/SqlYacc\.3\.tab\.c/ {
	sub(/SqlYacc\.tab\.c/, "./src/SqlYacc.cpp.3", $0);
	print $0;
	next;
}
/SqlYacc\.3\.y/ {
	sub(/SqlYacc\.3\.y/, "./sc/SqlYacc.3.y", $0);
	print $0;
	next;
}
{
	print $0;
}
' SqlYacc.3.tab.c > SqlYacc.cpp.3

gawk '
/SqlYacc\.3\.y/ {
	sub(/SqlYacc\.3\.y/, "./src/SqlYacc.3.y", $0);
	print $0;
	next;
}
/SqlYacc.3\.tab\.h/ {
	sub(/SqlYacc\.3\.tab\.h/, "./det/SqlYacc.3.hpp", $0);
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
' SqlYacc.3.tab.h > ../det/SqlYacc.3.hpp
