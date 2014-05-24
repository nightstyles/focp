
FOCP_OS?=$(shell uname -s)

ifneq ($(filter Windows%, $(FOCP_OS)),)
	FOCP_OS=WINDOWS
endif

ifneq ($(filter CYGWIN_NT%, $(FOCP_OS)),)
	FOCP_OS=CYGWIN_NT
endif

ifeq ($(FOCP_OS),Linux)
	FOCP_OS=LINUX
endif

ifeq ($(FOCP_OS),CYGWIN_NT)
	FOCP_MAKETOOL_DIR=cygwincmd
endif

ifeq ($(FOCP_OS),LINUX)
	FOCP_MAKETOOL_DIR=linuxcmd
endif

ifeq ($(FOCP_OS),WINDOWS)
	FOCP_MAKETOOL_DIR=wincmd/bin
endif

ifneq ($(FOCP_OS),WINDOWS)
	PATH:=$(FOCP_VOB_DIR)/MakeOption/$(FOCP_MAKETOOL_DIR):$(PATH)
endif

FOCP_MKDIR=mkdir -p
FOCP_CP=cp -rf
FOCP_RM=rm -rf
FOCP_MK=make -f
FOCP_MK2=make -C
FOCP_CHMOD=chmod

ifeq ($(FOCP_OS),WINDOWS)
	FOCP_MKDIR=mkdir.exe -p
	FOCP_CHMOD=chmod.exe
endif

all:
	flex -Li+ SqlLex.1.l
ifeq ($(FOCP_OS),WINDOWS)
	xsh SqlLex.1.awk
else
	sh SqlLex.1.awk
endif
	$(FOCP_RM) lex.yy.cc
	flex -Li+ SqlLex.2.l
ifeq ($(FOCP_OS),WINDOWS)
	xsh SqlLex.2.awk
else
	sh SqlLex.2.awk
endif
	$(FOCP_RM) lex.yy.cc
	bison -ld SqlYacc.1.y
ifeq ($(FOCP_OS),WINDOWS)
	xsh SqlYacc.1.awk
else
	sh SqlYacc.1.awk
endif
	$(FOCP_RM) SqlYacc.1.tab.c
	$(FOCP_RM) SqlYacc.1.tab.h
	bison -ld SqlYacc.2.y
ifeq ($(FOCP_OS),WINDOWS)
	xsh SqlYacc.2.awk
else
	sh SqlYacc.2.awk
endif
	$(FOCP_RM) SqlYacc.2.tab.c
	$(FOCP_RM) SqlYacc.2.tab.h
	bison -ld SqlYacc.3.y
ifeq ($(FOCP_OS),WINDOWS)
	xsh SqlYacc.3.awk
else
	sh SqlYacc.3.awk
endif
	$(FOCP_RM) SqlYacc.3.tab.c
	$(FOCP_RM) SqlYacc.3.tab.h
