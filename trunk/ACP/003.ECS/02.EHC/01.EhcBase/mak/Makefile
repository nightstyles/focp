
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

FOCP_MODULE_NAME=ebs
FOCP_TARGET_TYPE=DLL

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC
FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/003.ECS/02.EHC/01.EhcBase/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/003.ECS/02.EHC/01.EhcBase/src

include $(FOCP_VOB_DIR)/focp/AFW/AFC/AfcLib.incl

#debug
export FOCP_APP_NAME=ecs
export FOCP_APPLICATION_TYPE_NUMBER=3
include $(FOCP_VOB_DIR)/focp/AFW/AFC/AfcModule.incl
include $(FOCP_VOB_DIR)/focp/AFW/AFD/AfdModule.incl
include $(FOCP_VOB_DIR)/focp/AFW/AFS/AfsModule.incl

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl
