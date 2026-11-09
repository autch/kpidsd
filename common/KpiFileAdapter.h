#pragma once

#include "kpi.h"
#include "CAbstractFile.h"

// libdsd のパーサーに IKpiFile を読ませるためのアダプタ。IKpiFile の参照は持たない
class KpiFileAdapter : public CAbstractFile
{
private:
	IKpiFile* kpiFile;

public:
	explicit KpiFileAdapter(IKpiFile* file) : kpiFile(file) {}

	void Close() override {}

	BOOL Read(LPVOID buffer, DWORD bytesToRead, LPDWORD pBytesRead) const override
	{
		const DWORD bytesRead = kpiFile->Read(buffer, bytesToRead);
		if (pBytesRead != nullptr)
			*pBytesRead = bytesRead;
		return bytesRead > 0 && bytesRead <= bytesToRead;
	}
	BOOL Seek(int64_t distance, uint64_t* newPos, DWORD moveMethod) const override
	{
		const uint64_t pos = kpiFile->Seek(distance, moveMethod);
		if (newPos != nullptr)
			*newPos = pos;
		return pos != KPI_FILE_EOF;
	}
	uint64_t FileSize() const override
	{
		return kpiFile->GetSize();
	}
	uint64_t Tell() const override
	{
		return kpiFile->Seek(0, FILE_CURRENT);
	}
};
