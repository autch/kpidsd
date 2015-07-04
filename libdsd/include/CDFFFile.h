#pragma once

/* Based on "Direct Stream Digital Interchange File Format, DSDIFF, version 1.5" from Philips */

#include "dff_types.h"

struct ChunkHeader
{
	uint8_t ckID[4];
	uint64_t ckDataSize;
};

class Chunk
{
public:
	uint8_t ckID[4];
	uint64_t ckDataSize;

	std::vector<Chunk> localChunks;
	uint64_t offsetToData;

	void setID(uint8_t* id) { memcpy(ckID, id, 4); }
};

class CDFFFile
{
private:
	HANDLE hFile;

	Chunk frm8;

	uint64_t readBE64(uint8_t* p);
	uint32_t readBE32(uint8_t* p);

	int readChunks(Chunk parent, int hasLocalChunks);

	uint64_t tell()
	{
		LARGE_INTEGER li = { 0 };
		::SetFilePointerEx(hFile, li, &li, FILE_CURRENT);
		return li.QuadPart;
	}
public:
	CDFFFile();
	~CDFFFile();

	BOOL Open(LPCSTR cszFileName);
	void Close();

};
