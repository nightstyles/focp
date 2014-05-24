
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

FOCP_MODULE_NAME=AFS
FOCP_TARGET_TYPE=DLL

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/04.AFS/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC/04.AFS/src

FOCP_LIB_LIST+=AfcBase ADT APU

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl

ifeq ($(FOCP_OS), WINDOWS)
	FOCP_LIB_LIST+=netapi32 iphlpapi
endif

ifeq ($(FOCP_OS), CYGWIN_NT)
	FOCP_LIB_LIST+=netapi32 iphlpapi
endif
