
#pragma once

#include "kpi.h"
#include "kpi_impl.h"

class CAbstractKpi: public KbKpiDecoderImpl
{
public:
	virtual DWORD Open(const KPI_MEDIAINFO* pRequest, IKpiFile* file, IKpiFolder* folder) = 0; // returns how many songs in that file
	virtual void Close() = 0;

protected:
	void setBitrate(uint64_t samplesPerSec, uint64_t channels, IKpiTagInfo* pTagInfo)
	{
		wchar_t bitrate[32];
		long br;

		br = samplesPerSec * channels;
		if(br >= 1000 * 1000) 
			swprintf_s(bitrate, L"%d.%03dMbps", br / 1000 / 1000, (br / 1000) % 1000);
		else if(br >= 1000)
			swprintf_s(bitrate, L"%d.%03dkbps", br / 1000, br % 1000);
		else
			swprintf_s(bitrate, L"%dbps", br);

		pTagInfo->wSetValueW(SZ_KMP_NAME_BITRATE, -1, bitrate, -1);
	}
};
