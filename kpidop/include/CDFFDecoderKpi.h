#pragma once

#include "CDFFFile.h"
#include "CAbstractFile.h"
#include "CAbstractKpi.h"

#define SAMPLES_PER_BLOCK		(4096)

class CDFFDecoderKpi : public CAbstractKpi
{
private:
	CDFFFile file;
	CAbstractFile* pFile;
	KPI_MEDIAINFO mInfo;

	BYTE last_marker;

	PBYTE srcBuffer;
	DWORD srcBufferSize;
public:

	CDFFDecoderKpi();
	virtual ~CDFFDecoderKpi();

	DWORD Open(const KPI_MEDIAINFO* pRequest, IKpiFile* file, IKpiFolder* folder);
	DWORD WINAPI Select(DWORD dwNumber, const KPI_MEDIAINFO **ppMediaInfo, IKpiTagInfo *pTagInfo, DWORD dwTagGetFlags);

	void Close();
	UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag);
	DWORD  WINAPI Render(BYTE *pBuffer, DWORD dwSizeSample);
	DWORD  WINAPI UpdateConfig(void *pvReserved) { return 0; }

	void Reset();
};