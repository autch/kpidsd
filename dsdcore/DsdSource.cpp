#include "stdafx.h"

#include <algorithm>
#include <array>
#include <cwchar>

#include "DsdSource.h"
#include "KpiPartialFile.h"
#include "Reader.h"

namespace
{
	constexpr std::array<uint8_t, 256> makeBitReverse()
	{
		std::array<uint8_t, 256> table{};
		for (int i = 0; i < 256; i++)
		{
			int r = 0;
			for (int bit = 0; bit < 8; bit++)
				if (i & (1 << bit))
					r |= 0x80 >> bit;
			table[i] = static_cast<uint8_t>(r);
		}
		return table;
	}
	constexpr std::array<uint8_t, 256> bitReverse = makeBitReverse();

	std::wstring decode(UINT codepage, const std::string& text, DWORD flags)
	{
		if (text.empty() || text.size() > INT_MAX)
			return {};
		const int length = MultiByteToWideChar(codepage, flags, text.data(), static_cast<int>(text.size()), nullptr, 0);
		if (length <= 0)
			return {};
		std::wstring result(static_cast<size_t>(length), L'\0');
		MultiByteToWideChar(codepage, flags, text.data(), static_cast<int>(text.size()), result.data(), length);
		return result;
	}

	std::wstring decode(TextEncoding encoding, const std::string& text)
	{
		switch (encoding)
		{
		case TextEncoding::CP932:
			return decode(932, text, 0);
		case TextEncoding::UTF8OrCP932:
		{
			// UTF-8 として正しければ UTF-8、そうでなければ Shift_JIS
			std::wstring result = decode(CP_UTF8, text, MB_ERR_INVALID_CHARS);
			return result.empty() ? decode(932, text, 0) : result;
		}
		default:
			return {};
		}
	}

	// 空ならタグを設定しない (タイトルがなければ本体がファイル名を表示する)
	void setText(IKpiTagInfo* pTagInfo, const wchar_t* name, const std::wstring& value)
	{
		if (!value.empty())
			pTagInfo->wSetValueW(name, -1, value.c_str(), -1);
	}
}

DsdSource::DsdSource() = default;

DsdSource::~DsdSource()
{
	Close();
}

bool DsdSource::Open(IKpiFile* file)
{
	Close();
	// 大きさの分からないファイル (KPI_FILE_EOF) は、長さを決められないので開かない
	if (file->GetSize() == KPI_FILE_EOF)
		return false;

	auto fileReader = std::make_unique<KpiFileReader>(file);
	DsdLayout parsed;
	if (!ParseDsd(*fileReader, parsed))
		return false;

	kpiFile = file;
	kpiFile->AddRef();
	reader = std::move(fileReader);
	layout = parsed;
	Seek(0);
	return true;
}

void DsdSource::Close()
{
	reader.reset();
	if (kpiFile != nullptr)
		kpiFile->Release();
	kpiFile = nullptr;
	layout = DsdLayout();
	position = 0;
	raw.clear();
	rawBlock = UINT64_MAX;
}

void DsdSource::Seek(uint64_t pos)
{
	position = std::min(pos, layout.bytesPerChannel);
}

