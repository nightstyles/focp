
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

FOCP_MODULE_NAME=FLW
FOCP_TARGET_TYPE=DLL

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/09.Flow/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/09.Flow/src

FOCP_LIB_LIST+=AfcBase ADT APU AFS ASF RULE

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
