#pragma once
#ifndef kpi_decoderH
#define kpi_decoderH

#include "kpi.h"
#pragma pack(push, 1) //構造体のアラインメント

///////////////////////////////////////////////////////////////////////////////
/*デコーダプラグイン
  オーディオデータを PCM にデコード

  旧仕様(kmp_pi.h)で出来なかったループに関する細かな制御、1ファイル複数曲に
  きちんと対応。

  IKpiFolder/IKpiTagInfo/IKpiConfig/IKpiUnkProvider は無視
  IKpiFile::GetRealFileW で実ファイル名取得

  である程度のものは作成可能。とりあえず動くものを、いう程度であれば、
  kpi.h はじっくりとは読まなくても OK (ざっくりと読む必要はあるが)

  実ファイル名からファイルを開く場合、開いたファイルを閉じるまで IKpiFile を
  Release してはいけないことに注意。

  デコーダプラグインがエクスポートする関数
  typedef HRESULT (WINAPI *pfn_kpiCreateInstance)(REFIID riid,      //IKpiXXXModule の IID
                                                void **ppvObject, //IKpiXXXModule の戻り値
                                                IKpiUnknown *pUnknown);
  HRESULT WINAPI kpi_CreateInstance(REFIID riid, void **ppvObject, IKpiUnknown *pUnknown);
  riid に IID_IKpiDecoderModule が渡される。
  必要であれば pUnknown から IKpiConfig 等を取得すること。
  kpi_CreateConfig 等のヘルパー関数を呼ぶ場合は kpi.cpp もプロジェクトに追加する。

  *ppvObject にインスタンスを渡す場合は、適切な型にキャストすること。
  例えば、

    if(IsEqualIID(riid, IID_IKpiDecoderModule){
        CKpiXXXDecoderModule *pModule = new CKpiXXXDecoderModule;
        // *ppvObject = pModule; //NG
        *ppvObject = (IKpiDecoderModule*)pModule; //OK
    }

  のようにすること。CKpiXXXDecoderModule が複数のクラスから派生している場合、
  適切なポインタが渡されない可能性がある。
*/
///////////////////////////////////////////////////////////////////////////////
#define KPI_DECODER_MODULE_VERSION 200 

class __declspec(uuid("{EAEF6826-DE5A-4D10-A170-296DB8C6FD53}")) IKpiDecoder;
class __declspec(uuid("{2F02A4B1-F3FA-4725-AE31-54EA8A1EB095}")) IKpiDecoderModule;

#define IID_IKpiDecoder       __uuidof(IKpiDecoder)
#define IID_IKpiDecoderModule __uuidof(IKpiDecoderModule)

