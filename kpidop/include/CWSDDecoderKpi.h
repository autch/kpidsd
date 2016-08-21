
#pragma once

#include "CWSDFile.h"
#include "wsd_types.h"
#include "kpi_impl.h"
#include "CAbstractFile.h"
#include "CAbstractKpi.h"

#define SAMPLES_PER_BLOCK		(4096)

class CWSDDecoderKpi : public CAbstractKpi
{
private:
	CAbstractFile* pFile;
	CWSDFile file;
	KPI_MEDIAINFO mInfo;

	BYTE* srcBuffer;
	DWORD srcBufferSize;

	BYTE last_marker;

public:
	CWSDDecoderKpi();
	virtual ~CWSDDecoderKpi();

	DWORD Open(const KPI_MEDIAINFO* pRequest, IKpiFile* file, IKpiFolder* folder);
	DWORD WINAPI Select(DWORD dwNumber, const KPI_MEDIAINFO **ppMediaInfo, IKpiTagInfo *pTagInfo);
	void Close();
	UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag);
	DWORD  WINAPI Render(BYTE *pBuffer, DWORD dwSizeSample);
	DWORD  WINAPI UpdateConfig(void *pvReserved) { return 0; }
	void Reset();
};
