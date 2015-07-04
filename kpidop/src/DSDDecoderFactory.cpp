#include "stdafx.h"
#include "DSDDecoderFactory.h"
#include "CDSFDecoderKpi.h"
#include "CWSDDecoderKpi.h"

CAbstractKpi* CreateKpiDecoderInstance(LPCSTR szFileName)
{
	unsigned char* ext = _mbsrchr((unsigned char*)szFileName, '.');
	CAbstractKpi* kpi = NULL;

	if (ext == NULL)
		return NULL;

	if (_stricmp((char*)ext, ".dsf") == 0) {
		kpi = new CDSFDecoderKpi();
	}
	if (_stricmp((char*)ext, ".wsd") == 0) {
		kpi = new CWSDDecoderKpi();
	}

	return kpi;
}

BOOL GetDSDTagInfo(LPCSTR cszFileName, IKmpTagInfo* pInfo)
{
	unsigned char* ext = _mbsrchr((unsigned char*)cszFileName, '.');
	CAbstractKpi* kpi = FALSE;

	if (ext == NULL)
		return FALSE;

	if (_stricmp((char*)ext, ".dsf") == 0) {
		return CDSFDecoderKpi::GetTagInfo(cszFileName, pInfo);
	}
	if (_stricmp((char*)ext, ".wsd") == 0) {
		return CWSDDecoderKpi::GetTagInfo(cszFileName, pInfo);
	}

	return FALSE;
}