#pragma once

#include "wsd_types.h"

class CWSDFile
{
private:
	HANDLE hFile;

	WSD_GENERAL_INFO	header;
	WSD_DATA_SPEC		data_spec;
	WSD_TEXT			text;

	LARGE_INTEGER		liFileSize;

	uint64_t ntohllX(uint64_t be)
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

	uint64_t ntohll(uint64_t be)
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

	uint32_t ntohl(uint32_t be)
	{
		uint8_t* p = (uint8_t*)&be;
		uint32_t r = p[0];

		r = (r << 8) | p[1];
		r = (r << 8) | p[2];
		r = (r << 8) | p[3];

		return r;
	}

	uint16_t ntohs(uint16_t be)
	{
		uint8_t* p = (uint8_t*)&be;
		return (p[0] << 8) | p[1];
	}

public:
	CWSDFile();
	~CWSDFile();

	BOOL Open(LPCSTR szFileName);
	void Close();

	void Reset();

	HANDLE File() const { return hFile; }
	WSD_GENERAL_INFO* Header() { return &header; }
	WSD_DATA_SPEC* DataSpec() { return &data_spec; }
	WSD_TEXT* Text() { return &text; }

	BOOL Seek(LARGE_INTEGER distance, PLARGE_INTEGER newPos, DWORD moveMethod)
	{
		return ::SetFilePointerEx(hFile, distance, newPos, moveMethod);
	}
	BOOL Seek(uint64_t distance, uint64_t* newPos, DWORD moveMethod)
	{
		LARGE_INTEGER lidist, newpos = { 0 };
		BOOL r;

		lidist.QuadPart = (long long)distance;
		r = ::SetFilePointerEx(hFile, lidist, &newpos, moveMethod);
		if (newPos != NULL) *newPos = newpos.QuadPart;
		return r;
	}

	BOOL Read(LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead)
	{
		return ::ReadFile(hFile, buffer, bytesToRead, bytesRead, NULL);
	}

	uint64_t FileSize() const { return liFileSize.QuadPart; }
private:
	BOOL checkHeader();
};