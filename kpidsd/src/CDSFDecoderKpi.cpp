#include "stdafx.h"

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
	if (pFile != NULL) {
		((CKpiFileAdapter*)pFile)->GetKpiFile()->Release();
		delete pFile;
		pFile = NULL;
	}
	if (srcBuffer != NULL) {
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
	mInfo.dwFormatType = (DWORD)-1;

	mInfo.dwChannels = channels;

	mInfo.dwSampleRate = dsd_fs;
	mInfo.dwSeekableFlags = KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE | KPI_MEDIAINFO::SEEK_FLAGS_ACCURATE | KPI_MEDIAINFO::SEEK_FLAGS_ROUGH;

	mInfo.nBitsPerSample = 8;
	mInfo.dwUnitSample = file.FmtHeader()->block_size_per_channel;	// FIXME: –{“–H 

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
}

void CDSFDecoderKpi::Reset()
{
	file.Reset();

	samplesRendered = 0;
}

UINT64 WINAPI CDSFDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	uint64_t bytePos = qwPosSample;

	uint64_t blockPos = bytePos / file.FmtHeader()->block_size_per_channel;

	Reset();
	file.Seek(blockPos * file.FmtHeader()->block_size_per_channel * file.FmtHeader()->channel_num, NULL, FILE_CURRENT);
	samplesRendered = file.FmtHeader()->block_size_per_channel * blockPos;

	uint64_t newPos = blockPos;
	newPos *= file.FmtHeader()->block_size_per_channel;

	samplesRendered = blockPos * file.FmtHeader()->block_size_per_channel;

	return newPos;
}

DWORD CDSFDecoderKpi::Render(BYTE* buffer, DWORD dwSizeSample)
{
	DWORD dwBytesRead = 0;
	DWORD dwSize = dwSizeSample * mInfo.dwChannels;
	PBYTE d = buffer;
	uint64_t sampleCount = file.FmtHeader()->sample_count;
	DWORD dwBytesPerBlockChannel = file.FmtHeader()->block_size_per_channel;
	int bps = file.FmtHeader()->bits_per_sample;
	uint64_t dataEndPos = file.DataOffset() + file.DataHeader()->size - 12;
	DWORD totalSamplesWritten = 0, samplesWritten = 0;
	DWORD dwSamplesToRender = dwSizeSample;
	uint8_t* p;
	uint8_t* pe;

	::ZeroMemory(buffer, dwSize);
	while (dwSamplesToRender > 0 && samplesRendered < sampleCount)
	{
		if (file.Tell() >= dataEndPos) break;

		file.Read(srcBuffer, dwBytesPerBlockChannel * mInfo.dwChannels, &dwBytesRead);
		if (dwBytesRead < dwBytesPerBlockChannel * mInfo.dwChannels) break;

		if (dwSamplesToRender * 16 > sampleCount - samplesRendered) {
			dwSamplesToRender = (sampleCount - samplesRendered) / 16;
 		}

		samplesWritten = 0;
		for (int i = 0; i < dwBytesPerBlockChannel; ++i) {
			for (int ch = 0; ch < mInfo.dwChannels; ++ch) {
				*d++ = srcBuffer[(dwBytesPerBlockChannel * ch) + i];
			}
			samplesWritten++;
		}

		totalSamplesWritten += samplesWritten;
		dwSamplesToRender -= samplesWritten;
		samplesRendered += samplesWritten;
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
