// kpidop.cpp : DLL のエントリポイントと kpi_CreateInstance

#include "stdafx.h"

#include "KpiDopDecoderModule.h"

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
			*ppvObject = static_cast<IKpiDecoderModule*>(new KpiDopDecoderModule());
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
