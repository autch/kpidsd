
#pragma once

#include "CAbstractKpi.h"
#include "CDSFFile.h"
#include "dsf_types.h"

class CDSFDecoderKpi : public CAbstractKpi
{
private:
	CDSFFile file;

	BYTE* srcBuffer;
	DWORD srcBufferSize;
	
	uint64_t samplesRendered;

	SOUNDINFO soundinfo;
	BYTE last_marker;

	DWORD decodeLSBFirst(PBYTE buffer, DWORD dwSize);
	DWORD decodeMSBFirst(PBYTE buffer, DWORD dwSize);

	// http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv
	inline BYTE reverse(BYTE b) const
	{
		return ((b * 0x0202020202ULL & 0x010884422010ULL) % 1023) & 0xff;
	}

public:
	CDSFDecoderKpi();
	virtual ~CDSFDecoderKpi();

	BOOL Open(LPSTR szFileName, SOUNDINFO* pInfo);
	void Close();
	DWORD SetPosition(DWORD dwPosition);
	DWORD Render(BYTE* buffer, DWORD dwSize);
	void Reset();

	static BOOL GetTagInfo(const char *cszFileName, IKmpTagInfo *pInfo);
};
