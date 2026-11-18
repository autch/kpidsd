// DSDIFF (Philips "Direct Stream Digital Interchange File Format" 1.5、ビッグエンディアン)
//
// 親チャンクごとに、その範囲の中だけで子チャンクを読む (範囲付きの再帰下降)。子の長さは親の終わりで
// 切り詰める。受け付けるチャンクは (親, 子) の組で決め、仕様にない場所にあるものは無視する。
// 同じチャンクが 2 つ以上あれば最初のものを使う。
//
//   FRM8 ('DSD ')
//     FVER           主バージョン (最上位バイト) が 1 でなければ開かない。なくても開く
//     PROP ('SND ')  FS  CHNL  CMPR  (ABSS、LSCO などは使わない)
//                    CHNL のチャンネル ID からスピーカーの配置を決め、WAV の順に並べ替える
//     COMT           最初の General (album) Comment (cmtType 0) を使う。なければ最初のコメント
//     DSD            データ。1 バイトずつチャンネルが交互に並ぶ (MSB first)
//     DIIN           DIAR (アーティスト)  DITI (タイトル)
//     ID3            非公式。ID3v2 タグ。あれば DIIN と COMT は使わない
//     (DST、DSTI、MANF などは使わない。DST で圧縮されたものは開かない)

#include "stdafx.h"

#include "DsdLayout.h"
#include "Reader.h"

namespace
{
	// 壊れたファイルや細工されたファイルで、メモリや時間を使いすぎないための上限
	constexpr size_t MaxChunks = 10000;			// 正しいファイルのチャンクは数十個
	constexpr uint32_t MaxTextBytes = 65536;		// 1 つの文字列の長さ
	constexpr uint64_t ChunkHeaderSize = 12;		// ckID (4) + ckDataSize (8)

	constexpr uint32_t id(const char (&s)[5])
	{
		return (static_cast<uint32_t>(static_cast<uint8_t>(s[0])) << 24) | (static_cast<uint32_t>(static_cast<uint8_t>(s[1])) << 16)
			| (static_cast<uint32_t>(static_cast<uint8_t>(s[2])) << 8) | static_cast<uint8_t>(s[3]);
	}

	// CHNL のチャンネル ID からスピーカーの位置。LS / RS (サラウンド) は、SDK の 5.1ch の標準の配置
	// (FL FR FC LFE BL BR) に合わせて BL / BR にする。C000〜C999 などの分からない ID は 0
	uint32_t speaker(uint32_t chId)
	{
		using namespace Speaker;
		switch (chId)
		{
		case id("SLFT"): case id("MLFT"): return FL;
		case id("SRGT"): case id("MRGT"): return FR;
		case id("C   "): return FC;
		case id("LFE "): return LFE;
		case id("LS  "): return BL;
		case id("RS  "): return BR;
		default: return 0;
		}
	}

	struct Chunk
	{
		uint32_t id;
		uint64_t data;		// データの先頭
		uint64_t size;		// データの長さ (親の終わりで切り詰めたもの)
		uint64_t end;		// 次のチャンクの位置 (奇数長なら詰め物の 1 バイトを含む。親の終わりを超えない)
	};

	class Parser
	{
	public:
		explicit Parser(Reader& reader) : reader_(reader) {}

		bool Parse(DsdLayout& layout)
		{
			Chunk top;
			uint8_t formType[4];
			if (!header(0, reader_.Size(), top) || top.id != id("FRM8") || !read(top, 0, formType, 4)
				|| ReadBE32(formType) != id("DSD "))
				return false;
			children(top.id, top.data + 4, top.end);

			// DST で圧縮されたものと、必要なチャンクがないものは開かない
			if (!fs_ || !chnl_ || compression_ != id("DSD ") || !dsd_)
				return false;
			// 主バージョンが同じなら読める (後方互換)。違うものは形が変わっているかもしれない
			if (fver_ && majorVersion_ != 1)
				return false;
			layout.format = DsdFormat::DFF;
			layout.sampleRate = sampleRate_;
			layout.channels = channels_;
			layout.dataOffset = dataOffset_;
			layout.blockSize = 1;
			layout.bytesPerChannel = channels_ != 0 ? dataSize_ / channels_ : 0;
			SetSpeakerLayout(layout, speakers_.data(), speakers_.size());
			layout.encoding = TextEncoding::UTF8OrCP932;
			if (id3_)
			{
				layout.id3Offset = id3Offset_;
				layout.id3Size = id3Size_;
			}
			else
			{
				layout.text.title = title_;
				layout.text.artist = artist_;
				layout.text.comment = comment_;
			}
			return true;
		}

	private:
		bool header(uint64_t pos, uint64_t parentEnd, Chunk& chunk)
		{
			uint8_t h[ChunkHeaderSize];
			if (pos > parentEnd || parentEnd - pos < ChunkHeaderSize || !reader_.ReadAt(pos, h, sizeof h))
				return false;
			chunk.id = ReadBE32(h);
			chunk.data = pos + ChunkHeaderSize;
			const uint64_t declared = ReadBE64(h + 4);
			const uint64_t room = parentEnd - chunk.data;
			chunk.size = std::min(declared, room);
			chunk.end = declared >= room ? parentEnd : std::min(chunk.data + declared + (declared & 1), parentEnd);
			return true;
		}

		// チャンクのデータの offset バイト目から size バイト読む。チャンクに収まらなければ false
		bool read(const Chunk& chunk, uint64_t offset, void* dst, size_t size)
		{
			return offset <= chunk.size && size <= chunk.size - offset && reader_.ReadAt(chunk.data + offset, dst, size);
		}

