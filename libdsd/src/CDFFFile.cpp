#include "stdafx.h"
#include "CDFFFile.h"

CDFFFile::CDFFFile()
{

}

CDFFFile::~CDFFFile()
{
	CLargeFile::~CLargeFile();
}

void CDFFFile::Close()
{
	CLargeFile::Close();
}

BOOL CDFFFile::Open(LPCSTR cszFileName)
{
	if (!CLargeFile::Open(cszFileName))
		return FALSE;

	return TRUE;
}

