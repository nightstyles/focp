
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

export FOCP_APP_NAME=mcode
FOCP_MODULE_NAME=$(FOCP_APP_NAME)
FOCP_TARGET_TYPE=EXE

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFU/MsgParser

include $(FOCP_VOB_DIR)/focp/AFW/AFC/AfcModule.incl
include $(FOCP_VOB_DIR)/focp/AFW/AFD/AfdModule.incl
include $(FOCP_VOB_DIR)/focp/AFW/AFC/AfcLib.incl

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl

copy:
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/09.Flow/src/mLang.syn $(FOCP_APP_HOME)/bin/.
ifneq ($(FOCP_OS),WINDOWS)
	$(FOCP_RM) $(FOCP_APP_HOME)/bin/*.exe
endif
