#include "stdafx.h"
#include "CDSFFile.h"

CDSFFile::CDSFFile() : hFile(INVALID_HANDLE_VALUE)
{
	liDataOffset.QuadPart = 0;
}

CDSFFile::~CDSFFile()
{
	Close();
}

void CDSFFile::Close()
{
	if (hFile != INVALID_HANDLE_VALUE) {
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
}

BOOL CDSFFile::Open(LPCSTR szFileName)
{
	hFile = ::CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if (!checkHeader()) {
		Close();
		return FALSE;
	}

	liDataOffset.QuadPart = 0;
	::SetFilePointerEx(hFile, liDataOffset, &liDataOffset, FILE_CURRENT);

	Reset();

	return TRUE;
}

BOOL CDSFFile::checkHeader()
{
	DWORD dwBytesRead;
	const DWORD DSD_ = DSF_SIGNATURE_DSD_, fmt_ = DSF_SIGNATURE_fmt_, data = DSF_SIGNATURE_data;

	{
		if (::ReadFile(hFile, &header, sizeof header, &dwBytesRead, NULL) == FALSE)
			return FALSE;
		if (dwBytesRead != sizeof header)
			return FALSE;
		if (memcmp(&header.DSD_, &DSD_, sizeof header.DSD_) != 0)
			return FALSE;
		if (header.size != DSF_HEADER_SIZE)
			return FALSE;

		::GetFileSizeEx(hFile, &liFileSize);
		if (header.file_size != liFileSize.QuadPart)
			return FALSE;
	}
	{
		if (::ReadFile(hFile, &fmt_header, sizeof fmt_header, &dwBytesRead, NULL) == FALSE)
			return FALSE;
		if (dwBytesRead != sizeof fmt_header)
			return FALSE;
		if (memcmp(&fmt_header.fmt_, &fmt_, sizeof fmt_header.fmt_) != 0)
			return FALSE;
		if (fmt_header.size != DSF_fmt_SIZE)
			return FALSE;
		if (fmt_header.format_version != DSF_FORMAT_VERSION)
			return FALSE;
		if (fmt_header.format_id != DSF_FMTID_RAW)
			return FALSE;

		switch (fmt_header.channel_type) {
		case DSF_CHTYPE_MONO:
		case DSF_CHTYPE_STEREO:
		case DSF_CHTYPE_3CHS:
		case DSF_CHTYPE_QUAD:
		case DSF_CHTYPE_4CHS:
		case DSF_CHTYPE_5CHS:
		case DSF_CHTYPE_5_1CHS:
			break;
		default:
			return FALSE;
		}
		if (fmt_header.bits_per_sample != DSF_BPS_LSB && fmt_header.bits_per_sample != DSF_BPS_MSB)
			return FALSE;
	}
	{
		DSF_data_HEADER data_header;

		if (::ReadFile(hFile, &data_header, sizeof data_header, &dwBytesRead, NULL) == FALSE)
			return FALSE;
		if (dwBytesRead != sizeof data_header)
			return FALSE;
		if (memcmp(&data_header.data, &data, sizeof data_header.data) != 0)
			return FALSE;
	}
	return TRUE;
}

void CDSFFile::Reset()
{
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	::SetFilePointerEx(hFile, liDataOffset, NULL, FILE_BEGIN);
}
