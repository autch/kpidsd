#pragma once

#include "kpi.h"
#include "CAbstractFile.h"

class CKpiFileAdapter : public CAbstractFile
{
private:
	IKpiFile* kpiFile;
public:
	CKpiFileAdapter(IKpiFile* file) : kpiFile(file)
	{
	}
	virtual ~CKpiFileAdapter()
	{
		Close();
	}

	virtual void Close()
	{
		// do not kpiFile->Release()
	}

	virtual BOOL Read(LPVOID buffer, DWORD bytesToRead, LPDWORD pBytesRead) const
	{
		DWORD bytesRead = kpiFile->Read(buffer, bytesToRead);
		if (pBytesRead != NULL)
			*pBytesRead = bytesRead;
		return bytesRead > 0 && bytesRead <= bytesToRead;
	}
	virtual BOOL Seek(int64_t distance, uint64_t* newPos, DWORD moveMethod) const
	{
		uint64_t new_pos;
		new_pos = kpiFile->Seek(distance, moveMethod);
		if (newPos != NULL)
			*newPos = new_pos;
		return new_pos != KPI_FILE_EOF;
	}
	virtual uint64_t FileSize() const
	{
		return kpiFile->GetSize();
	}

	virtual uint64_t Tell() const
	{
		uint64_t pos = 0;
		Seek(0, &pos, FILE_CURRENT);
		return pos;
	}

	IKpiFile* GetKpiFile() const { return kpiFile; }
};
