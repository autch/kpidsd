#include "stdafx.h"
#include "CDSFDecoderKpi.h"
#include "dop.h"

CDSFDecoderKpi::CDSFDecoderKpi() : file(), srcBuffer(NULL)
{
}

CDSFDecoderKpi::~CDSFDecoderKpi()
{
	Close();
}

BOOL CDSFDecoderKpi::Open(LPSTR szFileName, SOUNDINFO* pInfo)
{
	if (!file.Open(szFileName))
		return FALSE;

	soundinfo.dwChannels = file.FmtHeader()->channel_num;

	switch (file.FmtHeader()->sampling_frequency) {
	case DSD_FREQ_64FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_64FS;
		break;
	case DSD_FREQ_128FS:
		soundinfo.dwSamplesPerSec = DOP_FREQ_128FS;
		break;
	default:
		// 256FS とかはこっちを通す
		if (file.FmtHeader()->sampling_frequency % 44100 == 0) {
			soundinfo.dwSamplesPerSec = file.FmtHeader()->sampling_frequency / 16;
		} else
			goto fail_cleanup;
	}
	soundinfo.dwReserved1 = soundinfo.dwReserved2 = 0;
	soundinfo.dwSeekable = 1;

	switch (pInfo->dwBitsPerSample) {
	case 0:
	case 24:
		soundinfo.dwBitsPerSample = 24;
		soundinfo.dwUnitRender = file.FmtHeader()->block_size_per_channel * soundinfo.dwChannels * 3 / 2;	// FIXME: 本当？ 
		break;
	case 32:
		soundinfo.dwBitsPerSample = pInfo->dwBitsPerSample;
		soundinfo.dwUnitRender = file.FmtHeader()->block_size_per_channel * soundinfo.dwChannels * 2;
		break;
	default:
		goto fail_cleanup;
	}

	uint64_t qwSamples = file.FmtHeader()->sample_count;
	qwSamples *= 1000;
	qwSamples /= file.FmtHeader()->sampling_frequency;
	soundinfo.dwLength = (DWORD)qwSamples;

	memcpy(pInfo, &soundinfo, sizeof soundinfo);

	srcBufferSize = file.FmtHeader()->block_size_per_channel * soundinfo.dwChannels;
	srcBuffer = new BYTE[srcBufferSize];

	Reset();

	return TRUE;

fail_cleanup:
	Close();
	return FALSE;
}

