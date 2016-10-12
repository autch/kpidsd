
#pragma once

#include "kpi.h"
#include "kpi_impl.h"

class CAbstractKpi: public KbKpiDecoderImpl
{
public:
	virtual DWORD Open(const KPI_MEDIAINFO* pRequest, IKpiFile* file, IKpiFolder* folder) = 0; // returns how many songs in that file
	virtual void Close() = 0;

protected:
	void setBitrate(uint64_t DSDsamplesPerSec, uint64_t samplesPerSec, uint64_t bitsPerSample, uint64_t channels, IKpiTagInfo* pTagInfo)
	{
		wchar_t bitrate[32];
		uint64_t br;

		br = samplesPerSec * bitsPerSample;
		if(br >= 1000 * 1000) 
			swprintf_s(bitrate, L"%lld.%03lldMbps", br / 1000 / 1000, (br / 1000) % 1000);
		else if(br >= 1000)
			swprintf_s(bitrate, L"%lld.%03lldkbps", br / 1000, br % 1000);
		else
			swprintf_s(bitrate, L"%lldbps", br);

		pTagInfo->wSetValueW(SZ_KMP_NAME_BITRATE_W, -1, bitrate, -1);
		{
			wchar_t samplesPerSec_text[32];

			swprintf_s(samplesPerSec_text, L"%lld.%03lldMHz", DSDsamplesPerSec / 1000 / 1000, (DSDsamplesPerSec / 1000) % 1000);
			pTagInfo->wSetValueW(L"DSD_SamplesPerSec", -1, samplesPerSec_text, -1);
		}
	}
};
