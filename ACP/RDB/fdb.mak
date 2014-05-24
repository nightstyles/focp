
ifeq ($(FOCP_VOB_DIR),)
	FOCP_VOB_DIR=/vob
endif

export FOCP_APP_NAME=fdb
export RDB_SUPPORT_DISK_DB=1

include $(FOCP_VOB_DIR)/ACP/RDB/rdb.mak
