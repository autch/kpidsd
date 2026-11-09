#include "stdafx.h"

#include "KpiDopDecoderModule.h"
#include "KpiDopDecoder.h"

namespace
{
	// {FFB4ED1D-F6C7-433C-8C7D-D63024C0B6BE}
	constexpr GUID kpidop_GUID = { 0xffb4ed1d, 0xf6c7, 0x433c, { 0x8c, 0x7d, 0xd6, 0x30, 0x24, 0xc0, 0xb6, 0xbe } };

	constexpr DWORD makeVersion(DWORD major, DWORD minor, DWORD patch) { return (major << 16) | (minor << 8) | patch; }
	// 本体に同梱されている v2.0.1 (131073) より大きくする
	constexpr DWORD KpiVersion = makeVersion(2, 1, 0);

	const KPI_DECODER_MODULEINFO moduleInfo = {
		sizeof(KPI_DECODER_MODULEINFO),
		KPI_DECODER_MODULE_VERSION,
		KpiVersion,
		KPI_MULTINST_INFINITE,
		kpidop_GUID,
		L"DSD Audio over PCM frames plugin for KbMedia Player",
		L"Copyright (c) 2015-2026, Autch.net",
		L".dsf/.dff/.wsd",
		L"",		// cszMultiSongExts
		nullptr,	// pvReserved1
		nullptr,	// pvReserved2
		KPI_DECODER_MODULEINFO::TAG_TITLE | KPI_DECODER_MODULEINFO::TAG_EXTRA,
		0,			// dwSupportConfig
		{ 0, 0, 0, 0 },
	};
}

void KpiDopDecoderModule::GetModuleInfo(const KPI_DECODER_MODULEINFO** ppInfo)
{
	*ppInfo = &moduleInfo;
}

DWORD KpiDopDecoderModule::Open(const KPI_MEDIAINFO* cpRequest, IKpiFile* pFile, IKpiFolder* pFolder, IKpiDecoder** ppDecoder)
{
	*ppDecoder = nullptr;
	KpiDopDecoder* decoder = nullptr;
	try
	{
		decoder = new KpiDopDecoder();
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
