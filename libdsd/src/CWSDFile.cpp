#include "stdafx.h"
#include "CWSDFile.h"

CWSDFile::CWSDFile() : hFile(INVALID_HANDLE_VALUE)
{

}

CWSDFile::~CWSDFile()
{
	Close();
}

void CWSDFile::Close()
{
	if (hFile != INVALID_HANDLE_VALUE) {
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
}

BOOL CWSDFile::Open(LPCSTR szFileName)
{
	hFile = ::CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if (!checkHeader()) {
		Close();
		return FALSE;
	}

	Reset();

	return TRUE;
}

void CWSDFile::Reset()
{
	LARGE_INTEGER liDataOffset;

	if (hFile == INVALID_HANDLE_VALUE)
		return;

	liDataOffset.QuadPart = header.dataOffset;
	::SetFilePointerEx(hFile, liDataOffset, NULL, FILE_BEGIN);
}

BOOL CWSDFile::checkHeader()
{
	DWORD dwBytesRead = 0;

	if (Read(&header, sizeof header, &dwBytesRead) == FALSE)
		return FALSE;
	if (dwBytesRead != sizeof header)
		return FALSE;
	if (memcmp(header.fileID, WSD_FILEID, 4) != 0)
		return FALSE;

	header.fileSize = ntohllX(header.fileSize);
	header.textOffset = ntohl(header.textOffset);
	header.dataOffset = ntohl(header.dataOffset);

	::GetFileSizeEx(hFile, &liFileSize);

	if (header.textOffset > liFileSize.QuadPart 
		|| header.textOffset < sizeof header + sizeof data_spec)
		return FALSE;
	if (header.dataOffset >= liFileSize.QuadPart)
		return FALSE;

	if (Read(&data_spec, sizeof data_spec, &dwBytesRead) == FALSE)
		return FALSE;
	if (dwBytesRead != sizeof data_spec)
		return FALSE;

	data_spec.playbackTime = ntohl(data_spec.playbackTime);
	data_spec.samplingFrequency = ntohl(data_spec.samplingFrequency);
	data_spec.channelAssignment = ntohl(data_spec.channelAssignment);
	data_spec.emphasis = ntohl(data_spec.emphasis);
	data_spec.timeReferenceHi = ntohll(data_spec.timeReferenceLo);	// typo‚Å‚Í‚È‚¢
	data_spec.timeReferenceLo = ntohll(data_spec.timeReferenceHi);	// typo‚Å‚Í‚È‚¢

	if (header.textOffset != 0) {
		Seek(header.textOffset, NULL, FILE_BEGIN);

		if (Read(&text, sizeof text, &dwBytesRead) == FALSE)
			return FALSE;
		if (dwBytesRead != sizeof text)
			return FALSE;
	}
	else
		::ZeroMemory(&text, sizeof text);

	return TRUE;
}
