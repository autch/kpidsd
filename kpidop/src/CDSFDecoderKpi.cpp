#include "stdafx.h"

#include "dop.h"
#include "CDSFDecoderKpi.h"
#include "CKpiFileAdapter.h"
#include "CKpiPartialFile.h"

CDSFDecoderKpi::CDSFDecoderKpi() : file(), pFile(NULL), srcBuffer(NULL)
{
}

CDSFDecoderKpi::~CDSFDecoderKpi()
{
	Close();
}

void CDSFDecoderKpi::Close()
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

DWORD CDSFDecoderKpi::Open(const KPI_MEDIAINFO* pRequest, IKpiFile* kpiFile, IKpiFolder* folder)
{
	CKpiFileAdapter* pKpiFile = new CKpiFileAdapter(kpiFile);

	if (!file.Open(pKpiFile)) {
		delete pKpiFile;
		return 0;
	}

	uint32_t dsd_fs = file.FmtHeader()->sampling_frequency;
	uint32_t channels = file.FmtHeader()->channel_num;

	ZeroMemory(&mInfo, sizeof(KPI_MEDIAINFO));
	mInfo.cb = sizeof(KPI_MEDIAINFO);
	mInfo.dwNumber = 1;
	mInfo.dwCount = 1;
	mInfo.dwFormatType = KPI_MEDIAINFO::FORMAT_DOP;

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
			mInfo.dwUnitSample = file.FmtHeader()->block_size_per_channel;	// FIXME: –{“–H 
			break;
		case 32:
		default:
			mInfo.nBitsPerSample = 32;
			mInfo.dwUnitSample = file.FmtHeader()->block_size_per_channel;
			break;
		}
		dsd2dop.Open(channels, mInfo.nBitsPerSample);
	}
	else
		mInfo.nBitsPerSample = 32;

	{
		uint64_t qwSamples = file.FmtHeader()->sample_count;
		qwSamples *= 1000 * 10000;
		qwSamples /= dsd_fs;
		mInfo.qwLength = qwSamples;
	}

	srcBufferSize = file.FmtHeader()->block_size_per_channel * channels;
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

void CDSFDecoderKpi::Reset()
{
	file.Reset();
	dsd2dop.Reset();

	samplesRendered = 0;
}

UINT64 WINAPI CDSFDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	uint64_t bytePos = qwPosSample;
	bytePos *= 16;
	bytePos >>= 3;

	uint64_t blockPos = bytePos / file.FmtHeader()->block_size_per_channel;

	Reset();
	file.Seek(blockPos * file.FmtHeader()->block_size_per_channel * file.FmtHeader()->channel_num, NULL, FILE_CURRENT);
	samplesRendered = file.FmtHeader()->block_size_per_channel * 8 * blockPos;

	uint64_t newPos = blockPos;
	newPos *= file.FmtHeader()->block_size_per_channel;
	newPos <<= 3;
	newPos /= 16;

	samplesRendered = blockPos * file.FmtHeader()->block_size_per_channel * 8;

	return newPos;
}

DWORD CDSFDecoderKpi::Render(BYTE* buffer, DWORD dwSizeSample)
{
	DWORD dwBytesRead = 0;
	DWORD dwSize = dwSizeSample * (mInfo.dwChannels * (mInfo.nBitsPerSample / 8));
	PBYTE d = buffer;
	uint64_t sampleCount = file.FmtHeader()->sample_count;
	DWORD dwBytesPerBlockChannel = file.FmtHeader()->block_size_per_channel;
	int bps = file.FmtHeader()->bits_per_sample;
	uint64_t dataEndPos = file.DataOffset() + file.DataHeader()->size - 12;
	DWORD totalSamplesWritten = 0, samplesWritten = 0;
	DWORD dwSamplesToRender = dwSizeSample;

	::ZeroMemory(buffer, dwSize);
	while (dwSamplesToRender > 0 && samplesRendered < sampleCount)
	{
		if (file.Tell() >= dataEndPos) break;

		file.Read(srcBuffer, dwBytesPerBlockChannel * mInfo.dwChannels, &dwBytesRead);
		if (dwBytesRead < dwBytesPerBlockChannel * mInfo.dwChannels) break;

		if (dwSamplesToRender * 16 > sampleCount - samplesRendered) {
			dwSamplesToRender = (sampleCount - samplesRendered) / 16;
 		}

		samplesWritten = dsd2dop.Render(srcBuffer, dwBytesRead, dwBytesPerBlockChannel, bps == DSF_BPS_LSB ? 1 : 0, d, dwSamplesToRender);
		d += samplesWritten * mInfo.dwChannels * (mInfo.nBitsPerSample / 8);
		totalSamplesWritten += samplesWritten;
		dwSamplesToRender -= samplesWritten;
		samplesRendered += samplesWritten * 16;
	}
	return totalSamplesWritten;
}

DWORD CDSFDecoderKpi::Select(DWORD dwNumber, const KPI_MEDIAINFO** ppMediaInfo, IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags)
{
	if (dwNumber != 1)
		return 0;

	if (ppMediaInfo != NULL)
		*ppMediaInfo = &mInfo;
	if (pTagInfo != NULL) {
		if (file.Header()->id3v2_pointer == 0) {
			return 1;
		}

		if (!file.Seek(file.Header()->id3v2_pointer, NULL, FILE_BEGIN))
			return 0;

		IKpiFile* kpiFile = ((CKpiFileAdapter*)pFile)->GetKpiFile();
		CKpiPartialFile partialFile(kpiFile, file.Header()->id3v2_pointer,
			pFile->FileSize());
		pTagInfo->GetTagInfo(&partialFile, NULL, KPI_TAGTYPE_ID3V2, dwTagGetFlags);
		kpiFile->Seek(file.Header()->id3v2_pointer, FILE_BEGIN);

		setBitrate(file.FmtHeader()->sampling_frequency, file.FmtHeader()->channel_num, pTagInfo);
	}

	return 1;
}
