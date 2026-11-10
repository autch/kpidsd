#include "stdafx.h"
#include "CDFFFile.h"

namespace
{
	// 壊れたファイルや細工されたファイルで、メモリや時間を使いすぎないための上限
	constexpr size_t MaxChunks = 10000;			// 正しいファイルのチャンクは数十個
	constexpr uint32_t MaxTextBytes = 65536;		// 1 つの文字列の長さ
	constexpr size_t MaxComments = 16;				// 覚えておくコメントの数 (使うのは最初の 1 つ)
	constexpr uint16_t MaxChannelIDs = 256;

	constexpr uint64_t ChunkHeaderSize = 12;		// ckID (4) + ckDataSize (8)
}

CDFFFile::CDFFFile() : CLargeFile(), frm8()
{
}

CDFFFile::~CDFFFile()
{
}

void CDFFFile::Close()
{
	CLargeFile::Close();
}

bool CDFFFile::readChunkHeader(uint64_t pos, uint64_t parentEnd, ChunkStep& step)
{
	if (pos > parentEnd || parentEnd - pos < ChunkHeaderSize)
		return false;
	if (!Seek(static_cast<int64_t>(pos), NULL, FILE_BEGIN))
		return false;

	DFFChunkHeader header;
	DWORD bytesRead = 0;
	if (!Read(&header, sizeof header, &bytesRead) || bytesRead != sizeof header)
		return false;
	header.ckID = ntohl(header.ckID);
	header.ckDataSize = ntohll(header.ckDataSize);

	step.headerOffset = pos;
	step.header = header;
	step.dataOffset = pos + ChunkHeaderSize;
	// 親の終わりを超える長さ (壊れたファイル、途中で切れたファイル) は親の終わりまでとみなす。
	// 奇数長のチャンクの後ろには 1 バイトの詰め物がある
	const uint64_t room = parentEnd - step.dataOffset;
	if (header.ckDataSize >= room)
		step.dataEndOffset = parentEnd;
	else
		step.dataEndOffset = std::min(step.dataOffset + header.ckDataSize + (header.ckDataSize & 1), parentEnd);
	return true;
}

bool CDFFFile::readField(const ChunkStep& step, uint64_t offset, void* dst, size_t size)
{
	const uint64_t length = step.dataEndOffset - step.dataOffset;
	if (offset > length || size > length - offset)
		return false;
	if (!Seek(static_cast<int64_t>(step.dataOffset + offset), NULL, FILE_BEGIN))
		return false;
	DWORD bytesRead = 0;
	return Read(dst, static_cast<DWORD>(size), &bytesRead) && bytesRead == size;
}

BOOL CDFFFile::Open(CAbstractFile* file)
{
	hFile = file;
	frm8 = FRM8Chunk();
	chunks = 0;

	const uint64_t size = FileSize();
	ChunkStep top;
	if (!readChunkHeader(0, size, top) || top.header.ckID != DFFID_FRM8)
		return FALSE;
	frm8.header = top.header;
	frm8.offsetToData = top.dataOffset;
	if (!readField(top, 0, &frm8.data, sizeof frm8.data))
		return FALSE;
	frm8.setupData();

	parseChildren(DFFID_FRM8, top.dataOffset + sizeof frm8.data, top.dataEndOffset);
	return TRUE;
}

void CDFFFile::parseChildren(DFFID parent, uint64_t begin, uint64_t end)
{
	uint64_t pos = begin;
	ChunkStep step;
	while (chunks < MaxChunks && readChunkHeader(pos, end, step))
	{
		chunks++;
		switch (parent)
		{
		case DFFID_FRM8: readFRM8Child(step); break;
		case DFFID_PROP: readPROPChild(step); break;
		case DFFID_DIIN: readDIINChild(step); break;
		default: break;
		}
		// 子チャンクの中でどこまで読んだかに関係なく、宣言された長さの終わりへ進む
		pos = step.dataEndOffset;
	}
}

void CDFFFile::readFRM8Child(const ChunkStep& step)
{
	switch (step.header.ckID)
	{
	case DFFID_FVER:
		if (frm8.fver.offsetToData == 0 && readField(step, 0, &frm8.fver.data, sizeof frm8.fver.data))
		{
			frm8.fver.header = step.header;
			frm8.fver.offsetToData = step.dataOffset;
			frm8.fver.setupData();
		}
		break;
	case DFFID_PROP:
	{
		// 音のプロパティ ('SND ') の PROP だけを、最初の 1 つだけ使う
		decltype(frm8.prop.data) data;
		if (frm8.prop.offsetToData != 0 || !readField(step, 0, &data, sizeof data))
			break;
		frm8.prop.data = data;
		frm8.prop.setupData();
		if (frm8.prop.data.propType != DFFID_SND_)
			break;
		frm8.prop.header = step.header;
		frm8.prop.offsetToData = step.dataOffset;
		parseChildren(DFFID_PROP, step.dataOffset + sizeof data, step.dataEndOffset);
		break;
	}
	case DFFID_DSD_:
		if (frm8.dsd.offsetToData == 0)
		{
			frm8.dsd.header = step.header;
			frm8.dsd.offsetToData = step.dataOffset;
		}
		break;
	case DFFID_COMT:
		if (frm8.comt.offsetToData == 0)
			readCOMT(step);
		break;
	case DFFID_DIIN:
		if (frm8.diin.offsetToData == 0)
		{
			frm8.diin.header = step.header;
			frm8.diin.offsetToData = step.dataOffset;
			parseChildren(DFFID_DIIN, step.dataOffset, step.dataEndOffset);
		}
		break;
	case DFFID_ID3_:
		// 非公式の ID3v2 タグ
		if (frm8.id3.offsetToData == 0)
		{
			frm8.id3.header = step.header;
			frm8.id3.offsetToData = step.dataOffset;
		}
		break;
	default:
		// DST、DSTI、MANF などは使わない
		break;
	}
}

