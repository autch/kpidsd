// fuzz-decoder - 壊れたファイルや細工されたファイルで kpidop / kpid2p のデコーダーを試す (libFuzzer)
//
// プラグインの DLL ではなく、デコーダーのソースを直接リンクする (AddressSanitizer をかけるため)。
// 1 つの入力について、パーサーだけを MemoryReader で試し、本体と同じ呼び方で次を行う。
//   - タグ取得用の Open (cpRequest = NULL) と Select (ppMediaInfo = NULL)
//   - kpidop と kpid2p のデコード用の Open、Select、Seek、Render (いくつかの位置と長さで)
// 本体のタグ解析に渡されたファイル (KpiPartialFile) は、本体と同じく最後まで読む。
//
// ビルド: cmake -DKPIDSD_FUZZ=ON (MSVC の /fsanitize=address,fuzzer を使う)
// 実行:   fuzz-decoder.exe -max_len=262144 -jobs=16 -workers=16 corpus_dir

#include "stdafx.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "DsdLayout.h"
#include "KpiDopDecoder.h"
#include "KpiD2pDecoder.h"
#include "Reader.h"

namespace
{
	class MemoryFile : public KbKpiFileImpl
	{
	public:
		MemoryFile(const uint8_t* data, size_t size) : data_(data), size_(size) {}

		DWORD WINAPI Read(void* pBuffer, DWORD dwSize) override
		{
			if (pos_ >= size_)
				return 0;
			const DWORD bytes = static_cast<DWORD>(std::min<UINT64>(dwSize, size_ - pos_));
			std::memcpy(pBuffer, data_ + pos_, bytes);
			pos_ += bytes;
			return bytes;
		}
		UINT64 WINAPI Seek(INT64 i64Pos, DWORD dwOrigin) override
		{
			const INT64 base = dwOrigin == FILE_BEGIN ? 0 : dwOrigin == FILE_CURRENT ? static_cast<INT64>(pos_) : static_cast<INT64>(size_);
			if (base + i64Pos < 0 || static_cast<UINT64>(base + i64Pos) > size_)
				return KPI_FILE_EOF;
			pos_ = static_cast<UINT64>(base + i64Pos);
			return pos_;
		}
		UINT64 WINAPI GetSize() override { return size_; }

	private:
		const uint8_t* data_;
		size_t size_;
		UINT64 pos_ = 0;
	};

	// 値は捨てる。GetTagInfo に渡されたファイルは最後まで読む
	class TagInfo : public KbKpiTagInfoImpl
	{
	public:
		DWORD WINAPI GetTagInfo(IKpiFile* pFile, IKpiFolder*, DWORD, DWORD) override
		{
			if (pFile != nullptr)
			{
				const UINT64 size = pFile->GetSize();
				pFile->Seek(0, FILE_BEGIN);
				std::vector<BYTE> buf(65536);
				UINT64 total = 0;
				while (total < size)
				{
					const DWORD got = pFile->Read(buf.data(), static_cast<DWORD>(buf.size()));
					if (got == 0)
						break;
					total += got;
				}
				pFile->Seek(static_cast<INT64>(size / 2), FILE_BEGIN);
				pFile->Read(buf.data(), 16);
			}
			return 0;
		}
		DWORD WINAPI GetValue(const wchar_t*, wchar_t* pszValue, int nSize) override
		{
			if (nSize > 0)
				pszValue[0] = 0;
			return 0;
		}
		void WINAPI SetOverwrite(BOOL) override {}
		void WINAPI SetPicture(DWORD, const wchar_t*, const wchar_t*, const wchar_t*, DWORD, DWORD, const BYTE*, DWORD) override {}
		void WINAPI aSetValueA(const char*, int, const char*, int) override {}
		void WINAPI aSetValueW(const char*, int, const wchar_t*, int) override {}
		void WINAPI aSetValueU8(const char*, int, const char*, int) override {}
		void WINAPI wSetValueA(const wchar_t*, int, const char*, int) override {}
		void WINAPI wSetValueW(const wchar_t* cszName, int, const wchar_t* cszValue, int nValueSize) override
		{
			// 文字列が NUL 終端されていることを確かめる (ASan が読み過ぎを見つける)
			if (nValueSize < 0)
				sink_ += std::wcslen(cszName) + std::wcslen(cszValue);
		}
		void WINAPI wSetValueU8(const wchar_t*, int, const char*, int) override {}
		void WINAPI u8SetValueA(const char*, int, const char*, int) override {}
		void WINAPI u8SetValueW(const char*, int, const wchar_t*, int) override {}
		void WINAPI u8SetValueU8(const char*, int, const char*, int) override {}

	private:
		size_t sink_ = 0;
	};

	// 1 回の Render の長さの上限。ファイル全体はデコードしない
	constexpr DWORD MaxFrames = 2048;

	template<class Decoder>
	void decode(const uint8_t* data, size_t size, DWORD rate, INT32 bits, bool tagOnly)
	{
		auto* file = new MemoryFile(data, size);
		KPI_MEDIAINFO request;
		kpi_InitMediaInfo(&request);
		request.dwSampleRate = rate;
		request.nBitsPerSample = bits;

		auto* decoder = new Decoder();
		if (!decoder->Open(tagOnly ? nullptr : &request, file))
		{
			decoder->Release();
			file->Release();
			return;
		}
		file->Release();	// デコーダーが AddRef して持つ

		auto* tags = new TagInfo();
		const KPI_MEDIAINFO* info = nullptr;
		if (tagOnly)
		{
			decoder->Select(1, nullptr, tags, KPI_TAGGET_FLAG_TITLE | KPI_TAGGET_FLAG_ARTIST);
		}
		else if (decoder->Select(1, &info, tags, KPI_TAGGET_FLAG_ALL) == 1 && info != nullptr)
		{
			const size_t frameBytes = static_cast<size_t>(info->dwChannels) * (std::abs(info->nBitsPerSample) / 8);
			std::vector<BYTE> buffer(frameBytes * MaxFrames);
			const UINT64 total = kpi_100nsToSample(info->qwLength, info->dwSampleRate);
			const UINT64 positions[] = { 0, 1, total / 2, total > 3 ? total - 3 : 0, total, total + 100, UINT64_MAX / 2 };
			const DWORD sizes[] = { MaxFrames, 1, 7, 333 };
			for (size_t i = 0; i < std::size(positions); i++)
			{
				decoder->Seek(positions[i], KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE);
				const DWORD frames = sizes[i % std::size(sizes)];
				decoder->Render(buffer.data(), frames);
				decoder->Render(buffer.data(), MaxFrames);
			}
		}
		decoder->Select(2, &info, nullptr, 0);	// 範囲外の曲番号
		tags->Release();
		decoder->Release();
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	{
		// パーサーが返した範囲は、ValidateDsdLayout を通っていればファイルに収まる
		MemoryReader reader(data, size);
		DsdLayout layout;
		if (ParseDsd(reader, layout) && !ValidateDsdLayout(layout, size))
			__debugbreak();
	}
	decode<KpiDopDecoder>(data, size, 0, -64, true);
	decode<KpiDopDecoder>(data, size, 0, -64, false);
	decode<KpiDopDecoder>(data, size, 0, 24, false);
	decode<KpiD2pDecoder>(data, size, 0, -64, false);
	decode<KpiD2pDecoder>(data, size, 352800, -32, false);
	return 0;
}
