#pragma once

#include "CDSFFile.h"
#include "dsf_types.h"
#include "CAbstractFile.h"
#include "CAbstractKpi.h"
#include "DSD2DoP.h"

class CDSFDecoderKpi : public CAbstractKpi
{
private:
	CDSFFile file;
	CAbstractFile* pFile;
	KPI_MEDIAINFO mInfo;
	DSD2DoP dsd2dop;

	BYTE* srcBuffer;
	DWORD srcBufferSize;
	
	uint64_t samplesRendered;

public:
	CDSFDecoderKpi();
	virtual ~CDSFDecoderKpi();

	DWORD Open(const KPI_MEDIAINFO* pRequest, IKpiFile* file, IKpiFolder* folder);
	DWORD WINAPI Select(DWORD dwNumber, const KPI_MEDIAINFO **ppMediaInfo, IKpiTagInfo *pTagInfo, DWORD dwTagGetFlags);
	void Close();
	UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag);
	DWORD  WINAPI Render(BYTE *pBuffer, DWORD dwSizeSample);
	DWORD  WINAPI UpdateConfig(void *pvReserved) { return 0; }
	void Reset();
};
