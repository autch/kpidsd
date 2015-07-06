#include "stdafx.h"
#include "CDFFDecoderKpi.h"
#include "dop.h"

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
	if (srcBuffer != NULL) {
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

	soundinfo.dwChannels = file.FRM8().prop.chnl.data.numChannels;

	switch (file.FRM8().prop.fs.data.sampleRate) {
	case DSD_FREQ_64FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_64FS;
		break;
	case DSD_FREQ_128FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_128FS;
		break;
	default:
		// 256FS ‚Æ‚©‚Í‚±‚Á‚¿‚ð’Ê‚·
		if (file.FRM8().prop.fs.data.sampleRate % 44100 == 0) {
			soundinfo.dwSamplesPerSec = file.FRM8().prop.fs.data.sampleRate / 16;
		}
		else
			goto fail_cleanup;
	}
	soundinfo.dwReserved1 = soundinfo.dwReserved2 = 0;
	soundinfo.dwSeekable = 1;

	switch (pInfo->dwBitsPerSample) {
	case 0:
	case 24:
		soundinfo.dwBitsPerSample = 24;
		soundinfo.dwUnitRender = 3 * soundinfo.dwChannels * SAMPLES_PER_BLOCK / 2;
		break;
	case 32:
		soundinfo.dwBitsPerSample = pInfo->dwBitsPerSample;
		soundinfo.dwUnitRender = 4 * soundinfo.dwChannels * SAMPLES_PER_BLOCK / 2;
		break;
	default:
		goto fail_cleanup;
	}

	{
		uint64_t dataBytes = file.FRM8().dsd.DataSize();
		uint64_t samples = dataBytes;

		samples <<= 3;
		samples *= 1000;
		samples /= file.FRM8().prop.fs.data.sampleRate;
		samples /= file.FRM8().prop.chnl.data.numChannels;

		soundinfo.dwLength = (DWORD)samples;
	}

	memcpy(pInfo, &soundinfo, sizeof soundinfo);

	srcBufferSize = SAMPLES_PER_BLOCK * soundinfo.dwChannels;
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

	uint64_t newPos = bytePos;
	newPos <<= 3;
	newPos *= 1000;
	newPos /= file.FRM8().prop.fs.data.sampleRate;
	newPos /= file.FRM8().prop.chnl.data.numChannels;

	last_marker = marker;

	return (DWORD)newPos;
}

DWORD CDFFDecoderKpi::Render(BYTE* buffer, DWORD dwSize)
{
	DWORD dwBytesRead = 0;
	PBYTE d = buffer, de = buffer + dwSize;
	BYTE marker = last_marker;
	BYTE frame[4] = { 0, 0, 0, 0 };
	DWORD dwBytesToWrite, dwFrameOffset;
	int channels = file.FRM8().prop.chnl.data.numChannels;

	if (soundinfo.dwBitsPerSample == 24) {
		dwBytesToWrite = 3;
		dwFrameOffset = 1;
	}
	else {
		dwBytesToWrite = 4;
		dwFrameOffset = 0;
	}

	::ZeroMemory(buffer, dwSize);
	while (d < de) {
		if (!file.Read(srcBuffer, srcBufferSize, &dwBytesRead))
			break;
		for (DWORD dwBytePos = 0; dwBytePos < dwBytesRead; dwBytePos += 2 * channels) {
			for (int ch = 0; ch < channels; ch++) {
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

	if (file.FRM8().diin.diar.artistText.length() > 0) {
		pInfo->SetValueA(SZ_KMP_TAGINFO_NAME_ARTIST, file.FRM8().diin.diar.artistText.c_str());
	}
	if (file.FRM8().diin.diti.titleText.length() > 0) {
		pInfo->SetValueA(SZ_KMP_TAGINFO_NAME_TITLE, file.FRM8().diin.diti.titleText.c_str());
	}

	file.Close();

	return TRUE;
}