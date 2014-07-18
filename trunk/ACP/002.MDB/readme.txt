00.MdbApp MDB应用的编译入口
   MdbLic  Mdb许可工具
   OdbcLic ODBC许可工具
   Makefile 编译入口
	make build/install/rebuild/release
   nms      配置文件
   runtime  配置文件
01.MdbSvr MDB服务模块
02.MdbApi 应用编程接口
03.MdbSql 是SQL模块代码
04.MdbLoc 是MdbApi的本地实现
06.MdbCon 是MdbApi的远程实现
05.MdbLsn 是MdbCon的服务端
07.MdbRep 是Mdb赋值模块，AS/LS的底层支撑
08.MdbSto 是Mdb物理数据库接口实现。
          目前采用ODBC，如果需要换成其他接口，需要重新实现。
