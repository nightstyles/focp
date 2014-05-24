
ifeq ($(FOCP_VOB_DIR),)
	ifneq ($(SVNVOB),)
		FOCP_VOB_DIR=$(SVNVOB)
	else
		ifneq ($(SVNREP),)
			FOCP_VOB_DIR=$(SVNREP)
		else
			FOCP_VOB_DIR=/vob
		endif
	endif
endif

FOCP_MODULE_NAME=AfcBase
FOCP_TARGET_TYPE=DLL
FOCP_VERSION_NAME=focp

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/01.Base/inc
FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/02.ADT/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/01.Base/src

ifeq ($(FOCP_SINGLE_INSTANCE),1)
	FOCP_MACRO_DEFINES+=FOCP_SINGLE_INSTANCE
endif

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
