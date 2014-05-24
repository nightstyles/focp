gawk '
/^#include <FlexLexer\.h>/ {
	print "#include \"SqlEnv.hpp\"";
	next;
}
/^#include <unistd\.h>/{
	next;
}
/^#define YY_DECL int yyFlexLexer::yylex\(\)/ {
	print "#define YY_DECL int yyFlexLexer::yylex(YYSTYPE* yylval)";
	next;
}
/lex\.yy\.cc/ {
	sub(/lex\.yy\.cc/, "./src/SqlLex.cpp", $0);
	print $0;
	next;
}
/SqlLex\.l/ {
	sub(/SqlLex\.1\.l/, "./src/SqlLex.l", $0);
	print $0;
	next;
}
/^#define FLEX_SCANNER/ {
	print "#include \"SqlApi.hpp\"";
	print $0;
	next;
}
{
	print $0;
}
' lex.yy.cc > SqlLex.cpp
