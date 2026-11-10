// kpi-render - kpidop.kpi / kpid2p.kpi をロードして曲を raw PCM に書き出す (検証用ホスト)
//
// usage: kpi-render [-k PLUGIN] [-r RATE] [-b BITS] [-n MAXFRAMES] file > out.raw
//        kpi-render [-k PLUGIN] [-r RATE] [-b BITS] [--tag-only] --info file
//        kpi-render [-k PLUGIN] [-r RATE] [-b BITS] --seek-check file...
//
// 1 つ目は曲を Seek(0) してから Render() し、プラグインが返したままのバイト列を stdout に書く。
//   stderr: "frames=N"
// --info は KPI_MEDIAINFO、Select() で設定されたタグ、本体のタグ解析 (GetTagInfo) の呼ばれ方を
//   UTF-8 で stdout に出す。--tag-only なら、本体がタグ取得用に開くときと同じく
//   cpRequest と ppMediaInfo を NULL にする。
// --seek-check はいろいろな位置にシークしてから半端な長さで Render() した結果が、
//   先頭から続けて Render() した結果と一致するかを確かめる。どちらのプラグインも出力は
//   位置だけで決まるので、シーク直後から一致しなければならない。曲全体はメモリに持たず、基準用に
//   もう 1 つデコーダーを開いて並べて Render() する。
//   DoP (dwFormatType == FORMAT_DOP) ではマーカーのバイトを除いて比べ、マーカーはシークを
//   またいでも 0x05 / 0xFA が交互に続くことを確かめる。
// -r / -b は cpRequest の dwSampleRate / nBitsPerSample (既定は 0 / -64)。
// PLUGIN の既定値は kpi-render.exe と同じディレクトリの kpidop.kpi。パス区切りを含まない名前も
// 同じディレクトリから探す。

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <fcntl.h>
#include <io.h>
#include <map>
#include <string>
#include <vector>

#include "kpi.h"
#include "kpi_decoder.h"
#include "kpi_impl.h"
#include "kpi_mingw_uuid.h"

namespace
{
	// ファイルを開いたまま読む (大きなファイルを全部メモリに読み込まない)。本体の IKpiFile と同じく、
	// 範囲外への Seek は KPI_FILE_EOF を返す
	class StdioFile : public KbKpiFileImpl
	{
	public:
		static StdioFile* Open(const wchar_t* path)
		{
			FILE* fp = _wfopen(path, L"rb");
			if (fp == nullptr)
				return nullptr;
			if (_fseeki64(fp, 0, SEEK_END) != 0)
			{
				std::fclose(fp);
				return nullptr;
			}
			const INT64 size = _ftelli64(fp);
			_fseeki64(fp, 0, SEEK_SET);
			return new StdioFile(fp, static_cast<UINT64>(size));
		}
		~StdioFile() override { std::fclose(fp_); }

		DWORD WINAPI Read(void* pBuffer, DWORD dwSize) override
		{
			if (pos_ >= size_)
				return 0;
			const DWORD bytes = static_cast<DWORD>(std::min<UINT64>(dwSize, size_ - pos_));
			const size_t got = std::fread(pBuffer, 1, bytes, fp_);
			pos_ += got;
			return static_cast<DWORD>(got);
		}
		UINT64 WINAPI Seek(INT64 i64Pos, DWORD dwOrigin) override
		{
			const INT64 base = dwOrigin == FILE_BEGIN ? 0 : dwOrigin == FILE_CURRENT ? static_cast<INT64>(pos_) : static_cast<INT64>(size_);
			if (base + i64Pos < 0 || static_cast<UINT64>(base + i64Pos) > size_)
				return KPI_FILE_EOF;
			pos_ = static_cast<UINT64>(base + i64Pos);
			_fseeki64(fp_, static_cast<INT64>(pos_), SEEK_SET);
			return pos_;
		}
		UINT64 WINAPI GetSize() override { return size_; }

	private:
		StdioFile(FILE* fp, UINT64 size) : fp_(fp), size_(size) {}
		FILE* fp_;
		UINT64 size_;
		UINT64 pos_ = 0;
	};

