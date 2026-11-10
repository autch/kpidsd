// kpid2p.cpp : プラグインの情報、DLL のエントリポイントと kpi_CreateInstance

#include "stdafx.h"

#include "KpiD2pDecoder.h"

namespace
{
	// {8AD2DC0A-C360-4C2E-ACC6-3C3EFD24C912}
	constexpr GUID kpid2p_GUID = { 0x8ad2dc0a, 0xc360, 0x4c2e, { 0xac, 0xc6, 0x3c, 0x3e, 0xfd, 0x24, 0xc9, 0x12 } };

	// 本体に同梱されている v1.0.0 (65536) より大きくする
	constexpr KPI_DECODER_MODULEINFO moduleInfo = MakeModuleInfo(MakePluginVersion(2, 0, 0), kpid2p_GUID,
		L"DSD to PCM plugin for KbMedia Player", L"Copyright (c) 2016-2026, Autch.net");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	if (dwReasonForCall == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hModule);
	return TRUE;
}

extern "C" HRESULT WINAPI kpi_CreateInstance(REFIID riid, void** ppvObject, IKpiUnknown* pUnknown)
{
	return CreateDsdDecoderModule<KpiD2pDecoder>(riid, ppvObject, moduleInfo);
}
