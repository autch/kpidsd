
#pragma once

#include "CAbstractKpi.h"
#include "CWSDFile.h"
#include "wsd_types.h"

#define SAMPLES_PER_BLOCK		(4096)

class CWSDDecoderKpi : public CAbstractKpi
{
private:
	CWSDFile file;

	BYTE* srcBuffer;
	DWORD srcBufferSize;

	SOUNDINFO soundinfo;
	BYTE last_marker;

public:
	CWSDDecoderKpi();
	virtual ~CWSDDecoderKpi();

	BOOL Open(LPSTR szFileName, SOUNDINFO* pInfo);
	void Close();
	DWORD SetPosition(DWORD dwPosition);
	DWORD Render(BYTE* buffer, DWORD dwSize);
	void Reset();

	static BOOL GetTagInfo(const char *cszFileName, IKmpTagInfo *pInfo);
};
