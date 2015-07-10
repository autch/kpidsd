#include "stdafx.h"
#include "CDSFDecoderKpi.h"
#include "dop.h"
#include "kpi.h"

CDSFDecoderKpi::CDSFDecoderKpi() : file(), srcBuffer(NULL)
{
}

CDSFDecoderKpi::~CDSFDecoderKpi()
{
	Close();
}

void CDSFDecoderKpi::Close()
{
	file.Close();
	if (srcBuffer != NULL)
	{
		delete[] srcBuffer;
		srcBuffer = NULL;
	}
}

BOOL CDSFDecoderKpi::Open(LPSTR szFileName, SOUNDINFO* pInfo)
{
	if (!file.Open(szFileName))
		return FALSE;

	uint32_t dsd_fs = file.FmtHeader()->sampling_frequency;
	uint32_t channels = file.FmtHeader()->channel_num;

	soundinfo.dwChannels = channels;

	switch (dsd_fs) {
	case DSD_FREQ_64FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_64FS;
		break;
	case DSD_FREQ_128FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_128FS;
		break;
	default:
		// 256FS とかはこっちを通す
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
		soundinfo.dwUnitRender = file.FmtHeader()->block_size_per_channel * channels * 3 / 2;	// FIXME: 本当？ 
		break;
	case 32:
	default:
		soundinfo.dwBitsPerSample = 32;
		soundinfo.dwUnitRender = file.FmtHeader()->block_size_per_channel * channels * 2;
		break;
	}

	uint64_t qwSamples = file.FmtHeader()->sample_count;
	qwSamples *= 1000;
	qwSamples /= dsd_fs;
	soundinfo.dwLength = (DWORD)qwSamples;

	memcpy(pInfo, &soundinfo, sizeof soundinfo);

	srcBufferSize = file.FmtHeader()->block_size_per_channel * channels;
	srcBuffer = new BYTE[srcBufferSize];

	Reset();

	return TRUE;

fail_cleanup:
	Close();
	return FALSE;
}

void CDSFDecoderKpi::Reset()
{
	file.Reset();

	last_marker = DOP_MARKER1;
	samplesRendered = 0;
}

DWORD CDSFDecoderKpi::SetPosition(DWORD dwPos)
{
	if (file.File() == INVALID_HANDLE_VALUE)
		return 0;

	uint64_t bytePos = dwPos;
	bytePos *= file.FmtHeader()->sampling_frequency;
	bytePos /= 1000;
	bytePos >>= 3;

	uint64_t blockPos = bytePos / file.FmtHeader()->block_size_per_channel;

	BYTE marker = last_marker;

	Reset();
	file.Seek(blockPos * file.FmtHeader()->block_size_per_channel * file.FmtHeader()->channel_num, NULL, FILE_CURRENT);
	samplesRendered = file.FmtHeader()->block_size_per_channel * 8 * blockPos;

	uint64_t newPos = blockPos;
	newPos *= file.FmtHeader()->block_size_per_channel;
	newPos <<= 3;
	newPos *= 1000;
	newPos /= file.FmtHeader()->sampling_frequency;

	last_marker = marker;

	return (DWORD)newPos;
}

DWORD CDSFDecoderKpi::Render(BYTE* buffer, DWORD dwSize)
{
	DWORD dwBytesRead = 0;
	PBYTE d = buffer, de = buffer + dwSize;
	uint64_t sampleCount = file.FmtHeader()->sample_count;
	DWORD dwBytesPerBlockChannel = file.FmtHeader()->block_size_per_channel;
	int bps = file.FmtHeader()->bits_per_sample;
	DWORD dwBytesRendered;
	uint64_t dataEndPos = file.DataOffset() + file.DataHeader()->size - 12;
	int channels = file.FmtHeader()->channel_num;

	::ZeroMemory(buffer, dwSize);
	while (d < de)
	{
		if (file.Tell() >= dataEndPos) break;

		file.Read(srcBuffer, dwBytesPerBlockChannel * channels, &dwBytesRead);
		if (dwBytesRead < dwBytesPerBlockChannel * channels) break;

		switch (bps)
		{
		case DSF_BPS_LSB:
			dwBytesRendered = decodeLSBFirst(d, de - d);
			break;
		case DSF_BPS_MSB:
			dwBytesRendered = decodeMSBFirst(d, de - d);
			break;
		}
		d += dwBytesRendered;
	}
	return d - buffer;
}

// reverse() の要るやつ
DWORD CDSFDecoderKpi::decodeLSBFirst(PBYTE buffer, DWORD dwSize)
{
	PBYTE d = buffer, de = buffer + dwSize;
	DWORD dwBytesPerBlockChannel = file.FmtHeader()->block_size_per_channel;
	uint64_t sampleCount = file.FmtHeader()->sample_count;
	BYTE marker = last_marker;
	BYTE frame[4] = { 0, 0, 0, 0 };
	DWORD dwBytesToWrite, dwFrameOffset;
	DWORD channels = file.FmtHeader()->channel_num;
	
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

	for (DWORD dwByteOffset = 0;
		dwByteOffset < dwBytesPerBlockChannel && samplesRendered < sampleCount;
		dwByteOffset += DOP_DSD_BYTES_PER_FRAME)
	{
		for (unsigned ch = 0; ch < channels; ch++)
		{
			PBYTE p = srcBuffer + (dwBytesPerBlockChannel * ch) + dwByteOffset;

			frame[3] = marker;
			frame[2] = reverse(*p++);
			frame[1] = reverse(*p++);
			memcpy(d, frame + dwFrameOffset, dwBytesToWrite);
			d += dwBytesToWrite;
		}
		marker ^= 0xff;
		samplesRendered += 16;
	}
	last_marker = marker;
	return d - buffer;
}