size_t DsdSource::Read(uint8_t* dst, size_t stride, size_t bytes)
{
	if (reader == nullptr)
		return 0;
	bytes = static_cast<size_t>(std::min<uint64_t>(bytes, layout.bytesPerChannel - position));
	const uint32_t channels = layout.channels;
	const uint32_t blockSize = layout.blockSize;
	size_t done = 0;

	if (blockSize == 1)
	{
		// 1 バイトずつチャンネルが交互に並ぶ (DSDIFF、WSD、ブロック長 1 の DSF)
		constexpr size_t chunk = 16384;
		while (done < bytes)
		{
			const size_t n = std::min(chunk, bytes - done);
			raw.resize(n * channels);
			if (!reader->ReadAt(layout.dataOffset + position * channels, raw.data(), raw.size()))
				break;	// 開いたときより短くなったファイル
			for (size_t i = 0; i < n; i++)
			{
				for (uint32_t ch = 0; ch < channels; ch++)
				{
					const uint8_t b = raw[i * channels + ch];
					dst[ch * stride + done + i] = layout.lsbFirst ? bitReverse[b] : b;
				}
			}
			position += n;
			done += n;
		}
		return done;
	}

	// チャンネルごとのブロックが交互に並ぶ (DSF)
	const uint64_t blockBytes = static_cast<uint64_t>(blockSize) * channels;
	while (done < bytes)
	{
		const uint64_t block = position / blockSize;
		const size_t offset = static_cast<size_t>(position % blockSize);
		if (rawBlock != block)
		{
			raw.resize(static_cast<size_t>(blockBytes));
			if (!reader->ReadAt(layout.dataOffset + block * blockBytes, raw.data(), raw.size()))
			{
				rawBlock = UINT64_MAX;
				break;	// 開いたときより短くなったファイル
			}
			rawBlock = block;
		}
		const size_t n = std::min<size_t>(blockSize - offset, bytes - done);
		for (uint32_t ch = 0; ch < channels; ch++)
		{
			const uint8_t* src = raw.data() + static_cast<size_t>(ch) * blockSize + offset;
			uint8_t* out = dst + ch * stride + done;
			if (layout.lsbFirst)
			{
				for (size_t i = 0; i < n; i++)
					out[i] = bitReverse[src[i]];
			}
			else
				std::memcpy(out, src, n);
		}
		position += n;
		done += n;
	}
	return done;
}

void DsdSource::SetTags(IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags)
{
	if (reader == nullptr)
		return;

	if (layout.id3Size != 0)
	{
		// ID3v2 の範囲だけを本体に解析させる
		auto* partial = new KpiPartialFile(kpiFile, layout.id3Offset, layout.id3Offset + layout.id3Size);
		pTagInfo->GetTagInfo(partial, nullptr, KPI_TAGTYPE_ID3V2, dwTagGetFlags);
		partial->Release();
		reader->Invalidate();	// 本体が IKpiFile の位置を動かした
	}
	else
	{
		// 本体のタグ解析を止めてから、ファイルの文字列を設定する
		pTagInfo->GetTagInfo(nullptr, nullptr, KPI_TAGTYPE_NONE, dwTagGetFlags);
		const DsdText& text = layout.text;
		const TextEncoding encoding = layout.encoding;
		setText(pTagInfo, SZ_KMP_NAME_TITLE_W, decode(encoding, text.title));
		setText(pTagInfo, SZ_KMP_NAME_ARTIST_W, decode(encoding, text.artist));
		setText(pTagInfo, SZ_KMP_NAME_ALBUM_W, decode(encoding, text.album));
		setText(pTagInfo, SZ_KMP_NAME_COMPOSER_W, decode(encoding, text.composer));
		setText(pTagInfo, SZ_KMP_NAME_GENRE_W, decode(encoding, text.genre));
		setText(pTagInfo, SZ_KMP_NAME_DATE_W, decode(encoding, text.date));
		setText(pTagInfo, SZ_KMP_NAME_COMMENT_W, decode(encoding, text.comment));
	}

	// ビットレートはチャンネルあたり (DSD64 なら 2.822Mbps)
	wchar_t value[32];
	const uint32_t rate = layout.sampleRate;
	swprintf_s(value, L"%u.%03uMbps", rate / 1000 / 1000, (rate / 1000) % 1000);
	pTagInfo->wSetValueW(SZ_KMP_NAME_BITRATE_W, -1, value, -1);
	swprintf_s(value, L"%u.%03uMHz", rate / 1000 / 1000, (rate / 1000) % 1000);
	pTagInfo->wSetValueW(L"DSD_SamplesPerSec", -1, value, -1);
}
