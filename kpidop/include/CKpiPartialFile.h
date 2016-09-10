#pragma once

// http://kbmplaybbs.dip.jp/?md=ov&no=5321&pn=5314&ln=1
// IKpiFile の一部分だけを扱う。
// 無圧縮アーカイブなどで使う。
class CKpiPartialFile : public KbKpiUnknownImpl<IKpiFile>
{
private:
	IKpiFile* m_pFile;
	UINT64 m_qwCurrentPos;
	UINT64 m_qwStartOffset;
	UINT64 m_qwEndOffset;
	UINT64 m_qwSize;

public:
	CKpiPartialFile(IKpiFile* pFile,
		UINT64 qwStartOffset,
		UINT64 qwEndOffset)
	{
		if (qwEndOffset < qwStartOffset) {
			qwEndOffset = qwStartOffset;
		}
		m_pFile = pFile;
		m_qwCurrentPos = m_qwStartOffset = qwStartOffset;
		m_qwEndOffset = qwEndOffset;
		m_qwSize = qwEndOffset - qwStartOffset;
		pFile->Seek(qwStartOffset, FILE_BEGIN);
		pFile->AddRef();
	}
	virtual ~CKpiPartialFile(void)
	{
		m_pFile->Release();
	}

	// IKpiFile 派生
	DWORD WINAPI GetFileName(wchar_t *pszName, DWORD dwSize) { pszName[0] = 0; return 0; }
	BOOL WINAPI GetRealFileW(const wchar_t **ppszFileName) { *ppszFileName = NULL; return FALSE; }
	BOOL WINAPI GetRealFileA(const char **ppszFileName) { *ppszFileName = NULL; return FALSE; }
	BOOL WINAPI GetBuffer(const BYTE **ppBuffer, size_t *pstSize) { *ppBuffer = NULL; *pstSize = 0; return FALSE; }
	BOOL WINAPI CreateClone(IKpiFile **ppFile) { *ppFile = NULL; return FALSE; }
	BOOL WINAPI Abort(void) { return FALSE; }
	//↑ここまで未対応で OK

	DWORD WINAPI Read(void *pBuffer, DWORD dwSize)
	{
		if (dwSize + m_qwCurrentPos > m_qwEndOffset) {
			dwSize = (DWORD)(m_qwEndOffset - m_qwCurrentPos);
		}
		dwSize = m_pFile->Read(pBuffer, dwSize);
		m_qwCurrentPos += dwSize;
		return dwSize;
	}

	UINT64 WINAPI Seek(__int64 i64Pos, DWORD dwOrigin)
	{
		switch (dwOrigin) {
		case FILE_BEGIN:
			i64Pos += m_qwStartOffset;
			break;
		case FILE_CURRENT:
			i64Pos += m_qwCurrentPos;
			break;
		case FILE_END:
			i64Pos += m_qwEndOffset;
			break;
		}
		if (i64Pos < (__int64)m_qwStartOffset) {
			i64Pos = m_qwStartOffset;
		}
		else if (i64Pos > (__int64)m_qwEndOffset) {
			i64Pos = m_qwEndOffset;
		}
		m_qwCurrentPos = i64Pos;
		m_pFile->Seek(i64Pos, FILE_BEGIN);
		return m_qwCurrentPos - m_qwStartOffset;
	}

	UINT64 WINAPI GetSize(void)
	{
		return m_qwSize;
	}
};
