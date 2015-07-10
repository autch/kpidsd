#include "stdafx.h"
#include "CDFFDecoderKpi.h"
#include "dop.h"
#include "kpi.h"

CDFFDecoderKpi::CDFFDecoderKpi() : file(), srcBuffer(NULL)
{
}

CDFFDecoderKpi::~CDFFDecoderKpi()
{
	Close();
}

void CDFFDecoderKpi::Close()
{
	file.Close();
	if (srcBuffer != NULL)
	{
		delete[] srcBuffer;
		srcBuffer = NULL;
	}
}

void CDFFDecoderKpi::Reset()
{
	file.Reset();
	last_marker = DOP_MARKER1;
}

BOOL CDFFDecoderKpi::Open(LPSTR szFileName, SOUNDINFO* pInfo)
{
	if (!file.Open(szFileName))
		return FALSE;

	// DST compression is not supported
	if (file.FRM8().prop.cmpr.compressionName != CMPR_NAME_DSD)
		return FALSE;

	uint32_t dsd_fs = file.FRM8().prop.fs.data.sampleRate;
	uint32_t channels = file.FRM8().prop.chnl.data.numChannels;

	soundinfo.dwChannels = channels;

	switch (dsd_fs) {
	case DSD_FREQ_64FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_64FS;
		break;
	case DSD_FREQ_128FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_128FS;
		break;
	default:
		// 256FS Ç∆Ç©ÇÕÇ±Ç¡ÇøÇí Ç∑
		if (dsd_fs % 44100 == 0)
			soundinfo.dwSamplesPerSec = dsd_fs / 16;
		else
			goto fail_cleanup;
	}
	soundinfo.dwReserved1 = soundinfo.dwReserved2 = 0;
	soundinfo.dwSeekable = 1;

	pInfo->dwBitsPerSample = GetMyProfileInt("kpidop", "BitsPerDoPFrame", pInfo->dwBitsPerSample);

	switch (pInfo->dwBitsPerSample)
	{
	case 0:
	case 24:
		soundinfo.dwBitsPerSample = 24;
		soundinfo.dwUnitRender = 3 * channels * SAMPLES_PER_BLOCK / 2;
		break;
	case 32:
	default:
		soundinfo.dwBitsPerSample = 32;
		soundinfo.dwUnitRender = 4 * channels * SAMPLES_PER_BLOCK / 2;
		break;
	}

	{
		uint64_t samples = file.FRM8().dsd.DataSize();

		samples <<= 3;
		samples *= 1000;
		samples /= dsd_fs;
		samples /= channels;

		soundinfo.dwLength = (DWORD)samples;
	}

	memcpy(pInfo, &soundinfo, sizeof soundinfo);

	srcBufferSize = SAMPLES_PER_BLOCK * channels;
	srcBuffer = new BYTE[srcBufferSize];

	Reset();

	return TRUE;

fail_cleanup:
	Close();
	return FALSE;
}

DWORD CDFFDecoderKpi::SetPosition(DWORD dwPos)
{
	if (file.File() == INVALID_HANDLE_VALUE)
		return 0;

	uint64_t bytePos = dwPos;
	bytePos *= file.FRM8().prop.fs.data.sampleRate;
	bytePos *= file.FRM8().prop.chnl.data.numChannels;
	bytePos /= 1000;
	bytePos >>= 3;

	BYTE marker = last_marker;

	Reset();
	file.Seek(bytePos, NULL, FILE_CURRENT);

	last_marker = marker;

	// bytePos Ç©ÇÁãÅÇﬂÇΩÉ~Éäïbà íuÇÕÅAÉÇÉmÉâÉã 64FS Ç…Ç®Ç¢ÇƒÇ‡ dwPos Ç∆ 1ms ñ¢ñûÇÃåÎç∑Ç≈ÇµÇ©ñ≥Ç¢
	return dwPos;
}

DWORD CDFFDecoderKpi::Render(BYTE* buffer, DWORD dwSize)
{
	DWORD dwBytesRead = 0;
	PBYTE d = buffer, de = buffer + dwSize;
	BYTE marker = last_marker;
	BYTE frame[4] = { 0, 0, 0, 0 };
	DWORD dwBytesToWrite, dwFrameOffset;
	int channels = file.FRM8().prop.chnl.data.numChannels;
	uint64_t dsdEndPos = file.FRM8().dsd.OffsetToData() + file.FRM8().dsd.DataSize();

	if (soundinfo.dwBitsPerSample == 24)
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
	while (d < de) {
		DWORD dwBytesToRead = srcBufferSize;

		if (file.Tell() >= dsdEndPos) break;

		if (dsdEndPos - file.Tell() < dwBytesToRead)
			dwBytesToRead = (DWORD)(dsdEndPos - file.Tell());

		if (!file.Read(srcBuffer, dwBytesToRead, &dwBytesRead))
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

	return d - buffer;
}

BOOL CDFFDecoderKpi::GetTagInfo(const char *cszFileName, IKmpTagInfo *pInfo)
{
	CDFFFile file;

	if (!file.Open(cszFileName))
		return FALSE;

	if (file.FRM8().diin.diar.artistText.length() > 0)
		pInfo->SetValueA(SZ_KMP_TAGINFO_NAME_ARTIST, file.FRM8().diin.diar.artistText.c_str());
	if (file.FRM8().diin.diti.titleText.length() > 0)
		pInfo->SetValueA(SZ_KMP_TAGINFO_NAME_TITLE, file.FRM8().diin.diti.titleText.c_str());
	if (file.FRM8().comt.comments.size() > 0)
	{
		std::vector<Comment>::iterator it = file.FRM8().comt.comments.begin();
		if (it != file.FRM8().comt.comments.end())
			pInfo->SetValueA(SZ_KMP_TAGINFO_NAME_COMMENT, it->commentText.c_str());
	}

	file.Close();
	return TRUE;
}