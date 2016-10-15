#include "stdafx.h"

#include "dop.h"

#include "CDFFDecoderKpi.h"
#include "CKpiFileAdapter.h"

CDFFDecoderKpi::CDFFDecoderKpi() : file(), pFile(NULL), srcBuffer(NULL)
{
}

CDFFDecoderKpi::~CDFFDecoderKpi()
{
	Close();
}

DWORD CDFFDecoderKpi::Select(DWORD dwNumber, const KPI_MEDIAINFO** ppMediaInfo, IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags)
{
	if (dwNumber != 1)
		return 0;

	if (ppMediaInfo != NULL)
		*ppMediaInfo = &mInfo;
	if (pTagInfo != NULL) {
		// do not use builtin tag parser
		pTagInfo->GetTagInfo(NULL, NULL, KPI_TAGTYPE_NONE, 0);

		if (file.FRM8().diin.diar.artistText.length() > 0)
			pTagInfo->wSetValueA(SZ_KMP_NAME_ARTIST_W, -1, file.FRM8().diin.diar.artistText.c_str(), -1);
		if (file.FRM8().diin.diti.titleText.length() > 0)
			pTagInfo->wSetValueA(SZ_KMP_NAME_TITLE_W, -1, file.FRM8().diin.diti.titleText.c_str(), -1);
		if (file.FRM8().comt.comments.size() > 0)
		{
			std::vector<Comment>::iterator it = file.FRM8().comt.comments.begin();
			if (it != file.FRM8().comt.comments.end())
				pTagInfo->wSetValueA(SZ_KMP_NAME_COMMENT_W, -1, it->commentText.c_str(), -1);
		}

		setBitrate(file.FRM8().prop.fs.data.sampleRate, file.FRM8().prop.chnl.data.numChannels, pTagInfo);
	}

	return 1;
}

void CDFFDecoderKpi::Close()
{
	file.Close();
	dsd2dop.Close();
	if (pFile != NULL) {
		((CKpiFileAdapter*)pFile)->GetKpiFile()->Release();
		delete pFile;
		pFile = NULL;
	}
	if (srcBuffer != NULL)
	{
		delete[] srcBuffer;
		srcBuffer = NULL;
	}
}

UINT64 CDFFDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	uint64_t bytePos = qwPosSample;
	bytePos *= 16; // 16 bits in one DoP sample
	bytePos *= file.FRM8().prop.chnl.data.numChannels;
	bytePos >>= 3;

	Reset();
	file.Seek(bytePos, NULL, FILE_CURRENT);

	return qwPosSample;
}

void CDFFDecoderKpi::Reset()
{
	file.Reset();
	dsd2dop.Reset();
}

DWORD CDFFDecoderKpi::Open(const KPI_MEDIAINFO* pRequest, IKpiFile* kpiFile, IKpiFolder* folder)
{
	CKpiFileAdapter* pKpiFile = new CKpiFileAdapter(kpiFile);
	if (!file.Open(pKpiFile)) {
		delete pKpiFile;
		return 0;
	}

	// DST compression is not supported
	if (file.FRM8().prop.cmpr.compressionName != CMPR_NAME_DSD) {
		delete pKpiFile;
		return 0;
	}

	::ZeroMemory(&mInfo, sizeof mInfo);
	mInfo.cb = sizeof(KPI_MEDIAINFO);
	mInfo.dwNumber = 1;
	mInfo.dwCount = 1;
	mInfo.dwFormatType = KPI_MEDIAINFO::FORMAT_DOP;

	uint32_t dsd_fs = file.FRM8().prop.fs.data.sampleRate;
	uint32_t channels = file.FRM8().prop.chnl.data.numChannels;

	mInfo.dwChannels = channels;

	switch (dsd_fs) {
	case DSD_FREQ_64FS:
		mInfo.dwSampleRate = DOP_FREQ_64FS;
		break;
	case DSD_FREQ_128FS:
		mInfo.dwSampleRate = DOP_FREQ_128FS;
		break;
	default:
		// 256FS ‚Æ‚©‚Í‚±‚Á‚¿‚ð’Ê‚·
		if (dsd_fs % 44100 == 0)
			mInfo.dwSampleRate = dsd_fs / 16;
		else
			goto fail_cleanup;
	}
	mInfo.dwSeekableFlags = KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE | KPI_MEDIAINFO::SEEK_FLAGS_ACCURATE | KPI_MEDIAINFO::SEEK_FLAGS_ROUGH;

	//pInfo->dwBitsPerSample = GetMyProfileInt("kpidop", "BitsPerDoPFrame", pInfo->dwBitsPerSample);

	if (pRequest != NULL) {
		switch (pRequest->nBitsPerSample)
		{
		case 0:
		case 24:
			mInfo.nBitsPerSample = 24;
			break;
		case 32:
		default:
			mInfo.nBitsPerSample = 32;
			break;
		}
		dsd2dop.Open(channels, mInfo.nBitsPerSample);
	}
	else
		mInfo.nBitsPerSample = 32;
	mInfo.dwUnitSample = SAMPLES_PER_BLOCK;

	{
		uint64_t samples = file.FRM8().dsd.DataSize();

		samples <<= 3;
		samples *= 1000 * 10000;
		samples /= dsd_fs;
		samples /= channels;

		mInfo.qwLength = samples;
	}

	srcBufferSize = SAMPLES_PER_BLOCK * channels;
	srcBuffer = new BYTE[srcBufferSize];

	Reset();

	kpiFile->AddRef();
	this->pFile = pKpiFile;

	return mInfo.dwCount;

fail_cleanup:
	Close();
	delete pKpiFile;
	return 0;
}

DWORD CDFFDecoderKpi::Render(BYTE* buffer, DWORD dwSizeSample)
{
	DWORD dwBytesRead = 0;
	DWORD dwSize = dwSizeSample * (mInfo.dwChannels * (mInfo.nBitsPerSample / 8));
	PBYTE d = buffer;
	uint64_t dsdEndPos = file.FRM8().dsd.OffsetToData() + file.FRM8().dsd.DataSize();
	DWORD totalSamplesWritten = 0, samplesWritten = 0;
	DWORD dwSamplesToRender = dwSizeSample;

	::ZeroMemory(buffer, dwSize);
	while (dwSamplesToRender > 0) {
		DWORD dwBytesToRead = srcBufferSize;

		if (file.Tell() >= dsdEndPos) break;

		if (dsdEndPos - file.Tell() < dwBytesToRead)
			dwBytesToRead = (DWORD)(dsdEndPos - file.Tell());

		if (!file.Read(srcBuffer, dwBytesToRead, &dwBytesRead))
			break;

		samplesWritten = dsd2dop.Render(srcBuffer, dwBytesRead, 1, 0, d, dwSamplesToRender);
		d += samplesWritten * mInfo.dwChannels * (mInfo.nBitsPerSample / 8);
		totalSamplesWritten += samplesWritten;
		dwSamplesToRender -= samplesWritten;

		if (dwBytesRead < srcBufferSize)
			break;
	}

	return totalSamplesWritten;
}
