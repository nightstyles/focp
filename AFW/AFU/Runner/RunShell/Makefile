
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

FOCP_MODULE_NAME=RunShell
FOCP_TARGET_TYPE=EXE

FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell
FOCP_LIB_LIST+=user32

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
