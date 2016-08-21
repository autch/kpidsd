
#pragma once

#include "kpi.h"
#include "kpi_impl.h"

class CAbstractKpi: public KbKpiDecoderImpl
{
public:
	virtual DWORD Open(const KPI_MEDIAINFO* pRequest, IKpiFile* file, IKpiFolder* folder) = 0; // returns how many songs in that file
	virtual void Close() = 0;
};
