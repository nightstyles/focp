
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
	PATH:=$(FOCP_VOB_DIR)/focp/MakeOption/$(FOCP_MAKETOOL_DIR):$(PATH)
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
	flex -Li+ SqlLex.l
ifeq ($(FOCP_OS),WINDOWS)
	xsh SqlLex.awk
else
	sh SqlLex.awk
endif
	$(FOCP_RM) lex.yy.cc
	bison -ld SqlYacc.y
ifeq ($(FOCP_OS),WINDOWS)
	xsh SqlYacc.awk
else
	sh SqlYacc.awk
endif
	$(FOCP_RM) SqlYacc.tab.c
	$(FOCP_RM) SqlYacc.tab.h
