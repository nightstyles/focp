
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

export FOCP_APP_NAME=logs

export FOCP_APPLICATION_TYPE_NUMBER=1

export FOCP_FREE_LICENSE=1

FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/001.LOG/LogService/mak

include $(FOCP_VOB_DIR)/focp/AFW/AFU/Main/Makefile

copy:
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/ACP/001.LOG/runtime/* $(FOCP_APP_HOME)
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/ACP/001.LOG/nms $(FOCP_APP_HOME)/../.
ifneq ($(FOCP_OS),WINDOWS)
	$(FOCP_RM) $(FOCP_APP_HOME)/bin/*.exe
endif
