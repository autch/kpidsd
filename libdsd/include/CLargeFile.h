#pragma once

class CLargeFile
{
protected:
	HANDLE			hFile;
	LARGE_INTEGER	liFileSize;

public:
	CLargeFile();
	virtual ~CLargeFile();

	virtual BOOL Open(LPCSTR szFileName);
	virtual void Close();

	virtual void Reset() { Seek(0, NULL, FILE_BEGIN); }

	HANDLE File() const { return hFile; }
	uint64_t FileSize() const { return liFileSize.QuadPart; }

	BOOL Seek(uint64_t distance, uint64_t* newPos, DWORD moveMethod) const
	{
		LARGE_INTEGER lidist, newpos = { 0 };
		BOOL r;

		lidist.QuadPart = (long long)distance;
		r = ::SetFilePointerEx(hFile, lidist, &newpos, moveMethod);
		if (newPos != NULL) *newPos = newpos.QuadPart;
		return r;
	}
	uint64_t Tell() const
	{
		LARGE_INTEGER zero = { 0 };

		::SetFilePointerEx(hFile, zero, &zero, FILE_CURRENT);
		return (uint64_t)zero.QuadPart;
	}

	BOOL Read(LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead) const
	{
		return ::ReadFile(hFile, buffer, bytesToRead, bytesRead, NULL);
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
