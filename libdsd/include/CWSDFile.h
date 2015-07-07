#pragma once

#include "wsd_types.h"
#include "CLargeFile.h"

class CWSDFile: public CLargeFile
{
private:
	WSD_GENERAL_INFO	header;
	WSD_DATA_SPEC		data_spec;
	WSD_TEXT			text;

	BOOL checkHeader();

public:
	CWSDFile();
	~CWSDFile();

	BOOL Open(LPCSTR szFileName);
	void Close();

	void Reset();

	WSD_GENERAL_INFO* Header() { return &header; }
	WSD_DATA_SPEC* DataSpec() { return &data_spec; }
	WSD_TEXT* Text() { return &text; }
};