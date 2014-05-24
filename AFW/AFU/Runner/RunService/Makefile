
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

FOCP_MODULE_NAME=RunService
FOCP_TARGET_TYPE=EXE

FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunService
include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
