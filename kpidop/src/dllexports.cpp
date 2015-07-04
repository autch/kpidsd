
#include "stdafx.h"
#include "kmp_pi.h"
#include "kpi.h"
#include "kpi_version.h"

extern "C"
KMPMODULE* APIENTRY kmp_GetTestModule()
{
	static const char* pszExts[] = { ".dsf", ".dff", ".wsd", NULL };
	static KMPMODULE kpiModule =
	{
		KMPMODULE_VERSION,		// DWORD dwVersion;
		KPI_VERSION,    		// DWORD dwPluginVersion;
		KPI_COPYRIGHT, 			// const char	*pszCopyright;
		KPI_DESC,				// const char	*pszDescription;
		pszExts,				// const char	**ppszSupportExts;
		1,						// DWORD dwReentrant;
		kpiInit,				// void (WINAPI *Init)(void);
		kpiDeinit,				// void (WINAPI *Deinit)(void);
		kpiOpen,				// HKMP (WINAPI *Open)(const char *cszFileName, SOUNDINFO *pInfo);
		NULL,	            	// HKMP (WINAPI *OpenFromBuffer)(const BYTE *Buffer, DWORD dwSize, SOUNDINFO *pInfo);
		kpiClose,				// void (WINAPI *Close)(HKMP hKMP);
		kpiRender,				// DWORD (WINAPI *Render)(HKMP hKMP, BYTE* Buffer, DWORD dwSize);
		kpiSetPosition			// DWORD (WINAPI *SetPosition)(HKMP hKMP, DWORD dwPos);
	};
	return &kpiModule;
}

extern "C"
BOOL WINAPI kmp_GetTestTagInfo(const char *cszFileName, IKmpTagInfo *pTagInfo)
{
	return kpiGetTagInfo(cszFileName, pTagInfo);
}