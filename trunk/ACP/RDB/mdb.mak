
ifeq ($(FOCP_VOB_DIR),)
	FOCP_VOB_DIR=/vob
endif

export FOCP_APP_NAME=mdb
export RDB_SUPPORT_MEMORY_DB=1

include $(FOCP_VOB_DIR)/ACP/RDB/rdb.mak
