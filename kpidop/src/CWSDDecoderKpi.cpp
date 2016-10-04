#include "stdafx.h"

#include "CWSDDecoderKpi.h"
#include "dop.h"
#include "CKpiFileAdapter.h"

CWSDDecoderKpi::CWSDDecoderKpi() : file(), pFile(NULL), srcBuffer(NULL)
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
}

void CWSDDecoderKpi::Reset()
{
	file.Reset();
	last_marker = DOP_MARKER1;
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
			mInfo.dwUnitSample = SAMPLES_PER_BLOCK / 2;
			break;
		case 32:
		default:
			mInfo.nBitsPerSample = 32;
			mInfo.dwUnitSample = SAMPLES_PER_BLOCK / 2;
			break;
		}
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

fail_cleanup:
	Close();
	return 0;
}

UINT64 WINAPI CWSDDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	uint64_t bytePos = qwPosSample;
	bytePos *= 16;
	bytePos *= mInfo.dwChannels;
	bytePos >>= 3;

	BYTE marker = last_marker;

	Reset();
	file.Seek(bytePos, NULL, FILE_CURRENT);

	last_marker = marker;

	return qwPosSample;
}

DWORD CWSDDecoderKpi::Render(BYTE* buffer, DWORD dwSizeSample)
{
	DWORD dwBytesRead = 0;
	DWORD dwSize = dwSizeSample * (mInfo.dwChannels * (mInfo.nBitsPerSample / 8));
	PBYTE d = buffer, de = buffer + dwSize;
	BYTE marker = last_marker;
	BYTE frame[4] = { 0, 0, 0, 0 };
	DWORD dwBytesToWrite, dwFrameOffset;
	int channels = file.DataSpec()->channels;

	if (mInfo.nBitsPerSample == 24)
	{
		dwBytesToWrite = 3;
		dwFrameOffset = 1;
	}
	else
	{
		dwBytesToWrite = 4;
		dwFrameOffset = 0;
	}

	::ZeroMemory(buffer, dwSize);
	while (d < de)
	{
		if (!file.Read(srcBuffer, srcBufferSize, &dwBytesRead))
			break;
		for (DWORD dwBytePos = 0; dwBytePos < dwBytesRead; dwBytePos += 2 * channels)
		{
			for (int ch = 0; ch < channels; ch++)
			{
				PBYTE p = srcBuffer + dwBytePos + ch;
				PBYTE pp = srcBuffer + dwBytePos + channels + ch;

				frame[3] = marker;
				frame[2] = *p;
				frame[1] = *pp;
				memcpy(d, frame + dwFrameOffset, dwBytesToWrite);
				d += dwBytesToWrite;
			}
			marker ^= 0xff;
		}
		if (dwBytesRead < srcBufferSize)
			break;
	}

	last_marker = marker;

	return (DWORD)(d - buffer) / mInfo.dwChannels / (mInfo.nBitsPerSample / 8);
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

		setBitrate(file.DataSpec()->samplingFrequency, file.DataSpec()->channels, pTagInfo);
	}

	return 1;
}
