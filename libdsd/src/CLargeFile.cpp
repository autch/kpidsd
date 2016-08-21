#include "stdafx.h"
#include "CLargeFile.h"

CLargeFile::CLargeFile() : hFile(NULL)
{
}

CLargeFile::~CLargeFile()
{
	Close();
}

void CLargeFile::Close()
{
	if (hFile != NULL) {
		hFile->Close();
		hFile = NULL;
		// do not `delete hFile` here, it's done by decoder
	}
}