///////////////////////////////////////////////////////////////////////////////
struct KPI_DECODER_MODULEINFO
{
    enum{
        TAG_TITLE  = 0x01,   //タイトル取得対応(Media クスプローラでの表示用情報)
        TAG_EXTRA  = 0x01<<1,//詳細(デコード用に開いたときだけ取得可能な情報)対応
        TAG_DECODER_EXCLUSIVE = 0x01<<2,//デコード中にタグ取得不可(未実装)
            //dwMultipleInstance が KPI_MULTINST_ZERO かそれ以外かでも違いがある(後述)
    };
    DWORD cb;             //=sizeof(KPI_DECODER_MODULEINFO)
    DWORD dwModuleVersion;//=KPI_DECODER_MODULE_VERSION
    DWORD dwPluginVersion;//プラグインのバージョン
    DWORD dwMultipleInstance;//KPI_MULTINST_INFINITE/ZERO/ONE/UNIQUE
    GUID  guid;           //プラグイン識別子
    const wchar_t *cszDescription;  //プラグインの名称
    const wchar_t *cszCopyright;    //著作権(複数名いる場合は改行文字可)
    const wchar_t *cszSupportExts;  //対応拡張子('/' で区切る)(例：".wav/.mp3", ".aaa/.bbb")(拡張子も含める)
    const wchar_t *cszMultiSongExts;//対応拡張子のうち1つのファイルに複数曲含む形式
    void  *pvReserved1;//予約(must be NULL)
    void  *pvReserved2;//予約(must be NULL)
    DWORD dwSupportTagInfo; //タグ取得対応(TAG_TITLE/EXTRA/DECODER_EXCLUSIVE の組み合わせ)
    DWORD dwSupportConfig;  //IKpiConfig による設定の読み書きに未対応なら 0
    DWORD dwReserved[4];    //must be 0
/*
    dwMultipleInstance: デコーダインスタンスを同時に生成出来る数
                        (旧仕様の dwReentrant に相当)
      KPI_MULTINST_INFINITE: (メモリが許す限り)デコーダインスタンスを同時に何個でも作成可能(旧仕様のdwReentrant==1)
      KPI_MULTINST_ZERO:     常に別プロセス起動(旧仕様のdwReentrant==0xFFFFFFFF)
      KPI_MULTINST_ONE:      デコーダインスタンスを同時に1個まで作成可能
                             (2個目以降は別プロセス起動)(旧仕様の dwRrentrant == 0)
      KPI_MULTINST_ZERO + 2: デコーダインスタンスを同時に2個まで作成可能
                             (3個目以降は別プロセス起動)
      KPI_MULTINST_ZERO + N: デコーダインスタンスを同時にN個まで作成可能
                             ((N+1)個目以降は別プロセス起動)
      KPI_MULTINST_UNIQUE:   プロセスをまたいでもデコーダインスタンスは常に1個しか作成不可

    通常は KPI_MULTINST_INFINITE/ZERO/ONE のいずれか
    なるべく dwMultipleInstance=KPI_MULTINST_INFINITE となることが望ましい
    dwMultipleInstance=KPI_MULTINST_ZERO とするべきなのは以下のような場合
      ・プラグインが依存するライブラリや DLL が複数インスタンス同時作成に非対応
      ・その DLL が別のプラグインからも使用される可能性がある
        (↑の可能性がほぼない場合は KPI_MULTINST_ONE で良い)
      ・プラグインや依存ライブラリの動作が不安定で、プラグインが原因で起こる
        強制終了やハングアップに本体が巻き込まれないようにしたい場合
        (kbrunkpi.exe が巻き込まれるが、kbrunkpi.exe がハングアップしても本体は
        それを検知する)
      ・デバッグが困難になるので、デバッグモードでは KPI_MULTINST_ONE にすると良い
    dwSupportTagInfo :TAG_TITLE/EXTRA 各フラグの組み合わせ
      0:         タグ取得非対応
      TAG_TITLE: タイトル取得対応(Media クスプローラでの表示用情報)
         タグ取得専用モードでの情報取得に対応していればセットする。
         Open の第1引数(cpRequest) と Select の第2引数(ppMediaInfo) が NULL
         のときがタグ取得専用で開く
         セットされていないときは Media エクスプローラで情報表示時に Open が
         呼ばれない。
      TAG_EXTRA:  詳細(デコード用に開いたときだけ取得可能な情報)対応
         デコード用に開いた時だけ取得可能な情報がある場合にセット。
         Open の第1引数(cpRequest) と Select の第2引数(ppMediaInfo) が共に非NULL
         の場合がデコード用で開く
      TAG_DECODER_EXCLUSIVE: デコード中は別プロセス経由でタグ取得
         ライブラリの仕様上デコード中にタグ取得出来ない場合はセットする。
         セットされているときは別プロセス経由でタグ取得を行う

      タイトル/アーティストの取得は非対応(本体に任せる)だがその他の追加情報の取得
      には対応する場合は TAG_TITLE はセットしないで TAG_EXTRA だけをセットする
      (デコード用以外では Open を呼ばない)

      TAG_DECODER_EXCLUSIVE がセットされている場合、再生用にデコーダを Open して
      いる場合は別プロセス経由でタグ取得を行う(滅多にないと思うが、再生中の曲しか
      タグ取得出来ず、複数インスタンスを同時に作成出来ないようなライブラリに対応
      するためのもの)

      TAG_DECODER_EXCLUSIVE がセットされていて dwMultipleInstance=KPI_MULTINST_ZERO
      の場合はタグ取得を(デコード中でなくても)常に別プロセス経由で行う。

      TAG_DECODER_EXCLUSIVE がセットされていない場合は、同時作成可能なインスタンス数に
      制限があっても、タグ取得は(デコード中であっても)常に本体プロセスから行う。

      dwSupportTagInfo=0 のときは Media エクスプローラでのタイトル取得にプラグインは
      使用されない(Open が呼ばれない)
      (dwSupportTagInfo&TAG_TITLE) !=0 のときは Media エクスプローラでのタイトル
      取得時に
      ・Open の第1引数(cpRequest)に NULL
      ・Select の第2引数(ppMediaInfo)に NULL、第4引数の dwTagGetFlags に必要な情報
        だけをセット(現在の実装では KPI_TAGGET_FLAG_TITLE|KPI_TAGGET_FLAG_ARTIST)

      を渡して呼び出される。このとき、Select の第4引数(dwTagGetFlags) で指定して
      いない情報はなるべく通知しないこと(通知しても構わないが無視される)

    cszSupportExts: 対応拡張子
      '/' で区切り、拡張子 . も含める
      ".mp1/.mp2/.mp3" のようにする
    cszMultiSongExts: 対応拡張子のうち 1つのファイルに複数の曲を含む可能性があるもの
      cszSupportExts の方にも含めていないと認識されない
      cszSupportExts = ".aaa/.bbb;
      cszMultiSongExts = ".aaa/.ccc";
      例えば↑のような場合、.aaa と .bbb が対応拡張子と見なされ、そのうち拡張子 .aaa は
      1つのファイルに複数曲含む形式として認識されるが、拡張子 .ccc は無視される
      他のプラグインが拡張子 .ccc に対応していても影響しない
*/
};
///////////////////////////////////////////////////////////////////////////////
struct KPI_MEDIAINFO
{
    enum{
        SEEK_DISABLE        = 0,    //シーク不可
        SEEK_FLAGS_SAMPLE   = 0x01, //サンプル単位の高精度なシークに対応
        SEEK_FLAGS_ACCURATE = 0x02, //歌詞との同期に支障がない程度に正確なシーク対応
        SEEK_FLAGS_ROUGH    = 0x04, //精度は悪いがシーク対応
        FORMAT_PCM = 0,    //通常の PCM
        FORMAT_DOP = 1,    //最終出力段までデータの加工をしない(リプレイゲインやフェードアウトの処理もしない)
        FORMAT_PLAYER = 2  //演奏専用(IKpiPlayer 用)
    };
    DWORD  cb;             //=sizeof(KPI_MEDIAINFO)
    DWORD  dwNumber;       //曲番号(1ベース)、曲番号0は存在しない(エラーとして扱われる)
                           //Select で選曲した番号にすること
    DWORD  dwCount;        //曲の数、曲数 0 は存在しない(エラーとして扱われる)
    DWORD  dwFormatType;   //FORMAT_PCM or FORMAT_DOP(DOP の場合、最終出力段までデータの加工をしないことが保証される)
    DWORD  dwSampleRate;   //再生周波数(44100, 48000 etc...)
    INT32  nBitsPerSample; //量子化ビット数(8bit, 16bit, 24bit, 32bit, -32bit, -64bit)
                           //float の場合、値を負にする(32bit float => -32, 64bit float => -64)
    DWORD  dwChannels;     //チャネル数(1 or 2)
    DWORD  dwSpeakerConfig;//0(マルチチャンネル対応のために予約)
    UINT64 qwLength;       //１ループ目(イントロ＋ループ１回)の曲の長さ(単位は100ナノ秒)
    UINT64 qwLoop;         //２ループ目以降の１ループの長さ(単位は100ナノ秒)
    UINT64 qwFadeOut;      //フェードアウト時間(単位は100ナノ秒)(フェードアウトしない場合は 0, 本体の設定に従う場合は-1)
                           //(0 でも -1 でもない場合はデータの推奨値)
    DWORD  dwLoopCount;    //ループ回数(ループ曲でない or 本体の設定に従う場合は0、0 以外の場合はデータの推奨値)
    DWORD  dwUnitSample;   //Render の第2引数(dwSizeSample) に渡すべき数(いくつでも良い場合は 0)
                           //なるべく 0 にすること(実装が面倒なら 0 以外でも OK)
                           //16384 より大きな値にしてはいけない(その場合は頑張って実装すること)
    DWORD  dwSeekableFlags;//Seek への対応フラグ
           //シークに対応していない場合は 0
           //サンプル単位の正確なシークに対応する場合は SEEK_FLAGS_SAMPLE をセット
           //歌詞同期可能な精度のシークに対応する場合は SEEK_FLAGS_ACCURATE をセット
           //精度は悪いがシークに対応する場合は SEEK_FLAGS_ROUGH をセット
           //シークの精度: SAMPLE > ACCURATE > ROUGH
           //シークの速度: ROUGH > ACCURATE > SAMPLE
           //(全部対応する場合は dwSeekableFlag = SEEK_FLAGS_SAMPLE | SEEK_FLAGS_ACCURATE | SEEK_FLAGS_ROUGH となる)
    DWORD  dwVideoWidth;   //映像の横幅(100%時)(映像情報を持たない場合は 0)
    DWORD  dwVideoHeight;  //映像の高さ(100%時)(映像情報を持たない場合は 0)
    DWORD  dwReserved[6];  //予約(0 にすること)
/*
    ・再生周波数、ビット数、チャネル数、ループ回数、フェードアウト時間要求
    IKpiDecoderModule::Open の cpRequest の各値が 0 以外の場合は、可能なら
    その形式で、不可能ならそれに最も近い形式でファイルを開くこと。
    値が 0 の場合はプラグインのデフォルト値で開くこと

    qwLength, qwLoop, qwFadeOut の時間の単位は 100ナノ秒=(1/10000ミリ秒)
    例えば qwLength が 1秒 なら 1秒=1000ms=1000*10000 となる

    ループ１回あたりの演奏時間がわからず、無限ループする場合(Render の戻り値が
    いつまで経っても dwUnitSample より小さくならない可能性がある場合)は
    qwLoop == (UINT64)-1

    演奏時間がわかっていてループしない曲の場合(最も一般的なケース):
        qwLength == 曲の長さ, qwLoop == 0
        このとき qwFadeOut と dwLoopCount の値は無視される

    演奏時間(イントロ+ループ1回)とループ時間がともに分かる曲の場合：
        実際の演奏時間 = qwLength + (dwLoopCount-1)*qwLoop + qwFadeOut
        ただし、
            dwLoopCount == 0 の場合、dwLoopCount は本体の設定値に従う
            qwFadeOut == -1 の場合、qwFadeOut は本体の設定値に従う

    演奏時間はわかるが、ループ時間が分からない(無限ループするかもしれない)場合
        qwLength == 演奏時間(=無音検出を開始する時間),
        qwLoop == -1
        qwFadeOut == 0 or -1 or (データから得た値)
        dwLoopCount == 無視
        このとき、
        ・プレイヤーの設定が「単曲リピート」以外の再生モードの場合は
          qwLength 時間だけ再生後、qwFadeOut だけフェードアウトして演奏を打ち切る
          実際の演奏時間 = qwLength + qwFadeOut
        ・演奏モードが「単曲リピート」の場合は qwLength 時間だけ再生後、
          無音が検出されるまで無限ループ再生する

    演奏時間はわからないが、いつかは演奏終了する場合
    (無音検出による演奏の打ち切りが不要な場合):
        qwLength == -1, qwLoop == 0, qwFadeOut == 0(無視)
        この場合、プレイヤーが表示する曲の長さは ???? となる

    演奏時間がわからず、いつ演奏終了するかもわからない場合：
    (無音検出による演奏打ち切りが必要な場合):
        qwLength == -1, qwLoop == -1
        このとき、
        ・プレイヤーが表示する曲の長さは
          デフォルトの曲の長さ(DefaultLength) + デフォルトのフェードアウト時間(DefaultFade)になる
        ・プレイヤーの演奏モードが「単曲リピート」でない場合は DefaultLength だけ
          再生後、DefaultFade 時間かけてフェードアウトして演奏を打ち切る
        ・演奏モードが「単曲リピート」の場合は DefaultLength だけ再生後、
          無音が検出されるまで無限ループ再生する

    最終的な曲の長さ(qwTotalLength)は
    1. qwTotalLength = qwLength + (dwLoopCount-1)*qwLoop + qwFadeOut (100ns単位)
    2. ↑をサンプル単位に変換(小数点以下切り捨て)
       qwTotalSample = kpi_100nsToSample(qwTotalLength, dwSampleRate);
    3. ↑を100ns 単位に変換(小数点以下切り捨て)
       qwTotalLength = kpi_SampleTo100ns(qwTotalSample, dwSampleRate);
       この時点で 1. とは異なった値になるが、本当の演奏時間と同じになる
    4. 表示は↑をミリ秒単位に変換(小数点以下四捨五入)
       qwTotalLengthMs = (qwTotalLength + 5000) / 10000;
    のような手順で算出する。

    演奏時間のプレイリストへの保存もこの手順でミリ秒単位で算出した値を記録する。
    したがって、ライブラリが返す曲の長さがサンプル単位の場合 qwLength の算出時に
    小数点以下切り捨て/四捨五入のどちらを選んでも曲の長さは同じとなる。

    ループ曲(qwLoop != 0) の演奏終了の処理は本体が行うため、ループ曲の場合は
    Render の戻り値を常に第2引数(dwSizeSample)と同じとなるようにすること。
    デコードライブラリの仕様上それが不可能な場合は、
    IKpiDecoderModule::Open の第1引数(cpRequest)の dwLoopCount/qwFadeOut
    の値に従った長さだけ再生後、プラグイン側で演奏終了の処理を行うこと
    その場合、qwLoop の値は 0 にしなければならない(0以外だと本体がループ曲と
    判断して演奏終了処理を行おうとするため)
*/
};
///////////////////////////////////////////////////////////////////////////////
class IKpiDecoder : public IKpiUnknown
{//オーディオデータを PCM にデコード
 //プラグイン実装の負担を軽減するため、いくつかのメソッドは呼び出しの順序等が
 //決まっている。「XXX 前(後)に XXX した結果は未定義」と書いてある場合、その
 //ような呼び出しがされないことをプラグインは想定して良い。
 //(Select しないで Render や Seek が呼ばれる等)
public:
    virtual DWORD  WINAPI Select(DWORD dwNumber,                    //曲番号
                                 const KPI_MEDIAINFO **ppMediaInfo, //may be NULL
                                 IKpiTagInfo *pTagInfo,             //may be NULL
                                 DWORD dwTagGetFlags)=0;//どのフィールドのタグを取得するかのフラグ
    virtual DWORD  WINAPI Render(BYTE *pBuffer, DWORD dwSizeSample)=0;
    virtual UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag)=0;
    virtual DWORD  WINAPI UpdateConfig(void *pvReserved)=0; //設定が変更されたら呼ばれる
