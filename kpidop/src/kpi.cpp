
#include "stdafx.h"
#include "kmp_pi.h"
#include "CAbstractKpi.h"
#include "DSDDecoderFactory.h"

char g_szIniFileName[MAX_PATH];
extern HMODULE g_hModule;

void WINAPI kpiInit()
{
	char* pDot;

	GetModuleFileName(::g_hModule, ::g_szIniFileName, MAX_PATH);
	pDot = strrchr(g_szIniFileName, '.');
	strncpy_s(pDot, MAX_PATH - (pDot - g_szIniFileName), ".ini", 4);
}

void WINAPI kpiDeinit()
{
}

HKMP WINAPI kpiOpen(const char* cszFileName, SOUNDINFO* pInfo)
{
	CAbstractKpi* d = CreateKpiDecoderInstance(cszFileName);
	if (d == NULL) return NULL;

	if (d->Open((LPSTR)cszFileName, pInfo))
		return (HKMP)d;
	delete d;
	return NULL;
}

void WINAPI kpiClose(HKMP hKMP)
{
	CAbstractKpi* d = (CAbstractKpi*)hKMP;
	if (d) {
		d->Close();
		delete d;
	}
}

DWORD WINAPI kpiRender(HKMP hKMP, BYTE* Buffer, DWORD dwSize)
{
	CAbstractKpi* d = (CAbstractKpi*)hKMP;
	if (d)
		return d->Render(Buffer, dwSize);
	return 0;
}

DWORD WINAPI kpiSetPosition(HKMP hKMP, DWORD dwPos)
{
	CAbstractKpi* d = (CAbstractKpi*)hKMP;
	if (d)
		return d->SetPosition(dwPos);
	return 0;
}

UINT GetMyProfileInt(LPSTR szSectionName, LPSTR szKeyName, INT nDefault)
{
	return ::GetPrivateProfileInt(szSectionName, szKeyName, nDefault, ::g_szIniFileName);
}

BOOL WINAPI kpiGetTagInfo(const char *cszFileName, IKmpTagInfo *pTagInfo)
{
	return GetDSDTagInfo(cszFileName, pTagInfo);
}
