
#include "../BaseDef.hpp"

void TcpService();
void UdpService();

int main(int argc, char* argv[])
{
	if(argc > 1)
	{
		if(!strcmp(argv[1], "-u"))
			UdpService();
		else if(!strcmp(argv[1], "-t"))
			TcpService();
	}
	return 0;
}