		// count バイトの文字列を読む。チャンクの終わりと MaxTextBytes を超えては読まない。NUL より後ろは使わない
		std::string text(const Chunk& chunk, uint64_t offset, uint32_t count)
		{
			if (offset > chunk.size)
				return {};
			const size_t length = static_cast<size_t>(std::min<uint64_t>({ count, chunk.size - offset, MaxTextBytes }));
			std::string s(length, char());
			if (length == 0 || !reader_.ReadAt(chunk.data + offset, s.data(), length))
				return {};
			s.resize(std::strlen(s.c_str()));
			return s;
		}

		void children(uint32_t parent, uint64_t begin, uint64_t end)
		{
			uint64_t pos = begin;
			Chunk chunk;
			while (chunks_ < MaxChunks && header(pos, end, chunk))
			{
				chunks_++;
				if (parent == id("FRM8"))
					frm8Child(chunk);
				else if (parent == id("PROP"))
					propChild(chunk);
				else if (parent == id("DIIN"))
					diinChild(chunk);
				pos = chunk.end;	// どこまで読んだかに関係なく、宣言された長さの終わりへ進む
			}
		}

		void frm8Child(const Chunk& chunk)
		{
			uint8_t b[16];
			if (chunk.id == id("PROP") && !prop_)
			{
				// 音のプロパティ ('SND ') の PROP だけを使う
				if (read(chunk, 0, b, 4) && ReadBE32(b) == id("SND "))
				{
					prop_ = true;
					children(chunk.id, chunk.data + 4, chunk.end);
				}
			}
			else if (chunk.id == id("DSD ") && !dsd_)
			{
				dsd_ = true;
				dataOffset_ = chunk.data;
				dataSize_ = chunk.size;
			}
			else if (chunk.id == id("FVER") && !fver_ && read(chunk, 0, b, 4))
			{
				fver_ = true;
				majorVersion_ = b[0];
			}
			else if (chunk.id == id("COMT") && !comt_)
			{
				comt_ = true;
				comment_ = comment(chunk);
			}
			else if (chunk.id == id("DIIN") && !diin_)
			{
				diin_ = true;
				children(chunk.id, chunk.data, chunk.end);
			}
			else if (chunk.id == id("ID3 ") && !id3_ && chunk.size > 0)
			{
				id3_ = true;
				id3Offset_ = chunk.data;
				id3Size_ = chunk.size;
			}
		}

		// numComments (2) に続いて、コメントが並ぶ。1 つのコメントは
		// 時刻 (6)、cmtType (2)、cmtRef (2)、count (4)、文字列 (偶数長になるよう詰め物が付く)
		std::string comment(const Chunk& chunk)
		{
			constexpr uint64_t CommentHeaderSize = 14;
			uint8_t b[CommentHeaderSize];
			if (!read(chunk, 0, b, 2))
				return {};
			const unsigned count = ReadBE16(b);
			uint64_t pos = 2;
			std::string first;
			for (unsigned i = 0; i < count && read(chunk, pos, b, sizeof b); i++)
			{
				const uint16_t type = ReadBE16(b + 6);
				const uint32_t length = ReadBE32(b + 10);
				const uint64_t textPos = pos + CommentHeaderSize;
				if (type == 0)
					return text(chunk, textPos, length);		// General (album) Comment
				if (i == 0)
					first = text(chunk, textPos, length);
				pos = textPos + length + (length & 1);
			}
			return first;
		}

		void propChild(const Chunk& chunk)
		{
			uint8_t b[4];
			if (chunk.id == id("FS  ") && !fs_ && read(chunk, 0, b, 4))
			{
				fs_ = true;
				sampleRate_ = ReadBE32(b);
			}
			else if (chunk.id == id("CHNL") && !chnl_ && read(chunk, 0, b, 2))
			{
				chnl_ = true;
				channels_ = ReadBE16(b);
				// チャンネル ID (4 バイトずつ) からスピーカーの位置を決める。分からない ID は 0
				speakers_.clear();
				for (uint32_t i = 0; i < channels_ && i < MaxDsdChannels && read(chunk, 2 + i * 4, b, 4); i++)
					speakers_.push_back(speaker(ReadBE32(b)));
			}
			else if (chunk.id == id("CMPR") && !cmpr_ && read(chunk, 0, b, 4))
			{
				cmpr_ = true;
				compression_ = ReadBE32(b);
			}
		}

		void diinChild(const Chunk& chunk)
		{
			uint8_t b[4];
			if (chunk.id == id("DIAR") && !diar_ && read(chunk, 0, b, 4))
			{
				diar_ = true;
				artist_ = text(chunk, 4, ReadBE32(b));
			}
			else if (chunk.id == id("DITI") && !diti_ && read(chunk, 0, b, 4))
			{
				diti_ = true;
				title_ = text(chunk, 4, ReadBE32(b));
			}
		}

		Reader& reader_;
		size_t chunks_ = 0;
		bool prop_ = false, fs_ = false, chnl_ = false, cmpr_ = false, dsd_ = false;
		bool comt_ = false, diin_ = false, diar_ = false, diti_ = false, id3_ = false, fver_ = false;
		uint8_t majorVersion_ = 0;
		uint32_t sampleRate_ = 0, channels_ = 0, compression_ = 0;
		uint64_t dataOffset_ = 0, dataSize_ = 0, id3Offset_ = 0, id3Size_ = 0;
		std::string title_, artist_, comment_;
		std::vector<uint32_t> speakers_;	// CHNL のチャンネルごとのスピーカーの位置 (最大 MaxDsdChannels 個)
	};
}

bool ParseDFF(Reader& reader, DsdLayout& layout)
{
	return Parser(reader).Parse(layout);
}