/*
    DWORD WINAPI Select(DWORD dwNumber,
                        const KPI_MEDIAINFO **ppMediaInfo,//may be NULL
                        IKpiTagInfo *pTagInfo,            //may be NULL
                        DWORD dwTagGetFlags)=0;//どのフィールドのタグを取得するかのフラグ
      選曲する(dwNumber=曲番号 は 1 ベース、曲番号0 は存在しない)
      KbMedia Player は Render/Seek の前に必ず Select を呼び出す。

      1ファイル1曲の場合は dwNumber は常に 1 が渡される。
      1ファイル複数曲の場合は dwNumber に 0 が渡されることがある。この場合は
      オープンしているデータ全体を1つの曲またはアルバムと見なしてのタグ取得を
      行う。Media エクスプローラのリスト部分に「ファイル名::曲番号」のように
      表示されず、単に「ファイル名」が表示されているときの情報に相当する。
      (「タイトル」がアルバムやゲーム名になるようなイメージ)

      dwNumber=0 の場合はデコードは行わないため、Render/Seek は呼ばれないことを
      想定して良い。

      タグ取得に対応する場合はここでタグ取得を行う。ID3v1/v2 や ApeTag 等、
      本体が既知のタグ形式であればそのルーチンを利用することが出来る。
      IKpiTagInfo(kpi.h) も合わせて参照すること。

      IKpiDecoderModule::Open の cpRequest に NULL が渡された場合、タグ取得用
      に開くことを意味し、ppMediaInfo も NULL が渡される。
      ppMediaInfo==NULL のとき、タグ情報だけを返す(引数の NULL チェックが必要)
      この場合、実際に曲が切り替わるかどうかは未定義(Render/Seek されないこと
      を想定して良い)

      主に Media エクスプローラ上でのタイトル取得時に NULL が渡される。

      dwTagGetFlags は取得するタグのフィールド。KPI_TAGGET_FLAG_TITLE 等が
      セットされる。セットされない情報は省略して良い。(通知しても構わないが、
      本体によって無視される)

      デコード用に開く(cpRequest!=NULL)場合は dwTagGetFlags=KPI_TAGGET_FLAG_ALL
      となり、タグ取得用に開く(cpRequest==NULL)場合は dwTagGetFlags に最小限の
      フラグだけが与えられる。

      pTagInfo == NULL のとき、タグ情報の取得を省略すること。
      必ず pTagInfo の NULL チェックを行うこと。
      滅多になくて気づきにくいため注意すること。
      CueSheet に登録された曲の再生時に pTagInfo==NULL が渡される。

      ppMediaInfo!=NULL のとき、*ppMediaInfo に曲の長さや再生周波数等の情報を
      入れる。*ppMediaInfo の寿命は次に IKpiDecoder のメソッドが呼ばれるまで。
      切り替え直後の再生位置は未定義(本体は Render 前に必ず Seek を呼び出す)
      切り替えに成功したら切り替え後の曲番号を、失敗したら 0 を返し、*ppMediaInfo
      に NULL を入れる。

      Select に失敗後に Render/Seek した結果は未定義。(呼ばれないことを想定して良い)
      失敗しても、他の曲番号での Select は可能なら成功すること。

    DWORD WINAPI Render(BYTE *pBuffer, DWORD dwSizeSample);
      PCM にデコードした結果を pBuffer に入れる。
      実際にデコードしたサンプル数を返す。
      演奏が終了した場合は dwSizeSample より小さい値を返す。
      dwSizeSample は pBuffer のサイズ(サンプル単位)
      dwSizeSample はサンプル単位なので、実際のバッファサイズ(dwBufferSize)は
      dwBufferSize = dwSizeSample * (dwChannels * ((abs(nBitsPerSample)/8)) となることに注意。
      KPI_MEDIAINFO::dwUnitSample != 0 の場合は、dwSizeSample には KPI_MEDIAINFO::dwUnitSample が渡される
      KPI_MEDIAINFO::dwUnitSample == 0 の場合は、dwSizeSample には任意の値が渡される
      Select の呼び出し前 or 失敗後の Render の結果は未定義。
      Select 直後で Seek 前の Render の結果は未定義。

    UINT64 WINAPI Seek(UINT64 qwPosSample, DWORD dwFlag);
      qwPosSample はシーク先位置で、単位はサンプル単位。
      実際のシーク先位置(サンプル単位)を返す
      実際のシーク先位置と qwPosSample が完全に一致する必要はないが、
      qwPosSample よりも後方にシークされることは推奨しない。
      (CueSheet を再生時に支障をきたす可能性があるため)
      dwFlag はシークの精度で、精度によってシークの処理時間が著しく変化する場合は
      dwFlag の値によって処理を区別すること。
        0: プラグインのデフォルトの精度でシーク
        SEEK_FLAGS_SAMPLE：  サンプル単位の精度でシーク(未対応の場合は先頭に戻す)
        SEEK_FLAGS_ACCURATE：歌詞同期に支障がない精度でシーク(SAMPLE精度も含めて未対応の場合は先頭に戻す)
        SEEK_FLAGS_ROUGH：   速度優先でシーク(ROUGH より高い精度も含めて未対応の場合は先頭に戻す)

      シークに対応しない場合は先頭に戻して 0 を返す。
      歌詞がない曲を再生時は SEEK_FLAGS_ROUGH
      歌詞が存在する曲を再生時は SEEK_FLAGS_ACCURATE
      CueSheet 再生時は SEEK_FLAGS_SAMPLE を指定してシークする。
      指定した以上の精度でシークすること。(不可能なら先頭に戻すこと)
      (ROUGH 指定時に SAMPLE 精度は OK だが、SAMPLE 指定時に ROUGH 精度は NG)

      Select の呼び出し前 or 失敗後の Seek の結果は未定義・

      dwFlag は実は本体が未だに未実装(汗；
      常に SEEK_FLAGS_SAMPLE しか渡しておらず、公開中のプラグインのほぼ全てが
      dwFlag を見ていない。

    DWORD WINAPI UpdateConfig(void *pvReserved);
      IKpiDecoderModule::ApplyConfig の呼び出しで KPI_CFGRET_NOTIFY が返された
      場合のみ、本体によってデコーダのインスタンスごとに呼ばれる。
      呼ばれた時点で設定は既に変更済みであり、どの設定が変更されたかを知ること
      は出来ない。
      pvReserved は将来の予約(NULL が渡される)
      戻り値の意味は将来の予約 => 0 を返すこと。
      IKpiDecoderModule::ApplyConfig も合わせて参照すること。
*/
};
///////////////////////////////////////////////////////////////////////////////
class IKpiDecoderModule : public IKpiUnknown
{
public:
    virtual void  WINAPI GetModuleInfo(const KPI_DECODER_MODULEINFO **ppInfo)=0;
    virtual DWORD WINAPI Open(const KPI_MEDIAINFO *cpRequest,//再生周波数等の要求値が含まれた KPI_MEDIAINFO
                              IKpiFile     *pFile,   //音楽データ
                              IKpiFolder   *pFolder, //音楽データがあるフォルダ
                              IKpiDecoder **ppDecoder)=0;
    virtual BOOL  WINAPI EnumConfig(IKpiConfigEnumerator *pEnumerator)=0;
    virtual DWORD WINAPI ApplyConfig(const wchar_t *cszSection,//セクション名
                                     const wchar_t *cszKey,    //キー名
                                     INT64  nValue, //変更後の値(BOOL/INT の場合)
                                     double dValue, //変更後の値(FLOAT の場合)
                                     const wchar_t *cszValue)=0;//変更後の値(文字列)
/*
    void  WINAPI GetModuleInfo(const KPI_DECODER_MODULEINFO **ppInfo);
      KPI_DECODER_MODULEINFO を *ppInfo に設定して返す
      KPI_DECODER_MODULEINFO は旧プラグインの KMPMODULE に相当する
      次の IKpiDecoderModule のメソッド((GetModuleInfo/Open/EnumConfig/ApplyConfig のいずれか)
      が呼び出された時点で *ppInfo の内容が破棄されて良い
      例えば ApplyConfig 呼び出し後に対応拡張子が変化した場合、変化した時点で
      *ppInfo の内容が書き変わって良い
      GetModuleInfo は ApplyConfig 呼び出し後に本体から再度呼ばれる

      IKpiConfig に対応する場合、kpi_CreateConfig による IKpiConfig のインスタンス
      取得は、遅くとも kpi_CreateInstance 内で実行すること。
      kpi_CreateInstance から戻った段階で未だ kpi_CreateConfig が呼ばれていない
      場合、設定の保存を本体に管理させることは出来ない。
      (EnumConfig/ApplyConfig は有効だがバイナリ以外の型は扱えない)

    DWORD WINAPI Open(const KPI_MEDIAINFO *cpRequest,//再生周波数等の要求値が含まれた KPI_MEDIAINFO
                              IKpiFile     *pFile,   //音楽データ
                              IKpiFolder   *pFolder, //音楽データがあるフォルダ
                              IKpiDecoder **ppDecoder);
      pFile/pFolder から曲を開く。
      成功したら IKpiDecoder のインスタンスを *ppDecoder に設定し、含まれている
      曲の数を返す。(エラーの場合は *ppDecoder に NULL を設定して 0 を返す)

      実ファイル名から開くメソッドは提供されない。
      実ファイル名から開きたい場合は IKpiFile::GetRealFileW/A を呼び出す。

      cpRequest : 以下の各値が 0 以外だった場合は要求値。可能なら要求値に最も
                  近い形で応じる。(要求を無視しても構わない)
      ・dwSampleRate   : 再生周波数
      ・nBitsPerSample : ビット数(通常は -64 が渡される)
      ・qwFadeOut      : フェードアウト時間
      ・dwLoopCount    : ループ回数
      dwSampleRate は無理して応じる必要はない。
      nBitsPerSample はデコーダが対応する中で最も高い演算精度を返すこと。
      16bit の精度しかないのに -64 に対応する必要はない。
      ループ曲に対応するが、デコードライブラリの仕様上演奏終了の処理を本体に
      制御させることが出来ない場合は dwLoopCount/qwFadeOut の値から演奏時間を
      算出し、プラグイン側で演奏終了の処理を行うこと。

      cpRequest == NULL の場合はタグ取得用に開く。
      (主に Media エクスプローラでのタイトル表示用)
      IKpiDecoder::Select の ppMediaInfo にも NULL が渡されるようになる。
      この場合はファイルオープンの処理を最小限に抑えること。
      (データの有効性のチェックを省略しても良い)
      IKpiTagInfo への情報通知もタイトルとアーティストだけで良い。
      (タイトル/アーティスト以外の情報を通知しても構わないが無視される)

      KPI_DECODER_MODULEINFO::dwSupportTagInfo==0 の場合は Media エクスプローラ
      での情報取得時に Open を呼ばない。タイトル取得は本体に任せて追加情報だけを
      取得する場合は dwSupportTagInfo を 0 にすることで Media エクスプローラの
      パフォーマンス低下を抑えることが出来る。

    BOOL WINAPI EnumConfig(IKpiConfigEnumerator *pEnumerator);
      設定項目を列挙する。
      列挙された設定項目を元に本体が設定画面を自動生成する。
      設定項目がない場合は FALSE を、ある場合は TRUE を返す。
      KPI_DECODER_MODULEINFO::dwSupportConfig=0 の場合は呼び出されない。
      KPI_CFG_SECTION/KEY 構造体及び IKpiConfigEnumerator も合わせて参照。
      kpi_CreateInstance から戻るまでに kpi_CreateConfig が呼ばれない場合、
      列挙しても設定が保存されない。(バイナリの型は例外)

    DWORD WINAPI ApplyConfig(const wchar_t *cszSection,//セクション名
                             const wchar_t *cszKey,    //キー名
                             INT64  nValue, //変更後の値(BOOL/INT の場合)
                             double dValue, //変更後の値(FLOAT の場合)
                             const wchar_t *cszValue)=0;//変更後の値(文字列)
      cszSection と cszKey に該当する設定の変更を「適用」する直前に呼ばれる。
      呼ばれた時点ではまだ m_pConfig は変更されていない。
      ここで m_pConfig とは kpi_CreateConfig で取得した IKpiConfig の
      インスタンスとする。

      kpi_CreateInstance から戻るまでに kpi_CreateConfig が呼ばれない場合、
      バイナリ以外の型では ApplyConfig が呼ばれない。(設定の保存場所を決定する
      ことが出来ないため。ApplyConfig 内でkpi_CreateConfig を呼べば良いと考える
      かもしれないが、設定画面上で現在の設定値を確認することが出来ない)

      設定を「適用」するのはプラグイン自身である。SetXXX しないまま ApplyConfig
      から戻った場合、設定は何も変わらないことに注意。

      nValue/dValue/cszValue に渡される値は EnumConfig で列挙された情報を元に
      した範囲内に収まっていることが保証される。
      EnumConfig では表現出来ないような範囲を持つ場合はここで nValue 等を検証
      し、修正の必要があればプラグイン側で SetXXX を呼び出す。

      本体側での範囲チェックで十分で、nValue/dValue/cszValue の検証が不要な
      場合は、渡された cszValue をそのまま SetStr すれば良い。

      m_pConfig->SetStr(cszSection, cszKey, cszValue);//cszValue に変更がない場合

      cszKey に空文字列(後述)が渡された場合もそのまま SetStr して問題ない。
      (本体によって無視される)

      渡された値の修正が必要になるのは、例えば以下のような場合である。

      ・文字列系で大文字を小文字に変換したい
      ・最小値～最大値の範囲内であっても不正となるような値が存在する
      ・値の正当性は複数のキーの組み合わせで決まる

      検証の結果、値に変更がある場合は、該当キーの型に適合した Set 系メソッドを
      呼び出す。

      m_pConfig->SetInt(cszSection, cszKey, nValue);//BOOL/INT
      m_pConfig->SetFloat(cszSection, cszKey, dValue);//FLOAT
      m_pConfig->SetStr(cszSection, cszKey, cszValue);//BOOL/INT/FLOAT/BIN 以外

      必要であればこの関数内で該当セクション/キーに関するプラグイン独自の
      設定画面を出すことが出来る(通常は KPI_CFG_TYPE_BIN で必要)

      KPI_CFG_TYPE_BIN で本体に設定の保存を管理させる場合には独自の設定画面を
      「適用」したときに SetBin を呼び出せば良い。
      独自の設定画面を表示する直前に GetBin すれば、現時点で本体が保持している
      設定データのバイナリ値を取得することが出来る。

      ApplyConfig はセクション単位で呼ばれる。実際に呼び出すのは、設定画面上で
      値が変更されたキーのみであるため、呼び出されたら値が変わっていると考えて
      差し支えない。

      本体は同一セクション内で、値が変更されたキーのみを EnumConfig で列挙された
      順に呼び出す。変更された全てのキーに対して ApplyConfig を呼び出した後、
      最後に cszKey に空文字列が渡される。

      例えば、cszSection1 に cszKey1, cszKey2, cszKey3 がこの順に EnumConfig
      されているとして cszKey1 と cszKey3 が変更された状態で「適用」ボタンが
      押されたとすると

      ApplyConfig(cszSection1, cszKey1, ...);
      //ApplyConfig(cszSection1, cszKey2, ...);//変更されていないので呼ばれない
      ApplyConfig(cszSection1, cszKey3, ...);
      ApplyConfig(cszSection1, L"", ...);//第3引数以降は無視すること。
      //ApplyConfitg(cszSection2, ...);//設定の適用はセクション単位なのでここでは呼ばれない

      のような順で呼び出す。

      ApplyConfig 内では、cszSection/cszKey 以外の任意のセクション・キー
      (EnumConfig で列挙されないセクション/キーも含む)に対して SetXXX/GetXXX
      することが出来る。

      もし同一セクション内の複数のキーの組み合わせで最終的な正しい値が確定する
      ような場合は、cszKey に空文字列が渡された時点で他のキーと合わせて SetXXX
      をすれば良い。値の一貫性が問題になる場合は EnumConfig では列挙されない別
      のキーに保存するという方法もある。例えば設定画面上では 3 種のキーを BOOL
      のように使うがその保存値は別のキー(設定画面上には非表示)にするなど。

      SetXXX が呼ばれない場合、設定の変更は適用されないことに注意(プラグインが
      独自に設定の保存管理を行う場合は除く)

      戻り値は設定の変更が反映されるタイミングである。以下の値を返すこと

      KPI_CFGRET_OK: 設定は即座に反映される(IKpiXXX::UpdateConfig() は呼ばれない)
      KPI_CFGRET_NOTIFY: 各種インスタンスへの通知が必要
                         (IKpiXXX::UpdateConfig()が呼ばれる)
      KPI_CFGRET_RELOAD_DATA: データの再ロードが必要
                              (次回 IKpiXXXModule::Open 後から反映される)
      KPI_CFGRET_RELOAD_KPI:  プラグインの再ロードが必要
      KPI_CFGRET_REBOOT_PLAYER:プレイヤーの再起動が必要(特別な事情がない限り
                               RELOAD_KPI で足りる筈だが)
      KPI_CFGRET_REBOOT_OS:    OS の再起動が必要
      KPI_CFGRET_UNKNOWN:   不明(外部プログラムを呼び出す等、設定関係の依存
                            コードの詳細が分からない等)

      KPI_CFGRET_NOTIFY を返した場合は、IKpiXXXModule::Open で獲得した
      各インスタンス(IKpiXXX)に対して本体が UpdateConfig を呼び出す。
      正確には ApplyConfig の都度ではなく、最後に cszKey に空文字列を渡して
      呼び出した後に呼び出す。cszKey に空文字列を渡したときの戻り値だけから判断
      するのではなく、最後の ApplyConfig の呼び出しまでに一度でも CFGRET_NOTIFY
      が戻れば呼び出す。

      UpdateConfig に対応することで、デコード中にリアルタイムに設定変更を反映
      させることが出来るが、反映されるまでにかかる時間は本体の「先読みバッファ」
      の設定による。(既定値は 5 秒のため、5 秒程度かかってしまう)

      ApplyConfig が呼ばれるのとは別プロセスで IKpiXXX が動作中の場合、
      そちらの IKpiXXX に対しても UpdateConfig が呼び出される。

      この場合、呼び出された IKpiXXX 内では ApplyConfig が呼ばれないことになる
      が、kpi_CreateConfig で取得した IKpiConfig は全てのプロセス間で設定が共有
      されている。

      そのため IKpiXXX::UpdateConfig に対応する場合は IKpiXXX の各インスタンス
      に対しても m_pConfig を渡しておき、設定の取得が必要な場合は m_pConfig から
      取得するようにしなければならない。

      ApplyConfig 時にプラグイン内部で設定を変更、その設定値を IKpiXXX が参照
      するような方式で実装すると、ApplyConfig を呼び出したのとは別プロセスで
      IKpiXXX が動作していた場合に反映されないことになる。

    IKpiDecoderModule のインスタンス作成と破棄はプラグインがロードされている間
    一度だけ行なわれる。

    必要な初期化はインスタンス作成時、後始末はインスタンス破棄時に行うこと。

    本体はプラグインをロード後 Open まで行わず、最低限の情報だけ取得後にアンロード
    することがある。例えば起動時にプラグインを初めて認識した場合は GetModuleInfo
    だけ呼び出し後にアンロードする。設定画面の表示時、プラグインを使用中でなければ

    GetModuleInfo/EnumConfig/ApplyConfig

    しか呼ばず、設定画面を閉じたところでアンロードする。

    もしプラグインの初期化に時間がかかる場合は Open 時に初期化を1度だけ実行するなど、
    プラグイン側で独自に工夫すること。その際は未初期化のまま IKpiDecoderModule
    を破棄されたり、どのメソッドが最初に呼ばれても動作に問題が起こらないように
    注意すること。
*/
};
///////////////////////////////////////////////////////////////////////////////
//デコーダプラグインが別のデコーダプラグインを使用するためのヘルパー関数
//
//  cpRequest の KPI_MEDIAINFO::nBitsPerSample に 0 以外を入れるとデコーダが
//  返すビット数が要求通りになる(本体が内部で要求されたビット数に変換する)
//
//  kbrunkpi.exe 経由の場合は未実装(必ず失敗する)
//
///////////////////////////////////////////////////////////////////////////////
extern DWORD WINAPI kpi_CreateDecoder(IKpiUnkProvider *pProvider,
                                      const KPI_MEDIAINFO *cpRequest,//再生周波数等の要求値が含まれた KPI_MEDIAINFO
                                      IKpiFile     *pFile,   //音楽データ
                                      IKpiFolder   *pFolder, //音楽データがあるフォルダ
                                      IKpiDecoder **ppDecoder);