void CDFFFile::readPROPChild(const ChunkStep& step)
{
	PROPChunk& prop = frm8.prop;
	switch (step.header.ckID)
	{
	case DFFID_FS__:
		if (prop.fs.offsetToData == 0 && readField(step, 0, &prop.fs.data, sizeof prop.fs.data))
		{
			prop.fs.header = step.header;
			prop.fs.offsetToData = step.dataOffset;
			prop.fs.setupData();
		}
		break;
	case DFFID_CHNL:
		if (prop.chnl.offsetToData == 0 && readField(step, 0, &prop.chnl.data, sizeof prop.chnl.data))
		{
			prop.chnl.header = step.header;
			prop.chnl.offsetToData = step.dataOffset;
			prop.chnl.setupData();
			const uint16_t ids = std::min(prop.chnl.data.numChannels, MaxChannelIDs);
			for (uint16_t i = 0; i < ids; i++)
			{
				DFFID id = 0;
				if (!readField(step, sizeof prop.chnl.data + i * sizeof id, &id, sizeof id))
					break;
				prop.chnl.chID.push_back(ntohl(id));
			}
		}
		break;
	case DFFID_CMPR:
		if (prop.cmpr.offsetToData == 0 && readField(step, 0, &prop.cmpr.data, sizeof prop.cmpr.data))
		{
			prop.cmpr.header = step.header;
			prop.cmpr.offsetToData = step.dataOffset;
			prop.cmpr.setupData();
			assignBuffer(prop.cmpr.data.count, step.dataEndOffset, prop.cmpr.compressionName);
		}
		break;
	case DFFID_ABSS:
		if (prop.abss.offsetToData == 0 && readField(step, 0, &prop.abss.data, sizeof prop.abss.data))
		{
			prop.abss.header = step.header;
			prop.abss.offsetToData = step.dataOffset;
			prop.abss.setupData();
		}
		break;
	case DFFID_LSCO:
		if (prop.lsco.offsetToData == 0 && readField(step, 0, &prop.lsco.data, sizeof prop.lsco.data))
		{
			prop.lsco.header = step.header;
			prop.lsco.offsetToData = step.dataOffset;
			prop.lsco.setupData();
		}
		break;
	default:
		break;
	}
}

void CDFFFile::readDIINChild(const ChunkStep& step)
{
	DIINChunk& diin = frm8.diin;
	switch (step.header.ckID)
	{
	case DFFID_DIAR:
		if (diin.diar.offsetToData == 0)
			readText(step, diin.diar, diin.diar.data.count, diin.diar.artistText);
		break;
	case DFFID_DITI:
		if (diin.diti.offsetToData == 0)
			readText(step, diin.diti, diin.diti.data.count, diin.diti.titleText);
		break;
	default:
		// EMID、MARK などは使わない
		break;
	}
}

// DIAR / DITI: count (4 バイト) と文字列
void CDFFFile::readText(const ChunkStep& step, DFFChunk& chunk, uint32_t& count, std::string& text)
{
	uint32_t be = 0;
	if (!readField(step, 0, &be, sizeof be))
		return;
	chunk.header = step.header;
	chunk.offsetToData = step.dataOffset;
	count = ntohl(be);
	assignBuffer(count, step.dataEndOffset, text);
}

void CDFFFile::readCOMT(const ChunkStep& step)
{
	COMTChunk& comt = frm8.comt;
	if (!readField(step, 0, &comt.data, sizeof comt.data))
		return;
	comt.header = step.header;
	comt.offsetToData = step.dataOffset;
	comt.setupData();

	uint64_t offset = sizeof comt.data;
	for (uint32_t i = 0; i < comt.data.numComments && comt.comments.size() < MaxComments; i++)
	{
		Comment comment;
		if (!readField(step, offset, &comment.data, sizeof comment.data))
			break;
		comment.setupData();
		offset += sizeof comment.data;
		assignBuffer(comment.data.count, step.dataEndOffset, comment.commentText);
		comt.comments.push_back(comment);
		// コメントの後ろには、長さを偶数にするための詰め物がある
		offset += static_cast<uint64_t>(comment.data.count) + (comment.data.count & 1);
		if (offset >= step.dataEndOffset - step.dataOffset)
			break;
	}
}

// 今の位置から bytesToRead バイトの文字列を読む。limit (チャンクの終わり) を超えては読まない
void CDFFFile::assignBuffer(uint32_t bytesToRead, uint64_t limit, std::string& target)
{
	const uint64_t pos = Tell();
	if (pos >= limit)
		bytesToRead = 0;
	else if (bytesToRead > limit - pos)
		bytesToRead = static_cast<uint32_t>(limit - pos);
	// 長すぎる文字列は先頭だけ使う
	bytesToRead = std::min(bytesToRead, MaxTextBytes);

	std::string buf(bytesToRead, char());
	DWORD bytesRead = 0;
	if (bytesToRead > 0)
		Read(buf.data(), bytesToRead, &bytesRead);
	buf.resize(bytesRead);
	// NUL より後ろは使わない
	target.assign(buf.c_str());
}
