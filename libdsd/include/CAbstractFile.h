#pragma once

class CAbstractFile
{
public:
	virtual ~CAbstractFile() {}

	virtual void Close() = 0;

	virtual void Reset() const { Seek(0, NULL, FILE_BEGIN); }

	virtual BOOL Read(LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead) const = 0;
	virtual BOOL Seek(int64_t distance, uint64_t* newPos, DWORD moveMethod) const = 0;
	virtual uint64_t FileSize() const = 0;

	virtual uint64_t Tell() const = 0;
};
