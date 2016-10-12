#include "stdafx.h"

#include "KpiD2pDecoderModule.h"

HRESULT WINAPI kpi_CreateInstance(REFIID riid, void **ppvObject, IKpiUnknown *pUnknown)
{
	*ppvObject = NULL;
	if (!IsEqualIID(riid, IID_IKpiDecoderModule)) {
		return E_NOINTERFACE;
	}
	IKpiDecoderModule *pDecoderModule = new KpiD2pDecoderModule();
	*ppvObject = pDecoderModule;
	return S_OK;
}
