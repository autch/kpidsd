// kpid2p.cpp : DLL のエントリポイントと kpi_CreateInstance

#include "stdafx.h"

#include "KpiD2pDecoderModule.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	if (dwReasonForCall == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hModule);
	return TRUE;
}

extern "C" HRESULT WINAPI kpi_CreateInstance(REFIID riid, void** ppvObject, IKpiUnknown* pUnknown)
{
	if (IsEqualIID(riid, IID_IKpiDecoderModule))
	{
		try
		{
			*ppvObject = static_cast<IKpiDecoderModule*>(new KpiD2pDecoderModule());
			return S_OK;
		}
		catch (...)
		{
			*ppvObject = nullptr;
			return E_OUTOFMEMORY;
		}
	}
	*ppvObject = nullptr;
	return E_NOINTERFACE;
}