// reverse() の要らないやつ
DWORD CDSFDecoderKpi::decodeMSBFirst(PBYTE buffer, DWORD dwSize)
{
	PBYTE d = buffer, de = buffer + dwSize;
	DWORD dwBytesPerBlockChannel = file.FmtHeader()->block_size_per_channel;
	uint64_t sampleCount = file.FmtHeader()->sample_count;
	BYTE marker = last_marker;
	BYTE frame[4] = { 0, 0, 0, 0 };
	DWORD dwBytesToWrite, dwFrameOffset;
	DWORD channels = file.FmtHeader()->channel_num;

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

	for (DWORD dwByteOffset = 0;
		dwByteOffset < dwBytesPerBlockChannel && samplesRendered < sampleCount;
		dwByteOffset += DOP_DSD_BYTES_PER_FRAME)
	{
		for (unsigned ch = 0; ch < channels; ch++)
		{
			PBYTE p = srcBuffer + (dwBytesPerBlockChannel * ch) + dwByteOffset;

			frame[3] = marker;
			frame[2] = *p++;
			frame[1] = *p++;
			memcpy(d, frame + dwFrameOffset, dwBytesToWrite);
			d += dwBytesToWrite;
		}
		marker ^= 0xff;
		samplesRendered += 16;
	}
	last_marker = marker;
	return d - buffer;
}

#include "CID3V2.h"
#include <stdlib.h>

bool setID3V2AsKMPTag(CID3V2Tag& tag, ID3V2_ID frameId, IKmpTagInfo* pInfo, const char* szKMPTagName);

BOOL CDSFDecoderKpi::GetTagInfo(const char *cszFileName, IKmpTagInfo *pInfo)
{
	CDSFFile file;

	if (!file.Open(cszFileName))
		return FALSE;
	
	if (file.Header()->id3v2_pointer == 0)
	{
		file.Close();
		return TRUE;
	}

	if (!file.Seek(file.Header()->id3v2_pointer, NULL, FILE_BEGIN))
		goto fail_cleanup;

	DWORD dwTagSize = (DWORD)(file.FileSize() - file.Header()->id3v2_pointer);
	BYTE* buf = new BYTE[dwTagSize];

	DWORD dwBytesRead = 0;

	file.Read(buf, dwTagSize, &dwBytesRead);
	if (dwBytesRead != dwTagSize) 
	{
		delete[] buf;
		goto fail_cleanup;
	}

	// file はフォーマットチェックとしてしか使わない
	{
		CID3V2Tag tag;

		if (tag.Parse(buf, dwTagSize))
		{
			bool haveDate = false;

			setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TIT2"), pInfo, SZ_KMP_TAGINFO_NAME_TITLE);
			setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TPE1"), pInfo, SZ_KMP_TAGINFO_NAME_ARTIST);
			setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TALB"), pInfo, SZ_KMP_TAGINFO_NAME_ALBUM);
			setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TCON"), pInfo, SZ_KMP_TAGINFO_NAME_GENRE);
			setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TSSE"), pInfo, SZ_KMP_TAGINFO_NAME_COMMENT);
			setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TRCK"), pInfo, SZ_KMP_TAGINFO_NAME_TRACKNUMBER);

			if (tag.Version() == 3)
			{
				std::string datetime;
				CID3V2TextFrame f;

				if ((f = tag.FindTextFrame(MAKE_ID3V2_ID_S("TYER"))).text != NULL)
				{
					datetime.append((const char*)f.text);
					haveDate = true;
				}
				if ((f = tag.FindTextFrame(MAKE_ID3V2_ID_S("TDAT"))).text != NULL)
				{
					char szDate[8];
					_snprintf_s(szDate, sizeof szDate, "-%.2s-%.2s", f.text + 2, f.text);
					datetime.append(szDate);
					haveDate = true;
				}
				if ((f = tag.FindTextFrame(MAKE_ID3V2_ID_S("TIME"))).text != NULL)
				{
					char szTime[8];
					_snprintf_s(szTime, sizeof szTime, " %.2s:%.2s", f.text, f.text + 2);
					datetime.append(szTime);
					haveDate = true;
				}
				if (haveDate)
					pInfo->SetValueA(SZ_KMP_TAGINFO_NAME_DATE, datetime.c_str());
			}
			if (!haveDate && !setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TDRC"), pInfo, SZ_KMP_TAGINFO_NAME_DATE))
				setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TDRL"), pInfo, SZ_KMP_TAGINFO_NAME_DATE);
		}
	}

	delete[] buf;

	file.Close();
	return TRUE;

fail_cleanup:
	file.Close();
	return FALSE;
}

bool setID3V2AsKMPTag(CID3V2Tag& tag, ID3V2_ID frameId, IKmpTagInfo* pInfo, const char* szKMPTagName)
{
	CID3V2TextFrame frame = tag.FindTextFrame(frameId);
	if (frame.text == NULL)
		return false;

	switch (frame.encoding)
	{
	case ID3V2_ENCODING_ISO8859_1:
		pInfo->SetValueA(szKMPTagName, (const char*)frame.text);
		break;
	case ID3V2_ENCODING_UTF16BOM:
		pInfo->SetValueW(szKMPTagName, (const WCHAR*)(frame.text + 2));
		break;
	case ID3V2_ENCODING_UTF8:
		pInfo->SetValueU8(szKMPTagName, (const char*)frame.text);
		break;
	case ID3V2_ENCODING_UTF16BE:
		pInfo->SetValueW(szKMPTagName, (const WCHAR*)frame.text);
		break;
	}
	return true;
}