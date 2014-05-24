
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

FOCP_MODULE_NAME=FUN
FOCP_TARGET_TYPE=DLL

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/07.Functor/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/07.Functor/src

FOCP_LIB_LIST+=AfcBase ADT APU AFS ASF

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
