#include "stdafx.h"
#include "CDFFFile.h"

CDFFFile::CDFFFile()
{

}

CDFFFile::~CDFFFile()
{
	Close();
}

BOOL CDFFFile::Open(LPCSTR cszFileName)
{
	hFile = ::CreateFile(cszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	return TRUE;
}

void CDFFFile::Close()
{
	if (hFile != INVALID_HANDLE_VALUE) {
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
}

uint64_t CDFFFile::readBE64(uint8_t* p)
{
	uint64_t r = p[0];

	r = (r << 8) | p[1];
	r = (r << 8) | p[2];
	r = (r << 8) | p[3];
	r = (r << 8) | p[4];
	r = (r << 8) | p[5];
	r = (r << 8) | p[6];
	r = (r << 8) | p[7];

	return r;
}

uint32_t CDFFFile::readBE32(uint8_t* p)
{
	uint32_t r = p[0];

	r = (r << 8) | p[1];
	r = (r << 8) | p[2];
	r = (r << 8) | p[3];

	return r;
}
