
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

FOCP_MODULE_NAME=MCI
FOCP_TARGET_TYPE=DLL

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/Release/AFC/inc
FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/02.MdbApi/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/02.MdbApi/src

FOCP_LIBDIR_LIST+=$(FOCP_VOB_DIR)/focp/Release/AFC/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)

include $(FOCP_VOB_DIR)/focp/Release/AFC/AfcLib.incl
include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
