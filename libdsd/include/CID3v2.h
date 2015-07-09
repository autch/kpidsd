#pragma once

#include "ID3v2.h"
#include <vector>
#include <map>

struct CID3V2Frame;
struct CID3V2TextFrame;

typedef std::map<ID3V2_ID, CID3V2Frame> ID3V2FrameMap;
typedef std::map<ID3V2_ID, CID3V2Frame> ID3V2TextFrameMap;

class CID3V2Tag
{
private:
	uint8_t* wholeTag;
	uint32_t tagSize;

	ID3V2Header header;
	ID3V23ExtHeader extHeader3;
	ID3V24ExtHeader extHeader4;
	ID3V24Footer footer;

	ID3V2FrameMap frames;

protected:
	void read(void* dest, uint8_t*& src, uint32_t size)
	{
		memcpy(dest, src, size);
		src += size;
	}
	uint32_t read32(uint8_t*& p)
	{
		uint8_t buf[4];
		uint32_t r = 0;

		read(buf, p, 4);
		r =   (buf[0] << 24)
			| (buf[1] << 16)
			| (buf[2] << 8)
			|  buf[3];
		return r;
	}

	syncsafe_t readSyncInt32(uint8_t*& p)
	{
		uint8_t buf[4];
		syncsafe_t r = 0;
		read(buf, p, 4);

		r = ((buf[0] & 0x7f) << 21)
			| ((buf[1] & 0x7f) << 14)
			| ((buf[2] & 0x7f) << 7)
			| (buf[3] & 0x7f);
		return r;
	}
	uint8_t read8(uint8_t*& p)
	{
		return *p++;
	}


	void close();
public:
	CID3V2Tag();
	~CID3V2Tag();

	int Version() const { return header.version; }

	bool Parse(uint8_t* data, uint32_t size);
	CID3V2TextFrame FindTextFrame(ID3V2_ID id);
};


struct CID3V2Frame
{
	uint8_t id[4];
	ID3V2FrameHeader header;
	uint8_t* data;

	CID3V2Frame() : data(NULL)
	{
		memset(id, 0, sizeof id);
	}

	CID3V2Frame(const ID3V2FrameHeader& h, const uint8_t* d) : CID3V2Frame()
	{
		header = h;
		memcpy(id, &header.id, sizeof id);
		data = new uint8_t[header.size];
		memcpy(data, d, header.size);
	}
	CID3V2Frame(const CID3V2Frame& frame) : CID3V2Frame()
	{
		header = frame.header;
		memcpy(id, &header.id, sizeof id);
		data = new uint8_t[header.size];
		memcpy(data, frame.data, header.size);
	}

	CID3V2Frame& operator=(const CID3V2Frame& other)
	{
		if (data != NULL)
			delete[] data;
		header = other.header;
		memcpy(id, &header.id, sizeof id);
		data = new uint8_t[header.size];
		memcpy(data, other.data, header.size);

		return *this;
	}

	virtual ~CID3V2Frame()
	{
		if (data != NULL)
			delete[] data;
	}

	virtual bool operator<(const CID3V2Frame& other) const
	{
		return header.id < other.header.id;
	}
};

struct CID3V2TextFrame : public CID3V2Frame
{
	uint8_t encoding;
	uint8_t* text;

	CID3V2TextFrame() : CID3V2Frame(), encoding(0), text(NULL)
	{

	}

	CID3V2TextFrame(const CID3V2Frame& frame) : CID3V2Frame(frame)
	{
		uint8_t* p = frame.data;

		encoding = *p++;
		text = new uint8_t[frame.header.size + 2]; // -1 for enc byte, +2 for (UTF16-safe) NULL
		memset(text, 0, frame.header.size + 2);
		memcpy(text, p, frame.header.size - 1);
	}
	CID3V2TextFrame(const CID3V2TextFrame& frame) : CID3V2Frame(frame)
	{
		uint8_t* p = frame.data;

		encoding = *p++;
		text = new uint8_t[frame.header.size + 2]; // -1 for enc byte, +2 for (UTF16-safe) NULL
		memset(text, 0, frame.header.size + 2);
		memcpy(text, p, frame.header.size - 1);
	}
	CID3V2TextFrame& operator=(const CID3V2TextFrame& other)
	{
		CID3V2Frame::operator=(other);

		uint8_t* p = data;

		encoding = *p++;
		text = new uint8_t[header.size + 2]; // -1 for enc byte, +2 for (UTF16-safe) NULL
		memset(text, 0, header.size + 2);
		memcpy(text, p, header.size - 1);

		return *this;
	}

	virtual ~CID3V2TextFrame()
	{
		if (text != NULL)
			delete[] text;
	}
};
