#include "stdafx.h"

#include "CDSFDecoderKpi.h"
#include "CKpiFileAdapter.h"
#include "CKpiPartialFile.h"

CDSFDecoderKpi::CDSFDecoderKpi() : file(), pFile(NULL), srcBuffer(NULL), dsd2pcm()
{
}

CDSFDecoderKpi::~CDSFDecoderKpi()
{
	Close();
}

void CDSFDecoderKpi::Close()
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
	mInfo.dwFormatType = KPI_MEDIAINFO::FORMAT_PCM;

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
		mInfo.dwUnitSample = 4096;
	}

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
	dsd2pcm.Reset();

	samplesRendered = 0;
}

UINT64 WINAPI CDSFDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	double rate = (double)(file.FmtHeader()->sampling_frequency) / (double)mInfo.dwSampleRate;
	uint64_t bytePos = qwPosSample;
	bytePos *= rate;
	bytePos >>= 3;

	uint64_t blockPos = bytePos / file.FmtHeader()->block_size_per_channel;

	Reset();
	file.Seek(blockPos * file.FmtHeader()->block_size_per_channel * file.FmtHeader()->channel_num, NULL, FILE_CURRENT);
	samplesRendered = file.FmtHeader()->block_size_per_channel * 8 * blockPos;

	uint64_t newPos = blockPos;
	newPos *= file.FmtHeader()->block_size_per_channel;
	newPos <<= 3;
	newPos /= rate;

	return newPos;
}

DWORD CDSFDecoderKpi::Render(BYTE* buffer, DWORD dwSizeSample)
{
	DWORD dwBytesRead = 0;
	DWORD dwSize = dwSizeSample * (mInfo.dwChannels * (mInfo.nBitsPerSample / 8));
	PBYTE d = buffer, de = buffer + dwSize;
	DWORD dwBytesPerBlockChannel = file.FmtHeader()->block_size_per_channel;
	int bps = file.FmtHeader()->bits_per_sample;
	uint64_t dataEndPos = file.DataOffset() + file.DataHeader()->size - 12;
	uint64_t totalSamplesWritten = 0, samplesWritten = 0;
	DWORD dwSamplesToRender = dwSizeSample;

	::ZeroMemory(buffer, dwSize);
	while (dwSamplesToRender > 0)
	{
		if (!dsd2pcm.isInFlush() && file.Tell() >= dataEndPos) {
			dsd2pcm.RenderLast();
		}

		if (!dsd2pcm.isInFlush()) {
			file.Read(srcBuffer, dwBytesPerBlockChannel * mInfo.dwChannels, &dwBytesRead);
			if (dwBytesRead < dwBytesPerBlockChannel * mInfo.dwChannels) {
				dsd2pcm.RenderLast();
			}
		}

		samplesWritten = dsd2pcm.Render(srcBuffer, dwBytesRead, dwBytesPerBlockChannel, bps == DSF_BPS_LSB ? 1 : 0, d, dwSamplesToRender);
		d += samplesWritten * mInfo.dwChannels * (mInfo.nBitsPerSample / 8);
		totalSamplesWritten += samplesWritten;
		if (dsd2pcm.isInFlush() && samplesWritten < dwSamplesToRender)
			break;
		dwSamplesToRender -= samplesWritten;
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

		setBitrate(file.FmtHeader()->sampling_frequency, mInfo.dwSampleRate, abs(mInfo.nBitsPerSample), file.FmtHeader()->channel_num, pTagInfo);
	}

	return 1;
}