	// Select() で設定されたタグと、GetTagInfo() の呼ばれ方を集める。本体のタグ解析ルーチンは持たない
	class TagInfo : public KbKpiTagInfoImpl
	{
	public:
		std::map<std::wstring, std::wstring> values;
		struct Call
		{
			DWORD type;
			bool hasFile;
			std::vector<BYTE> contents;	// 渡された IKpiFile の中身
		};
		std::vector<Call> calls;

		DWORD WINAPI GetTagInfo(IKpiFile* pFile, IKpiFolder*, DWORD dwType, DWORD) override
		{
			Call call{ dwType, pFile != nullptr, {} };
			if (pFile != nullptr)
			{
				// 本体と同じく、渡されたファイルを先頭から終わりまで読む
				const UINT64 size = pFile->GetSize();
				if (size != KPI_FILE_EOF && size < (64 << 20) && pFile->Seek(0, FILE_BEGIN) == 0)
				{
					call.contents.resize(static_cast<size_t>(size));
					call.contents.resize(pFile->Read(call.contents.data(), static_cast<DWORD>(size)));
				}
			}
			calls.push_back(std::move(call));
			return 0;
		}
		DWORD WINAPI GetValue(const wchar_t* cszName, wchar_t* pszValue, int nSize) override
		{
			const auto found = values.find(cszName);
			if (found == values.end() || nSize <= 0)
				return 0;
			wcsncpy_s(pszValue, static_cast<size_t>(nSize) / sizeof(wchar_t), found->second.c_str(), _TRUNCATE);
			return static_cast<DWORD>(found->second.size());
		}
		void WINAPI SetOverwrite(BOOL) override {}
		void WINAPI SetPicture(DWORD, const wchar_t*, const wchar_t*, const wchar_t*, DWORD, DWORD, const BYTE*, DWORD) override {}

		void WINAPI aSetValueA(const char* cszName, int nNameSize, const char* cszValue, int nValueSize) override
		{ set(widen(CP_ACP, cszName, nNameSize), L"(A)" + widen(CP_ACP, cszValue, nValueSize)); }
		void WINAPI aSetValueW(const char* cszName, int nNameSize, const wchar_t* cszValue, int nValueSize) override
		{ set(widen(CP_ACP, cszName, nNameSize), wide(cszValue, nValueSize)); }
		void WINAPI aSetValueU8(const char* cszName, int nNameSize, const char* cszValue, int nValueSize) override
		{ set(widen(CP_ACP, cszName, nNameSize), widen(CP_UTF8, cszValue, nValueSize)); }
		// ACP に頼る設定はテストで見分けられるように印を付ける
		void WINAPI wSetValueA(const wchar_t* cszName, int nNameSize, const char* cszValue, int nValueSize) override
		{ set(wide(cszName, nNameSize), L"(A)" + widen(CP_ACP, cszValue, nValueSize)); }
		void WINAPI wSetValueW(const wchar_t* cszName, int nNameSize, const wchar_t* cszValue, int nValueSize) override
		{ set(wide(cszName, nNameSize), wide(cszValue, nValueSize)); }
		void WINAPI wSetValueU8(const wchar_t* cszName, int nNameSize, const char* cszValue, int nValueSize) override
		{ set(wide(cszName, nNameSize), widen(CP_UTF8, cszValue, nValueSize)); }
		void WINAPI u8SetValueA(const char* cszName, int nNameSize, const char* cszValue, int nValueSize) override
		{ set(widen(CP_UTF8, cszName, nNameSize), L"(A)" + widen(CP_ACP, cszValue, nValueSize)); }
		void WINAPI u8SetValueW(const char* cszName, int nNameSize, const wchar_t* cszValue, int nValueSize) override
		{ set(widen(CP_UTF8, cszName, nNameSize), wide(cszValue, nValueSize)); }
		void WINAPI u8SetValueU8(const char* cszName, int nNameSize, const char* cszValue, int nValueSize) override
		{ set(widen(CP_UTF8, cszName, nNameSize), widen(CP_UTF8, cszValue, nValueSize)); }

	private:
		void set(std::wstring name, std::wstring value) { values[std::move(name)] = std::move(value); }

