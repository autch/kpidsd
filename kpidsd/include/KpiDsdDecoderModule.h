#pragma once

#include "kpi_impl.h"

class KpiDsdDecoderModule: public KbKpiDecoderModuleImpl
{
private:
	KPI_DECODER_MODULEINFO moduleInfo;

public:
	KpiDsdDecoderModule();
	~KpiDsdDecoderModule();

	void  WINAPI GetModuleInfo(const KPI_DECODER_MODULEINFO **ppInfo);
	DWORD WINAPI Open(const KPI_MEDIAINFO *cpRequest,//再生周波数/ループ数要求等
		IKpiFile     *pFile,           //音楽データ
		IKpiFolder   *pFolder,         //音楽データがあるフォルダ
		IKpiDecoder **ppDecoder);

	// TODO: stub!
	BOOL  WINAPI EnumConfig(IKpiConfigEnumerator *pEnumerator)
	{
		return FALSE;
	}

	DWORD WINAPI ApplyConfig(const wchar_t *cszSection,//セクション名
		const wchar_t *cszKey,    //キー名
		INT64  nValue, //変更後の値(BOOL/INT の場合)
		double dValue, //変更後の値(FLOAT の場合)
		const wchar_t *cszValue) //変更後の値(文字列)
	{
		return KPI_CFGRET_OK;
	}


};