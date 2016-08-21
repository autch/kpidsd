#include "stdafx.h"

#include "KpiDopDecoderModule.h"
#include "CAbstractKpi.h"
#include "CDFFDecoderKpi.h"
#include "CDSFDecoderKpi.h"
#include "CWSDDecoderKpi.h"

// {FFB4ED1D-F6C7-433C-8C7D-D63024C0B6BE}
static const GUID kpidop_GUID =
{ 0xffb4ed1d, 0xf6c7, 0x433c,{ 0x8c, 0x7d, 0xd6, 0x30, 0x24, 0xc0, 0xb6, 0xbe } };

#define MAKE_SEMVER(major, minor, patch)	(((major) << 16) | ((minor) << 8) | (patch))
#define KPI_VERSION		MAKE_SEMVER(2, 0, 0)

#ifdef _DEBUG
#define DEBUG_MSG		L" [DEBUG]"
#else
#define DEBUG_MSG		L""
#endif
static const wchar_t* kpi_description = L"DSD over PCM frames plugin for KbMedia Player" DEBUG_MSG;
static const wchar_t* kpi_copyright = L"Copyright(c) 2015, 2016, Autch.net";
static const wchar_t* kpi_supportExts = L".dsf/.dff/.wsd";

KpiDopDecoderModule::KpiDopDecoderModule() : KbKpiDecoderModuleImpl(),
moduleInfo{
	sizeof(KPI_DECODER_MODULEINFO),
	KPI_DECODER_MODULE_VERSION,
	KPI_VERSION,
	KPI_MULTINST_INFINITE,
	kpidop_GUID,
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

KpiDopDecoderModule::~KpiDopDecoderModule()
{
}

void KpiDopDecoderModule::GetModuleInfo(const KPI_DECODER_MODULEINFO ** ppInfo)
{
	*ppInfo = &moduleInfo;
}

DWORD KpiDopDecoderModule::Open(const KPI_MEDIAINFO * cpRequest, IKpiFile * pFile, IKpiFolder * pFolder, IKpiDecoder ** ppDecoder)
{
	CAbstractKpi* pDecoder;
	uint8_t signature[8];

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
