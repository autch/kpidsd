
#pragma once

#include "kpi.h"
#include "kpi_impl.h"

class CAbstractKpi: public KbKpiDecoderImpl
{
public:
	virtual DWORD Open(const KPI_MEDIAINFO* pRequest, IKpiFile* file, IKpiFolder* folder) = 0; // returns how many songs in that file
	virtual void Close() = 0;

protected:
	void setBitrate(uint64_t DSDsamplesPerSec, uint64_t samplesPerSec, uint64_t bitsPerSample, uint64_t channels, IKpiTagInfo* pTagInfo);
};
