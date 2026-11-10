#include "stdafx.h"
#include "CWSDFile.h"

CWSDFile::CWSDFile() : CLargeFile()
{

}

CWSDFile::~CWSDFile()
{
}

void CWSDFile::Close()
{
	CLargeFile::Close();
}

BOOL CWSDFile::Open(CAbstractFile* file)
{
	hFile = file;

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
	if (hFile == NULL)
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

	// textOffset が 0 ならテキストはない。あるなら、ヘッダの後ろ、データの前に収まっていなければならない。
	// データはヘッダ (とテキスト) の後ろからファイルの終わりまで
	const uint64_t headerEnd = sizeof header + sizeof data_spec;
	if (header.dataOffset < headerEnd || header.dataOffset >= FileSize())
		return FALSE;
	if (header.textOffset != 0
		&& (header.textOffset < headerEnd || static_cast<uint64_t>(header.textOffset) + sizeof text > header.dataOffset))
		return FALSE;

	if (Read(&data_spec, sizeof data_spec, &dwBytesRead) == FALSE)
		return FALSE;
	if (dwBytesRead != sizeof data_spec)
		return FALSE;

	data_spec.playbackTime = ntohl(data_spec.playbackTime);
	data_spec.samplingFrequency = ntohl(data_spec.samplingFrequency);
	data_spec.channelAssignment = ntohl(data_spec.channelAssignment);
	data_spec.emphasis = ntohl(data_spec.emphasis);
	data_spec.timeReferenceHi = ntohll(data_spec.timeReferenceLo);	// typoではない
	data_spec.timeReferenceLo = ntohll(data_spec.timeReferenceHi);	// typoではない

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
