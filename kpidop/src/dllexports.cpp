
#include "stdafx.h"
#include "kmp_pi.h"
#include "kpi.h"
#include "KpiDopDecoderModule.h"

HRESULT WINAPI kpi_CreateInstance(REFIID riid, void **ppvObject, IKpiUnknown *pUnknown)
{
	*ppvObject = NULL;
	if (!IsEqualIID(riid, IID_IKpiDecoderModule)) {
		return E_NOINTERFACE;
	}
	IKpiDecoderModule *pDecoderModule = new KpiDopDecoderModule();
	*ppvObject = pDecoderModule;
	return S_OK;
}
