#pragma once

#include "dsf_types.h"
#include "CLargeFile.h"

class CDSFFile: public CLargeFile
{
private:
	DSF_HEADER header;
	DSF_fmt_HEADER fmt_header;

	LARGE_INTEGER liDataOffset;

	BOOL checkHeader();
public:
	CDSFFile();
	~CDSFFile();

	BOOL Open(LPCSTR szFileName);
	void Close();

	void Reset();

	DSF_HEADER* Header() { return &header; }
	DSF_fmt_HEADER* FmtHeader() { return &fmt_header;  }
};
