#pragma once
#ifndef kpi_implH
#define kpi_implH

#include "kpi.h"
///////////////////////////////////////////////////////////////////////////////
//IKpiXXX の QueryInterface/AddRef/Release
//及び実装が必須ではないメソッドのデフォルトの動作を実装したクラス群
//プラグイン開発時、面倒ならこれらのクラスから派生させれば良いが、必須ではない
//
//ここにないもので IKpiUnknown の派生クラスを実装する場合は
//
//class CKpiUnArc : public KbKpiUnknwonImpl<IKpiUnArc>
//{
//};
//または
//typedef KbKpiUnknwonImpl<IKpiUnArc> CKpiUnArc;
//
//class CKpiPlayer : public KbKpiUnknwonImpl<IKpiPlayer>
//{
//};
//または
//typedef KbKpiUnknwonImpl<IKpiPlayer> CKpiPlayer;
//
//のようにすれば QueryInterface と AddRef/Release だけは自動的に実装される
//
//KbKpiFileImpl 以外は typedef しているだけなので、他のメソッドは全て実装
//する必要がある。
class IKpiDecoder;      //kpi_decoder.h に移動したので前方宣言しておく
class IKpiDecoderModule;//kpi_decoder.h に移動したので前方宣言しておく
///////////////////////////////////////////////////////////////////////////////
template<class T> class KbKpiUnknownImpl : public T
{
protected:
    LONG m_cRef;
    KbKpiUnknownImpl(void):m_cRef(1){}
    virtual ~KbKpiUnknownImpl(void){}
public:
    //IUnknown 派生
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef(void);
    ULONG   STDMETHODCALLTYPE Release(void);
};
///////////////////////////////////////////////////////////////////////////////
template<class T>
HRESULT WINAPI KbKpiUnknownImpl<T>::QueryInterface(REFIID riid,
                                                   void **ppvObject)
{
    if (!ppvObject) {
        return E_INVALIDARG;
    }
    else if(IsEqualGUID(riid, __uuidof(T))){
        *ppvObject = (T*)this;
    }
    else if(IsEqualGUID(riid, __uuidof(IKpiUnknown))){
        *ppvObject = (IKpiUnknown*)this;
    }
    //else if(IsEqualGUID(riid, __uuidof(IUnknown))){
    //    *ppvObject = (IUnknown*)this;
    //}
    //↑typedef IUnknown IKpiUnknown;
    //  IUnknown == IKpiUnknown なので必要なし
    else{
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////
template<class T>
ULONG WINAPI KbKpiUnknownImpl<T>::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}
///////////////////////////////////////////////////////////////////////////////
template<class T>
ULONG WINAPI KbKpiUnknownImpl<T>::Release(void)
{
    ULONG ret = InterlockedDecrement(&m_cRef);
    if(ret == 0){
        delete this;
    }
    return ret;
}
///////////////////////////////////////////////////////////////////////////////
class KbKpiFileImpl : public KbKpiUnknownImpl<IKpiFile>
{//デコーダプラグインは使用するのみ
 //本体(またはプラグインを使用するソフト)が実装
 //アーカイブプラグインでも実装が必要
public:
    //IKpiFile 派生
    DWORD WINAPI GetFileName(wchar_t *pszName, DWORD dwSize){pszName[0] = 0;return 0;}
    BOOL  WINAPI GetRealFileW(const wchar_t **ppszFileName){*ppszFileName=NULL;return FALSE;}
    BOOL  WINAPI GetRealFileA(const char    **ppszFileName){*ppszFileName=NULL;return FALSE;}
    BOOL  WINAPI GetBuffer(const BYTE **ppBuffer, size_t *pstSize){*ppBuffer=NULL;*pstSize=0;return FALSE;}
    BOOL  WINAPI CreateClone(IKpiFile **ppFile){*ppFile=NULL;return FALSE;}
    BOOL  WINAPI Abort(void){return FALSE;}
/*  以下は必ず実装しなければならない
    DWORD  WINAPI Read(void *pBuffer, DWORD dwSize);
    UINT64 WINAPI Seek(INT64 i64Pos, DWORD dwOrigin);
    UINT64 WINAPI GetSize(void);
*/
};
///////////////////////////////////////////////////////////////////////////////
typedef KbKpiUnknownImpl<IKpiFolder> KbKpiFolderImpl;
/*
class KbKpiFolderImpl : public KbKpiUnknownImpl<IKpiFolder>
{//本体(またはプラグインを使用するソフト)が実装
 //デコーダプラグイン、アーカイブプラグインは使用するのみで実装不要
public:
    IKpiFolder 派生
    以下は必ず実装しなければならない
    DWORD WINAPI GetFolderName(wchar_t *pszName, DWORD dwSize);
    DWORD WINAPI EnumFiles(DWORD dwIndex, wchar_t *pszName, DWORD dwSize, DWORD dwLevel);
    BOOL  WINAPI OpenFile(const wchar_t* cszName, IKpiFile **ppFile);
    BOOL  WINAPI OpenFolder(const wchar_t* cszName, IKpiFolder **ppFolder);
};
*/
///////////////////////////////////////////////////////////////////////////////
class KbKpiNullFolder : public KbKpiFolderImpl
{//実質的に何もしない IKpiFolder
 //本体(またはプラグインを使用するソフト)で必要(かも)
public:
    //IKpiFolder 派生
    DWORD WINAPI GetFolderName(wchar_t *pszName, DWORD dwSize){pszName[0] = 0;return 0;}
    DWORD WINAPI EnumFiles(DWORD dwIndex, wchar_t *pszName, DWORD dwSize, DWORD dwLevel){pszName[0] = 0; return 0;}
    BOOL  WINAPI OpenFile(const wchar_t* cszName, IKpiFile **ppFile){*ppFile=NULL;return FALSE;}
    BOOL  WINAPI OpenFolder(const wchar_t* cszName, IKpiFolder **ppFolder){*ppFolder=NULL;return FALSE;}
};
///////////////////////////////////////////////////////////////////////////////
typedef KbKpiUnknownImpl<IKpiDecoder> KbKpiDecoderImpl;
/*
class KbKpiDecoderImpl : public KbKpiUnknownImpl<IKpiDecoder>
{//デコーダプラグインで実装が必要
public:
    //IKpiDecoder 派生
    //以下は必ず実装しなければならない
    DWORD  WINAPI Select(DWORD dwNumber,                    //曲番号
                         const KPI_MEDIAINFO **ppMediaInfo, //may be NULL
                         IKpiTagInfo *pTagInfo,             //may be NULL
                         DWORD dwTagGetFlags);//どのフィールドのタグを取得するかのフラグ
    DWORD  WINAPI Render(BYTE *pBuffer, DWORD dwSizeSample);
    UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag);
    DWORD  WINAPI UpdateConfig(void *pvReserved){return 0;}
    //UpdateConfig は多くの場合実装不要だが、後で実装したくなる
    //かもしれないので敢えてここでは実装しない(必要ならコピペ)
};
*/
///////////////////////////////////////////////////////////////////////////////
typedef KbKpiUnknownImpl<IKpiDecoderModule> KbKpiDecoderModuleImpl;
/*
class KbKpiDecoderModuleImpl : public KbKpiUnknownImpl<IKpiDecoderModule>
{//デコーダプラグインで実装が必要
public:
    //IKpiDecoderModule 派生
    //以下は必ず実装しなければならない
    void  WINAPI GetModuleInfo(const KPI_DECODER_MODULEINFO **ppInfo);
    DWORD WINAPI Open(const KPI_MEDIAINFO *cpRequest,//再生周波数等の要求値が含まれた KPI_MEDIAINFO
                              IKpiFile     *pFile,   //音楽データ
                              IKpiFolder   *pFolder, //音楽データがあるフォルダ
                              IKpiDecoder **ppDecoder);
    BOOL  WINAPI EnumConfig(IKpiConfigEnumerator *pEnumerator){return FALSE;}
    DWORD WINAPI ApplyConfig(const wchar_t *cszSection,//セクション名
                             const wchar_t *cszKey,    //キー名
                             INT64  nValue, //変更後の値(BOOL/INT の場合)
                             double dValue, //変更後の値(FLOAT の場合)
                             const wchar_t *cszValue){//変更後の値(文字列)
        return KPI_CFGRET_OK;
    }
    //EnumConfig/ApplyConfig は多くの場合実装不要だが、後で実装したくなる
    //かもしれないので敢えてここでは実装しない(必要ならコピペ)
};
*/
///////////////////////////////////////////////////////////////////////////////
typedef KbKpiUnknownImpl<IKpiTagInfo> KbKpiTagInfoImpl;
/*
class KbKpiTagInfoImpl : public KbKpiUnknownImpl<IKpiTagInfo>
{//本体が実装
 //デコーダプラグインは使用するのみ。実装不要
 //アーカイブプラグインは使用も実装も不要
public:
    //IKpiTagInfo 派生
    //以下は必ず実装しなければならない
    void WINAPI aSetValueA(const char    *cszNameA,   int nNameSize,  //Name: Ansi
                           const char    *cszValueA,  int nValueSize);//Value:Ansi
    void WINAPI aSetValueW(const char    *cszNameA,   int nNameSize,  //Name: Ansi
                           const wchar_t *cszValueW,  int nValueSize);//Value: Utf16
    void WINAPI aSetValueU8(const char   *cszNameA,   int nNameSize,  //Name: Ansi
                            const char   *cszValueU8, int nValueSize);//Value: Utf8
    //Name=utf16 版
    void WINAPI wSetValueA(const wchar_t  *cszNameW,   int nNameSize,  //Name: utf15
                           const char     *cszValueA,  int nValueSize);//Value; Ansi
    void WINAPI wSetValueW(const wchar_t  *cszNameW,   int nNameSize,  //Name: Utf16
                           const wchar_t  *cszValueW,  int nValueSize);//Value: Utf16
    void WINAPI wSetValueU8(const wchar_t *cszNameW,   int nNameSize,  //Name; utf16
                            const char    *cszValueU8, int nValueSize);//Utf8: utf8
    //Name=utf8 版
    void WINAPI u8SetValueA(const char    *cszNameU8,  int nNameSize,  //Name: utf8
                            const char    *cszValueA,  int nValueSize);//Value: Ansi
    void WINAPI u8SetValueW(const char    *cszNameU8,  int nNameSize,  //Name; utf8
                            const wchar_t *cszValueW,  int nValueSize);//Value: Utf16
    void WINAPI u8SetValueU8(const char   *cszNameU8,  int nNameSize,  //Name: utf8
                             const char   *cszValueU8, int nValueSize);//Value: Utf8
};
*/
#endif