		// nSize は -1 なら NUL 終端、そうでなければバイト数
		static std::wstring wide(const wchar_t* text, int nSize)
		{
			if (text == nullptr)
				return {};
			return nSize < 0 ? std::wstring(text) : std::wstring(text, wcsnlen(text, static_cast<size_t>(nSize) / sizeof(wchar_t)));
		}
		static std::wstring widen(UINT codepage, const char* text, int nSize)
		{
			if (text == nullptr)
				return {};
			const int bytes = nSize < 0 ? static_cast<int>(std::strlen(text)) : static_cast<int>(strnlen(text, static_cast<size_t>(nSize)));
			const int length = MultiByteToWideChar(codepage, 0, text, bytes, nullptr, 0);
			std::wstring result(static_cast<size_t>(length), L'\0');
			MultiByteToWideChar(codepage, 0, text, bytes, result.data(), length);
			return result;
		}
	};

	struct Options
	{
		DWORD rate = 0;
		INT32 bits = -64;
		bool tagOnly = false;
	};

	void usage()
	{
		std::fputs("usage: kpi-render [-k PLUGIN] [-r RATE] [-b BITS] [-n MAXFRAMES] file > out.raw\n"
		           "       kpi-render [-k PLUGIN] [-r RATE] [-b BITS] [--tag-only] --info file\n"
		           "       kpi-render [-k PLUGIN] [-r RATE] [-b BITS] --seek-check file...\n", stderr);
		std::exit(2);
	}

	// 曲数を返す。失敗したら 0
	DWORD openDecoder(IKpiDecoderModule* module, const wchar_t* path, const Options& options, IKpiDecoder** ppDecoder)
	{
		*ppDecoder = nullptr;
		auto* file = StdioFile::Open(path);
		if (file == nullptr)
		{
			std::fwprintf(stderr, L"%ls: cannot read\n", path);
			return 0;
		}

		KPI_MEDIAINFO request;
		kpi_InitMediaInfo(&request);
		request.dwSampleRate = options.rate;
		request.nBitsPerSample = options.bits;
		const DWORD count = module->Open(options.tagOnly ? nullptr : &request, file, nullptr, ppDecoder);
		file->Release();
		if (count == 0 || *ppDecoder == nullptr)
		{
			std::fwprintf(stderr, L"%ls: cannot open\n", path);
			return 0;
		}
		return count;
	}

	const KPI_MEDIAINFO* selectTrack(IKpiDecoder* decoder, const wchar_t* path, TagInfo* tags)
	{
		const KPI_MEDIAINFO* info = nullptr;
		if (decoder->Select(1, &info, tags, KPI_TAGGET_FLAG_ALL) != 1 || info == nullptr)
		{
			std::fwprintf(stderr, L"%ls: cannot select\n", path);
			return nullptr;
		}
		const int bits = std::abs(info->nBitsPerSample);
		if (info->dwChannels == 0 || (bits != 16 && bits != 24 && bits != 32 && bits != 64))
		{
			std::fwprintf(stderr, L"%ls: unexpected format %u ch %d bit\n", path, info->dwChannels, info->nBitsPerSample);
			return nullptr;
		}
		return info;
	}

	size_t frameBytes(const KPI_MEDIAINFO* info)
	{
		return static_cast<size_t>(info->dwChannels) * (std::abs(info->nBitsPerSample) / 8);
	}

	// frames フレームまで Render() する。request ごとの長さを sizes から順に選ぶ。返り値は書けたフレーム数
	size_t renderFrames(IKpiDecoder* decoder, size_t bytesPerFrame, std::vector<BYTE>& out, size_t frames,
		const std::vector<DWORD>& sizes = { 65536 })
	{
		out.resize(frames * bytesPerFrame);
		size_t done = 0;
		for (size_t k = 0; done < frames; k++)
		{
			const DWORD request = static_cast<DWORD>(std::min<size_t>(frames - done, sizes[k % sizes.size()]));
			const DWORD rendered = decoder->Render(out.data() + done * bytesPerFrame, request);
			done += rendered;
			if (rendered < request)
				break;
		}
		out.resize(done * bytesPerFrame);
		return done;
	}

