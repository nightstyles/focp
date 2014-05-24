
Main:
	主控应用，包含main函数，并自动连接AFC库。参考Sample如何使用该主控应用模块。

Runner:
	RunService -u
		实现统一创建子进程的服务。应用可用下列AFC函数以通知[udp]该服务创建子进程。
		AFC_API void StartApp(int32 nArgc, char* sArgv[], bool bLoop=false, uint16 nVirtualPort=0);
	RunService -t
		实现统一的基于标准IO的交互式管理。[tcp server]
	RunShell 【-a -r -v -h】 x.exe
		-a，以后台方式启动应用
		-r，当应用退出后，自动重新拉起应用
		-vPort，指定虚拟端口连接到[RunService -t]，以提供标准IO交互。
		-h，仅用于windows，当指定-v参数时需要制定该选项。
	Run
		是一个命令行工具，提供以下基本命令：
		Help：
			打印该工具的命令行帮助信息
		List:
			列出连接到[RunService -t]的所有服务
		Start [-r] [-aPort|-bPort] 应用命令行
			应用命令行最好是全路径模式，否则有可能执行不了。
			-r同RunShell的-r选项
			-a同RunShell的-v选项，启动应用并直接连接到应用上，当启动应用失败时系统会挂住【可执行文件必须有效】。
			-b同RunShell的-v选项，启动应用但不接连接到应用上。
		Connect Port
			连接到由端口指定相应的应用中，向用户提供标准IO交互。
			Port是指-a,-b指定的端口参数，可用list命令看看系统中有哪些端口。
	SetBuf
			该模块用于RunShell拉起子进程，并指定-v参数时，修改子进程的标准输出为非缓冲模式。
			应用启动时标准输出为行缓冲模式，当用管道创建子进程时，变成了全缓冲模式，这里修改为非缓冲模式以提供交互能力。
			popen函数不具有交互能力的关键就在于此。
			
Runner的后续开发计划，
	（1）Runner可以连接到上一级Runner，以形成一颗树，那么从一个节点登录，可以管控所有子节点的应用。
		  该功能的实现可能会调整当前的体系结构。
	（2）AFC模块中添加基于标准IO的命令行接口，可参考telnet命令行接口实现/vob/focp/BaseService/TelnetSerivce。
			并应该支持多会话接入，相当于多个终端可以连接进来，而又互不干扰。
	（3）AFC模块中实现类似popen的功能接口，需要提供三个文件句柄CFile，分别用于输入、输出、错误输出。也需要能实现交互，传统的popen不能实现交互。
			可参考Run模块的Start命令-a选项的实现。

   
