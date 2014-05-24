
#include "RdbApi.hpp"

FOCP_BEGIN();

static const char* g_ErrorInfo[] = 
{
	"RDB_BACKUP_FAILURE",		// backup database system failure
	"RDB_CREATE_REDO_FAILURE",  // create redo system failure
	"RDB_WRITERDO_FAILURE",		// write redo failure
	"RDB_INVALID_INPUT",		// invalid parameter
	"RDB_INVALID_NAME",			// invalid name: dbname, table name, field name, and so on
	"RDB_INVALID_STORAGE",		// invalid storage type of the table
	"RDB_INVALID_TYPE",			// invalid data type of the field
	"RDB_INVALID_SIZE",			// invalid data length of the field
	"RDB_LACK_MEMORY",			// temporary lack memory space
	"RDB_LACK_STORAGE",			// lack storage space
	"RDB_REPEAT_FIELD",			// redefine field
	"RDB_DB_EXIST",				// the database exist in the system;
	"RDB_DB_NOTEXIST",			// the database not exist in the system
	"RDB_DB_BUSY",				// the database is busy
	"RDB_DB_NOSELECTED",		// there is not any database selected
	"RDB_DB_MOUNTED",			// the database is mounted
	"RDB_DB_NOTMOUNTED",		// the database isnot mounted
	"RDB_TABLE_EXIST",			// the table exists in the database system
	"RDB_TABLE_NOTEXIST",		// not exist the table in the database system
	"RDB_RECORD_TOOLONG",		// the record cann't be more than VMM_MAX_MAPSIZE
	"RDB_VIEW_EXIST",			// the view exists in the database system
	"RDB_VIEW_NOTEXIST",		// the view not exist in the database system
	"RDB_INDEX_EXIST",			// the index exists in the database system
	"RDB_INDEX_NOTEXIST",		// the index not exists in the database system
	"RDB_INDEX_TOOMANY",		// the index is too many
	"RDB_DETAIL_EXIST",			// exist the detail table
	"RDB_DETAIL_NOTEXIST",		// not exist the detail table
	"RDB_FIELD_NOTEXIST",		// the field not exist
	"RDB_FIELD_TOOMANY",		// the field is too many
	"RDB_FIELD_ISNULL",			// the field is null
	"RDB_REFUSE_MODIFY",		// refuse modify define
	"RDB_REFUSE_RRDB",			// refuse remove the database
	"RDB_INVALID_FOREIGN_INDEX",// invalid foreign index
	"RDB_INDEXFIELD_TOOMANY",	// index field too many
	"RDB_INVALID_INDEXFIELD",	// index index field
	"RDB_TRAVEL_FAILURE",		// tavel table failure
	"RDB_INVALID_COND",			// invalid query condition
	"RDB_RECORD_NOTEXIST_IN_PRIMARY_TABLE",	// the new record not exist in the primary table;
	"RDB_UNIQUE_INDEX_CONFLICT",// the new record has been existed in the table;
	"RDB_RECORD_NOT_EXIST",		// the record not exist in the table;
	"RDB_OPENFILE_FAILURE"		// open file failure
};

const char* GetErrorInfo(uint32 nCode)
{
	return g_ErrorInfo[nCode];
}

FOCP_END();