	void printUtf8(const std::wstring& text)
	{
		const int bytes = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
		std::string utf8(static_cast<size_t>(bytes), '\0');
		WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), utf8.data(), bytes, nullptr, nullptr);
		std::fwrite(utf8.data(), 1, utf8.size(), stdout);
	}

	int info(IKpiDecoderModule* module, const wchar_t* path, const Options& options)
	{
		IKpiDecoder* decoder = nullptr;
		const DWORD count = openDecoder(module, path, options, &decoder);
		if (count == 0)
			return 1;
		TagInfo tags;
		_setmode(_fileno(stdout), _O_BINARY);
		std::printf("count=%lu\n", count);
		if (options.tagOnly)
		{
			// タグ取得用: ppMediaInfo は NULL
			if (decoder->Select(1, nullptr, &tags, KPI_TAGGET_FLAG_TITLE | KPI_TAGGET_FLAG_ARTIST) != 1)
			{
				std::fwprintf(stderr, L"%ls: cannot select\n", path);
				decoder->Release();
				return 1;
			}
		}
		else
		{
			const KPI_MEDIAINFO* media = selectTrack(decoder, path, &tags);
			if (media == nullptr)
			{
				decoder->Release();
				return 1;
			}
			std::printf("format=%lu\nrate=%lu\nchannels=%lu\nbits=%d\nlength=%llu\nloop=%llu\nfadeout=%lld\nseekable=%lu\nunit=%lu\n",
				media->dwFormatType, media->dwSampleRate, media->dwChannels, media->nBitsPerSample,
				media->qwLength, media->qwLoop, static_cast<long long>(media->qwFadeOut), media->dwSeekableFlags, media->dwUnitSample);
		}
		for (size_t i = 0; i < tags.calls.size(); i++)
		{
			const auto& call = tags.calls[i];
			std::printf("gettaginfo.%zu.type=%lu\n", i, call.type);
			if (call.hasFile)
			{
				std::printf("gettaginfo.%zu.size=%zu\ngettaginfo.%zu.data=", i, call.contents.size(), i);
				for (const BYTE b : call.contents)
					std::printf("%02x", b);
				std::printf("\n");
			}
		}
		for (const auto& [name, value] : tags.values)
			printUtf8(L"tag." + name + L"=" + value + L"\n");
		decoder->Release();
		return 0;
	}

	int render(IKpiDecoderModule* module, const wchar_t* path, const Options& options, size_t maxframes)
	{
		IKpiDecoder* decoder = nullptr;
		if (openDecoder(module, path, options, &decoder) == 0)
			return 1;
		const KPI_MEDIAINFO* media = selectTrack(decoder, path, nullptr);
		if (media == nullptr)
		{
			decoder->Release();
			return 1;
		}

		_setmode(_fileno(stdout), _O_BINARY);
		const size_t bytesPerFrame = frameBytes(media);
		decoder->Seek(0, KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE);
		std::vector<BYTE> buf;
		size_t frames = 0;
		while (frames < maxframes)
		{
			const size_t request = std::min<size_t>(maxframes - frames, 65536);
			const size_t rendered = renderFrames(decoder, bytesPerFrame, buf, request);
			std::fwrite(buf.data(), 1, buf.size(), stdout);
			frames += rendered;
			if (rendered < request)
				break;
		}
		std::fflush(stdout);
		std::fprintf(stderr, "frames=%zu\n", frames);
		decoder->Release();
		return 0;
	}

	// DoP のマーカーが 0x05 / 0xFA の交互に続いているかを数える
	class MarkerCheck
	{
	public:
		MarkerCheck(size_t bytesPerFrame, size_t bytesPerWord) : frame_(bytesPerFrame), word_(bytesPerWord) {}

		// data の各フレームのマーカーを調べ、おかしなフレームの数を返す
		size_t feed(const std::vector<BYTE>& data)
		{
			size_t errors = 0;
			for (size_t f = 0; f + frame_ <= data.size(); f += frame_)
			{
				const BYTE marker = data[f + word_ - 1];
				for (size_t w = word_ - 1; w < frame_; w += word_)
				{
					if (data[f + w] != marker)
						errors++;	// チャンネルごとに違う
				}
				if ((marker != 0x05 && marker != 0xFA) || (last_ != 0 && marker == last_))
					errors++;
				last_ = marker;
			}
			return errors;
		}

	private:
		size_t frame_;
		size_t word_;
		BYTE last_ = 0;
	};

	bool sameFrames(const BYTE* a, const BYTE* b, size_t bytes, size_t bytesPerWord, bool dop)
	{
		if (!dop)
			return std::memcmp(a, b, bytes) == 0;
		for (size_t i = 0; i < bytes; i++)
		{
			if (i % bytesPerWord != bytesPerWord - 1 && a[i] != b[i])
				return false;
		}
		return true;
	}

	// Render() を繰り返して frames フレーム読み捨てる。読めたフレーム数を返す
	size_t skipFrames(IKpiDecoder* decoder, size_t bytesPerFrame, size_t frames, std::vector<BYTE>& scratch)
	{
		size_t done = 0;
		while (done < frames)
		{
			const DWORD request = static_cast<DWORD>(std::min<size_t>(frames - done, 65536));
			scratch.resize(static_cast<size_t>(request) * bytesPerFrame);
			const DWORD rendered = decoder->Render(scratch.data(), request);
			done += rendered;
			if (rendered < request)
				break;
		}
		return done;
	}

	// 曲全体をメモリに持たないように、基準用にもう 1 つデコーダーを開き、並べて Render() して比べる
	int seekCheck(IKpiDecoderModule* module, const wchar_t* path, const Options& options)
	{
		IKpiDecoder* decoder = nullptr;
		IKpiDecoder* reference = nullptr;
		if (openDecoder(module, path, options, &decoder) == 0)
			return 1;
		if (openDecoder(module, path, options, &reference) == 0)
		{
			decoder->Release();
			return 1;
		}
		const KPI_MEDIAINFO* media = selectTrack(decoder, path, nullptr);
		const KPI_MEDIAINFO* refMedia = selectTrack(reference, path, nullptr);
		if (media == nullptr || refMedia == nullptr)
		{
			decoder->Release();
			reference->Release();
			return 1;
		}
		const size_t bytesPerFrame = frameBytes(media);
		const size_t bytesPerWord = std::abs(media->nBitsPerSample) / 8;
		const bool dop = media->dwFormatType == KPI_MEDIAINFO::FORMAT_DOP;
		// qwLength はサンプル数を 100ns 単位に切り捨てたものなので、切り上げて戻す (周波数は 10MHz 未満)
		const UINT64 total = (media->qwLength * media->dwSampleRate + 10000000 - 1) / 10000000;
		MarkerCheck markers(bytesPerFrame, bytesPerWord);		// シークする側。シークをまたいでも交互に続く
		MarkerCheck refMarkers(bytesPerFrame, bytesPerWord);	// 基準の側
		std::vector<BYTE> out, ref, scratch;
		int fails = 0;

		// 先頭から続けて Render() したときの長さと、DoP のマーカー
		reference->Seek(0, KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE);
		size_t frames = 0;
		size_t refMarkerErrors = 0;
		for (;;)
		{
			const DWORD request = 65536;
			ref.resize(static_cast<size_t>(request) * bytesPerFrame);
			const DWORD rendered = reference->Render(ref.data(), request);
			ref.resize(static_cast<size_t>(rendered) * bytesPerFrame);
			if (dop)
				refMarkerErrors += refMarkers.feed(ref);
			frames += rendered;
			if (rendered < request || frames > total + 1000)
				break;
		}
		if (frames != total)
		{
			std::fwprintf(stderr, L"%ls: rendered %zu frames, the length says %llu\n", path, frames, total);
			fails++;
		}
		if (refMarkerErrors != 0)
		{
			std::fwprintf(stderr, L"%ls: DoP markers are broken\n", path);
			fails++;
		}

		std::vector<UINT64> positions = { 0, 1, 7, 1000, 4095, 4096, 4097, frames / 3, frames / 2, frames - 1, frames, frames + 500 };
		static const std::vector<DWORD> sizes = { 1, 333, 32, 7, 4096, 50, 8191 };
		for (const UINT64 pos : positions)
		{
			if (pos > frames + 500)
				continue;
			const UINT64 expect = std::min<UINT64>(pos, frames);
			const UINT64 got = decoder->Seek(pos, KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE);
			if (got != expect)
			{
				std::fwprintf(stderr, L"%ls: Seek(%llu) returned %llu, expected %llu\n", path, pos, got, expect);
				fails++;
				continue;
			}
			// 基準は先頭から Render() して、シーク先まで読み捨てる
			reference->Seek(0, KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE);
			skipFrames(reference, bytesPerFrame, static_cast<size_t>(got), scratch);

			// 半端な長さで最後まで Render() して、基準の続きと比べる
			const size_t rest = static_cast<size_t>(frames - got);
			size_t rendered = 0;
			bool differs = false;
			for (size_t k = 0; ; k++)
			{
				const DWORD request = sizes[k % sizes.size()];
				out.resize(static_cast<size_t>(request) * bytesPerFrame);
				const DWORD n = decoder->Render(out.data(), request);
				out.resize(static_cast<size_t>(n) * bytesPerFrame);
				ref.resize(out.size());
				const DWORD m = n > 0 ? reference->Render(ref.data(), n) : 0;
				if (m != n || !sameFrames(out.data(), ref.data(), out.size(), bytesPerWord, dop))
					differs = true;
				if (dop && markers.feed(out) != 0)
				{
					std::fwprintf(stderr, L"%ls: DoP markers after Seek(%llu) are broken\n", path, pos);
					fails++;
				}
				rendered += n;
				if (n < request || rendered > rest + 100)
					break;
			}
			if (rendered != rest)
			{
				std::fwprintf(stderr, L"%ls: Render after Seek(%llu) gave %zu frames, expected %zu\n", path, pos, rendered, rest);
				fails++;
			}
			else if (differs)
			{
				std::fwprintf(stderr, L"%ls: PCM after Seek(%llu) differs\n", path, pos);
				fails++;
			}
		}
		std::fwprintf(stderr, L"%ls: %ls (%zu frames)\n", path, fails ? L"NG" : L"ok", frames);
		decoder->Release();
		reference->Release();
		return fails ? 1 : 0;
	}

	std::wstring moduleDirectory()
	{
		wchar_t path[MAX_PATH];
		const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
		std::wstring dir(path, len);
		return dir.substr(0, dir.find_last_of(L"\\/") + 1);
	}
}

