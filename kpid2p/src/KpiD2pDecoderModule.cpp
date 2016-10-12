#include "stdafx.h"

#include "KpiD2pDecoderModule.h"
#include "CAbstractKpi.h"
#include "CDFFDecoderKpi.h"
#include "CDSFDecoderKpi.h"
#include "CWSDDecoderKpi.h"

// {8AD2DC0A-C360-4C2E-ACC6-3C3EFD24C912}
static const GUID kpid2p_GUID =
{ 0x8ad2dc0a, 0xc360, 0x4c2e,{ 0xac, 0xc6, 0x3c, 0x3e, 0xfd, 0x24, 0xc9, 0x12 } };

#define MAKE_SEMVER(major, minor, patch)	(((major) << 16) | ((minor) << 8) | (patch))
#define KPI_VERSION		MAKE_SEMVER(1, 0, 0)

#ifdef _DEBUG
#define DEBUG_MSG		L" [DEBUG]"
#else
#define DEBUG_MSG		L""
#endif
static const wchar_t* kpi_description = L"DSD to PCM plugin for KbMedia Player" DEBUG_MSG;
static const wchar_t* kpi_copyright = L"Copyright(c) 2016, Autch.net";
static const wchar_t* kpi_supportExts = L".dsf/.dff/.wsd";

KpiD2pDecoderModule::KpiD2pDecoderModule() : KbKpiDecoderModuleImpl(),
moduleInfo{
	sizeof(KPI_DECODER_MODULEINFO),
	KPI_DECODER_MODULE_VERSION,
	KPI_VERSION,
	KPI_MULTINST_INFINITE,
	kpid2p_GUID,
	kpi_description,
	kpi_copyright,
	kpi_supportExts,
	L"", // multisongexts
	L"", // codecs
	L"", // containers
	KPI_DECODER_MODULEINFO::TAG_TITLE | KPI_DECODER_MODULEINFO::TAG_EXTRA,
	0,
	0, 0, 0, 0 // reserved[4]
	}
{
}

KpiD2pDecoderModule::~KpiD2pDecoderModule()
{
}

void KpiD2pDecoderModule::GetModuleInfo(const KPI_DECODER_MODULEINFO ** ppInfo)
{
	*ppInfo = &moduleInfo;
}

DWORD KpiD2pDecoderModule::Open(const KPI_MEDIAINFO * cpRequest, IKpiFile * pFile, IKpiFolder * pFolder, IKpiDecoder ** ppDecoder)
{
	CAbstractKpi* pDecoder;
	uint8_t signature[4];

	pFile->Seek(0, FILE_BEGIN);
	pFile->Read(signature, sizeof signature);
	pFile->Seek(0, FILE_BEGIN);

	if (memcmp(signature, "FRM8", 4) == 0) {
		// DFF
		pDecoder = new CDFFDecoderKpi();
	} else if (memcmp(signature, "DSD ", 4) == 0) {
		// DSF
		pDecoder = new CDSFDecoderKpi();
	} else if (memcmp(signature, "1bit", 4) == 0) {
		// WSD
		pDecoder = new CWSDDecoderKpi();
	} else {
		// !?
		*ppDecoder = NULL;
		return 0;
	}
	DWORD dwCount = pDecoder->Open(cpRequest, pFile, pFolder);
	if (dwCount == 0) {
		delete pDecoder;
		*ppDecoder = NULL;
		return 0;
	}
	*ppDecoder = (IKpiDecoder*)pDecoder;

	return dwCount;
}
