
#if defined(RDB_SUPPORT_MEMORY_DB) && defined(RDB_SUPPORT_DISK_DB)
#include "SqlYacc.cpp.3"
#elif defined(RDB_SUPPORT_MEMORY_DB)
#include "SqlYacc.cpp.1"
#elif defined(RDB_SUPPORT_DISK_DB)
#include "SqlYacc.cpp.2"
#endif
