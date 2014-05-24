
#include "AfcBase.hpp"

#ifdef WINDOWS
#include <windows.h>
#include <direct.h>
#include <string.h>

#define DIRMAGIC 0xddaa

FOCP_BEGIN();

struct dirent
{
	char* d_name;
	bool bDirectory;
};

typedef struct DIR DIR;

struct DIR
{
	ulong  _d_hdir;              /* directory handle */
	char*  _d_dirname;           /* directory name */
	uint32 _d_magic;             /* magic cookie for verifying handle */
	uint32 _d_nfiles;            /* no. of files remaining in buf */
	char   _d_buf[sizeof(WIN32_FIND_DATA)];  /* buffer for a single file */
	dirent _d_dirent;
};

static DIR* opendir(const char *dirname)
{
	char *name;
	int len;
	DIR *dir;
	HANDLE h;

	len = strlen(dirname);
	name = new char[len+5];
	strcpy(name, dirname);

	if (len-- && name[len] != '\\')
		strcat(name, "\\*.*");
	else
		strcat(name,"*.*");

	dir = new DIR;

	if ((h = FindFirstFile((LPSTR)name, (LPWIN32_FIND_DATA)&dir->_d_buf[0])) == (HANDLE)-1)
	{
		delete[] name;
		delete dir;
		return (NULL);
	}

	dir->_d_nfiles  = 1;
	dir->_d_hdir    = (unsigned int)h;
	dir->_d_dirname = name;
	dir->_d_magic   = DIRMAGIC;
	return dir;
}

static void rewinddir(DIR *dir)
{
	HANDLE h;

	if (dir->_d_magic != DIRMAGIC)
		return;

	FindClose((HANDLE)dir->_d_hdir);

	if ((h = FindFirstFile((LPSTR)dir->_d_dirname,
						   (LPWIN32_FIND_DATA)&dir->_d_buf[0])) != (HANDLE)-1)
	{
		dir->_d_hdir   = (unsigned int)h;
		dir->_d_nfiles = 1;
	}
}

static struct dirent * readdir(DIR *dir)
{
	WIN32_FIND_DATA *ff;

	if (dir->_d_magic != DIRMAGIC)
		return (NULL);

	if (dir->_d_nfiles == 0)
	{
		if (FindNextFile((HANDLE)dir->_d_hdir,
						 (LPWIN32_FIND_DATA)&dir->_d_buf[0]) != TRUE)
			return (NULL);
	}
	else
		dir->_d_nfiles = 0;

	ff = (WIN32_FIND_DATA *)(dir->_d_buf);
	dir->_d_dirent.d_name = ff->cFileName;
	dir->_d_dirent.bDirectory = ((ff->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==FILE_ATTRIBUTE_DIRECTORY);

	return &dir->_d_dirent;
}

static int closedir(DIR *dir)
{
	if (dir == NULL || dir->_d_magic != DIRMAGIC)
		return (-1);

	dir->_d_magic = 0;
	FindClose((HANDLE)dir->_d_hdir);
	delete[] dir->_d_dirname;
	delete dir;

	return 0;
}
FOCP_END();
#else
#include <dirent.h>
#endif

FOCP_BEGIN();

AFCBASE_API void* OpenDirectory(const char* sDirectory)
{
	return opendir(sDirectory);
}

AFCBASE_API void CloseDirectory(void* pDirectory)
{
	closedir((DIR*)pDirectory);
}

AFCBASE_API const char* ReadDirectory(void* pDirectory, bool &bIsDirectory)
{
	struct dirent * pDir = readdir((DIR*)pDirectory);
	if(pDir == NULL)
		return NULL;
#ifdef WINDOWS
	bIsDirectory = pDir->bDirectory;
#else
	bIsDirectory = (pDir->d_type == DT_DIR);//需要定义宏_BSD_SOURCE
#endif
	return pDir->d_name;
}

AFCBASE_API void RewindDirectory(void* pDirectory)
{
	rewinddir((DIR*)pDirectory);
}

FOCP_END();
