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
	DWORD WINAPI Open(const KPI_MEDIAINFO *cpRequest,//�Đ����g��/���[�v���v����
		IKpiFile     *pFile,           //���y�f�[�^
		IKpiFolder   *pFolder,         //���y�f�[�^������t�H���_
		IKpiDecoder **ppDecoder);

	// TODO: stub!
	BOOL  WINAPI EnumConfig(IKpiConfigEnumerator *pEnumerator)
	{
		return FALSE;
	}

	DWORD WINAPI ApplyConfig(const wchar_t *cszSection,//�Z�N�V������
		const wchar_t *cszKey,    //�L�[��
		INT64  nValue, //�ύX��̒l(BOOL/INT �̏ꍇ)
		double dValue, //�ύX��̒l(FLOAT �̏ꍇ)
		const wchar_t *cszValue) //�ύX��̒l(������)
	{
		return KPI_CFGRET_OK;
	}


};