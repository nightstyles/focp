
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

FOCP_MODULE_NAME=LogService
FOCP_TARGET_TYPE=DLL

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/Release/AFC/inc
FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/001.LOG/LogService/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/001.LOG/LogService/src

FOCP_LIB_HOME=$(FOCP_APP_HOME)/lib/Services
FOCP_LIBDIR_LIST+=$(FOCP_VOB_DIR)/focp/Release/AFC/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)

include $(FOCP_VOB_DIR)/focp/Release/AFC/AfcLib.incl
include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