void CDSFDecoderKpi::Close()
{
	file.Close();
	if (srcBuffer != NULL) {
		delete[] srcBuffer;
		srcBuffer = NULL;
	}
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
	LARGE_INTEGER li = { 0 };
	DWORD dwBytesRendered;

	::ZeroMemory(buffer, dwSize);
	while (d < de) {
		li.QuadPart = 0;
		file.Seek(li, &li, FILE_CURRENT);
		if (file.Header()->id3v2_pointer > 0 && (uint64_t)li.QuadPart >= file.Header()->id3v2_pointer) return 0;

		file.Read(srcBuffer, dwBytesPerBlockChannel * file.FmtHeader()->channel_num, &dwBytesRead);
		if (dwBytesRead == 0 || dwBytesRead < dwBytesPerBlockChannel) break;

		switch (bps) {
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
	
	if (soundinfo.dwBitsPerSample == 24) {
		dwBytesToWrite = 3;
		dwFrameOffset = 1;
	}
	else {
		dwBytesToWrite = 4;
		dwFrameOffset = 0;
	}

	for (DWORD dwByteOffset = 0;
		dwByteOffset < dwBytesPerBlockChannel && samplesRendered < sampleCount;
		dwByteOffset += DOP_DSD_BYTES_PER_FRAME) {
		for (unsigned ch = 0; ch < channels; ch++) {
			PBYTE p = srcBuffer + (dwBytesPerBlockChannel * ch) + dwByteOffset;

			frame[3] = marker;
			frame[2] = reverse(*p++);
			frame[1] = reverse(*p++);
			memcpy(d, frame + dwFrameOffset, dwBytesToWrite);
			d += dwBytesToWrite;
		}
		marker ^= 0xff;
		samplesRendered++;
		if (samplesRendered >= sampleCount)
			goto success;
	}
success:
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

	if (soundinfo.dwBitsPerSample == 24) {
		dwBytesToWrite = 3;
		dwFrameOffset = 1;
	}
	else {
		dwBytesToWrite = 4;
		dwFrameOffset = 0;
	}

	for (DWORD dwByteOffset = 0;
		dwByteOffset < dwBytesPerBlockChannel && samplesRendered < sampleCount;
		dwByteOffset += DOP_DSD_BYTES_PER_FRAME) {
		for (unsigned ch = 0; ch < channels; ch++) {
			PBYTE p = srcBuffer + (dwBytesPerBlockChannel * ch) + dwByteOffset;

			frame[3] = marker;
			frame[2] = *p++;
			frame[1] = *p++;
			memcpy(d, frame + dwFrameOffset, dwBytesToWrite);
			d += dwBytesToWrite;
		}
		marker ^= 0xff;
		samplesRendered++;
		if (samplesRendered >= sampleCount)
			goto success;
	}
success:
	last_marker = marker;
	return d - buffer;
}

#include "id3tag.h"
#include <stdlib.h>

BOOL SetID3TagAsKMPTag(struct id3_tag* tag, const char* frame_id, IKmpTagInfo* pInfo, const char* kmpTagID);

BOOL CDSFDecoderKpi::GetTagInfo(const char *cszFileName, IKmpTagInfo *pInfo)
{
	CDSFFile file;

	if (!file.Open(cszFileName))
		return FALSE;
	
	if (file.Header()->id3v2_pointer == 0)
		goto fail_cleanup;

	if (!file.Seek(file.Header()->id3v2_pointer, NULL, FILE_BEGIN))
		goto fail_cleanup;

	DWORD dwTagSize = (DWORD)(file.FileSize().QuadPart - file.Header()->id3v2_pointer);
	BYTE* buf = new BYTE[dwTagSize];

	DWORD dwBytesRead = 0;

	file.Read(buf, dwTagSize, &dwBytesRead);
	if (dwBytesRead != dwTagSize) {
		delete[] buf;
		goto fail_cleanup;
	}

	struct id3_tag* tag = id3_tag_parse(buf, dwTagSize);
	if (tag == NULL) {
		delete[] buf;
		goto fail_cleanup;
	}

	SetID3TagAsKMPTag(tag, "TIT2", pInfo, SZ_KMP_TAGINFO_NAME_TITLE);
	SetID3TagAsKMPTag(tag, "TPE1", pInfo, SZ_KMP_TAGINFO_NAME_ARTIST);
	SetID3TagAsKMPTag(tag, "TALB", pInfo, SZ_KMP_TAGINFO_NAME_ALBUM);
	SetID3TagAsKMPTag(tag, "TCON", pInfo, SZ_KMP_TAGINFO_NAME_GENRE);
	SetID3TagAsKMPTag(tag, "TDRC", pInfo, SZ_KMP_TAGINFO_NAME_DATE);
	//SetID3TagAsKMPTag(tag, "COMM", pInfo, SZ_KMP_TAGINFO_NAME_COMMENT);
	SetID3TagAsKMPTag(tag, "TSSE", pInfo, SZ_KMP_TAGINFO_NAME_COMMENT);
	SetID3TagAsKMPTag(tag, "TRCK", pInfo, SZ_KMP_TAGINFO_NAME_TRACKNUMBER);

	id3_tag_delete(tag);
	delete[] buf;

	file.Close();
	return TRUE;

fail_cleanup:
	file.Close();
	return FALSE;
}

BOOL SetID3TagAsKMPTag(struct id3_tag* tag, const char* frame_id, IKmpTagInfo* pInfo, const char* kmpTagID)
{
	struct id3_frame* frame;

	frame = id3_tag_findframe(tag, frame_id, 0);
	if (frame != NULL) {
		id3_utf8_t* utf8 = NULL;

		switch (frame->fields[1].type) {
		case ID3_FIELD_TYPE_STRING:
			utf8 = id3_ucs4_utf8duplicate(id3_field_getstring(&frame->fields[1]));
			break;
		case ID3_FIELD_TYPE_STRINGLIST:
		{
			int nStr = id3_field_getnstrings(&frame->fields[1]);
			if (nStr > 0) {
				utf8 = id3_ucs4_utf8duplicate(id3_field_getstrings(&frame->fields[1], 0));
			}
			break;
		}
		case ID3_FIELD_TYPE_STRINGFULL:
			utf8 = id3_ucs4_utf8duplicate(id3_field_getfullstring(&frame->fields[1]));
			break;
		}
		if (utf8 != NULL) {
			pInfo->SetValueU8(kmpTagID, (const char*)utf8);
			free(utf8);
			return TRUE;
		}
	}
	return FALSE;
}