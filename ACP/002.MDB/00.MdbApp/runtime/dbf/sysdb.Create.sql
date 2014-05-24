
mdb

use sysdb;

create table MdbSecurity
(
	MdbName string size=64,
	UserName string size=8,
	PassWord string size=8
);
create unique index MdbSecurityIndex On MdbSecurity(MdbName) by rbtree;

create table AccessControl
(
	HostId uint64 not null,
	MdbName string size=64 not null,
	UserName string size=8,
	PassWord string size=8
);
create unique index AccessControlIndex On AccessControl(HostId,MdbName,UserName) by rbtree;

create table MdbStoreEvent
(
	TableName string size=64 not null,
	PrimaryKey string size=256,
	Action uint32 not null,
	EventTime uint32 not null
);
create unique index MainIndex on MdbStoreEvent(TableName,PrimaryKey) by rbtree;

create table sysdbStoreEvent
(
	TableName string size=64 not null,
	PrimaryKey string size=256,
	Action uint32 not null,
	EventTime uint32 not null
);
create unique index sysdbMainIndex on sysdbStoreEvent(TableName,PrimaryKey) by rbtree;

quit;
