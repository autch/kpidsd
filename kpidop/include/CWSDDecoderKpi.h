#pragma once

#include "CWSDFile.h"
#include "wsd_types.h"
#include "CAbstractFile.h"
#include "CAbstractKpi.h"
#include "DSD2DoP.h"

#define SAMPLES_PER_BLOCK		(4096)

class CWSDDecoderKpi : public CAbstractKpi
{
private:
	CWSDFile file;
	CAbstractFile* pFile;
	KPI_MEDIAINFO mInfo;
	DSD2DoP dsd2dop;

	BYTE* srcBuffer;
	DWORD srcBufferSize;

public:
	CWSDDecoderKpi();
	virtual ~CWSDDecoderKpi();

	DWORD Open(const KPI_MEDIAINFO* pRequest, IKpiFile* file, IKpiFolder* folder);
	DWORD WINAPI Select(DWORD dwNumber, const KPI_MEDIAINFO **ppMediaInfo, IKpiTagInfo *pTagInfo, DWORD dwTagGetFlags);
	void Close();
	UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag);
	DWORD  WINAPI Render(BYTE *pBuffer, DWORD dwSizeSample);
	DWORD  WINAPI UpdateConfig(void *pvReserved) { return 0; }
	void Reset();
};
