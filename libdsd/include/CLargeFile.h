#pragma once

#include "CAbstractFile.h"

class CLargeFile
{
protected:
	CAbstractFile*	hFile;

public:
	CLargeFile();
	virtual ~CLargeFile();

	virtual BOOL Open(CAbstractFile* file) = 0;
	virtual void Close();

	virtual void Reset() { Seek(0, NULL, FILE_BEGIN); }

	uint64_t FileSize() const { return hFile->FileSize(); }

	BOOL Seek(int64_t distance, uint64_t* newPos, DWORD moveMethod) const
	{
		return hFile->Seek(distance, newPos, moveMethod);
	}
	uint64_t Tell() const
	{
		uint64_t newPos = 0;
		hFile->Seek(0, &newPos, FILE_CURRENT);
		return newPos;
	}

	BOOL Read(LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead) const
	{
		return hFile->Read(buffer, bytesToRead, bytesRead);
	}

	uint64_t ntohllX(uint64_t be) const
	{
		uint8_t* p = (uint8_t*)&be;
		uint64_t r = p[4];

		r = (r << 8) | p[5];
		r = (r << 8) | p[6];
		r = (r << 8) | p[7];
		r = (r << 8) | p[0];
		r = (r << 8) | p[1];
		r = (r << 8) | p[2];
		r = (r << 8) | p[3];

		return r;
	}

	uint64_t ntohll(uint64_t be) const
	{
		uint8_t* p = (uint8_t*)&be;
		uint64_t r = p[0];

		r = (r << 8) | p[1];
		r = (r << 8) | p[2];
		r = (r << 8) | p[3];
		r = (r << 8) | p[4];
		r = (r << 8) | p[5];
		r = (r << 8) | p[6];
		r = (r << 8) | p[7];

		return r;
	}

	uint32_t ntohl(uint32_t be) const
	{
		uint8_t* p = (uint8_t*)&be;
		uint32_t r = p[0];

		r = (r << 8) | p[1];
		r = (r << 8) | p[2];
		r = (r << 8) | p[3];

		return r;
	}

	uint16_t ntohs(uint16_t be) const
	{
		uint8_t* p = (uint8_t*)&be;
		return (p[0] << 8) | p[1];
	}
};
