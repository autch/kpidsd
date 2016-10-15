#include "stdafx.h"

#include "KpiDsdDecoderModule.h"

HRESULT WINAPI kpi_CreateInstance(REFIID riid, void **ppvObject, IKpiUnknown *pUnknown)
{
	*ppvObject = NULL;
	if (!IsEqualIID(riid, IID_IKpiDecoderModule)) {
		return E_NOINTERFACE;
	}
	IKpiDecoderModule *pDecoderModule = new KpiDsdDecoderModule();
	*ppvObject = pDecoderModule;
	return S_OK;
}