int wmain(int argc, wchar_t** argv)
{
	std::wstring plugin = L"kpidop.kpi";
	size_t maxframes = SIZE_MAX;
	bool seek_check = false;
	bool show_info = false;
	Options options;
	std::vector<const wchar_t*> files;

	for (int i = 1; i < argc; i++)
	{
		if (!std::wcscmp(argv[i], L"-k") && i + 1 < argc) plugin = argv[++i];
		else if (!std::wcscmp(argv[i], L"-r") && i + 1 < argc) options.rate = static_cast<DWORD>(_wtoi(argv[++i]));
		else if (!std::wcscmp(argv[i], L"-b") && i + 1 < argc) options.bits = _wtoi(argv[++i]);
		else if (!std::wcscmp(argv[i], L"-n") && i + 1 < argc) maxframes = static_cast<size_t>(_wtoi64(argv[++i]));
		else if (!std::wcscmp(argv[i], L"--seek-check")) seek_check = true;
		else if (!std::wcscmp(argv[i], L"--info")) show_info = true;
		else if (!std::wcscmp(argv[i], L"--tag-only")) options.tagOnly = true;
		else if (argv[i][0] != L'-') files.push_back(argv[i]);
		else usage();
	}
	if (files.empty() || (!seek_check && files.size() != 1) || (seek_check && show_info) || (options.tagOnly && !show_info))
		usage();
	if (plugin.find_first_of(L"\\/") == std::wstring::npos)
		plugin = moduleDirectory() + plugin;

	HMODULE dll = LoadLibraryW(plugin.c_str());
	const auto create = dll ? reinterpret_cast<pfn_kpiCreateInstance>(GetProcAddress(dll, "kpi_CreateInstance")) : nullptr;
	IKpiDecoderModule* module = nullptr;
	if (!create || create(IID_IKpiDecoderModule, reinterpret_cast<void**>(&module), nullptr) != S_OK)
	{
		std::fwprintf(stderr, L"%ls: cannot load plugin\n", plugin.c_str());
		return 1;
	}

	int ret = 0;
	if (seek_check)
	{
		for (const wchar_t* file : files)
			ret |= seekCheck(module, file, options);
	}
	else if (show_info)
		ret = info(module, files[0], options);
	else
		ret = render(module, files[0], options, maxframes);

	module->Release();
	FreeLibrary(dll);
	return ret;
}
