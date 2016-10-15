#include "stdafx.h"

#include "KpiDsdDecoderModule.h"
#include "CAbstractKpi.h"
#include "CDFFDecoderKpi.h"
#include "CDSFDecoderKpi.h"
#include "CWSDDecoderKpi.h"

// {EFFCF24A-81A0-4B99-915A-EC5002F927E5}
static const GUID kpidsd_GUID =
{ 0xeffcf24a, 0x81a0, 0x4b99,{ 0x91, 0x5a, 0xec, 0x50, 0x2, 0xf9, 0x27, 0xe5 } };

#define MAKE_SEMVER(major, minor, patch)	(((major) << 16) | ((minor) << 8) | (patch))
#define KPI_VERSION		MAKE_SEMVER(1, 0, 0)

#ifdef _DEBUG
#define DEBUG_MSG		L" [DEBUG]"
#else
#define DEBUG_MSG		L""
#endif
static const wchar_t* kpi_description = L"Direct DSD player plugin for KbMedia Player" DEBUG_MSG;
static const wchar_t* kpi_copyright = L"Copyright(c) 2016, Autch.net";
static const wchar_t* kpi_supportExts = L".dsf/.dff/.wsd";

KpiDsdDecoderModule::KpiDsdDecoderModule() : KbKpiDecoderModuleImpl(),
moduleInfo{
	sizeof(KPI_DECODER_MODULEINFO),
	KPI_DECODER_MODULE_VERSION,
	KPI_VERSION,
	KPI_MULTINST_INFINITE,
	kpidsd_GUID,
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

KpiDsdDecoderModule::~KpiDsdDecoderModule()
{
}

void KpiDsdDecoderModule::GetModuleInfo(const KPI_DECODER_MODULEINFO ** ppInfo)
{
	*ppInfo = &moduleInfo;
}

DWORD KpiDsdDecoderModule::Open(const KPI_MEDIAINFO * cpRequest, IKpiFile * pFile, IKpiFolder * pFolder, IKpiDecoder ** ppDecoder)
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
	}
	else {
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
