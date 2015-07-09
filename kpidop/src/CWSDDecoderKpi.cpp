#include "stdafx.h"
#include "CWSDDecoderKpi.h"
#include "dop.h"
#include <stdlib.h>
#include <string.h>

CWSDDecoderKpi::CWSDDecoderKpi() : file(), srcBuffer(NULL)
{

}

CWSDDecoderKpi::~CWSDDecoderKpi()
{
	Close();
}

void CWSDDecoderKpi::Close()
{
	file.Close();
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

BOOL CWSDDecoderKpi::Open(LPSTR szFileName, SOUNDINFO* pInfo)
{
	if (!file.Open(szFileName))
		return FALSE;

	uint32_t dsd_fs = file.DataSpec()->samplingFrequency;
	uint32_t channels = file.DataSpec()->channels;

	soundinfo.dwChannels = channels;

	switch (dsd_fs) {
	case DSD_FREQ_64FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_64FS;
		break;
	case DSD_FREQ_128FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_128FS;
		break;
	default:
		// 256FS ‚Æ‚©‚Í‚±‚Á‚¿‚ð’Ê‚·
		if (dsd_fs % 44100 == 0)
			soundinfo.dwSamplesPerSec = dsd_fs / 16;
		else
			goto fail_cleanup;
	}
	soundinfo.dwReserved1 = soundinfo.dwReserved2 = 0;
	soundinfo.dwSeekable = 1;

	switch (pInfo->dwBitsPerSample)
	{
	case 0:
	case 24:
		soundinfo.dwBitsPerSample = 24;
		soundinfo.dwUnitRender = 3 * channels * SAMPLES_PER_BLOCK / 2;
		break;
	case 32:
		soundinfo.dwBitsPerSample = pInfo->dwBitsPerSample;
		soundinfo.dwUnitRender = 4 * channels * SAMPLES_PER_BLOCK / 2;
		break;
	default:
		goto fail_cleanup;
	}

	{
		uint64_t samples = file.FileSize() - file.Header()->dataOffset;
		
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

DWORD CWSDDecoderKpi::SetPosition(DWORD dwPos)
{
	if (file.File() == INVALID_HANDLE_VALUE)
		return 0;

	uint64_t bytePos = dwPos;
	bytePos *= file.DataSpec()->samplingFrequency;
	bytePos *= file.DataSpec()->channels;
	bytePos /= 1000;
	bytePos >>= 3;

	BYTE marker = last_marker;

	Reset();
	file.Seek(bytePos, NULL, FILE_CURRENT);

	last_marker = marker;

	// bytePos ‚©‚ç‹‚ß‚½ƒ~ƒŠ•bˆÊ’u‚ÍAƒ‚ƒmƒ‰ƒ‹ 64FS ‚É‚¨‚¢‚Ä‚à dwPos ‚Æ 1ms –¢–ž‚ÌŒë·‚Å‚µ‚©–³‚¢
	return dwPos;
}

DWORD CWSDDecoderKpi::Render(BYTE* buffer, DWORD dwSize)
{
	DWORD dwBytesRead = 0;
	PBYTE d = buffer, de = buffer + dwSize;
	BYTE marker = last_marker;
	BYTE frame[4] = { 0, 0, 0, 0 };
	DWORD dwBytesToWrite, dwFrameOffset;
	int channels = file.DataSpec()->channels;

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

	return d - buffer;
}

void trim(const char* szName, IKmpTagInfo* pInfo, uint8_t* buf, size_t size)
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

	pInfo->SetValueA(szName, tmp);

	delete[] tmp;
}

BOOL CWSDDecoderKpi::GetTagInfo(const char *cszFileName, IKmpTagInfo *pInfo)
{
	CWSDFile file;

	if (!file.Open(cszFileName))
		return FALSE;

	trim(SZ_KMP_TAGINFO_NAME_TITLE, pInfo, file.Text()->title, sizeof file.Text()->title);
	trim(SZ_KMP_TAGINFO_NAME_ARTIST, pInfo, file.Text()->artist, sizeof file.Text()->artist);
	trim(SZ_KMP_TAGINFO_NAME_ALBUM, pInfo, file.Text()->album, sizeof file.Text()->album);
	trim(SZ_KMP_TAGINFO_NAME_GENRE, pInfo, file.Text()->genre, sizeof file.Text()->genre);
	trim(SZ_KMP_TAGINFO_NAME_COMMENT, pInfo, file.Text()->comment, sizeof file.Text()->comment);
	trim(SZ_KMP_TAGINFO_NAME_DATE, pInfo, file.Text()->dateAndTime, sizeof file.Text()->dateAndTime);

	file.Close();
	return TRUE;
}