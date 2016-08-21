#pragma once

#include "dsf_types.h"
#include "CLargeFile.h"

class CDSFFile: public CLargeFile
{
private:
	DSF_HEADER header;
	DSF_fmt_HEADER fmt_header;
	DSF_data_HEADER data_header;

	uint64_t dataOffset;

	BOOL checkHeader();
public:
	CDSFFile();
	~CDSFFile();

	BOOL Open(CAbstractFile* file);
	void Close();

	void Reset();

	DSF_HEADER* Header() { return &header; }
	DSF_fmt_HEADER* FmtHeader() { return &fmt_header;  }
	DSF_data_HEADER* DataHeader() { return &data_header; }
	uint64_t DataOffset() const { return dataOffset; }
};
