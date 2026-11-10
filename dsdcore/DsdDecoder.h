#pragma once

#include "kpi.h"
#include "kpi_decoder.h"
#include "kpi_impl.h"
#include "DsdSource.h"

// kpidop と kpid2p のデコーダーに共通の部分。1 ファイル 1 曲。
// 派生クラスは setup (出力の形式を決める)、render、seek だけを書く
class DsdDecoderBase : public KbKpiDecoderImpl
{
public:
	DsdDecoderBase();

	// cpRequest は NULL のことがある (タグ取得用)。開けなければ false
	bool Open(const KPI_MEDIAINFO* cpRequest, IKpiFile* pFile);

	DWORD  WINAPI Select(DWORD dwNumber, const KPI_MEDIAINFO** ppMediaInfo, IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags) final;
	DWORD  WINAPI Render(BYTE* pBuffer, DWORD dwSizeSample) final;
	UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag) final;
	DWORD  WINAPI UpdateConfig(void* pvReserved) final { return 0; }

protected:
	// info の dwFormatType、dwSampleRate、nBitsPerSample と totalSamples (出力のサンプル数) を決める。
	// 開けなければ false。タグ取得用 (cpRequest == NULL) なら、重い準備は省いてよい
	virtual bool setup(const KPI_MEDIAINFO* cpRequest) = 0;
	// 例外は DsdDecoderBase が受け止める
	virtual DWORD render(BYTE* pBuffer, DWORD dwSizeSample) = 0;
	// position は totalSamples 以下にしてある
	virtual void seek(uint64_t position) = 0;

	// ファイルが開いたときより短くなったなど、曲を早く終えるとき
	void truncate(uint64_t samples) { totalSamples = samples; }

	DsdSource source;
	KPI_MEDIAINFO info;
	uint64_t totalSamples = 0;
};

// KPI_DECODER_MODULEINFO を作る
constexpr DWORD MakePluginVersion(DWORD major, DWORD minor, DWORD patch) { return (major << 16) | (minor << 8) | patch; }
constexpr KPI_DECODER_MODULEINFO MakeModuleInfo(DWORD version, const GUID& guid, const wchar_t* description, const wchar_t* copyright)
{
	return {
		sizeof(KPI_DECODER_MODULEINFO),
		KPI_DECODER_MODULE_VERSION,
		version,
		KPI_MULTINST_INFINITE,
		guid,
		description,
		copyright,
		L".dsf/.dff/.wsd",
		L"",		// cszMultiSongExts
		nullptr,	// pvReserved1
		nullptr,	// pvReserved2
		KPI_DECODER_MODULEINFO::TAG_TITLE | KPI_DECODER_MODULEINFO::TAG_EXTRA,
		0,			// dwSupportConfig
		{ 0, 0, 0, 0 },
	};
}

// Decoder (DsdDecoderBase の派生) を作る IKpiDecoderModule。設定は持たない
template<class Decoder>
class DsdDecoderModule : public KbKpiDecoderModuleImpl
{
public:
	explicit DsdDecoderModule(const KPI_DECODER_MODULEINFO& moduleInfo) : moduleInfo_(moduleInfo) {}

	void WINAPI GetModuleInfo(const KPI_DECODER_MODULEINFO** ppInfo) override { *ppInfo = &moduleInfo_; }

	DWORD WINAPI Open(const KPI_MEDIAINFO* cpRequest, IKpiFile* pFile, IKpiFolder* pFolder, IKpiDecoder** ppDecoder) override
	{
		*ppDecoder = nullptr;
		Decoder* decoder = nullptr;
		try
		{
			decoder = new Decoder();
			if (!decoder->Open(cpRequest, pFile))
			{
				decoder->Release();
				return 0;
			}
		}
		catch (...)
		{
			if (decoder != nullptr)
				decoder->Release();
			return 0;
		}
		*ppDecoder = static_cast<IKpiDecoder*>(decoder);
		return 1;
	}

	BOOL  WINAPI EnumConfig(IKpiConfigEnumerator* pEnumerator) override { return FALSE; }
	DWORD WINAPI ApplyConfig(const wchar_t*, const wchar_t*, INT64, double, const wchar_t*) override { return KPI_CFGRET_OK; }

private:
	const KPI_DECODER_MODULEINFO& moduleInfo_;
};

// kpi_CreateInstance の中身
template<class Decoder>
HRESULT CreateDsdDecoderModule(REFIID riid, void** ppvObject, const KPI_DECODER_MODULEINFO& moduleInfo)
{
	*ppvObject = nullptr;
	if (!IsEqualIID(riid, IID_IKpiDecoderModule))
		return E_NOINTERFACE;
	try
	{
		*ppvObject = static_cast<IKpiDecoderModule*>(new DsdDecoderModule<Decoder>(moduleInfo));
		return S_OK;
	}
	catch (...)
	{
		return E_OUTOFMEMORY;
	}
}
