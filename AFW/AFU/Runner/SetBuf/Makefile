
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

FOCP_MODULE_NAME=SetBuf
FOCP_TARGET_TYPE=DLL

FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/SetBuf

FOCP_LIB_HOME=$(FOCP_APP_HOME)/bin
FOCP_LIBDIR_LIST+=$(FOCP_APP_HOME)/lib

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
