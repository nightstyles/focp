
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

export FOCP_APP_NAME=Smp

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
	FOCP_MACRO_DEFINES+=UNIX
	FOCP_MAKETOOL_DIR=cygwincmd
endif

ifeq ($(FOCP_OS),LINUX)
	FOCP_MACRO_DEFINES+=UNIX
	FOCP_MAKETOOL_DIR=linuxcmd
endif

ifeq ($(FOCP_OS),WINDOWS)
	FOCP_MAKETOOL_DIR=wincmd
endif

ifneq ($(FOCP_OS),WINDOWS)
	FOCP_MACRO_DEFINES+=_POSIX_PTHREAD_SEMANTICS REENTRANT
	PATH:=$(FOCP_VOB_DIR)/focp/MakeOption/$(FOCP_MAKETOOL_DIR):$(PATH)
endif

FOCP_MACRO_DEFINES+=$(FOCP_OS)

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
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/SetBuf
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunService
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell

buildclean:
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/SetBuf buildclean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell buildclean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunService buildclean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell buildclean

veryclean:
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/SetBuf veryclean

installclean:
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/SetBuf installclean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell installclean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunService installclean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell installclean

clean:
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/SetBuf clean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell clean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunService clean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell clean

execlean:
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/SetBuf execlean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell execlean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunService execlean
	$(FOCP_MK2) $(FOCP_VOB_DIR)/focp/AFW/AFU/Runner/RunShell execlean
