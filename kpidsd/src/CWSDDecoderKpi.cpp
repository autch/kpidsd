#include "stdafx.h"

#include "CWSDDecoderKpi.h"
#include "CKpiFileAdapter.h"

CWSDDecoderKpi::CWSDDecoderKpi() : file(), pFile(NULL)
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
}

void CWSDDecoderKpi::Reset()
{
	file.Reset();
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
	mInfo.dwFormatType = (DWORD)-1;
	mInfo.dwChannels = channels;
	mInfo.dwSampleRate = dsd_fs;
	mInfo.dwSeekableFlags = KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE | KPI_MEDIAINFO::SEEK_FLAGS_ACCURATE | KPI_MEDIAINFO::SEEK_FLAGS_ROUGH;

	mInfo.nBitsPerSample = 8;
	mInfo.dwUnitSample = SAMPLES_PER_BLOCK;

	{
		uint64_t samples = file.FileSize() - file.Header()->dataOffset;
		
		samples <<= 3;
		samples *= 1000 * 10000;
		samples /= dsd_fs;
		samples /= channels;

		mInfo.qwLength = samples;
	}

	Reset();
	kpiFile->AddRef();
	this->pFile = pKpiFile;

	return mInfo.dwCount;
}

UINT64 WINAPI CWSDDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	uint64_t bytePos = qwPosSample;
	bytePos *= mInfo.dwChannels;

	Reset();
	file.Seek(bytePos, NULL, FILE_CURRENT);

	return qwPosSample;
}

DWORD CWSDDecoderKpi::Render(BYTE* buffer, DWORD dwSizeSample)
{
	DWORD dwBytesRead = 0;
	DWORD dwSize = dwSizeSample * mInfo.dwChannels;
	PBYTE d = buffer;
	DWORD totalSamplesWritten = 0, samplesWritten = 0;
	DWORD dwSamplesToRender = dwSizeSample;

	::ZeroMemory(buffer, dwSize);
	while (dwSamplesToRender > 0)
	{
		DWORD dwBytesToRead = dwSamplesToRender * mInfo.dwChannels;

		if (!file.Read(d, dwBytesToRead, &dwBytesRead))
			break;

		samplesWritten = dwBytesRead / mInfo.dwChannels;
		d += dwBytesRead;
		totalSamplesWritten += samplesWritten;
		dwSamplesToRender -= samplesWritten;
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

		setBitrate(file.DataSpec()->samplingFrequency, file.DataSpec()->channels, pTagInfo);
	}

	return 1;
}
