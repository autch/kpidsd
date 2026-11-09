#include "stdafx.h"

#include "KpiD2pDecoderModule.h"
#include "KpiD2pDecoder.h"

namespace
{
	// {8AD2DC0A-C360-4C2E-ACC6-3C3EFD24C912}
	constexpr GUID kpid2p_GUID = { 0x8ad2dc0a, 0xc360, 0x4c2e, { 0xac, 0xc6, 0x3c, 0x3e, 0xfd, 0x24, 0xc9, 0x12 } };

	constexpr DWORD makeVersion(DWORD major, DWORD minor, DWORD patch) { return (major << 16) | (minor << 8) | patch; }
	// 本体に同梱されている v1.0.0 (65536) より大きくする
	constexpr DWORD KpiVersion = makeVersion(2, 0, 0);

	const KPI_DECODER_MODULEINFO moduleInfo = {
		sizeof(KPI_DECODER_MODULEINFO),
		KPI_DECODER_MODULE_VERSION,
		KpiVersion,
		KPI_MULTINST_INFINITE,
		kpid2p_GUID,
		L"DSD to PCM plugin for KbMedia Player",
		L"Copyright (c) 2016-2026, Autch.net",
		L".dsf/.dff/.wsd",
		L"",		// cszMultiSongExts
		nullptr,	// pvReserved1
		nullptr,	// pvReserved2
		KPI_DECODER_MODULEINFO::TAG_TITLE | KPI_DECODER_MODULEINFO::TAG_EXTRA,
		0,			// dwSupportConfig
		{ 0, 0, 0, 0 },
	};
}

void KpiD2pDecoderModule::GetModuleInfo(const KPI_DECODER_MODULEINFO** ppInfo)
{
	*ppInfo = &moduleInfo;
}

DWORD KpiD2pDecoderModule::Open(const KPI_MEDIAINFO* cpRequest, IKpiFile* pFile, IKpiFolder* pFolder, IKpiDecoder** ppDecoder)
{
	*ppDecoder = nullptr;
	KpiD2pDecoder* decoder = nullptr;
	try
	{
		decoder = new KpiD2pDecoder();
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
