
#include <stdio.h>

struct A
{
	A()
	{
		setvbuf(stdout, NULL, _IONBF, 0);
	}
};

static A g_oInitialize;
