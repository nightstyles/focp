
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

export FOCP_APP_NAME=mdb

export FOCP_APPLICATION_TYPE_NUMBER=2

FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/02.MdbApi/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/03.MdbSql/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/04.MdbLoc/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/05.MdbLsn/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/06.MdbCon/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/07.MdbRep/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/08.MdbSto/mak
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/01.MdbSvr
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/00.MdbApp/OdbcLic
FOCP_MODULE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/00.MdbApp/MdbLic

# extend Main.cpp for the detail application
#FOCP_MACRO_DEFINES+=AFC_APP_CLASS=CMdbApplication
#FOCP_SOURCE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/00.MdbApp
#FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/00.MdbApp
#FOCP_INCLUDE_LIST+=$(FOCP_VOB_DIR)/focp/ACP/002.MDB/02.MdbApi/inc
#FOCP_LIB_LIST+=MCI

include $(FOCP_VOB_DIR)/focp/AFW/AFU/Main/Makefile

buildclean:
	$(FOCP_RM) $(FOCP_APP_HOME)/lib
	$(FOCP_RM) $(FOCP_APP_HOME)/obj
	$(FOCP_RM) $(FOCP_APP_HOME)/bin/Odbc*
	$(FOCP_RM) $(FOCP_APP_HOME)/bin/Mdb*
	$(FOCP_RM) $(FOCP_APP_HOME)/bin/mdb*

copy:
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/ACP/002.MDB/00.MdbApp/runtime/* $(FOCP_APP_HOME)
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/ACP/002.MDB/00.MdbApp/nms $(FOCP_APP_HOME)/../.
ifneq ($(FOCP_OS),WINDOWS)
	$(FOCP_RM) $(FOCP_APP_HOME)/bin/*.exe
endif
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/Release/Mdb/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)/*MdbLoc* $(FOCP_APP_HOME)/lib/.

other:
	$(FOCP_RM) $(FOCP_VOB_DIR)/focp/Release/Mdb/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)
	$(FOCP_RM) $(FOCP_VOB_DIR)/focp/Release/Mdb/obj/$(FOCP_OS_NAME)/$(FOCP_COMPILER)
	$(FOCP_RM) $(FOCP_VOB_DIR)/focp/Release/Mdb/inc
	$(FOCP_MKDIR) $(FOCP_VOB_DIR)/focp/Release/Mdb/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)
	$(FOCP_MKDIR) $(FOCP_VOB_DIR)/focp/Release/Mdb/obj/$(FOCP_OS_NAME)/$(FOCP_COMPILER)
	$(FOCP_MKDIR) $(FOCP_VOB_DIR)/focp/Release/Mdb/inc
	$(FOCP_CP) $(FOCP_APP_HOME)/lib/* $(FOCP_VOB_DIR)/focp/Release/Mdb/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)/.
	$(FOCP_CP) $(FOCP_APP_HOME)/obj/* $(FOCP_VOB_DIR)/focp/Release/Mdb/obj/$(FOCP_OS_NAME)/$(FOCP_COMPILER)/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/ACP/002.MDB/02.MdbApi/inc/MdbApi.hpp $(FOCP_VOB_DIR)/focp/Release/Mdb/inc/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/ACP/002.MDB/00.MdbApp/Copy.mak $(FOCP_VOB_DIR)/focp/Release/Mdb/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/ACP/002.MDB/00.MdbApp/MdbLib.incl $(FOCP_VOB_DIR)/focp/Release/Mdb/.

release: CleanMacroDefine buildclean makedir lib exe other copy installclean

