
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

FOCP_WITHOUT_MACRODEF=1
export FOCP_USER_HOME=$(FOCP_VOB_DIR)/focp
export FOCP_PROJECT_NAME=Release
export FOCP_APP_NAME=AFC
export FOCP_LIB_HOME=$(FOCP_APP_HOME)/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)
export FOCP_OBJ_HOME=$(FOCP_APP_HOME)/obj/$(FOCP_OS_NAME)/$(FOCP_COMPILER)

include $(FOCP_VOB_DIR)/focp/AFW/AFC/AfcModule.incl
include $(FOCP_VOB_DIR)/focp/AFW/AFD/AfdModule.incl
include $(FOCP_VOB_DIR)/focp/AFW/AFS/AfsModule.incl

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl

copy:
	$(FOCP_RM) $(FOCP_APP_HOME)/inc
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/inc/01.Base
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/inc/02.ADT
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/inc/03.APU
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/inc/04.AFS
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/inc/05.ASF
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/inc/06.ACM
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/inc/07.Functor
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/inc/08.Rule
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/inc/09.Flow
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/AFC.hpp $(FOCP_APP_HOME)/inc/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/AfcLib.incl $(FOCP_APP_HOME)/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/Copy.mak $(FOCP_APP_HOME)/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/01.Base/inc $(FOCP_APP_HOME)/inc/01.Base/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/02.ADT/ADT.hpp $(FOCP_APP_HOME)/inc/02.ADT/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/02.ADT/inc $(FOCP_APP_HOME)/inc/02.ADT/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/03.APU/APU.hpp $(FOCP_APP_HOME)/inc/03.APU/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/03.APU/inc $(FOCP_APP_HOME)/inc/03.APU/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/04.AFS/AFS.hpp $(FOCP_APP_HOME)/inc/04.AFS/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/04.AFS/inc $(FOCP_APP_HOME)/inc/04.AFS/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/05.ASF/ASF.hpp $(FOCP_APP_HOME)/inc/05.ASF/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/05.ASF/inc $(FOCP_APP_HOME)/inc/05.ASF/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/06.ACM/ACM.hpp $(FOCP_APP_HOME)/inc/06.ACM/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/06.ACM/inc $(FOCP_APP_HOME)/inc/06.ACM/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/07.Functor/FUN.hpp $(FOCP_APP_HOME)/inc/07.Functor/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/07.Functor/inc $(FOCP_APP_HOME)/inc/07.Functor/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/08.Rule/Rule.hpp $(FOCP_APP_HOME)/inc/08.Rule/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/08.Rule/inc $(FOCP_APP_HOME)/inc/08.Rule/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/09.Flow/FLW.hpp $(FOCP_APP_HOME)/inc/09.Flow/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/AFW/AFC/09.Flow/inc $(FOCP_APP_HOME)/inc/09.Flow/.
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/MakeOption/makecmd/MacroDefine clean
	$(FOCP_RM) $(FOCP_APP_HOME)/bin
	$(FOCP_RM) $(FOCP_APP_HOME)/static
	$(FOCP_RM) $(FOCP_APP_HOME)/cfg
