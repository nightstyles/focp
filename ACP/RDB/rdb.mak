
ifeq ($(FOCP_VOB_DIR),)
	FOCP_VOB_DIR=/vob
endif

include $(FOCP_VOB_DIR)/AFW/AFC/AfcModule.incl
include $(FOCP_VOB_DIR)/AFW/AFD/AfdModule.incl
include $(FOCP_VOB_DIR)/AFW/AFS/AfsModule.incl

FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/ACP/RDB/VmmApi/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/ACP/RDB/RdbApi/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/ACP/RDB/SqlApi/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/ACP/RDB/RdbService

include $(FOCP_VOB_DIR)/AFW/AFU/Main/Makefile

copy:
	$(FOCP_CP) $(FOCP_VOB_DIR)/ACP/RDB/runtime/* $(FOCP_APP_HOME)
	$(FOCP_CP) $(FOCP_VOB_DIR)/ACP/RDB/NmsCfg $(FOCP_APP_HOME)/../.
ifneq ($(FOCP_OS),WINDOWS)
	$(FOCP_RM) $(FOCP_APP_HOME)/bin/*.exe
endif
