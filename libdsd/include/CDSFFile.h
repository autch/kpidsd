#pragma once

#include "dsf_types.h"

class CDSFFile
{
private:
	HANDLE hFile;

	DSF_HEADER header;
	DSF_fmt_HEADER fmt_header;

	LARGE_INTEGER liDataOffset;
	LARGE_INTEGER liFileSize;
public:
	CDSFFile();
	~CDSFFile();

	BOOL Open(LPCSTR szFileName);
	void Close();

	void Reset();

	HANDLE File() const { return hFile; }
	DSF_HEADER* Header() { return &header; }
	DSF_fmt_HEADER* FmtHeader() { return &fmt_header;  }

	LARGE_INTEGER FileSize() { return liFileSize; }

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

private:
	BOOL checkHeader();
};
