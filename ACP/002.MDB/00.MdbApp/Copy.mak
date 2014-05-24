
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

include $(FOCP_VOB_DIR)/focp/MakeOption/option.incl

other:
	$(FOCP_MKDIR) $(FOCP_APP_HOME)/lib/Services
ifeq ($(FOCP_OS),WINDOWS)
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/Release/Mdb/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)/*.dll $(FOCP_APP_HOME)/lib/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/Release/Mdb/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)/Services/*.dll $(FOCP_APP_HOME)/lib/.
else
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/Release/Mdb/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)/*.so $(FOCP_APP_HOME)/lib/.
	$(FOCP_CP) $(FOCP_VOB_DIR)/focp/Release/Mdb/lib/$(FOCP_OS_NAME)/$(FOCP_COMPILER)/Services/*.so $(FOCP_APP_HOME)/lib/Services/.
endif
