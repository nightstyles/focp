
#include "MdbApi.hpp"

#ifndef _FOCP_MDB_ERROR_HPP_
#define _FOCP_MDB_ERROR_HPP_

FOCP_BEGIN();

enum
{
// rdb error code
	MDB_SUCCESS = 0,			// success code;
	MDB_INVALID_INPUT,			// invalid parameter
	MDB_INVALID_NAME,			// invalid name: dbname, table name, field name, and so on
	MDB_INVALID_TYPE,			// invalid data type of the field
	MDB_INVALID_SIZE,			// invalid data length of the field
	MDB_REPEAT_FIELD,			// redefine field
	MDB_DB_EXIST,				// the database exist in the system;
	MDB_DB_NOTEXIST,			// the database not exist in the system
	MDB_DB_BUSY,				// the database is busy
	MDB_DB_NOSELECTED,			// there is not any database selected
	MDB_TABLE_EXIST,			// the table exists in the database system
	MDB_TABLE_NOTEXIST,			// not exist the table in the database system
	MDB_RECORD_TOOLONG,			// the record cann't be more than VMM_MAX_MAPSIZE
	MDB_RECORD_TOOMANY,			// the record too many
	MDB_INDEX_EXIST,			// the index exists in the database system
	MDB_INDEX_NOTEXIST,			// the index not exists in the database system
	MDB_INDEX_TOOMANY,			// the index is too many
	MDB_DETAIL_EXIST,			// exist the detail table
	MDB_DETAIL_NOTEXIST,		// not exist the detail table
	MDB_FIELD_NOTEXIST,			// the field not exist
	MDB_FIELD_TOOMANY,			// the field is too many
	MDB_FIELD_ISNULL,			// the field is null
	MDB_INVALID_FOREIGN_INDEX,	// invalid foreign index
	MDB_INDEXFIELD_TOOMANY,		// index field too many
	MDB_INVALID_INDEXFIELD,		// index index field
	MDB_INVALID_INDEX_ARITHMETIC,		// invalid index arithmetic
	MDB_TRAVEL_FAILURE,			// tavel table failure
	MDB_INVALID_COND,			// invalid query condition
	MDB_RECORD_NOTEXIST_IN_PRIMARY_TABLE,	// the new record not exist in the primary table;
	MDB_UNIQUE_INDEX_CONFLICT,	// the new record has been existed in the table;
	MDB_RECORD_NOT_EXIST,		// the record not exist in the table;
	MDB_TRY_UPDATE_PRIMARYKEY, // try to update the primary key;
	MDB_TRIGGER_CHECKFAILURE,
	MDB_INVALID_INTERFACE,
	MDB_NONEMPTY_TABLE,
	MDB_FORBIT_ACCESS,
};

bool MCI_API RegMdbError(uint32 nCode, const char* sInfo);

FOCP_END();

#endif
