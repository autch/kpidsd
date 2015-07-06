#include "CAbstractKpi.h"
#include "CDFFFile.h"

#define SAMPLES_PER_BLOCK		(4096)

class CDFFDecoderKpi : public CAbstractKpi
{
private:
	CDFFFile file;
	SOUNDINFO soundinfo;

	BYTE last_marker;

	PBYTE srcBuffer;
	DWORD srcBufferSize;
public:

	CDFFDecoderKpi();
	virtual ~CDFFDecoderKpi();

	BOOL Open(LPSTR szFileName, SOUNDINFO* pInfo);
	void Close();
	DWORD SetPosition(DWORD dwPosition);
	DWORD Render(BYTE* buffer, DWORD dwSize);
	void Reset();

	static BOOL GetTagInfo(const char *cszFileName, IKmpTagInfo *pInfo);

};