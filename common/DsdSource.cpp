#include "stdafx.h"

#include <algorithm>
#include <array>
#include <cwchar>

#include "DsdSource.h"
#include "KpiFileAdapter.h"
#include "KpiPartialFile.h"

#include "CDFFFile.h"
#include "CDSFFile.h"
#include "CWSDFile.h"

namespace
{
	// これより多いチャンネル数のファイルは壊れているとみなす
	constexpr uint32_t MaxChannels = 16;
	// DSF のブロック長の上限 (仕様では 4096 固定)
	constexpr uint32_t MaxBlockSize = 1 << 20;

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

	std::wstring decode(UINT codepage, const char* text, size_t size, DWORD flags)
	{
		if (size == 0 || size > INT_MAX)
			return {};
		const int length = MultiByteToWideChar(codepage, flags, text, static_cast<int>(size), nullptr, 0);
		if (length <= 0)
			return {};
		std::wstring result(static_cast<size_t>(length), L'\0');
		MultiByteToWideChar(codepage, flags, text, static_cast<int>(size), result.data(), length);
		return result;
	}

	// Shift_JIS (CP932) 固定で読む
	std::wstring decodeCP932(const std::string& text)
	{
		return decode(932, text.data(), text.size(), 0);
	}

	// UTF-8 として正しければ UTF-8、そうでなければ CP932 として読む
	std::wstring decodeUTF8OrCP932(const std::string& text)
	{
		std::wstring result = decode(CP_UTF8, text.data(), text.size(), MB_ERR_INVALID_CHARS);
		if (result.empty())
			result = decodeCP932(text);
		return result;
	}

