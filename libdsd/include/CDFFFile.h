#pragma once

/* Based on "Direct Stream Digital Interchange File Format, DSDIFF, version 1.5" from Philips */

#include <string>

#include "dff_types.h"
#include "CLargeFile.h"

// チャンクのヘッダと、データの範囲 (親チャンクの終わりで切り詰めたもの)
struct ChunkStep
{
	uint64_t headerOffset;
	DFFChunkHeader header;
	uint64_t dataOffset;
	uint64_t dataEndOffset;
};

// DSDIFF のパーサー。親チャンクごとに、その範囲の中だけで子チャンクを読む (範囲付きの再帰下降)。
// どのチャンクを受け付けるかは (親, 子) の組で決める。仕様にない場所にあるチャンクは無視し、
// 同じチャンクが 2 つ以上あれば最初のものを使う。
//   FRM8 ('DSD ')
//     FVER, PROP ('SND ') [FS, CHNL, CMPR, ABSS, LSCO], DSD, COMT, DIIN [DIAR, DITI],
//     ID3 (非公式。ID3v2 タグ)
class CDFFFile: public CLargeFile
{
private:
	FRM8Chunk	frm8;
	size_t		chunks = 0;		// 読んだチャンクの数

	bool readChunkHeader(uint64_t pos, uint64_t parentEnd, ChunkStep& step);
	// チャンクのデータの offset バイト目から size バイト読む。チャンクに収まらなければ false
	bool readField(const ChunkStep& step, uint64_t offset, void* dst, size_t size);
	void assignBuffer(uint32_t bytesToRead, uint64_t limit, std::string& target);

	void parseChildren(DFFID parent, uint64_t begin, uint64_t end);
	void readFRM8Child(const ChunkStep& step);
	void readPROPChild(const ChunkStep& step);
	void readDIINChild(const ChunkStep& step);
	void readCOMT(const ChunkStep& step);
	void readText(const ChunkStep& step, DFFChunk& chunk, uint32_t& count, std::string& text);

public:
	CDFFFile();
	~CDFFFile();

	FRM8Chunk& FRM8() { return frm8; }

	void Reset() { Seek(frm8.dsd.OffsetToData(), NULL, FILE_BEGIN); }
	BOOL Open(CAbstractFile* file);
	void Close();
};
