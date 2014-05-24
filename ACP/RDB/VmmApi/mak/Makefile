
ifeq ($(FOCP_VOB_DIR),)
	FOCP_VOB_DIR=/vob
endif

FOCP_MODULE_NAME=VmmApi
FOCP_TARGET_TYPE=DLL

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/AFW/AFC $(FOCP_VOB_DIR)/ACP/RDB/VmmApi/inc $(FOCP_VOB_DIR)/ACP/RDB/VmmApi/det
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/ACP/RDB/VmmApi/src

include $(FOCP_VOB_DIR)/AFW/AFC/AfcLib.incl
include $(FOCP_VOB_DIR)/MakeOption/option.incl
