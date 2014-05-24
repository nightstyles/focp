
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

FOCP_MODULE_NAME=UdpFile
FOCP_TARGET_TYPE=DLL

FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFC $(FOCP_VOB_DIR)/focp/AFW/AFD/UdpFile/inc
FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/AFW/AFD/UdpFile/src

FOCP_LIB_HOME2:=$(FOCP_LIB_HOME)
ifneq ($(FOCP_LIB_HOME2),)
	FOCP_LIB_HOME=$(FOCP_LIB_HOME2)/FileInterfaces
	FOCP_LIBDIR_LIST+=$(FOCP_LIB_HOME2)
else
	FOCP_LIB_HOME=$(FOCP_APP_HOME)/lib/FileInterfaces
	FOCP_LIBDIR_LIST+=$(FOCP_APP_HOME)/lib
endif

include $(FOCP_VOB_DIR)/focp/AFW/AFC/AfcLib.incl
include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl



