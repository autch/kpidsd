#include "stdafx.h"
#include "CWSDFile.h"

CWSDFile::CWSDFile() : CLargeFile()
{

}

CWSDFile::~CWSDFile()
{
	CLargeFile::~CLargeFile();
}

void CWSDFile::Close()
{
	CLargeFile::Close();
}

BOOL CWSDFile::Open(LPCSTR szFileName)
{
	if (!CLargeFile::Open(szFileName))
		return FALSE;

	if (!checkHeader())
	{
		Close();
		return FALSE;
	}

	Reset();

	return TRUE;
}

void CWSDFile::Reset()
{
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	Seek(header.dataOffset, NULL, FILE_BEGIN);
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

	if (header.textOffset != 0)
	{
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
