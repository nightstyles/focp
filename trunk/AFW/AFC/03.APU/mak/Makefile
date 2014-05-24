
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

FOCP_MODULE_NAME=APU
FOCP_TARGET_TYPE=DLL

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/03.APU/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/03.APU/src

FOCP_LIB_LIST+=AfcBase ADT

FOCP_DEBUG_COMPILE_FILE=$(FOCP_VOB_DIR)/focp/AFW/AFC/03.APU/mak/debug.incl
include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
