#pragma once

/* Based on "Direct Stream Digital Interchange File Format, DSDIFF, version 1.5" from Philips */

#include "dff_types.h"
#include "CLargeFile.h"

struct ChunkHeader
{
	uint8_t ckID[4];
	uint64_t ckDataSize;
};

class CDFFFile: public CLargeFile
{
private:
	FRM8Chunk	frm8;

public:
	CDFFFile();
	~CDFFFile();

	BOOL Open(LPCSTR cszFileName);
	void Close();

};