extern DWORD WINAPI kpi_CreateDecoder(IKpiUnknown *pUnknown, //kpi_CreateInstance の第3引数から直接取得
                                      const KPI_MEDIAINFO *cpRequest,//再生周波数等の要求値が含まれた KPI_MEDIAINFO
                                      IKpiFile     *pFile,   //音楽データ
                                      IKpiFolder   *pFolder, //音楽データがあるフォルダ
                                      IKpiDecoder **ppDecoder);
//pProvider から IKpiDecoder のインスタンスを取得
//pProvider->CreateInstance(IID_IKpiDecoder, (void*)cpRequest, pFile, pFolder, NULL, (void**)ppDecoder);
//と同じ
//kpi_CreateInstance の第3引数から直接 IKpiDecoder を取得することも出来る
//
///////////////////////////////////////////////////////////////////////////////
//デコーダプラグイン使用側用のヘルパー関数(デコーダプラグイン作成には不要)
//プラグイン使用者(通常は KbMedia Player)が呼び出す場合は kpi_decoder.cpp も
//プロジェクトに追加する
///////////////////////////////////////////////////////////////////////////////
extern HRESULT WINAPI kpi_CreateDecoderModule(pfn_kpiCreateInstance fnCreateInstance,
                                              IKpiDecoderModule **ppModule, IKpiUnknown *pUnknown);
