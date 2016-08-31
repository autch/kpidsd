#pragma once

/* Based on "Direct Stream Digital Interchange File Format, DSDIFF, version 1.5" from Philips */

#include "dff_types.h"
#include "CLargeFile.h"

struct ChunkStep
{
	uint64_t headerOffset;
	DFFChunkHeader header;
	uint64_t dataOffset;
	uint64_t dataEndOffset;
};

class CDFFFile: public CLargeFile
{
private:
	FRM8Chunk	frm8;
	std::stack<ChunkStep> stack;

	bool readChunkHeader(DFFChunkHeader& header, ChunkStep& step);
	void assignBuffer(uint32_t bytesToRead, std::string& target);

	bool readFRM8(ChunkStep& step, DFFChunkHeader& hdr);
	bool readPROP(ChunkStep& step, DFFChunkHeader& hdr);
	bool readDIIN(ChunkStep& step, DFFChunkHeader& hdr);
public:
	CDFFFile();
	~CDFFFile();

	FRM8Chunk& FRM8() { return frm8; }

	void Reset() { Seek(frm8.dsd.OffsetToData(), NULL, FILE_BEGIN); }
	BOOL Open(CAbstractFile* file);
	void Close();
};
