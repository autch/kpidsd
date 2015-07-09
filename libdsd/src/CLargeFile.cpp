#include "stdafx.h"
#include "CLargeFile.h"

CLargeFile::CLargeFile() : hFile(INVALID_HANDLE_VALUE)
{
	liFileSize.QuadPart = 0;
}

CLargeFile::~CLargeFile()
{
	Close();
}

void CLargeFile::Close()
{
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
}

BOOL CLargeFile::Open(LPCSTR szFileName)
{
	hFile = ::CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	::GetFileSizeEx(hFile, &liFileSize);

	return TRUE;
}
