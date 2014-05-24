
ifeq ($(FOCP_VOB_DIR),)
	FOCP_VOB_DIR=/vob
endif

export FOCP_APP_NAME=jc

FOCP_MODULE_NAME=$(FOCP_APP_NAME)
FOCP_TARGET_TYPE=EXE

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/ICP/jcc/jc_api/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/ICP/jcc/jc_api/src

include $(FOCP_VOB_DIR)/MakeOption/option.incl
