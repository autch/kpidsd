
#pragma once

#include "CDSFFile.h"
#include "dsf_types.h"
#include "CAbstractFile.h"
#include "kpi_impl.h"
#include "CAbstractKpi.h"

class CDSFDecoderKpi : public CAbstractKpi
{
private:
	CAbstractFile* pFile;
	CDSFFile file;
	KPI_MEDIAINFO mInfo;

	BYTE* srcBuffer;
	DWORD srcBufferSize;
	
	uint64_t samplesRendered;

	BYTE last_marker;

	DWORD decodeLSBFirst(PBYTE buffer, DWORD dwSize);
	DWORD decodeMSBFirst(PBYTE buffer, DWORD dwSize);

	// http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv
	inline BYTE reverse(BYTE b) const
	{
		return ((b * 0x0202020202ULL & 0x010884422010ULL) % 1023) & 0xff;
	}

public:
	CDSFDecoderKpi();
	virtual ~CDSFDecoderKpi();

	DWORD Open(const KPI_MEDIAINFO* pRequest, IKpiFile* file, IKpiFolder* folder);
	DWORD WINAPI Select(DWORD dwNumber, const KPI_MEDIAINFO **ppMediaInfo, IKpiTagInfo *pTagInfo);
	void Close();
	UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag);
	DWORD  WINAPI Render(BYTE *pBuffer, DWORD dwSizeSample);
	DWORD  WINAPI UpdateConfig(void *pvReserved) { return 0; }
	void Reset();
};
