#include "stdafx.h"

#include "CWSDDecoderKpi.h"
#include "CKpiFileAdapter.h"

CWSDDecoderKpi::CWSDDecoderKpi() : file(), pFile(NULL), srcBuffer(NULL), dsd2pcm()
{

}

CWSDDecoderKpi::~CWSDDecoderKpi()
{
	Close();
}

void CWSDDecoderKpi::Close()
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

void CWSDDecoderKpi::Reset()
{
	file.Reset();
	dsd2pcm.Reset();
}

DWORD CWSDDecoderKpi::Open(const KPI_MEDIAINFO* pRequest, IKpiFile* kpiFile, IKpiFolder* folder)
{
	CKpiFileAdapter* pKpiFile = new CKpiFileAdapter(kpiFile);
	if (!file.Open(pKpiFile)) {
		delete pKpiFile;
		return 0;
	}

	uint32_t dsd_fs = file.DataSpec()->samplingFrequency;
	uint32_t channels = file.DataSpec()->channels;

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
	}

	{
		uint64_t samples = file.FileSize() - file.Header()->dataOffset;
		
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
}

UINT64 WINAPI CWSDDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	double rate = (double)(file.DataSpec()->samplingFrequency) / (double)mInfo.dwSampleRate;
	uint64_t bytePos = qwPosSample;
	bytePos *= rate;
	bytePos *= mInfo.dwChannels;
	bytePos >>= 3;

	Reset();
	file.Seek(bytePos, NULL, FILE_CURRENT);

	return qwPosSample;
}

DWORD CWSDDecoderKpi::Render(BYTE* buffer, DWORD dwSizeSample)
{
	DWORD dwBytesRead = 0;
	DWORD dwSize = dwSizeSample * (mInfo.dwChannels * (mInfo.nBitsPerSample / 8));
	PBYTE d = buffer;
	DWORD totalSamplesWritten = 0, samplesWritten = 0;
	DWORD dwSamplesToRender = dwSizeSample;

	::ZeroMemory(buffer, dwSize);
	while (dwSamplesToRender > 0)
	{
		if (!dsd2pcm.isInFlush()) {
			file.Read(srcBuffer, srcBufferSize, &dwBytesRead);
			if (dwBytesRead < srcBufferSize) {
				dsd2pcm.RenderLast();
			}
		}

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

void trim(const wchar_t* szName, IKpiTagInfo* pInfo, uint8_t* buf, size_t size)
{
	char* tmp = new char[size + 1];

	memcpy(tmp, buf, size);
	tmp[size] = '\0';

	char* pe = tmp + size;
	while (pe != tmp && *--pe == ' ')
	{
		//
	}
	if (*pe == ' ')
		*pe = '\0';
	else
		*(pe + 1) = '\0';

	pInfo->wSetValueA(szName, -1, tmp, -1);

	delete[] tmp;
}

DWORD CWSDDecoderKpi::Select(DWORD dwNumber, const KPI_MEDIAINFO** ppMediaInfo, IKpiTagInfo* pTagInfo, DWORD dwTagGetFlags)
{
	if (dwNumber != 1)
		return 0;

	if (ppMediaInfo != NULL)
		*ppMediaInfo = &mInfo;
	if (pTagInfo != NULL) {
		// do not use builtin tag parser
		pTagInfo->GetTagInfo(NULL, NULL, KPI_TAGTYPE_NONE, 0);

		trim(SZ_KMP_NAME_TITLE_W, pTagInfo, file.Text()->title, sizeof file.Text()->title);
		trim(SZ_KMP_NAME_ARTIST_W, pTagInfo, file.Text()->artist, sizeof file.Text()->artist);
		trim(SZ_KMP_NAME_ALBUM_W, pTagInfo, file.Text()->album, sizeof file.Text()->album);
		trim(SZ_KMP_NAME_GENRE_W, pTagInfo, file.Text()->genre, sizeof file.Text()->genre);
		trim(SZ_KMP_NAME_COMMENT_W, pTagInfo, file.Text()->comment, sizeof file.Text()->comment);
		trim(SZ_KMP_NAME_DATE_W, pTagInfo, file.Text()->dateAndTime, sizeof file.Text()->dateAndTime);

		setBitrate(file.DataSpec()->samplingFrequency, mInfo.dwSampleRate, abs(mInfo.nBitsPerSample), file.DataSpec()->channels, pTagInfo);
	}

	return 1;
}
