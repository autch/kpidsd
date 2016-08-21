#include "stdafx.h"
#include "CDSFDecoderKpi.h"
#include "dop.h"
#include "kpi.h"
#include "CKpiFileAdapter.h"

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
	if (srcBuffer != NULL)
	{
		delete[] srcBuffer;
		srcBuffer = NULL;
	}
}

DWORD CDSFDecoderKpi::Open(const KPI_MEDIAINFO* pRequest, IKpiFile* kpiFile, IKpiFolder* folder)
{
	CKpiFileAdapter* pFile = new CKpiFileAdapter(kpiFile);

	if (!file.Open(pFile)) {
		delete pFile;
		return 0;
	}

	uint32_t dsd_fs = file.FmtHeader()->sampling_frequency;
	uint32_t channels = file.FmtHeader()->channel_num;

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
		// 256FS とかはこっちを通す
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
			mInfo.dwUnitSample = file.FmtHeader()->block_size_per_channel;	// FIXME: 本当？ 
			break;
		case 32:
		default:
			mInfo.nBitsPerSample = 32;
			mInfo.dwUnitSample = file.FmtHeader()->block_size_per_channel;
			break;
		}
	}
	else
		mInfo.nBitsPerSample = 32;

	uint64_t qwSamples = file.FmtHeader()->sample_count;
	qwSamples *= 1000 * 10000;
	qwSamples /= dsd_fs;
	mInfo.qwLength = qwSamples;

	srcBufferSize = file.FmtHeader()->block_size_per_channel * channels;
	srcBuffer = new BYTE[srcBufferSize];

	Reset();
	kpiFile->AddRef();
	this->pFile = pFile;

	return mInfo.dwCount;

fail_cleanup:
	Close();
	delete pFile;
	return 0;
}

void CDSFDecoderKpi::Reset()
{
	file.Reset();

	last_marker = DOP_MARKER1;
	samplesRendered = 0;
}

UINT64 WINAPI CDSFDecoderKpi::Seek(UINT64 qwPosSample, DWORD dwFlag)
{
	uint64_t bytePos = qwPosSample;
	bytePos *= 16;
	bytePos >>= 3;

	uint64_t blockPos = bytePos / file.FmtHeader()->block_size_per_channel;

	BYTE marker = last_marker;

	Reset();
	file.Seek(blockPos * file.FmtHeader()->block_size_per_channel * file.FmtHeader()->channel_num, NULL, FILE_CURRENT);
	samplesRendered = file.FmtHeader()->block_size_per_channel * 8 * blockPos;

	uint64_t newPos = blockPos;
	newPos *= file.FmtHeader()->block_size_per_channel;
	newPos <<= 3;
	newPos /= 16;

	last_marker = marker;

	return newPos;
}

DWORD CDSFDecoderKpi::Render(BYTE* buffer, DWORD dwSizeSample)
{
	DWORD dwBytesRead = 0;
	DWORD dwSize = dwSizeSample * (mInfo.dwChannels * (mInfo.nBitsPerSample / 8));
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
	return (d - buffer) / mInfo.dwChannels / (mInfo.nBitsPerSample / 8);
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
#include "kmp_pi.h"
#include <stdlib.h>

bool setID3V2AsKMPTag(CID3V2Tag& tag, ID3V2_ID frameId, IKpiTagInfo* pInfo, const wchar_t* szKMPTagName);


DWORD CDSFDecoderKpi::Select(DWORD dwNumber, const KPI_MEDIAINFO** ppMediaInfo, IKpiTagInfo* pTagInfo)
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
			return 1;

		DWORD dwTagSize = (DWORD)(file.FileSize() - file.Header()->id3v2_pointer);
		BYTE* buf = new BYTE[dwTagSize];

		DWORD dwBytesRead = 0;

		file.Read(buf, dwTagSize, &dwBytesRead);
		if (dwBytesRead != dwTagSize)
		{
			delete[] buf;
			return 1;
		}

		// file はフォーマットチェックとしてしか使わない
		{
			CID3V2Tag tag;

			if (tag.Parse(buf, dwTagSize))
			{
				bool haveDate = false;

				setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TIT2"), pTagInfo, SZ_KMP_NAME_TITLE);
				setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TPE1"), pTagInfo, SZ_KMP_NAME_ARTIST);
				setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TALB"), pTagInfo, SZ_KMP_NAME_ALBUM);
				setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TCON"), pTagInfo, SZ_KMP_NAME_GENRE);
				setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TSSE"), pTagInfo, SZ_KMP_NAME_COMMENT);
				setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TRCK"), pTagInfo, SZ_KMP_NAME_TRACKNUMBER);

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
						pTagInfo->wSetValueA(SZ_KMP_NAME_DATE, -1, datetime.c_str(), -1);
				}
				if (!haveDate && !setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TDRC"), pTagInfo, SZ_KMP_NAME_DATE))
					setID3V2AsKMPTag(tag, MAKE_ID3V2_ID_S("TDRL"), pTagInfo, SZ_KMP_NAME_DATE);
			}
		}

		delete[] buf;
	}

	return 1;
}

bool setID3V2AsKMPTag(CID3V2Tag& tag, ID3V2_ID frameId, IKpiTagInfo* pInfo, const wchar_t* szKMPTagName)
{
	CID3V2TextFrame frame = tag.FindTextFrame(frameId);
	if (frame.text == NULL)
		return false;

	switch (frame.encoding)
	{
	case ID3V2_ENCODING_ISO8859_1:
		pInfo->wSetValueA(szKMPTagName, -1, (const char*)frame.text, -1);
		break;
	case ID3V2_ENCODING_UTF16BOM:
		pInfo->wSetValueW(szKMPTagName, -1, (const WCHAR*)(frame.text + 2), -1);
		break;
	case ID3V2_ENCODING_UTF8:
		pInfo->wSetValueU8(szKMPTagName, -1, (const char*)frame.text, -1);
		break;
	case ID3V2_ENCODING_UTF16BE:
		pInfo->wSetValueW(szKMPTagName, -1, (const WCHAR*)frame.text, -1);
		break;
	}
	return true;
}