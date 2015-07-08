#include "stdafx.h"
#include "CID3v2.h"
#include <string.h>

CID3V2Tag::CID3V2Tag() : wholeTag(NULL), tagSize(0)
{

}

CID3V2Tag::~CID3V2Tag()
{
	close();
}

void CID3V2Tag::close()
{
	if (wholeTag != NULL)
	{
		delete[] wholeTag;
		wholeTag = NULL;
		tagSize = 0;
	}
}

bool CID3V2Tag::Parse(uint8_t* data, uint32_t size)
{
	uint8_t* p;
	uint8_t* pe;
	uint32_t actualTagSize;

	if (wholeTag != NULL) close();

	tagSize = size;
	wholeTag = new uint8_t[tagSize];
	memcpy(wholeTag, data, tagSize);

	p = wholeTag;

	read(header.id, p, 3);
	if (memcmp(header.id, ID3V2_HEADER_ID, sizeof header.id) != 0)
		return false;
	header.version = read8(p);
	header.revision = read8(p);
	header.flags = read8(p);
	header.size = readSyncInt32(p);

	actualTagSize = header.size;
	if (actualTagSize > size)
		actualTagSize = size;

	pe = p + actualTagSize;

	switch (header.version)
	{
	case ID3V2_VERSION_3:

		if (ID3V23_HEADER_EXT_HEADER(header.flags))
		{
			extHeader3.size = readSyncInt32(p);
			extHeader3.flags1 = read8(p);
			extHeader3.flags2 = read8(p);
			extHeader3.padding_size = readSyncInt32(p);
			if (ID3V24_EXT_CRC(extHeader3.flags1))
			{
				p += 4;
			}
		}

		break;
	case ID3V2_VERSION_4:
		if (ID3V23_HEADER_EXT_HEADER(header.flags))
		{
			extHeader4.size = readSyncInt32(p);
			extHeader4.flag_size = read8(p);
			extHeader4.flags = read8(p);
			p += extHeader4.size - 6;
		}

		break;
	default:
		return false;
	}

	ID3V2FrameHeader fHeader;
	uint8_t* fData;

	while (p < pe)
	{
		fHeader.id = read32(p);
		if (memcmp(&fHeader.id, "\0\0\0\0", 4) == 0)	// padding found
			break;
		if (header.version == ID3V2_VERSION_4)
			fHeader.size = readSyncInt32(p);
		else
			fHeader.size = read32(p);
		fHeader.flags1 = read8(p);
		fHeader.flags2 = read8(p);

		fData = new uint8_t[fHeader.size];
		read(fData, p, fHeader.size);

		CID3V2Frame frame(fHeader, fData);

		if (frames.find(fHeader.id) == frames.end())
			frames[fHeader.id] = frame; //CID3V2Frame(fHeader, fData);

		delete[] fData;
	}

	return true;
}

CID3V2TextFrame CID3V2Tag::FindTextFrame(ID3V2_ID id)
{
	ID3V2FrameMap::iterator it = frames.find(id);
	if (it == frames.end())
		return CID3V2TextFrame();
	if (it->second.id[3] != 'T')
		return CID3V2TextFrame();

	return CID3V2TextFrame(it->second);
}
