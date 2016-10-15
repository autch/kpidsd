#include "stdafx.h"

#include "CDFFDecoderKpi.h"
#include "CKpiFileAdapter.h"

CDFFDecoderKpi::CDFFDecoderKpi() : file(), pFile(NULL)
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
	if (pFile != NULL) {
		((CKpiFileAdapter*)pFile)->GetKpiFile()->Release();
		delete pFile;
		pFile = NULL;
	}
}

UINT64 CDFFDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	uint64_t bytePos = qwPosSample;
	bytePos *= file.FRM8().prop.chnl.data.numChannels;

	Reset();
	file.Seek(bytePos, NULL, FILE_CURRENT);

	return qwPosSample;
}

void CDFFDecoderKpi::Reset()
{
	file.Reset();
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
	mInfo.dwFormatType = (DWORD)-1;

	uint32_t dsd_fs = file.FRM8().prop.fs.data.sampleRate;
	uint32_t channels = file.FRM8().prop.chnl.data.numChannels;

	mInfo.dwChannels = channels;
	mInfo.dwSampleRate = dsd_fs;
	mInfo.dwSeekableFlags = KPI_MEDIAINFO::SEEK_FLAGS_SAMPLE | KPI_MEDIAINFO::SEEK_FLAGS_ACCURATE | KPI_MEDIAINFO::SEEK_FLAGS_ROUGH;
	mInfo.nBitsPerSample = 8;
	mInfo.dwUnitSample = SAMPLES_PER_BLOCK;

	{
		uint64_t samples = file.FRM8().dsd.DataSize();

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

DWORD CDFFDecoderKpi::Render(BYTE* buffer, DWORD dwSizeSample)
{
	DWORD dwBytesRead = 0;
	DWORD dwSize = dwSizeSample * mInfo.dwChannels;
	PBYTE d = buffer;
	uint64_t dsdEndPos = file.FRM8().dsd.OffsetToData() + file.FRM8().dsd.DataSize();
	DWORD totalSamplesWritten = 0, samplesWritten = 0;
	DWORD dwSamplesToRender = dwSizeSample;

	::ZeroMemory(buffer, dwSize);
	while (dwSamplesToRender > 0) {
		DWORD dwBytesToRead = dwSamplesToRender * mInfo.dwChannels;

		if (file.Tell() >= dsdEndPos) break;

		if (dsdEndPos - file.Tell() < dwBytesToRead)
			dwBytesToRead = (DWORD)(dsdEndPos - file.Tell());

		if (!file.Read(d, dwBytesToRead, &dwBytesRead))
			break;

		samplesWritten = dwBytesRead / mInfo.dwChannels;
		d += dwBytesRead;
		totalSamplesWritten += samplesWritten;
		dwSamplesToRender -= samplesWritten;
	}

	return totalSamplesWritten;
}
