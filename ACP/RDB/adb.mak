
ifeq ($(FOCP_VOB_DIR),)
	FOCP_VOB_DIR=/vob
endif

export FOCP_APP_NAME=adb
export RDB_SUPPORT_DISK_DB=1
export RDB_SUPPORT_MEMORY_DB=1

include $(FOCP_VOB_DIR)/ACP/RDB/rdb.mak

env2:
	$(FOCP_MK2) $(FOCP_VOB_DIR)/ACP/RDB/SqlApi/mak env