	// 固定長の欄から、NUL より前を取り出して末尾の空白を除く
	std::string fixedField(const uint8_t* field, size_t size)
	{
		size_t length = 0;
		while (length < size && field[length] != 0)
			length++;
		while (length > 0 && field[length - 1] == ' ')
			length--;
		return std::string(reinterpret_cast<const char*>(field), length);
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

DsdSource::Format DsdSource::Detect(IKpiFile* file)
{
	uint8_t signature[4] = {};
	if (file->Seek(0, FILE_BEGIN) != 0 || file->Read(signature, sizeof signature) != sizeof signature)
		return Format::None;
	file->Seek(0, FILE_BEGIN);

	if (std::memcmp(signature, "DSD ", 4) == 0)
		return Format::DSF;
	if (std::memcmp(signature, "FRM8", 4) == 0)
		return Format::DFF;
	if (std::memcmp(signature, "1bit", 4) == 0)
		return Format::WSD;
	return Format::None;
}

bool DsdSource::IsSupportedRate(uint32_t rate)
{
	for (const uint32_t base : { 44100u, 48000u })
	{
		for (uint32_t multiple = 64; multiple <= 1024; multiple *= 2)
		{
			if (rate == base * multiple)
				return true;
		}
	}
	return false;
}

bool DsdSource::Open(IKpiFile* file)
{
	Close();

	const Format detected = Detect(file);
	if (detected == Format::None)
		return false;
	// 大きさの分からないファイル (KPI_FILE_EOF) は、長さを決められないので開かない
	if (file->GetSize() == KPI_FILE_EOF)
		return false;

	kpiFile = file;
	adapter = std::make_unique<KpiFileAdapter>(file);
	bool ok = false;
	switch (detected)
	{
	case Format::DSF: ok = openDSF(); break;
	case Format::DFF: ok = openDFF(); break;
	case Format::WSD: ok = openWSD(); break;
	default: break;
	}
	if (!ok || !IsSupportedRate(sampleRate) || channels == 0 || channels > MaxChannels)
	{
		Close();
		return false;
	}

	format = detected;
	kpiFile->AddRef();
	Seek(0);
	return true;
}

void DsdSource::Close()
{
	dsf.reset();
	dff.reset();
	wsd.reset();
	adapter.reset();
	if (kpiFile != nullptr && format != Format::None)
		kpiFile->Release();
	kpiFile = nullptr;

	format = Format::None;
	sampleRate = channels = blockSize = 0;
	bytesPerChannel = dataOffset = position = 0;
	lsbFirst = false;
	filePosition = rawBlock = UINT64_MAX;
	raw.clear();
}

bool DsdSource::openDSF()
{
	dsf = std::make_unique<CDSFFile>();
	if (!dsf->Open(adapter.get()))
		return false;

	const DSF_fmt_HEADER* fmt = dsf->FmtHeader();
	sampleRate = fmt->sampling_frequency;
	channels = fmt->channel_num;
	blockSize = fmt->block_size_per_channel;
	lsbFirst = fmt->bits_per_sample == DSF_BPS_LSB;
	if (channels == 0 || channels > MaxChannels || blockSize == 0 || blockSize > MaxBlockSize)
		return false;

	// data チャンクの size は 12 バイトのヘッダを含む。データはブロック単位で、最後のブロックは 0 で埋まっている
	dataOffset = dsf->DataOffset();
	const uint64_t fileSize = kpiFile->GetSize();
	if (dataOffset > fileSize)
		return false;
	const uint64_t chunkSize = dsf->DataHeader()->size;
	const uint64_t dataBytes = std::min(chunkSize >= DSF_data_SIZE ? chunkSize - DSF_data_SIZE : 0, fileSize - dataOffset);
	const uint64_t blocks = dataBytes / (static_cast<uint64_t>(blockSize) * channels);
	bytesPerChannel = std::min((fmt->sample_count + 7) / 8, blocks * blockSize);
	return true;
}

bool DsdSource::openDFF()
{
	dff = std::make_unique<CDFFFile>();
	if (!dff->Open(adapter.get()))
		return false;

	FRM8Chunk& frm8 = dff->FRM8();
	// DST で圧縮されたものには対応しない
	if (frm8.data.formType != DFFID_DSD_ || frm8.prop.cmpr.data.compressionType != CMPR_ID_DSD_)
		return false;
	if (frm8.dsd.ID() != DFFID_DSD_ || frm8.dsd.OffsetToData() == 0)
		return false;

	sampleRate = frm8.prop.fs.data.sampleRate;
	channels = frm8.prop.chnl.data.numChannels;
	if (channels == 0 || channels > MaxChannels)
		return false;

	// 1 バイトずつチャンネルが交互に並ぶ。途中で切れたファイルは読める分だけにする
	dataOffset = frm8.dsd.OffsetToData();
	const uint64_t fileSize = kpiFile->GetSize();
	if (dataOffset > fileSize)
		return false;
	bytesPerChannel = std::min(frm8.dsd.DataSize(), fileSize - dataOffset) / channels;
	return true;
}

bool DsdSource::openWSD()
{
	wsd = std::make_unique<CWSDFile>();
	if (!wsd->Open(adapter.get()))
		return false;

	sampleRate = wsd->DataSpec()->samplingFrequency;
	channels = wsd->DataSpec()->channels;
	if (channels == 0 || channels > MaxChannels)
		return false;

	// データはファイルの終わりまで、1 バイトずつチャンネルが交互に並ぶ
	dataOffset = wsd->Header()->dataOffset;
	const uint64_t fileSize = kpiFile->GetSize();
	if (dataOffset > fileSize)
		return false;
	bytesPerChannel = (fileSize - dataOffset) / channels;
	return true;
}

void DsdSource::Seek(uint64_t pos)
{
	position = std::min(pos, bytesPerChannel);
}

bool DsdSource::readAt(uint64_t offset, uint8_t* dst, size_t bytes)
{
	if (filePosition != offset)
	{
		if (kpiFile->Seek(static_cast<INT64>(offset), FILE_BEGIN) != offset)
		{
			filePosition = UINT64_MAX;
			return false;
		}
		filePosition = offset;
	}
	while (bytes > 0)
	{
		const DWORD request = static_cast<DWORD>(std::min<size_t>(bytes, 1 << 20));
		const DWORD got = kpiFile->Read(dst, request);
		filePosition += got;
		if (got < request)
			return false;
		dst += got;
		bytes -= got;
	}
	return true;
}

size_t DsdSource::Read(uint8_t* dst, size_t stride, size_t bytes)
{
	if (format == Format::None)
		return 0;
	bytes = static_cast<size_t>(std::min<uint64_t>(bytes, bytesPerChannel - position));
	if (bytes == 0)
		return 0;
	return format == Format::DSF ? readBlocks(dst, stride, bytes) : readInterleaved(dst, stride, bytes);
}

size_t DsdSource::readInterleaved(uint8_t* dst, size_t stride, size_t bytes)
{
	constexpr size_t chunk = 16384;
	size_t done = 0;
	while (done < bytes)
	{
		const size_t n = std::min(chunk, bytes - done);
		raw.resize(n * channels);
		if (!readAt(dataOffset + position * channels, raw.data(), raw.size()))
			break;	// 開いたときより短くなったファイル
		for (size_t i = 0; i < n; i++)
		{
			for (uint32_t ch = 0; ch < channels; ch++)
				dst[ch * stride + done + i] = raw[i * channels + ch];
		}
		position += n;
		done += n;
	}
	return done;
}

size_t DsdSource::readBlocks(uint8_t* dst, size_t stride, size_t bytes)
{
	const uint64_t blockBytes = static_cast<uint64_t>(blockSize) * channels;
	size_t done = 0;
	while (done < bytes)
	{
		const uint64_t block = position / blockSize;
		const size_t offset = static_cast<size_t>(position % blockSize);
		if (rawBlock != block)
		{
			raw.resize(static_cast<size_t>(blockBytes));
			if (!readAt(dataOffset + block * blockBytes, raw.data(), raw.size()))
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
			if (lsbFirst)
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
	if (format == Format::None)
		return;

	const uint64_t fileSize = kpiFile->GetSize();
	switch (format)
	{
	case Format::DSF:
	{
		// ID3v2 はファイルの終わりまで
		const uint64_t pointer = dsf->Header()->id3v2_pointer;
		if (pointer != 0 && pointer < fileSize)
			delegateID3v2(pTagInfo, pointer, fileSize, dwTagGetFlags);
		else
			pTagInfo->GetTagInfo(nullptr, nullptr, KPI_TAGTYPE_NONE, dwTagGetFlags);
		break;
	}
	case Format::DFF:
	{
		FRM8Chunk& frm8 = dff->FRM8();
		const uint64_t start = frm8.id3.OffsetToData();
		if (start != 0 && start < fileSize && frm8.id3.DataSize() > 0)
		{
			// ID3 チャンクがあれば、DIIN と COMT は使わない
			delegateID3v2(pTagInfo, start, start + std::min(frm8.id3.DataSize(), fileSize - start), dwTagGetFlags);
			break;
		}
		// 文字コードの規定がない。UTF-8 として正しければ UTF-8、そうでなければ Shift_JIS として読む
		pTagInfo->GetTagInfo(nullptr, nullptr, KPI_TAGTYPE_NONE, dwTagGetFlags);
		setText(pTagInfo, SZ_KMP_NAME_TITLE_W, decodeUTF8OrCP932(frm8.diin.diti.titleText));
		setText(pTagInfo, SZ_KMP_NAME_ARTIST_W, decodeUTF8OrCP932(frm8.diin.diar.artistText));
		if (!frm8.comt.comments.empty())
			setText(pTagInfo, SZ_KMP_NAME_COMMENT_W, decodeUTF8OrCP932(frm8.comt.comments.front().commentText));
		break;
	}
	case Format::WSD:
	{
		// 文字コードの規定がない。Shift_JIS として読む
		pTagInfo->GetTagInfo(nullptr, nullptr, KPI_TAGTYPE_NONE, dwTagGetFlags);
		const WSD_TEXT* text = wsd->Text();
		const auto field = [](const auto& f) { return decodeCP932(fixedField(f, sizeof f)); };
		setText(pTagInfo, SZ_KMP_NAME_TITLE_W, field(text->title));
		setText(pTagInfo, SZ_KMP_NAME_ARTIST_W, field(text->artist));
		setText(pTagInfo, SZ_KMP_NAME_ALBUM_W, field(text->album));
		setText(pTagInfo, SZ_KMP_NAME_COMPOSER_W, field(text->composer));
		setText(pTagInfo, SZ_KMP_NAME_GENRE_W, field(text->genre));
		setText(pTagInfo, SZ_KMP_NAME_DATE_W, field(text->dateAndTime));
		setText(pTagInfo, SZ_KMP_NAME_COMMENT_W, field(text->comment));
		break;
	}
	default:
		break;
	}
	setDsdInfo(pTagInfo);
}

void DsdSource::delegateID3v2(IKpiTagInfo* pTagInfo, uint64_t start, uint64_t end, DWORD dwTagGetFlags)
{
	auto* partial = new KpiPartialFile(kpiFile, start, end);
	pTagInfo->GetTagInfo(partial, nullptr, KPI_TAGTYPE_ID3V2, dwTagGetFlags);
	partial->Release();
	filePosition = UINT64_MAX;	// 本体が IKpiFile の位置を動かした
}

void DsdSource::setDsdInfo(IKpiTagInfo* pTagInfo)
{
	// ビットレートはチャンネルあたり (DSD64 なら 2.822Mbps)
	wchar_t text[32];
	const uint32_t rate = sampleRate;
	if (rate >= 1000 * 1000)
		swprintf_s(text, L"%u.%03uMbps", rate / 1000 / 1000, (rate / 1000) % 1000);
	else
		swprintf_s(text, L"%u.%03ukbps", rate / 1000, rate % 1000);
	pTagInfo->wSetValueW(SZ_KMP_NAME_BITRATE_W, -1, text, -1);

	swprintf_s(text, L"%u.%03uMHz", rate / 1000 / 1000, (rate / 1000) % 1000);
	pTagInfo->wSetValueW(L"DSD_SamplesPerSec", -1, text, -1);
}
