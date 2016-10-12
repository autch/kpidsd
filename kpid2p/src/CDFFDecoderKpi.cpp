#include "stdafx.h"

#include "CDFFDecoderKpi.h"
#include "CKpiFileAdapter.h"

CDFFDecoderKpi::CDFFDecoderKpi() : file(), pFile(NULL), srcBuffer(NULL), dsd2pcm()
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

		setBitrate(file.FRM8().prop.fs.data.sampleRate, mInfo.dwSampleRate, abs(mInfo.nBitsPerSample), file.FRM8().prop.chnl.data.numChannels, pTagInfo);
	}

	return 1;
}

void CDFFDecoderKpi::Close()
{
	file.Close();
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
	dsd2pcm.Close();
}

UINT64 CDFFDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	double rate = (double)(file.FRM8().prop.fs.data.sampleRate) / (double)mInfo.dwSampleRate;
	uint64_t bytePos = qwPosSample;
	bytePos *= rate;
	bytePos *= file.FRM8().prop.chnl.data.numChannels;
	bytePos >>= 3;

	Reset();
	file.Seek(bytePos, NULL, FILE_CURRENT);

	return qwPosSample;
}

void CDFFDecoderKpi::Reset()
{
	file.Reset();
	dsd2pcm.Reset();
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
	mInfo.dwFormatType = KPI_MEDIAINFO::FORMAT_PCM;

	uint32_t dsd_fs = file.FRM8().prop.fs.data.sampleRate;
	uint32_t channels = file.FRM8().prop.chnl.data.numChannels;

	mInfo.dwChannels = channels;
	mInfo.dwSeekableFlags = KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE | KPI_MEDIAINFO::SEEK_FLAGS_ACCURATE | KPI_MEDIAINFO::SEEK_FLAGS_ROUGH;

	//pInfo->dwBitsPerSample = GetMyProfileInt("kpidop", "BitsPerDoPFrame", pInfo->dwBitsPerSample);

	if (pRequest != NULL) {
		if (pRequest->dwSampleRate > 0)
			mInfo.dwSampleRate = pRequest->dwSampleRate;
		else
			mInfo.dwSampleRate = 44100;
		switch (pRequest->nBitsPerSample)
		{
		case 16:
			mInfo.nBitsPerSample = 16;
			break;
		case 0:
		case 24:
		default:
			mInfo.nBitsPerSample = 24;
			break;
		case 32:
			mInfo.nBitsPerSample = 32;
			break;
		}
		dsd2pcm.Open(dsd_fs, mInfo.dwSampleRate, channels, mInfo.nBitsPerSample);
		mInfo.dwUnitSample = 4096;
	} else {
		mInfo.dwSampleRate = 44100;
		mInfo.dwUnitSample = SAMPLES_PER_BLOCK;
	}

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
	PBYTE d = buffer, de = buffer + dwSize;
	uint64_t dsdEndPos = file.FRM8().dsd.OffsetToData() + file.FRM8().dsd.DataSize();
	uint64_t totalSamplesWritten = 0, samplesWritten = 0;
	DWORD dwSamplesToRender = dwSizeSample;

	::ZeroMemory(buffer, dwSize);
	while (dwSamplesToRender > 0) {
		DWORD dwBytesToRead = srcBufferSize;

		if (!dsd2pcm.isInFlush() && file.Tell() >= dsdEndPos) {
			dsd2pcm.RenderLast();
		}

		if (dsdEndPos - file.Tell() < dwBytesToRead)
			dwBytesToRead = (DWORD)(dsdEndPos - file.Tell());

		if (!dsd2pcm.isInFlush() && !file.Read(srcBuffer, dwBytesToRead, &dwBytesRead))
			break;

		samplesWritten = dsd2pcm.Render(srcBuffer, dwBytesRead, 1, 0, d, dwSamplesToRender);
		d += samplesWritten * mInfo.dwChannels * (mInfo.nBitsPerSample / 8);
		totalSamplesWritten += samplesWritten;

		if (dsd2pcm.isInFlush() && samplesWritten < dwSamplesToRender)
			break;
		dwSamplesToRender -= samplesWritten;
		//if (dwBytesRead < srcBufferSize)
		//	break;
	}

	return totalSamplesWritten;
}
