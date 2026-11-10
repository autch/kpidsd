// kpidop.cpp : プラグインの情報、DLL のエントリポイントと kpi_CreateInstance

#include "stdafx.h"

#include "KpiDopDecoder.h"

namespace
{
	// {FFB4ED1D-F6C7-433C-8C7D-D63024C0B6BE}
	constexpr GUID kpidop_GUID = { 0xffb4ed1d, 0xf6c7, 0x433c, { 0x8c, 0x7d, 0xd6, 0x30, 0x24, 0xc0, 0xb6, 0xbe } };

	// 本体に同梱されている v2.0.1 (131073) より大きくする
	constexpr KPI_DECODER_MODULEINFO moduleInfo = MakeModuleInfo(MakePluginVersion(2, 1, 0), kpidop_GUID,
		L"DSD Audio over PCM frames plugin for KbMedia Player", L"Copyright (c) 2015-2026, Autch.net");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	if (dwReasonForCall == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hModule);
	return TRUE;
}

extern "C" HRESULT WINAPI kpi_CreateInstance(REFIID riid, void** ppvObject, IKpiUnknown* pUnknown)
{
	return CreateDsdDecoderModule<KpiDopDecoder>(riid, ppvObject, moduleInfo);
}