///////////////////////////////////////////////////////////////////////////////
//プラグイン使用時/作成時のヘルパー関数
//inline 関数として定義しているので kpi_decoder.cpp をプロジェクトに追加する必要はない
///////////////////////////////////////////////////////////////////////////////
inline UINT64 kpi_100nsToSample(UINT64 qw100ns, DWORD dwSampleRate)
{//100ナノ秒単位 => サンプル単位(小数点以下切り捨て)
    return qw100ns * dwSampleRate / (1000*10000);
}
inline UINT64 kpi_SampleTo100ns(UINT64 qwSample, DWORD dwSampleRate)
{//サンプル単位 => 100ナノ秒単位(小数点以下切り捨て)(0 で除算のチェックなし)
    return qwSample * (1000*10000) / dwSampleRate;
}
inline UINT64 kpi_MsTo100ns(UINT64 qwMs)
{//ミリ秒単位 => 100ナノ秒単位
 //DWORD で演算すると 7分を少し超えたあたりでオーバーフロー
    return qwMs * 10000;
}
inline void kpi_InitMediaInfo(KPI_MEDIAINFO *pMediaInfo)
{
    ZeroMemory(pMediaInfo, sizeof(KPI_MEDIAINFO));
    pMediaInfo->cb = sizeof(KPI_MEDIAINFO);
}
///////////////////////////////////////////////////////////////////////////////
#pragma pack(pop) //構造体のアラインメント
#endif
