#pragma once
#ifndef kpiH
#define kpiH

#pragma pack(push, 1) //構造体のアラインメント
/*

  KbMedia Player プラグイン SDK (2026/10/31版)(本体Ver 3.17beta3 用)
  (v3.17beta2 以前の本体でも実用上はほとんど問題なし)

  ・kmp_pi.h: 旧仕様のプラグインを作成・使用時に必要
              IKpiTagInfo の定義済み cszName を使いたい場合も必要
              旧仕様のプラグインは 64bit 版も含めて新仕様の正式リリース後も
              引き続き対応するが、制約が多いため新仕様への移行を推奨
              (旧仕様の方が作るのは簡単だが)
  ・kpi.h: 全てのプラグインで必要(旧仕様を除く)
  ・kpi_decoder.h: デコーダプラグイン作成・使用時に必要
                   演奏プラグイン作成・使用時にも必要
  ・kpi_player.h:  演奏プラグイン作成・使用時に必要
  ・kpi_unarc.h;   アーカイブプラグイン作成・使用時に必要
  ・kpi_device.h:  デバイスプラグイン作成・使用時に必要
  ・kpi_converter.h: 変換プラグイン作成・使用時に必要(v3.16 以降)
  ・kpi_impl.h:    IKpiUnknown の派生クラスで QueryInterface/AddRef/Release
                   の実装が面倒な場合に必要(使用は必須ではない)
  ・以下は各種プラグイン関係のヘルパー関数使用時にプロジェクトへの追加が
    必要になることがある(必須ではない)
    ・kpi.cpp: kpi_CreateConfig 等のヘルパー関数使用時(IKpiConfig に対応する
               場合はほぼ必須)
    ・kpi_decoder.cpp: デコーダプラグイン関係のヘルパー関数使用時
    ・kpi_player.cpp:  演奏プラグイン関係のヘルパー関数使用時
    ・kpi_unarc.cpp:   アーカイブプラグイン関係のヘルパー関数使用時
    ・kpi_device.cpp:  デバイスプラグイン関係のヘルパー関数使用時
    ・kpi_converter.cpp: 変換プラグイン関係のヘルパー関数使用時

  とりあえず新しい仕様のプラグインを作ってみる場合は、

  IKpiFolder/IKpiTagInfo/IKpiConfig/IKpiUnkProvider は無視
  IKpiFile::GetRealFileW で実ファイル名取得

  である程度のものは作成可能。kpi.h は熟読不要。kpi_decoder.h を最初に
  読んだ方が理解しやすいかも。

  実ファイル名からファイルを開く場合、開いたファイルを閉じるまで IKpiFile を
  Release してはいけないことに注意。


  更新履歴

  2026/10/31:(v3.17beta3)
    ・文字コードを UTF-8(BOM付) に変更
    ・IKpiConfig のセクション名とキー名の名前付け規則を追加
      ・セクション名、キー名ともに大文字と小文字を区別しない
      ・セクション名、キー名ともに先頭と末尾に空白文字を使うのを禁止
      ・セクション名に使用出来ない文字
        ・改行・タブ文字その他の制御文字
        ・'=', '[', ']' は使用可能だが非推奨
      ・キー名に使用出来ない文字
        ・'=', '[', ']', 改行・タブ文字その他の制御文字
      ・ApplyConfig でセクション名・キー名の大文字と小文字を区別して比較するプラグインは修正が必要
    ・IKpiFile::Read の仕様説明追加
      ・本体が返す(プラグインが読む) IKpiFile::Read
        戻り値が dwSize より小さいときは残りのバッファを 0 で埋める(未定義=>ゼロクリアに仕様変更)
        (それを前提にしてはいけない)
      ・プラグイン(主にアーカイブプラグイン、変換プラグイン) が返す IKpiFile::Read
        残りのバッファを 0 で埋めなくても良い(本体が戻り値をチェックする)
    ・IKpiConfig::GetStr の仕様変更
    ・IKpiFile::GetFileName, IKpiFolder::GetFolderName/EnumFiles の仕様説明追加
      バッファサイズが足りない時は空文字列にして必要なバッファサイズを返す
      ・GetFileName/GetFolderName/EnumFiles は明記してなかっただけで仕様は変更なし
      ・GetStr は文字列を切り捨てていたのを空文字列に仕様変更
        (ファイルパスの取得にも使われるため、切れた文字列が想定と異なる場所になる可能性を回避)
  2026/10/13:(v3.16)
    ・kpi_converter.h/cpp を追加(変換プラグイン)
  2017/03/06:
    ・kpi_unarc.h(アーカイブプラグイン用)を修正
    ・インターフェースは変更なし(GUID が変更になってるので v3.01以前では使用不可)
    ・kpi_CreateLocalFile/kpi_CreateMemoryFile を追加
    ・IKpiUnArcModule::Open の cszOutputPath 以下に展開結果を実ファイル出力した場合、
      アーカイブプラグインを使用する側(通常は KbMedia Player)が必要に応じて削除するようにした
  2017/01/16:
    ・特になし(正式版リリースに合わせて追記しただけ)
  2017/01/09:
    ・kpi_decoder.h/cpp: kpi_CreateDecoder を本体側で実装
    ・↑のビット数要求に関する記述を追加(本体によって要求ビット数に変換される)
    ・kpi_impl.h: KbKpiNullFolder(実質何もしない IKpiFolder) を追加
    ・kpi_device.h: IKpiOutDevice::Output に戻り値を追加
  2016/10/23:
    ・kpi_CreateConfig でプラグインの呼び出し側(通常は KbMedia Player 本体)が
      IKpiConfig 未対応の場合は常にデフォルト値を返す IKpiConfig を返すように
      した(kpi.cpp)
    ・IKpiConfig がなくても動作するように作成するのが面倒な場合に対応
  2016/10/10:
    ・kpi_device.h::KPI_DEVICE_FORMAT::dwFormatType を追加(DoP 対応用)
    ・デコーダプラグインは変更なし
  2016/10/02:
    ・KPI_DECODER_MODULEINFO::cszSupportCodecs/cszSupportSupportContainers を廃止
      (pvReserved1/pvReserved2 に変更)
    ・KPI_DECODER_MODULE_VERSION を 200 に変更
    ・デコーダプラグイン仕様はこれで確定(の予定)
  2016/09/12:
    ・IKpiTagInfo::SetPicture の仕様を変更
    ・IKpiTagInfo::GetTagInfo の第3引数(dwType)を修正
      ・ID3v1 と ID3v2 と Lyrics3Tag を区別
  2016/08/22:
    ・IKpiDecoder/IKpiDecoderModule を kpi_decoder.h に移動
    ・IKpiTagInfo にメソッド追加
        DWORD WINAPI GetTagInfo(IKpiFile *pFile, IKpiFolder *pFolder,
                                DWORD dwType, DWORD dwTagGetFlags);//本体が持っているタグ取得ルーチンを呼び出す
        DWORD WINAPI GetValue(const wchar_t *cszName, wchar_t *pszValue, int nSize);
        void  WINAPI SetOverwrite(BOOL bOverwrite);//上書きモードの切り替え
        void  WINAPI SetPicture(const wchar_t *cszName,   //"front", "back", etc...
                                const wchar_t *cszFormat, //"jpg", "png", "gif", "bmp", etc
                                const BYTE *pPictureData, DWORD dwSize);
    ・IKpiDecoder::Select/IKpiPlayer::Select の第4引数にタグ取得するフィールド
      を指定するフラグを追加
        DWORD  WINAPI Select(DWORD dwNumber,                    //曲番号
                             const KPI_MEDIAINFO **ppMediaInfo, //may be NULL
                             IKpiTagInfo *pTagInfo,             //may be NULL
                             DWORD dwTagGetFlags);//どのフィールドのタグを取得するかのフラグ
  2016/08/16:
    ・セクションの表示名とヘルプに対応
      ・KPI_CFG_SECTION 追加
      ・KPI_CFG_INFO => KPI_CFG_KEY に変更
      ・IKpiConfigEnumerator のメソッド追加
      ・IKpiConfig の uuid(IID_IKpiConfig) を変更(互換性がなくなったため)
    ・kpi_CreateConfig 呼び出しの有効期限を追加
      ・kpi_CreateInstance から戻るまでとした
  2016/08/08:
    ・IKpiConfig(設定の画面生成と保存管理)を大幅修正＆実装
    ・↑に伴うメソッド(IKpiDecoder::UpdateConfig)の追加
    ・アーカイブ/デバイス/演奏(主に動画用)関係のプラグイン化
    ・対応拡張子等を修正(KPI_DECODER_MODULEINFO::cszSupportExts 他)
    ・IKpiFile::GetBuffer の第2引数の型を DWORD => size_t に変更
      (64bit 版で 4GB の制限をなくすため)
    ・定数(IKpiFile::GET_BUFFER_MAXSIZE) を追加
      (IKpiFile::GetBuffer を本体がエミュレートしたときのサイズの制限)
    ・エクスポートする関数の名前を変更
      kpi_CreateDecoderModule => kpi_CreateInstance
    ・次の正式版までにやりたいことの 80% くらいは実装出来たつもり
      (残りの 20% に全体の80%(今までの4倍)の時間を費やしそうな気もするが)
    ・タグ取得周りが気に入らない…。Open/Select 時の引数の意味とかが見苦しい。

  2015/12/28: pfnCreateDecoderModule の戻り値を修正(BOOL* => BOOL)


  現プラグイン仕様(kmp_pi.h)の欠点
    ・ループ曲に未対応
    ・１ファイル複数曲への対応が不完全
      ・本体が知っている拡張子にしか対応出来ない
      ・曲の長さを取得出来ない(すべて無限ループする曲として扱われる)
      ・シークに対応出来ない
    ・外部ファイルを参照する形式のアーカイブ対応が不完全
      ・参照ファイルの取得方法を本体が知っている必要がある
      ・本体が知らない拡張子のプラグインに対しては、参照ファイルの取得が
        不要でも、本体は参照ファイルを(強引な方法で)取得しようとするため、
        非効率的である
    ・ファイルからの読み込みにしか対応出来ないため、アーカイブ内のデータ等、
      ファイルとして存在しないデータを再生するのにも必ず一時ファイルを作成
      しなければならない
    ・マルチチャンネル未対応
  新プラグイン仕様策定の目標
    ・上記欠点を解決
    ・アーカイブ関係のプラグイン化
    ・出力デバイス関係のプラグイン化
    ・動画関係のプラグイン化
    ・出来るだけパフォーマンスが低下しないように
    ・出来るだけ簡単にプラグインを開発出来るように(残念ながら旧仕様と比べると複雑で難しい)
    ・出来るだけ簡単にプラグインを使用出来るように(旧仕様と比べると数段難しい)
    ・C++ 以外の言語でも開発出来るように
      ・後回し(C++ => Delphi/C ヘッダへのコンバータが存在すれば解決？)
    ・拡張の余地を残す
    ・必要以上に欲張り過ぎない
    ・以下もやってみたいが後回し
      ・エフェクト関係のプラグイン化
      ・WAVE/AVI/OGG/Matrosuka 等、コンテナ対応のプラグイン化
      ・MIDI 関係にもプラグインで対応
      ・ファイルコンバータ関係のプラグイン化(MML=>MID等)
      ・動画の映像部分も含めたプラグイン化

*/
/*
    インターフェース(class IKpiXXX)

    IKpiUnknown : 全てのインターフェースは IKpiUnknown から派生する
                  IUnknown の typedef なので IUnknown と同じ
                  QuearyInterface で拡張の余地を残す
                  Release が呼ばれたら直ちに破棄するのではなく、内部で
                  参照カウントを持たせてカウンタが 0 になったら破棄する
                  実装が面倒なら kpi_impl.h で定義しているクラスから派生
                  させても良い(必須ではない)

    主に本体側で実装(IKpiFile のみアーカイブプラグインでも実装)
    IKpiFile    : ストリームからのファイルの読み込み(書き込みはなし)
    IKpiFolder  : フォルダに相当
    IKpiTagInfo : タグ情報取得用
    IKpiConfig  : 設定読み書き用
    IKpiUnkProvider : IKpiConfig 取得、プラグインからプラグインを使う場合等に使用
    IKpiXXXEnumerator: 列挙関係(IKpiTagInfo もある意味列挙系)

    これらのインターフェースは KbMedia Player 側で提供するのでプラグイン制作者
    が実装する必要はない。
    ただし、アーカイブプラグインを作る場合には IKpiFile の実装が必要。

    プラグイン側で実装
    IKpiDecoderModule : デコーダプラグイン(オーディオデータを PCM にデコード)
    IKpiDecoder       : ↑を実際に処理する
    IKpiUnArcModule   : アーカイブ展開プラグイン
    IKpiUnArc         : ↑を実際に処理する
    IKpiPlayerModule  : 演奏プラグイン(主に動画等)(PCM を取得出来ないが動画も扱える)
    IKpiPlayer        ; ↑を実際に処理する
    IKpiOutDeviceModule :デバイス出力プラグイン
    IkpiOutDevice       :↑を実際に処理する

    これらのインターフェースはプラグイン側で実装が必要。
    QueryInterface/AddRef/Release の実装が面倒なら kpi_impl.h で
    定義されているクラスから派生させても良い(必須ではない)

    構造体(struct KPI_XXX)

    KPI_DECODER_MODULEINFO :デコーダプラグインの情報(旧仕様の KMPMODULE の情報部分に相当)
    KPI_UNARC_MODULEINFO   :アーカイブプラグインの情報(↑のアーカイブ版)
    KPI_PLAYER_MODULEINFO  :演奏プラグインの情報(実際は KPI_DECODER_MODULEINFO の typedef)
    KPI_OUTDEVICE_MODULEINFO: デバイス出力プラグインの情報
    KPI_MEDIAINFO          :デコードする曲の情報(旧仕様の SOUNDINFO に相当)
    KPI_ARCHIVEINFO        :アーカイブファイル自体の情報
    KPI_FILEINFO           :アーカイブファイル内の格納ファイルの情報

    ほとんどの構造体は sizeof 値を格納するメンバ(cb) を持つ。
    構造体の拡張の余地を残す。

    IKpiXXX の Release し忘れに十分注意すること
    メモリリークするだけでなく、KbMedia Player がメモリアクセス違反で異常終了
    することがある。

    AddRef/Release は数え方に注意が必要。分かっていると思うが念のため書いて
    おくことにすると…。

    ・IKpiDecoderModule::Open の IKpiFile/IKpiFolder 等、メソッドの引数として
      渡されたインターフェースの、渡された時点でのプラグインが管理すべき参照
      カウントは 0 として数える。
      ・AddRef と Release の回数を同じにするということ。
      ・メソッドから戻った後も使用する場合は AddRef しなければならない。
        (不要になったら AddRef したのと同じ回数だけ Release しなければならない)
      ・メソッドから戻った後に使用しない場合は Release 不要。Release してはいけない
      ・一部のインターフェースはメソッドから戻った後の使用を認めていない(主に列挙系)
        (IKpiDecoder::Select 時の IKpiTagInfo や IKpiXXXEnumerator 等が該当)
    ・QueryInterface や IKpiFile::CreateClone, IKpiFolder::OpenFile 等で獲得した
      インスタンスの、獲得時点でのプラグインが管理すべき参照カウントは 1 として
      数える。
      ・AddRef より Release の回数の方が 1 回だけ多いということ。

    なお、本体から提供されるインターフェースに限り、

    ・インスタンスの作成元を、獲得したインスタンスより先に Release しても良い

    例えば IKpiFolder::OpenFile で IKpiFile を獲得後、獲得した IKpiFile より
    先に IKpiFolder を Release して良いということである。
    IKpiUnkProvider による IKpiConfig 獲得も同様。

    これはプラグイン作成側から見たときだけで、プラグイン使用時には当てはまらない。
    例えば本体が IKpiDecoder よりも先に IKpiDecoderModule を Release することは
    なく、そうなる可能性を想定してプラグインを作成する必要はない。

    例外は以下である。

    ・IKpiFile::GetRealFileW/A で得た実ファイルをメソッドから戻った後も使用する
      場合は AddRef しておき、実ファイルを閉じた後に Release しなければならない。
    ・IKpiFile::GetBuffer で得たメモリバッファをメソッドから戻った後も使用する
      場合は AddRef しておき、メモリバッファが不要になったら Release しなければならない。

    GetRealFileW/A/GetBuffer した結果だけを後から Release する手段がないため。

    Release し忘れを本体側で検出することは出来ず、本体の動作が不安定になること
    があるため、AddRef/Release の管理は十分に気を遣うこと。


    extern DWORD WINAPI kpi_CreateConfig(IKpiUnknown *pUnknown,
                                         const GUID *pGUID,
                                         DWORD *pdwPlatform,
                                         IKpiConfig **ppConfig);
    等、extern kpi_XXX

    で宣言されるヘルパー関数を使用する場合は kpi.cpp もプロジェクトに追加する
    (リンカエラー回避)

    IKpiXXXModule は以下の共通のメソッドを持つ
     ・GetModuleInfo: プラグインの情報を取得(引数の構造体(KPI_XXX_MODULEINFO)の型はプラグインの種類により異なる)
     ・Open :         IKpiXXX のインスタンスを取得する(引数はプラグインの種類により異なる)
     ・EnumConfig:    設定項目を列挙する
     ・ApplyConfig:   設定を「適用しようとした時」に呼ばれる。呼ばれた時点では
                      まだ設定は変更されていない。

    IKpiXXX は以下の共通のメソッドを持つ
     ・UpdateConfig: IKpiXXXModule::ApplyConfig を呼び出して設定を「適用した後に」呼ばれる。
                     IKpiXXX のインスタンスごとに呼ばれる。
*/

typedef IUnknown IKpiUnknown; //IKpiUnknown = IUnknown
class __declspec(uuid("{C3BCE8C9-3F3E-49D0-8278-6014D9667274}")) IKpiFile;
class __declspec(uuid("{E521AD43-9156-4923-880A-7B8A3D1CF97B}")) IKpiFolder;
class __declspec(uuid("{130903A6-D381-4136-832E-688689AEF030}")) IKpiTagInfo;
class __declspec(uuid("{EFE8AC3E-A5D4-403A-BBB3-BD09A1B292F4}")) IKpiConfigEnumerator;
class __declspec(uuid("{D01D08E8-9823-48D9-85E9-4F37DCC09EEF}")) IKpiConfig;
class __declspec(uuid("{C5CF866A-4A9B-46E1-9CA8-3025D20774E9}")) IKpiUnkProvider;

#define IID_IKpiUnknown     __uuidof(IKpiUnknown)
#define IID_IKpiFile        __uuidof(IKpiFile)
#define IID_IKpiFolder      __uuidof(IKpiFolder)
#define IID_IKpiTagInfo     __uuidof(IKpiTagInfo)
#define IID_IKpiConfigEnumerator __uuidof(IKpiConfigEnumerator)
#define IID_IKpiConfig      __uuidof(IKpiConfig)
#define IID_IKpiUnkProvider __uuidof(IKpiUnkProvider)

#define KPI_MULTINST_INFINITE 0//IKpiXXXModule::Open でインスタンスを同時に何個でも作成可能
#define KPI_MULTINST_ZERO     1//↑は常に別プロセス経由で取得
#define KPI_MULTINST_ONE      2//↑は同時に1個まで作成可能(2個目以降は別プロセス経由)
//(KPI_MULTIINST_ZERO + N )    //↑は同時にN個まで作成可能((N+1)個目以降は別プロセス経由)
#define KPI_MULTINST_UNIQUE (DWORD)-1 //↑はシステムに(プロセスをまたいでも)1個しか作成不可

//IKpiFile/IKpiFolder 他の文字列取得・列挙系メソッド
//
//  GetXXXName(..., wchar_t *pszName, DWORD dwSize, ...)
//  EnumXXX(DWORD dwIndex, wchar_t *pszName, DWORD dwSize, ...)
//
//の仕様
//
//pszName に格納すべきバッファ、dwSize にバッファのサイズ(文字数ではない)を渡す。
//格納すべき文字列が存在する(成功時)の場合は必要なバッファのサイズ(終端の 0 を含む)を返す。
//バッファのサイズが足りている場合は pszName に文字列を格納する。
//(戻り値は必要なバッファのサイズ==格納したバッファのサイズ)
//バッファのサイズが足りない場合は pszName に空文字列を格納する。
//格納すべき文字列がない場合は pszName に空文字列を格納して 0 を返す。
//
//列挙系(EnumXXX)では 0 を返したときに列挙終了。ある dwIndex に対して 0 を返した場合、
//その値以上の dwIndex に対しては必ず 0 を返す。
//
//pszName に NULL や dwSize に 2 より小さな値を渡してはいけない。Get/Enum 系が 0 を返す場合や
//バッファのサイズが足りない場合、pszName に空文字列が入る。途切れた文字列ではなく、空文字列
//が入ることに注意。渡したバッファのサイズよりも大きな値を返したら取得に失敗している。
//
///////////////////////////////////////////////////////////////////////////////
#define KPI_FILE_EOF (UINT64)-1 //サイズが不明時の GetSize() の戻り値
                                //失敗時の Seek() の戻り値
class IKpiFile : public IKpiUnknown
{
/*
    Cランタイムライブラリの FILE 構造体、Win32API の HANDLE hFile に相当する。
    通常は Read/Seek/GetSize しか使わなくて良いはず。
    GetRealFile や GetBuffer を使った場合は、取得したファイルやメモリバッファの
    寿命(IKpiFile を Release するまで)に十分気を遣うこと。
*/
public:
    enum{
        GET_BUFFER_MAXSIZE = 128*1024*1024 //GetBuffer が返す最大サイズ(128MB)
        //本体が GetBuffer をエミュレートする時のもので、実際のサイズの制限は
        //アーカイブプラグインや本体の実装による
        //扱う形式がこれより大きいことがあり得る場合、GetBuffer を呼ぶことは
        //推奨しない
    };
    virtual DWORD  WINAPI Read(void *pBuffer, DWORD dwSize)=0;
    virtual UINT64 WINAPI Seek(INT64 i64Pos, DWORD dwOrigin)=0;
    virtual UINT64 WINAPI GetSize(void)=0;
    virtual BOOL   WINAPI CreateClone(IKpiFile **ppFile)=0;
    virtual DWORD  WINAPI GetFileName(wchar_t *pszName, DWORD dwSize)=0;//dwSizeの単位はバッファサイズ(文字数ではない)
    virtual BOOL   WINAPI GetRealFileW(const wchar_t **ppszFileNameW)=0;
    virtual BOOL   WINAPI GetRealFileA(const char **ppszFileNameA)=0;
    virtual BOOL   WINAPI GetBuffer(const BYTE **ppBuffer, size_t *pstSize)=0;//32bit 版と 64bit 版で確保可能なバッファサイズの上限が異なる
    virtual BOOL   WINAPI Abort(void)=0;
/*
    DWORD WINAPI Read(void *pBuffer, DWORD dwSize);
      Cランタイムライブラリの の fread、Win32API の ReadFile に相当する。
      読み取ったデータのサイズを返す。dwSize より小さな値を返したら終端に
      到達したことを意味する。呼び出し側で常に戻り値を確認すること。
      dwSize より小さい値を返す場合、本体は残りのバッファを 0 で埋めるがそれを
      前提にして実装するべきではない。

      プラグイン(主にアーカイブプラグイン)が本体に提供する IKpiFile::Read では、
      残りのバッファを 0 で埋めることを必須としない。
      本体は必ず戻り値をチェックする。

    UINT64 WINAPI Seek(INT64 i64Pos, DWORD dwOrigin);
      Cランタイムライブラリの fseek、Win32API の SetFilePointer に相当する。
      成功した場合はシーク後の位置を返す。失敗した場合は KPI_FILE_EOF を返す。

      プラグインに対して本体から渡される IKpiFile::Seek の結果は、
      ・要求されたのと正確に同じ位置を返すか、KPI_FILE_EOF を返す。
      ・GetSize()==KPI_FILE_EOF の場合に dwOrigin==FILE_END で Seek した場合は
        KPI_FILE_EOF を返す。
      ・dwOrigin!=FILE_BEGIN && dwOrigin!=FILE_CURRENT && dwOrigin!=FILE_END の場合
        の Seek の結果は未定義。(FILE_BEGIN/CURRENT/END 以外を渡してはいけない)
      ・Seek が KPI_FILE_EOF を返した後の Read は 0 を返す。
        Seek が KPI_FILE_EOF を返した後、
        ・dwOrigin==FILE_CURRENT で Seek した場合は KPI_FILE_EOF を返す。
        ・dwOrigin==FILE_BEGIN or FILE_END で Seek した場合に成功すれば、
          次回以降の Read は成功する。
      ・KbMedia Player のプラグインを使うアプリケーションを作る場合、プラグインに渡す
        IKpiFile::Seek は上記を満たすように実装しなければならない
      ・KbMedia Player のプラグインを作成する場合は、現在位置を把握する目的以外で
        Seek の戻り値を確認する必要はあまりないが、Read の戻り値は常に確認するべき。

      プラグイン(主にアーカイブプラグイン)が本体に提供する IKpiFile::Seek の結果は、
      ・要求されたのと正確に同じ位置を返すか、要求されたより手前の位置、あるいは
        KPI_FILE_EOF を返す。要求されたより後ろの位置にシークしてはいけない。
      ・GetSize()==KPI_FILE_EOF の場合に dwOrigin==FILE_END で Seek した場合は
        KPI_FILE_EOF を返す。
      ・dwOrigin!=FILE_BEGIN && dwOrigin!=FILE_CURRENT && dwOrigin!=FILE_END の場合
        の Seek の結果は未定義
      ・Seek が KPI_FILE_EOF を返した後の Read の結果は未定義。
      ・Seek が KPI_FILE_EOF を返した後、
        ・dwOrigin==FILE_CURRENT で Seek した結果は未定義。
        ・dwOrigin==FILE_BEGIN or FILE_END で Seek した場合に成功すれば、
          次回以降の Read は成功する。
      ・要求先位置と異なる位置を返すことがあり、失敗した後の Read の結果が未定義のため、
        アーカイブプラグインを使用するアプリケーションを作る場合、常に Seek の戻り値を
        確認しなければならない。(プラグイン制作時はそこまで神経質になる必要はない)
      ・アーカイブプラグイン制作時は、結果が未定義になるような Seek/Read の呼ばれ方は
        されないことを想定して実装して良い。

    UINT64 WINAPI GetSize(void);
      ファイルサイズを返す。実行後にファイルポインタは変化しない。
      Cランタイムライブラリでは fseek(0,SEEK_END) 後の ftell に相当。
      (ファイルポインタは移動しないが)
      Win32API では GetFileSize に相当。
      サイズがわからない場合は KPI_FILE_EOF を返す。

    BOOL WINAPI CreateClone(IKpiFile **ppFile);
      使用中の IKpiFile とは独立して動作する IKpiFile が複数必要な場合に呼び出す。
      成功したら 0 以外を返して *ppFile に元の IKpiFile のコピーに相当するインスタンスを返す。
      失敗したら 0 を返して *ppFile に NULL を返す。
      アーカイブプラグインの開発において、あるファイルを解凍中に別のファイルを
      解凍するために必要になる。
      取得後の *ppFile のファイルポインタは先頭に設定される。
      (コピー元のファイルポインタと同じではないことに注意)

    DWORD WINAPI GetFileName(wchar_t *pszName, DWORD dwSize);
      IKpiFile のファイル名を得る(フルパスではない)
      dwSize >= MAX_PATH*sizeof(wchar_t) とすれば、バッファサイズが足りないことはほぼない
      バッファサイズが足りない場合は空文字列が入ることに注意。
      再生するファイルの拡張子を知りたい場合や、再生するファイルと同名の外部
      ファイルが必要な場合などに使う。(aaa.xxx を再生するのに aaa.yyy が必要など)
      例えば aaa.xxx に対して GetFileName() を呼ぶと "aaa.xxx" を返す。
      名前がない場合は pszName に空文字列を返す。

      フルパスを得る手段は提供されない。ファイルの場所を意識するべきではない。

    BOOL WINAPI GetRealFileW(const wchar_t **ppszFileNameW);
      IKpiFile の内容を実ファイルとして得る。
      成功したら 0 以外を返して *ppszFileNameW に実体のファイル名(フルパス)が入る。
      失敗したら 0 を返して *ppszFileNameW に NULL が入る。
      *ppszFileNameW 及び実ファイルは IKpiFile を Release するまで有効。
      従って、
       ・IKpiFile を Release する前に実ファイルを閉じなければならない。
       ・実ファイルを使う前に IKpiFile を Release してはいけない。
       ・元々実体がローカルファイルだった場合、IKpiFile を Release してから
         使用しても問題なく動作してしまうが、これはバグである。
      実ファイルの内容を書き換えたり、ファイルを移動・削除してはいけない。
      アーカイブ等を扱っている場合、KbMedia Player が IKpiFile を作成した
      時点では、実ファイルを作成していないことがある。その場合に GetRealFile
      を呼び出すと、テンポラリフォルダに一時ファイルを作成することになり、
      パフォーマンスの面で不利になる。

      ここで返す実ファイル名と GetFileName で返すファイル名は同じとは限らない
      実ファイル名が C:\temp\aaa.txt なのに GetFileName が bbb.txt を返すこと
      があることに注意

      ライブラリの都合でどうしても実ファイル名が必要な場合を除き、
      なるべく GetRealFileW/A は使わずに Read/Seek/GetBuffer で実装すること

      ファイル名を得ることだけが目的の場合は GetFileName を呼び出すこと。

    BOOL WINAPI GetRealFileA(const char **ppszFileNameA);
      GetRealFileW の ANSI 版
      使用するライブラリの都合上、UNICODE ファイル名に未対応なときに GetRealFileW
      の代わりに呼び出す。
      実ファイル名が ANSI で表現出来ない場合は、実ファイルのコピーをテンポラリに
      作成してコピーのファイル名を返す。

      ここで返す実ファイル名と GetFileName で返すファイル名は同じとは限らない
      実ファイル名が C:\temp\aaa.txt なのに GetFileName が bbb.txt を返すこと
      があることに注意。

      UNICODE ファイル名のコピーの ANSI ファイル名はランダムに生成される
      ただし、フォルダ名だけが UNICODE 名の場合、ファイル名部分はオリジナル
      と同じ名前になる。

    BOOL WINAPI GetBuffer(const BYTE **ppBuffer, size_t *pstSize);
      IKpiFile の内容をメモリバッファとして得る。
      成功したら 0 以外を返して *ppBuffer にメモリバッファが、*pstSize にメモリ
      バッファのサイズが入る。
      失敗したら 0 を返して *ppBuffer には NULL が、*pstSize には 0 が入る。
      *ppBuffer は IKpiFile を Release するまで有効。
      IKpiFile を Release 後は *ppBuffer にアクセスしてはいけない。
      メモリバッファの内容を書き換えてはいけない。

      IKpiFile の実体がメモリバッファでない場合(アーカイブプラグインが返す
      IKpiFile::GetBuffer に失敗した場合)や kbrunkpi を介している場合、
      KbMedia Player は GetBuffer をエミュレートする。
      そのとき、実体のサイズ(IKpiFile::GetSize が返す値)が大きい場合は失敗する
      ことがある。(最大で GET_BUFFER_MAXSIZE まで)
      これはサイズが GET_BUFFER_MAXSIZE より大きいと必ず GetBuffer に失敗する
      ことを意味する訳ではない。プラグインの実装によってはもっと大きなサイズ
      でも成功することがある。(64bit 版では 4GB を超えても成功することもあり得る)
      GET_BUFFER_MAXSIZE の実際の値は本体のプラットフォームやバージョンにより
      変更されることがある(特に 64bit 版)

      扱うファイル形式が通常 GET_BUFFER_MAXSIZE より小さく、Read/Seek を使うより
      GetBuffer を使った方が実装が容易であったりパフォーマンス面で有利ならば、
      GetBuffer を積極的に利用する。ただし GetBuffer はサイズが大きすぎると失敗
      することがあることに留意すること。

      もし GetBufferSize に失敗した場合は Read と GetSize を使ってプラグインが
      自分でメモリを割り当てるか、プラグインが扱う形式としてはサイズが大きすぎる
      と判断して適切に処理すること。

      GetSize/CreateClone/GetFileName/GetRealFile/GetBuffer のどのメソッドを呼び出しても
      ファイルポインタは変化せず、その後の Read/Seek の結果に影響しない。

    BOOL WINAPI Abort(void);
      処理を中断する。成功したら TRUE を返す。
      このメソッドを呼出し後、全てのメソッド呼び出しは失敗する。
      Abort() は任意のスレッドから非同期に呼び出される。
      Abort() は通常、プラグインを使用するアプリケーション(KbMedia Player)に
      よって呼び出される。
      プラグイン側でこのメソッドを呼び出してはいけない。
*/
};
///////////////////////////////////////////////////////////////////////////////
class IKpiFolder : public IKpiUnknown
{
/*
    IKpiDecoderModule/IKpiUnArcModule::Open 時に IKpiFile と共に渡される。
    IKpiFile が存在するフォルダに相当する。
    IKpiFile から得るデータだけでは処理出来ず、その同一フォルダ内の別データが
    必要な場合に使う。
    音楽系では再生に PCM ファイルを必要とする形式などで必要。
    アーカイブ系ではマルチボリュームや分割ファイルなどに対応するために必要。

    IKpiFolder にきちんと対応するのは困難かもしれない。

    プラグイン開発にこれを利用することは少ないはずだが、利用しなければ
    ならない場合は頑張ること。

    どうしても実装困難な場合は、IKpiFile::GetRealFileW/A で IKpiFile の実ファイルを
    取得して、実ファイルと同じフォルダに直接アクセスする。
    もし同一フォルダ内の関連ファイルも必要な場合は、必要なファイルを OpenFile すれば、
    再生する実ファイルと同じフォルダに関連ファイルの実ファイルも作成される。
    OpenFile しなかった場合、実ファイルと同じフォルダに関連ファイルが存在するかどうかは
    分からない。(例えばアーカイブでは存在しない可能性が高い)
    IKpiFile/IKpiFolder の実体の形式によってはうまく動作しない可能性がある。
    例えば http 上の(アーカイブでない)ファイルでは IKpiFolder が動作しない。

    プラグイン利用者側にとって IKpiFolder の実装はかなり困難であると思われる。
    私(Kobarin)だけが頑張れば良いことだが。

    例えば SPC を再生するときには、
       ・同一フォルダ内の拡張子 .7se の同名ファイル
       ・同一フォルダ内の拡張子 .700 の同名ファイル
       ・同一フォルダ内の 65816.7se
       ・同一フォルダ内の 65816.700
    が存在すれば、それを読み込む(Script700/7SE)

    また、MDX を再生するときには、
       ・同一フォルダ内の PDX ファイル(ファイル名は MDX ファイルに格納されている)
    が必要になる。

    これらをアーカイブにも対応させるための機構。

    アーカイブプラグインがマルチボリューム書庫に対応したい場合にも必要になる
*/
public:
    virtual DWORD WINAPI GetFolderName(wchar_t *pszName, DWORD dwSize)=0;//dwSize の単位はバッファサイズ(文字数ではない)
    virtual DWORD WINAPI EnumFiles(DWORD dwIndex, wchar_t *pszName, DWORD dwSize, DWORD dwLevel)=0;//dwSize の単位はバッファサイズ(文字数ではない)
    virtual BOOL  WINAPI OpenFile(const wchar_t* cszName, IKpiFile **ppFile)=0;
    virtual BOOL  WINAPI OpenFolder(const wchar_t* cszName, IKpiFolder **ppFolder)=0;
/*
    DWORD WINAPI GetFolderName(wchar_t *pszName, DWORD dwSize);
      IKpiFolder のフォルダ名を得る(フルパスではない)
      フォルダ名の末尾に \ はつかない。
      dwSize >= MAX_PATH*sizeof(wchar_t) とすれば、バッファサイズが足りないことはほぼない。
      バッファサイズが足りない場合は空文字列が入ることに注意。
      IKpiFolder の実体がアーカイブの場合、アーカイブファイル名から拡張子を除いた
      ものを返す。(IKpiFolder が aaa.lzh> の場合は "aaa" がフォルダ名となる)
      IKpiFolder がドライブのルートの場合、: を $ に置換したものを返す。
      (IKpiFolder が C:\ の場合は "C$" がフォルダ名となる)
      処理するファイルが置かれているフォルダの名前が必要な場合に使う。
      (フォルダ名を元に参照ファイル名が決まるなど)
      名前がない場合、pszName には空文字列が入る。

    DWORD WINAPI EnumFiles(DWORD dwIndex, wchar_t *pszName, DWORD dwSize, DWORD dwLevel);
      IKpiFolder と同じフォルダにあるファイルとフォルダを列挙する。
      dwSize >= MAX_PATH*sizeof(wchar_t) とすれば、バッファサイズが足りないことはほぼない
      バッファサイズが足りない場合は空文字列が入ることに注意。
      列挙される名前は全て IKpiFolder があるフォルダからの相対パスとなる。
      サブフォルダ以下も列挙する。
      パスの区切り文字は / が使われる。(\ ではない)
      dwLevel は検索するサブフォルダの深さ。
        ・dwLevel==0 の場合はサブフォルダ以下を列挙しない。(サブフォルダ名は列挙する)
        ・dwLevel==(DWORD)-1 の場合は階層制限なしに列挙。
      成功したら dwIndex に対応するファイル名またはフォルダ名を pszName に返して
      名前を格納するのに必要なバッファのサイズを返す。(0 以外を返す)
      名前の末尾が / でない場合はファイル名、末尾が / の場合はフォルダ名となる。
      失敗したら 0 を返して列挙終了。(これ以上の dwIndex を渡しても失敗)

      pszName は dwIndex と dwLevel の組合せで決まる。dwIndex と dwLevel の組合せ
      が変われば、dwIndex が同じでも pszName が変化する可能性があることに注意。
      例えば、
        dwIndex=0, dwLevel=0 のときに pszName="aaa.txt" となったからといって、
        dwIndex=0, dwLevel=1 のときに pszName="aaa.txt" となるとは限らない。

      列挙される順番は見つかった順でしかない。ファイル・フォルダのどちらが先に
      列挙されるかはわからないし、位置関係もばらばらの可能性があることに注意。
      例えば、dwLevel=2 で EnumFiles を呼び出した場合、
        dwIndex=0: xxx/yyy/aaa.txt
        dwIndex=1: xxx/bbb.txt
        dwIndex=2: xxx/yyy/ccc.txt
        のような順番で列挙されることがある。dwIndex=1 の時点で xxx/yyy/ フォルダ
        にはもうファイルが存在しないと決め付けてはいけない。

      EnumFiles は、IKpiFile の内容や IKpiFile::GetFileName、IKpiFolder::GetFolderName
      から必要な参照ファイル名を決定することが出来ない場合を除き、なるべく呼び出さない
      ようにすること。呼び出すなら、出来るだけ dwLevel < 2 にすること。

      列挙するファイルの中にアーカイブファイルがあった場合、ファイルと同時に
      フォルダとしても列挙され、フォルダとしての名前の末尾は > となる。
      例えば aaa.lzh が含まれている場合、aaa.lzh と aaa.lzh> が列挙される。
      aaa.lzh> の下は dwLevel の値に関わりなく列挙されない。
      aaa.lzh> 以下の列挙が必要な場合は OpenFolder("aaa.lzh>", &pArchiveFolder) の
      ようにしてから pArchiveFolder に対して EnumFiles を呼び出す。

    BOOL WINAPI OpenFile(const wchar_t* cszName, IKpiFile **ppFile);
      EnumFiles で返す名前に対応するファイルを取得する。
      取得した *ppFile は、必要がなくなったら Release すること。

      cszName には相対パス、絶対パスを使用可能。
      EnumFiles であらかじめ列挙しておく必要はない。
      パスの区切り文字は \ と / のどちらを使用しても良い。
      ただし、アーカイブの中を指定する場合は > を使用する。
      例1 ：OpenFile("a.txt", &pFile);        //相対パス
      例2 ：OpenFile("a\\b\\c.txt", &pFile);  // \ と / のどちらでも良い
      例2'：OpenFile("a/b/c.txt", &pFile);    // \ と / のどちらでも良い
      例3 ：OpenFile("..\\a.txt", &pFile);    //相対パス(親フォルダ)
      例4 ：OpenFile("C:\\a\\b.txt", &pFile); //絶対パス
      例5 ：OpenFile("a.lzh>b/c.txt", &pFile);//アーカイブ(相対パス)        #現在未実装
      例6 ：OpenFile("C:\\a.lzh>b.zip>c.txt", &pFile);//アーカイブ(絶対パス)#現在未実装
      例7 ：OpenFile("http://www.hoge.com/a.zip>b.txt", &pFile);//http://     #現在未実装

      例5,6 はパフォーマンスの関係上実装される見込みは少ない。
      どうしてもアーカイブの中のファイルを取得したければ、

      例5 : OpenFolder("a.lzh>", &pFolder);
            pFolder->OpenFile("b/c.txt", &pFile);
            または
            OpenFolder("a.lzh>b/", &pFolder;
            pFolder->OpenFile("c.txt", &pFile);
      例6 : OpenFolder("C:\\a.lzh>b.zip>", &pFolder);
            pFolder->OpenFile("c.txt", &pFile);

      のようにして、OpenFolder でアーカイブをフォルダとして取り出してからにする。
      OpenFolder はアーカイブの中のアーカイブも直接取得出来る。

      OpenFile で返した IKpiFile に対して GetRealFile した場合、出来る限り
      ディレクトリ階層を復元したファイル名を返すようになっている。
      ただし、例3 のように親フォルダのファイルを開いた場合は復元されないことがある。
      また、アーカイブプラグインの動作の仕様上、親フォルダを指定してなくてもうまく
      復元されないことがある。

    BOOL WINAPI OpenFolder(const wchar_t* cszName, IKpiFolder **ppFolder);
      EnumFiles で返す名前に対応するフォルダを取得する。
      取得した *ppFolder は、必要がなくなったら Release すること。

      cszName には相対パス、絶対パスを使用可能。
      EnumFiles であらかじめ列挙しておく必要はない。
      名前の末尾に \ または / または > が必要。(ついてないと必ず失敗)
      パスの区切り文字は \ と / のどちらを使用しても良い。
      ただし、アーカイブの中を指定する場合は > を使用する。
      例1 ：OpenFolder("a/", &pFolder);             //相対パス
      例2 ：OpenFolder("a/b/", &pFolder);           // \ と / のどちらでも良い
      例2'：OpenFolder("a\\b\\", &pFolder);         // \ と / のどちらでも良い
      例3 ：OpenFolder("../", &pFolder);            //相対パス(親フォルダ)
      例4 ：OpenFolder("C:\\a\\", &pFolder);        //絶対パス
      例5 ：OpenFolder("a.lzh>", &pFolder);         //アーカイブ(相対パス)
      例6 ：OpenFolder("a.lzh>b/", &pFolder);       //アーカイブ内フォルダ(相対パス)
      例7 ：OpenFolder("C:/a.lzh>b.zip>", &pFolder);//アーカイブ内アーカイブ(絶対パス)
      例8 ：OpenFolder("http://www.hoge.com/", &pFolder);//http    #現在未実装
*/
};
///////////////////////////////////////////////////////////////////////////////
//タグ取得関係
///////////////////////////////////////////////////////////////////////////////
enum{//IKpiTagInfo::GetTagInfo の dwType に渡す値
    KPI_TAGTYPE_NONE = 0, //本体が持っているタグ取得ルーチンは使わないことを通知
    KPI_TAGTYPE_ID3V2,    //ID3Tag(v2.2/2.3/2.4)(v1 は取得しない)
    KPI_TAGTYPE_APE,      //APETag(v2 がなければ v1 を取得)
    KPI_TAGTYPE_PSF,      //PSFTag
    KPI_TAGTYPE_ASF,      //ASFTag(WindowsMedia)
    KPI_TAGTYPE_RIFF,     //RIFF
    KPI_TAGTYPE_MP4,      //MP4(未実装)
    KPI_TAGTYPE_LYR3TAG,  //Lyrics3Tag
    KPI_TAGTYPE_ID3V1,    //ID3Tag(v1)
    //KPI_TAGTYPE_AUTO = (DWORD)-1, //拡張子を元に自動判別してタグ取得(↑で未定義の形式も含む)
                          //提供されない。GetTagInfo を1度も呼ばなければ Select 後に
                          //自動で呼び出される。
    //本体が自動で取得する場合、複数のタグ形式を含む場合は
    //値が小さいものを優先(MP3 なら ID3v2 => Ape => RIFF => Lyrics3Tag => ID3v1)
};
enum{//IKpiTagInfo::GetTagInfo の dwTagGetFlags に渡す値
    KPI_TAGGET_FLAG_NONE = 0,          //タグ取得を行わない
    KPI_TAGGET_FLAG_TITLE  = 0x01,     //タイトルを取得
    KPI_TAGGET_FLAG_ARTIST = 0x01 << 1,//アーティストを取得
    KPI_TAGGET_FLAG_ALBUM  = 0x01 << 2,//アルバム
    KPI_TAGGET_FLAG_GENRE  = 0x01 << 3,//ジャンル
    KPI_TAGGET_FLAG_DATE   = 0x01 << 4,//年
    KPI_TAGGET_FLAG_COMMENT= 0x01 << 5,//コメント
    KPI_TAGGET_FLAG_TRACK  = 0x01 << 6,//トラック番号/数、ディスク番号/数(個別に指定は不可)
    KPI_TAGGET_FLAG_ALBMART= 0x01 << 7,//アルバムアーティスト
    KPI_TAGGET_FLAG_COPYRIGHT= 0x01 << 8,//著作権
    KPI_TAGGET_FLAG_RG     = 0x01 << 9,  //リプレイゲイン各種(個別に指定は不可)
    KPI_TAGGET_FLAG_PICTURE = 0x01 << 10,//アルバムアート(埋め込み画像)
    //
    KPI_TAGGET_FLAG_GENERAL= 0x01 << 31, //その他全般(プラグインが決める)
    //KPI_TAGGET_FLAG_GENERAL がセットされない場合、一般的でないフィールドは取得しない
    //他は後で決める(全部で31個使えるので十分?)
    //
    KPI_TAGGET_FLAG_ID3 = KPI_TAGGET_FLAG_TITLE   |
                          KPI_TAGGET_FLAG_ARTIST  |
                          KPI_TAGGET_FLAG_ALBUM   |
                          KPI_TAGGET_FLAG_GENRE   |
                          KPI_TAGGET_FLAG_DATE    |
                          KPI_TAGGET_FLAG_COMMENT |
                          KPI_TAGGET_FLAG_TRACK   |
                          KPI_TAGGET_FLAG_ALBMART, //ID3タグ全般を取得
    KPI_TAGGET_FLAG_ALL = (DWORD)-1,//全ての情報を取得
};

class IKpiTagInfo : public IKpiUnknown
{/* タグ取得用
    実装するのは本体側
    プラグイン側は呼び出すだけ
    cszName/cszValue の終端が
     0 で終わる場合は     nNameSize/nValueSize に -1 を渡すこと
     0 で終わらない場合は nNameSize/nValueSize にそれぞれのバッファサイズを
                          渡すこと(文字数ではないことに注意)
    最も実装しやすいメソッドを呼べば良く、混在して呼び出しても構わない
    KbMedia Player は name/value ともに utf16 で管理しているため、
    wSetValueW が最も効率的に動作する(選べるなら wSetValueW で呼ぶことを推奨)
    value が面倒でも name だけでも選べるなら wSetValueX を推奨

    定義済みの name については kmp_pi.h を参照
*/
public:
    virtual DWORD WINAPI GetTagInfo(IKpiFile *pFile, IKpiFolder *pFolder,
                                        DWORD dwType, DWORD dwTagGetFlags)=0;//本体が持っているタグ取得ルーチンを呼び出す
    virtual DWORD WINAPI GetValue(const wchar_t *cszName, wchar_t *pszValue, int nSize)=0;
    virtual void WINAPI SetOverwrite(BOOL bOverwrite)=0;//上書きモードの切り替え
    virtual void WINAPI SetPicture(DWORD dwType,                 //ID3v2 の "APIC" 互換(0～20)(不明の場合は 0 or 3(Cover (front)))
                                   const wchar_t *cszType,       //"" の場合は dwType から本体が文字列生成(0="Other", 3="Cover (front)", etc...)
                                   const wchar_t *cszMIMEType,   //"image/jpeg", "image/png", "image/gif", "image/bmp" ("" の場合は自動判別)
                                   const wchar_t *cszDescription,//画像の表示名
                                   DWORD dwWidth, DWORD dwHeight,//不明な場合は0
                                   const BYTE *pPictureData, DWORD dwSize)=0;
    //Name=Ansi 版
    virtual void WINAPI aSetValueA(const char    *cszNameA,   int nNameSize,    //Name: Ansi
                                   const char    *cszValueA,  int nValueSize)=0;//Value:Ansi
    virtual void WINAPI aSetValueW(const char    *cszNameA,   int nNameSize,    //Name: Ansi
                                   const wchar_t *cszValueW,  int nValueSize)=0;//Value: Utf16
    virtual void WINAPI aSetValueU8(const char   *cszNameA,   int nNameSize,    //Name: Ansi
                                    const char   *cszValueU8, int nValueSize)=0;//Value: Utf8
    //Name=utf16 版
    virtual void WINAPI wSetValueA(const wchar_t  *cszNameW,   int nNameSize,    //Name: utf15
                                   const char     *cszValueA,  int nValueSize)=0;//Value; Ansi
    virtual void WINAPI wSetValueW(const wchar_t  *cszNameW,   int nNameSize,    //Name: Utf16
                                   const wchar_t  *cszValueW,  int nValueSize)=0;//Value: Utf16
    virtual void WINAPI wSetValueU8(const wchar_t *cszNameW,   int nNameSize,    //Name; utf16
                                    const char    *cszValueU8, int nValueSize)=0;//Utf8: utf8
    //Name=utf8 版
    virtual void WINAPI u8SetValueA(const char    *cszNameU8,  int nNameSize,    //Name: utf8
                                    const char    *cszValueA,  int nValueSize)=0;//Value: Ansi
    virtual void WINAPI u8SetValueW(const char    *cszNameU8,  int nNameSize,    //Name; utf8
                                    const wchar_t *cszValueW,  int nValueSize)=0;//Value: Utf16
    virtual void WINAPI u8SetValueU8(const char   *cszNameU8,  int nNameSize,    //Name: utf8
                                     const char   *cszValueU8, int nValueSize)=0;//Value: Utf8
/*
    DWORD WINAPI GetTagInfo(IKpiFile *pFile, IKpiFolder *pFolder,
                                  DWORD dwType, DWORD dwTagGetFlags);

      扱っているデータのタグ形式が本体が既知のものである場合、本体が持つタグ取得
      ルーチンを使うことが出来る。
      プラグインが独自に取得した情報を通知する場合は xSetValueX/SetPicture を呼び出す。
      GetTagInfo で本体が取得した情報とプラグインが通知した情報のどちらが
      適用されるかは上書きモード(SetOverWrite で設定)と呼び出しの順序で決まる。

      本体のタグ取得ルーチンを全く使わない場合は dwType に KPI_TAGTYPE_NONE を
      渡して GetTagInfo を呼び出すこと。(呼ばないと Select から戻った後に本体が
      タグ取得を行う)

      pFile/pFolder:
        通常は IKpiDecoderModule::Open で渡された IKpiFile/IKpiFolder をそのまま渡す。
        呼び出し前にファイルポインタを先頭に移動する必要はなく、呼び出し前後で
        ファイルポインタも変わらない。

        pFile をプラグインが独自に実装する場合、Read と Seek だけ実装すれば十分。
        その他のメソッド(GetBuffer/GetSize/CreateClone)を実装しても本体が呼び出す
        ことはない。

        プラグインが独自に実装する必要が生じることは少ない筈だが、kbtak.kpi が該当
        している。実装の必要がある場合は kbtak.kpi のソースコードを参考にすること。

        現在本体が対応している dwType で pFolder が必要なタグ形式はないため、
        pFolder に NULL を渡しても良い。pFolder が必要なタグ形式に対応した場合は
        その形式については渡すようにすること。pFolder が必要な形式で NULL を渡
        しても動作はするが正しい情報を取得することは出来ない。

        GetTagInfo は IKpiDecoder::Select/IKpiPlayer::Select 内で呼び出すもので
        あるため、呼び出しの時点ではデコード/演奏をまだ開始していない。
        そのため、デコードライブラリがデコード前の段階でもバックグラウンドでスレッドを
        作成して pFile にアクセスするような実装になっていない限り、Open 時の IKpiFile
        をそのまま渡しても問題は起こらない。

        もしデコードライブラリの実装上、GetTagInfo の呼び出し中に pFile を
        別スレッドで使用する場合は、CreateClone したものを渡すこと。
        (そのようなライブラリは少ないと思うが)

      dwType: タグの形式
        KPI_TAGTYPE_NONE : 本体が持っているタグ取得ルーチンを使わないことを通知
                           (pFile/pFolder/dwTagGetFlags は無視される
        KPI_TAGTYPE_ID3V2: ID3Tag(v2.2/2.3/2.4)(v1 は取得しない)
                           (pFolder は無視される)
        KPI_TAGTYPE_APE  : APETag(v2 がなければ v1 を取得)
                           (pFolder は無視される)
        KPI_TAGTYPE_PSF  : PSFTag
                           (pFolder は無視される)
        KPI_TAGTYPE_ASF  : ASFTag(WindowsMedia)
                           (pFolder は無視される)
        KPI_TAGTYPE_RIFF : RIFF
                           (pFolder は無視される)
        KPI_TAGTYPE_MP4  : MP4(未実装)((DWORD)-1 を返す)
                           (pFolder は無視される)
        KPI_TAGTYPE_LYR3TAG: Lyrics3Tag
                           (pFolder は無視される)
        KPI_TAGTYPE_ID3V1:   ID3Tag(v1)
                           (pFolder は無視される)
        KPI_TAGTYPE_AUTO ; 拡張子を元に自動判別してタグ取得(↑で未定義の形式も含む)
                           提供されない。Select から戻るまでに GetTagInfo を
                           1 度も呼ばなければ自動で呼び出される。

      タグの形式が複数種類ある場合は GetTagInfo を複数回呼び出す。
      dwType に応じたタグ取得に成功した(指定された形式のタグ情報を含んでいた場合)
      は 0 を返す。タグ形式が複数ある場合、0 を返却後は優先順位が低い形式では
      GetTagInfo を呼ばないようにすることでタグ取得を省略することが出来る。

      タグ形式がない場合は 1 を返す。指定の dwType に本体が未対応(未実装)の場合
      は (DWORD)-1 を返す。dwType が KPI_TAGTYPE_NONE の場合は常に 1 を返す。

      GetTagInfo が呼ばれなかった場合、本体は IKpiDecoder::Select から
      戻った後、データの拡張子を元にしたデフォルトのタグ取得を試みる。Select
      から戻った後本体にタグ取得させない場合は GetTagInfo を呼び出すこと。
      KPI_TAGTYPE_NONE 以外が渡された場合も、本体は Select から戻った後は
      タグ取得しない。

      KPI_TAGTYPE_AUTO は提供されない。Select から戻るまでに GetTagInfo が
      1度も呼ばれなければ Select 後に相当するルーチンが自動で呼び出されるが、
      Select から戻る前に明示的に呼び出すことは出来ない。プラグイン自身がタグの
      形式とその構造を知っている場合は、KPI_TAGTYPE_AUTO には頼らず、明示的に
      KPI_TAGTYPE_ID3 等の具体的な形式を渡すか、KPI_TAGTYPE_NONE を渡してプラグ
      イン自身で取得すること。

      dwTagGetFlags: 取得するタグのフィールド

      KPI_TAGGET_FLAG_TITLE/ARTIST/ALBUM etc の組み合わせ。
      通常は IKpiDecoder::Select の dwTagGetFlags をそのまま渡すか、その値から
      不要なフィールドを除いたものを渡す。

      例えばタイトルはプラグイン独自に取得する場合は dwTagGetFlags から
      KPI_TAGGET_FLAG_TITLE を除いて(dwTagGetFlags & ~KPI_TAG_FLAG_TITLE)
      から呼び出すか、呼び出し後に SetOverwrite(TRUE) してから xSetValueX する。

      不要なフィールドをフラグから外しても、形式や本体のバージョンによっては
      外したフィールドの情報も取得してしまうことがある。そのためフラグの除去
      は参考程度にとどめ、不要なフィールドを確実にプラグイン独自で取得した値
      に設定したい場合は GetTagInfo 呼び出し後に SetOverwrite(TRUE)
      してからそのフィールドの値を xSetValueX すること。

      KPI_TAGGET_FLAG_XXX で定義されないフィールドの値を取得したい場合は
      dwTagGetFlags に KPI_TAGGET_FLAG_GENERAL をセットすること。

    DWORD WINAPI GetValue(const wchar_t *cszName, wchar_t *pszValue, int nSize);

      cszName に相当するフィールドの値を取得する。
      GetTagInfo 呼び出し後に本体が取得した値をプラグインが使いたい場合
      に呼び出す。プラグインが自身で xSetValueX したデータも取得出来る。
      nSize は pszValue のバッファサイズ。(文字数単位ではない)
      戻り値は pszValue へのコピーに必要なバッファのサイズ。サイズが足りない
      場合は切り捨てられる。

      リプレイゲインも取得出来るが、リプレイゲインの処理は本体が行うため、
      プラグインで処理するべきではない。

      フィールド名の列挙は出来ないため、取得し得るフィールド名が予め分かって
      いる場合にしか使えない。(キリがないので対応は見合わせた)

    void WINAPI SetOverwrite(BOOL bOverwrite);

      上書きモードの切り替え。
      取得済みのフィールド名に対して xSetValueX/SetPicture したとき、
      上書きモードが TRUE の場合、後から通知した方が適用される。
      上書きモードが FALSE の場合、先に通知した方が適用される。

      IKpiDecoder::Select 呼び出し直後の上書きモードは FALSE である。
      従って GetTagInfo 後に SetOverwrite せずにプラグイン独自の情報を
      通知しても上書きされないことに注意。

      例えばタグの形式が ID3v2 の場合、GetDefaultTagInfo(KPI_TAGTYPE_ID3) を
      呼び出せば、ID3v2 にリプレイゲイン情報が含まれていれば本体がそれを取得する。
      扱っている形式が ID3v2 とは別に本体が知らないリプレイゲイン情報を含み、それは
      ID3v2 よりも優先度を低くしたいという状況では、

      GetTagInfo(pFile, pFolder, KPI_TAGTYPE_ID3, dwTagGetFlags);
      SetOverwrite(FALSE);//初期値が FALSE なので実際は呼び出し不要
      wSetValueW(L"replaygain_track_gain", ...");

      のようにすれば、ID3v2 にリプレイゲイン情報が含まれていない場合には
      プラグインが通知した情報が適用され、ID3v2 に含まれている場合は ID3v2 の方
      が適用される。この例で SetOverwrite に TRUE を渡した場合はプラグインが
      通知した方が適用される。

      SetOverwrite は呼び出し後の全ての xSetValueX/SetPicture に対して適用される。
      途中で上書きモードを切り替えることも出来る。
      先に xSetValueX/SetPicture してから SetOverwrite して GetTagInfo
      することも出来る。

      Select から戻るまでに GetTagInfo が 1 度も呼ばれない場合、その後の
      本体のタグ取得は Select から戻る直前の上書きモードで処理される。

    void WINAPI SetPicture(DWORD dwType,                 //ID3v2 の "APIC" 互換(0～20)(不明の場合は 0 or 3(Cover (front)))
                           const wchar_t *cszType,       //"" の場合は dwType から本体が文字列生成(0="Other", 3="Cover (front)", etc...)
                           const wchar_t *cszMIMEType,   //"image/jpeg", "image/png", "image/gif", "image/bmp" ("" の場合は自動判別)
                           const wchar_t *cszDescription,//画像の表示名
                           DWORD dwWidth, DWORD dwHeight,//不明な場合は0
                           const BYTE *pPictureData, DWORD dwSize)=0;

      画像(アルバムアート)を通知する。基本的に ID3v2 の "APIC" 互換である。

      dwType: 画像の種類。ID3v2 の "APIC" 互換の値を渡す。
              cszType != "" の場合は無視される。

         0: "Other"
         1: "32x32 pixels 'file icon' (PNG only)"
         2: "Other file icon"
         3: "Cover (front)"
         4: "Cover (back)"
         5: "Leaflet page",
         6: "Media (e.g. label side of CD)",
         7: "Lead artist/lead performer/soloist",
         8: "Artist/performer",
         9: "Conductor",
        10: "Band/Orchestra",
        11: "Composer",
        12: "Lyricist/text writer",
        13: "Recording Location",
        14: "During recording",
        15: "During performance",
        16: "Movie/video screen capture",
        17: "A bright coloured fish",
        18: "Illustration",
        19: "Band/artist logotype",
        20: "Publisher/Studio logotype"

        本体が未知(0～20 の範囲外)の値が渡された場合は 0("Other") と見なす。

      cszType: 画像の種類名。"" の場合は dwType に応じた種類名になる。
               NULL を渡してはいけない。

      cszMIMEType: MIME のタイプ。画像の形式。NULL を渡してはいけない。
        対応しているのは以下の通り。

        "image/jpeg" : JPEG 形式 ("image.jpg" も受け付けるが推奨しない)
        "image/png"  : PNG 形式
        "image/gif"  : GIF 形式
        "image/bmp"  : BMP 形式

        cszMIMEType に "" を渡した場合は本体が画像の形式を自動判別する。
        本体が未知の文字列が渡された場合は、画像の表示に失敗する。

      cszDescription: 画像の表示名。ない場合は "" を渡すこと。
                      NULL を渡してはいけない。

      dwWidth/dwHeight : 画像データの横幅と縦幅。不明な場合は 0 を渡す。

      pPictureData, dwSize: 画像データとそのサイズ

      音楽データと同一フォルダ内にある画像は本体が自動で読み取るため、
      通知する必要はない。

    void WINAPI xSetValueX(const char/wchar_t *cszName,  int nNameSize,
                           const char/wchar_t *cszValue, int nValueSize);

      cszName/cszValue の文字列の終端が 0 で終わる場合は nNameSize/nValueSize=-1
      とすること。終端が 0 で終わらない場合はバッファサイズ(文字数単位ではない)を渡すこと。
      cszName が重複した場合、上書きモードによって上書きされるか無視されるかが決まる。

      旧仕様の IKmpTagInfo::SetValue との互換性から、cszName/cszValue ともに
      NULL を渡した場合には GetTagInfo(NULL, NULL, KPI_TAGTYPE_NONE, 0);
      を呼び出すのと同じ効果がある。(Select から戻った後に本体がタグ取得しなくなる)
*/
};
///////////////////////////////////////////////////////////////////////////////
//設定関係
///////////////////////////////////////////////////////////////////////////////
enum{//KPK_CFG_INFO::dwType(設定項目の型)
    KPI_CFG_TYPE_BOOL,   //bool(表示は false/true, データは 0/1)
    KPI_CFG_TYPE_INT,    //INT64
    KPI_CFG_TYPE_FLOAT,  //double
    KPI_CFG_TYPE_STR,    //文字列
    KPI_CFG_TYPE_BIN,    //バイナリ(設定画面はプラグインが独自に作成)
    KPI_CFG_TYPE_FILE,   //ファイル名(ファイル選択ダイアログを表示)
    KPI_CFG_TYPE_FOLDER, //フォルダ名(フォルダ選択ダイアログを表示)(末尾に \ 付加)
    KPI_CFG_TYPE_EXT,    //拡張子('.'を含み、'/' で区切られた文字列)(例：".mp1/.mp2/.mp3")
};
enum{//IKpiXXXModule::ApplyConfig() の戻り値(設定が反映されるタイミング)
    KPI_CFGRET_OK,          //設定は即座に反映される
    KPI_CFGRET_NOTIFY,      //デコーダインスタンス等への変更通知が必要(IKpiXXX::Update()が呼ばれたら反映)
    KPI_CFGRET_RELOAD_DATA, //データの再ロードが必要(次にIKpiXXXModule::Open()が呼ばれた分から反映)
    KPI_CFGRET_RELOAD_KPI,  //プラグインの再ロードが必要
    KPI_CFGRET_REBOOT_PLAYER,//プレイヤー本体の再起動が必要(特別な事情がない限り
                             //RELOAD_KPI で足りる筈だが)
    KPI_CFGRET_REBOOT_OS,    //OS の再起動が必要
    KPI_CFGRET_UNKNOWN = (DWORD)-1
                             //不明(外部プログラムを呼び出す等、設定関係の依存
                             //コードの詳細が分からない等)
};
enum{//セクション名/キー名/値の最大長(文字数単位、終端の 0 を含む)
    KPI_MAX_SECTION = 1024,//セクション名
    KPI_MAX_KEY = 1024,    //キー名
    KPI_MAX_VALUE = 4096   //値
    //セクション名とキー名は特別な事情(プログラム的に生成される等)がない限り、
    //常識的な長さに抑えるようにすること
};
struct KPI_CFG_SECTION
{//IKpiConfigEnumerator::EnumSection に渡す構造体
 //セクションの表示名とヘルプ(対応しない場合は不要)
    const wchar_t *cszSection;//セクション名(マルチバイト文字非推奨だがやむを得ない場合は可)
    const wchar_t *cszSecDesc;//セクションの表示名(NULL の場合は cszSection と同じ)
    const wchar_t *cszSecHelp;//セクションに関するヘルプ(NULL の場合は非表示)
/*
    各種文字列は本体によってコピーされるため、静的なバッファに持つ必要はない
    (EnumSection に渡す前に動的に文字列生成、戻ったら解放しても問題なし)

    EnumKey で列挙されるセクションに表示名とヘルプも追加する場合に必要
    (表示名がセクション名と同じでヘルプがない場合は不要)

    cszSection:
      ・セクション名(マルチバイト非推奨)
      ・NULL と "" は使用不可(特にないなら "General" やプラグインのファイル名等を使う)
      ・マルチバイト文字非推奨。使うならなるべく cszSecDesc の方にする。
      ・ただし、デバイスプラグインではセクション名を
        「デバイス名-識別子」
        (例：プライマリ サウンド ドライバー-{00000000-0000-0000-0000-000000000000})
        とした場合は本体がデバイス用セクションと判断するため、使用しても良い。
      ・デバイス用セクションのように、名前をプログラム的に取得する等、やむを得ない
        場合はマルチバイト文字を使用しても良い。
      ・セクション名の文字列の長さは KPI_MAX_SECTION 以内に抑えること。
    cszSecDesc:
      ・セクションの表示名
      ・設定画面上は cszSection ではなく cszSecDesc の方が表示される。
      ・NULL or "" の場合は cszSection と同じと見なされる。
    cszSecHelp:
      ・セクションのヘルプ(NULL or ""の場合は非表示)

    IKpiConfig に対応していても、IKpiConfigEnumerator::EnumSection の呼び出し
    は必須ではない。(EnumKey の呼び出しは必須)
    EnumSection が呼ばれない場合、cszSecDesc/cszSecHelp ともに NULL と
    見なされる。(表示名がセクション名と同じでヘルプがないと見なされる)
    従って、cszSecDesc/cszSecHelp ともに NULL or "" なら EnumSection を呼ぶ
    必要はない。

    EnumKey で列挙されないセクション名を EnumSection で列挙した場合、
    設定画面上はセクションが認識されるが設定項目がない状態となる。
    (なので推奨しない)
*/
};
struct KPI_CFG_KEY
{//IKpiConfigEnumerator::EnumKey に渡す構造体
 //各種設定のセクション名やキー名、初期値、範囲
    DWORD dwType;             //型(KPI_CFG_TYPE_BOOL/INT etc...)
    const wchar_t *cszSection;//セクション名(表示名とヘルプは kPI_CFG_SECTION を参照)
    const wchar_t *cszKey;    //キー名(マルチバイト文字非推奨)
    const wchar_t *cszKeyDesc;//キーの表示名(NULL の場合は cszKey を表示)
    const wchar_t *cszDefault;//初期値(値を文字列化)("1", "256", "3.14", "abcd")
    const wchar_t *cszMin;    //最小値
    const wchar_t *cszMax;    //最大値(文字列の場合は最大長(終端の0を含む)(文字数単位))
    const wchar_t *cszList;   //値のリストをタブ文字\tで区切って文字列化("1\t3\t5", "abc\tabcd\tabc")
    const wchar_t *cszListDesc;//値のリストの表示名
    const wchar_t *cszHelp;   //ヘルプ(NULL の場合は非表示)
/*
    各種文字列は本体によってコピーされるため、静的なバッファに持つ必要はない
    (EnumKey に渡す前に動的に文字列生成、戻ったら解放しても問題なし)
    NULL と "" は同じと見なす
    NULL と "" は BOOL/INT/FLOAT で数値化出来ない値と見なす(0ではない)
    dwType:
      ・KPI_CFG_TYPE_BOOL の場合は cszMin/cszMax/cszList は無視される(NULL にすること)
        表示上の値は false か true だが、データ上は 0 か 1 となる
        (cszDefault もそれに合わせて "0" か "1" を渡すこと)
      ・KPI_CFG_TYPE_STR の場合は cszMin は無視される(NULL にすること)
      ・KPI_CFG_TYPE_EXT の場合は cszMin は無視される(NULL にすること)
        拡張子は '.' を含み、'/' で区切られた文字列(例：".mp1/.mp2/.mp3")
        KPI_DECODER_MODULEINFO::cszSupportExts/cszMultiSongExts 等にそのまま渡すことが出来る
        文字列は小文字に変換され、拡張子の重複チェックも行われる
      ・KPI_CFG_TYPE_BIN の場合は cszDefault/cszMin/cszMax/cszList/cszListDesc は無視される(NULL にすること)
        本体が引数として cszSection と cszKey を渡して  ApplyConfig() を呼び出すので、
        そこでプラグインが独自のダイアログを表示して設定する
        設定の保存場所を本体に管理させる場合は SetBin/GetBin を呼び出すこと
    cszSection:
      ・セクション名(マルチバイト非推奨)
      ・NULL と ""は使用不可(特にないなら "General" やプラグインのファイル名等を使う)
      ・セクション名の大文字と小文字は区別しない("General" と "general" は同じ扱い)
      ・セクション名の先頭と末尾に空白を入れたり、名前に改行・タブ文字その他の制御文字を使用することは禁止
        (キー名と異なり '=', '[', ']' は使用可能だが非推奨)
      ・デバイスプラグインではデバイス名と識別子を元にしたセクションを作成し、
        デバイスごとに設定出来るようにすると良い
        デバイス名-識別子(例：プライマリ サウンド ドライバー-{00000000-0000-0000-0000-000000000000})
        という名前のセクションは該当デバイス用のセクションと見なされ、必要に
        応じて設定画面上でセクションが自動選択される
      ・複数の形式や拡張子に対応するプラグインでは形式ごとにセクションを作成し、
        形式ごとに設定出来るようにすると良い
      ・セクションの表示名とヘルプにも対応したい場合は KPI_CFG_SECTION を参照
    cszKey:
      ・キー名(マルチバイト文字非推奨)
      ・NULL/空文字列は使用不可
      ・キー名の大文字と小文字は区別しない("Key" と "key" は同じ扱い)
      ・キー名の先頭と末尾に空白を入れたり、名前に '=', '[', ']', 改行・タブ文字その他の制御文字を使用することは禁止
      ・重複不可(後から列挙された方を採用)
    cszKeyDesc:
      ・キーの表示名(保存は cszKey で管理)
      ・NULL の場合は cszKey を表示
      ・cszKey が異なっていれば cszKeyDesc の重複も可能だが、設定画面上での区別
        がしづらいため(同一セクション内では)非推奨
    cszDefault:
      ・NULL 等、BOOL/INT/FLOAT で数値化出来ない場合は "0" と見なす
      ・BOOL の表示上の値は false/true だが、データ上は 0 か 1 となるため、
        cszDefault もそれに合わせて "0" か "1" を渡すこと
    cszMin/cszMax:
      ・NULL 等、INT で数値化出来ない場合は cszMin では _I64_MIN, cszMax では _I64_MAX と見なす
      ・NULL 等、FLOAT で数値化出来ない場合は cszMin では -DBL_MAX, cszMax では DBL_MAX と見なす
      ・BOOL では cszMin/cszMax は無視される(NULL にすること)
      ・STR では cszMin は無視される(NULL にすること)
                 cszMax は文字列の最大長(文字数単位)と見なす(終端の0を含む)
      ・FILE の場合は
          cszMin は GetOpenFileName の OPENFILENAME 構造体に渡す拡張子のフィルタ
          (OPENFILENAME::lpstrFilter に相当するが、区切りは \0 ではなくて \t とすることに注意)
          例0:"表示文字列1\t拡張子のリスト1\t表示文字列2\t拡張子のリスト2\t..."
          例1:"実行ファイル(*.exe)\t*.exe"
          例2:"Txt/Doc files(*.txt;*.doc)\t*.txt;*.doc\tTxt files(*.txt)\t*.txt\tDoc files(*.doc);*.doc")
          cszMax は文字列の最大長(文字数単位)(終端の0を含む)
      ・FOLDER の場合は cszMin は無視される
                        cszMax は文字列の最大長(文字数単位)(終端の0を含む)
      ・EXT の場合は cszMin は無視される
                     cszMax は文字列の最大長(文字数単位)(終端の0を含む)
      ・cszList!=NULL で cszMin/cszMax がともに NULL の場合は cszList 内の値以外は使用不可
      ・cszList!=NULL で cszMin/cszMax のどちらかが 非NULL の場合は cszList 内の値以外も使用可能
    cszList:
      ・cszList!=NULL の場合は値をコンボボックスで選択可能になる
      ・文字列をタブ文字(\t)で区切る
        例1: cszList = "1\t10\t100" だと 1, 10, 100 を選択可能になる
        例2: cszList = "abc\tdef\tghi" だと abc, def, ghi を選択可能になる
        例3: cszList = "\tabc\tdef" だと abc, def に加えて空文字列も選択可能になる
      ・cszList=NULL にすると cszDefault が選択可能になる
      ・cszMin/cszMax が共に NULL の場合はリスト内の値以外は使用不可
        (リスト以外の値が入力されたら cszDefault になる)
      ・cszMin/cszMax のどちらかが非 NULL の場合は cszList 内の値以外も使用可能
    cszListDesc:
      ・値の表示名のリスト。cszList よりも数が少ない場合、足りない分の表示は cszList と同じ
        例1：cszList="1\t2\t3", cszListDesc="値1\t値2\t値3" の場合、設定画面上では
            値1, 値2, 値3 と表示されるが、データ上の値は 1, 2, 3 となる
        例2：cszList="0\t128\t256", cszDescList="ゼロ" の場合、設定画面上の表示は
            ゼロ, 128, 256 となり、データ上の値は 0, 128, 256 となる
      ・CFG_TYPE_BOOL は cszType=CFG_TYPE_INT, cszMin=NULL, cszMax=NULL,
                         cszList="0\t1", cszDescList="false\ttrue" と同等
      ・NULL の場合は cszList が使われる
    cszHelp:
      ・非NULL の場合はヘルプを表示
*/
};
///////////////////////////////////////////////////////////////////////////////
class IKpiConfigEnumerator : public IKpiUnknown
{//各種設定項目の情報を列挙
 //本体が実装。プラグイン側は呼び出すだけ
public:
    virtual BOOL WINAPI EnumSection(const KPI_CFG_SECTION *pSection)=0;
    virtual BOOL WINAPI EnumKey(const KPI_CFG_KEY *pKey)=0;
/*
    BOOL WINAPI EnumSection(const KPI_CFG_SECTION *pSection);
      EnumKey で列挙するセクションの表示名とヘルプが必要な場合は呼び出す。
      EnumSection を呼ばない場合、セクションの表示名は KPI_CFG_KEY::cszSection
      そのものとなり、セクションのヘルプは表示されない。
      同じ名前のセクションを列挙した場合、後から呼び出した方が適用される。
      セクションの表示名とヘルプに対応しない場合は呼び出さなくて良い。
      pSection 内の各種文字列(cszXXX) の内容は EnumSection 内部でコピーされるため、
      呼び出し後に文字列を変更・破棄をしても構わない。

    BOOL WINAPI EnumKey(const KPI_CFG_KEY *pKey);
      各種設定項目の名前と型、値の取り得る範囲を本体に通知。
      通知された情報を元に本体が設定画面を自動生成。
      自動生成される画面が気に入らない場合はプラグイン側で独自に作成すること。
      最初に通知されたものから順に表示する。
      dwType == KPI_CFG_TYPE_BIN であるものが通知された場合、
      IKpiXXXModule::ApplyConfig で設定画面を呼び出して設定を行う。
      列挙された型と範囲を本体が受け入れた場合は TRUE を返す。
      pKey 内の各種文字列(cszXXX) の内容は EnumKey 内部でコピーされるため、
      呼び出し後に文字列を変更・破棄をしても構わない。

    セクションは EnumKey/EnumSection で先に呼び出したものから表示される。

    EnumSection を複数回呼び出した場合、先に呼び出した方のセクションから表示
    される。重複するセクションを列挙した場合、後から列挙した方が適用される。

    EnumKey を複数回呼び出した場合、先に呼び出した方のセクション/キーから
    表示される。重複するセクション/キーを列挙した場合、後から列挙した方が
    適用される。

    EnumSection と EnumKey はどちらを先に呼び出しても構わないが、本体の実装上、
    EnumSection を先に呼び出した方が、わずかだが効率が良くなる。

    EnumSection で列挙してるのに EnumKey では列挙されないようなセクションは
    推奨しない。

    IKpiConfigEnumerator のメソッドは IKpiXXXModule::EnumConfig() 関数内以外で
    呼び出してはいけない。(AddRef しておいて別のメソッド内で呼ぶなど)
*/
};
///////////////////////////////////////////////////////////////////////////////
class IKpiConfig : public IKpiUnknown
{//プラグインの設定
 //本体が実装。プラグイン側は呼び出すのみ。
public:
    //int(INT64)
    virtual void   WINAPI SetInt(const wchar_t *cszSection, const wchar_t *cszKey, INT64  nValue)=0;
    virtual INT64  WINAPI GetInt(const wchar_t *cszSection, const wchar_t *cszKey, INT64  nDefault)=0;
    //float(double)
    virtual void   WINAPI SetFloat(const wchar_t *cszSection, const wchar_t *cszKey, double dValue)=0;
    virtual double WINAPI GetFloat(const wchar_t *cszSection, const wchar_t *cszKey, double dDefault)=0;
    //str(wchar_t*)
    virtual void   WINAPI SetStr(const wchar_t *cszSection, const wchar_t *cszKey, const wchar_t *cszValue)=0;
    virtual DWORD  WINAPI GetStr(const wchar_t *cszSection, const wchar_t *cszKey, wchar_t *pszValue,
                                 DWORD dwSize, const wchar_t *cszDefault)=0;
    //binary
    virtual void   WINAPI SetBin(const wchar_t *cszSection, const wchar_t *cszKey, const BYTE *pBuffer, DWORD dwSize)=0;
    virtual DWORD  WINAPI GetBin(const wchar_t *cszSection, const wchar_t *cszKey, BYTE *pBuffer, DWORD dwSize)=0;
/*
    IKpiConfig のインスタンスは、プラグインがエクスポートする関数

    HRESULT WINAPI kpi_CreateInstance(REFIID riid,
                                      void **ppvObject,
                                      IKpiUnknown *pUnknown);

    の第3引数(pUnknown) から取得する。通常は kpi_CreateConfig ヘルパー関数を
    使用する。詳細は IKpiUnkProvider を参照。

    IKpiConfig に対応する場合、kpi_CreateConfig による IKpiConfig のインスタンス
    取得は、遅くとも kpi_CreateInstance 内で実行すること。
    kpi_CreateInstance から戻った段階で未だ kpi_CreateConfig が呼ばれていない
    場合、設定の保存を本体に管理させることは出来ない。
    (EnumConfig/ApplyConfig は有効だがバイナリ以外の型は扱えない)


    BOOL/INT では SetInt/GetInt で設定/取得する。
    FLOAT では SetFloat/GetFloat で設定/取得する。
    STR/FILE/FOLDER/EXT では SetStr/GetStr で設定/取得する。
    BIN では SetBin/GetBin で設定/取得する。

    SetBool/GetBool は提供されない。
      型が KPI_CFG_TYPE_BOOL である場合のデータ上の値は 0 / 1 である。(false / true ではない)
      GetInt の戻り値が 0 なら false, 0 以外なら true として処理すること。

    Set したのとは異なる型で Get した場合は型変換される。(SetBin/GetBin は除く)
      例えば SetInt/SetFloat したキーに対して GetStr した場合は、文字列化された
      値が返る。

      SetFloat したキーに対して GetInt した場合は整数化(小数点以下切り捨て)
      された値が返る。

      SetStr したキーに対して GetInt/GetFloat した場合は数値化に成功すればその
      値が、失敗した場合はデフォルト値が返る。

    SetBin したキーに対して GetInt/GetFloat/GetStr した結果は未定義。
      現在の実装では base64 エンコードした文字列として設定を保存しているため、
      GetStr すると base64 エンコードされた文字列が返る。

    SetInt/SetFloat/SetStr したキーに対して GetBin した結果は未定義。
      SetInt/SetFloat/SetStr は文字列化して保存される。GetBin で取得する場合は
      データを base64 エンコードされた文字列と見なすため、通常はデコードに失敗
      してゼロクリアされたデータが返るが、偶然デコードに成功してしまうことも
      理論的にはあり得る。

    GetStr:
      dwSize は pszValue のバッファサイズ(文字数単位ではない)
      文字列の格納に必要なバッファのサイズを返す。
      バッファサイズが足りない場合、空文字列が入る。切り捨てではないことに注意。
      該当するセクション/キーがなかった場合は cszDefault が返されるが、キーが
      存在して値が空文字列の場合は空文字列が返ることに注意すること。

    SetBin:
      dwSize は格納すべきバイナリデータ(pBuffer)のバッファサイズ
      格納すべきバイナリデータの設定画面はプラグイン側で作成する必要がある。

    GetBin:
      dwSize は pBuffer のサイズ
      戻り値は格納に必要なバッファのサイズ
      バッファサイズが足りない場合、pBuffer は dwSize 分だけゼロクリアされる(足りない分の切り捨てではない)
      格納したバッファのサイズが dwSize よりも小さい場合、残りはゼロクリアされる。
      64bit 版と 32bit 版とでバイナリデータに互換性がない場合は、キー名を変える
      などしておくこと。


  注意事項：

    IKpiXXXModule::EnumConfig で列挙されない(自動生成の設定画面で設定出来ない)
    情報でも、SetXXX/GetXXX することが出来る。
    (最後に設定を保存したときのプラグインのバージョンとか)

    GetXXX で取得した値が範囲内に収まっていることをある程度前提として良いが、
    範囲外でも不正終了/暴走しないように最低限の配慮は必要。

    EnumConfig を元に値の範囲チェックを(プラグイン自身が)行って保存してはいる
    が、プラグインの設定保存場所は %APPDATA% 内の INI ファイルにあり、ユーザー
    が直接編集することが可能であるため、全面的に信頼するべきではない。

    IKpiConfig の各種メソッドは任意のスレッドから呼び出すことが出来るが、以下の
    条件を全て満たしている場合は取得した結果が正しくない場合がある。

    1.プラグインが提供するメソッドを本体が呼び出し中でない
    2.IKpiXXXModule が提供するインスタンスが１つも存在していない

    言い換えるとロードはしているがプラグインを本体が使用していない状態である。

    この状況でプラグインが GetXXX を呼び出すことは通常はない筈だが、
    IKpiXXXModule が内部で作成したスレッド等が呼び出す等があり得る。
    (バックグラウンドで設定を定期的に読み取る等)
    設定の適用時(適用直前)に IKpiXXXModule::ApplyConfig が呼ばれ、適用後に
    IKpiXXX::UpdateConfig が呼ばれるため、定期的に設定を取得しにいく必要は
    ない筈である。

    IKpiConfig はプラグインが動作している全てのプロセスで共有される。
    kbrunkpi 経由で動作している場合、IKpiConfig を頻繁に呼び出すと
    パフォーマンスが低下するため、呼び出しの頻度は極力抑えること。

    IKpiConfig の呼び出しのタイミングとして推奨されるのは、

    ・IKpiXXXModule の各種メソッド内(GetModuleInfo/Open 等)
    ・IKpiXXX::UpdateConfig() 内
    ・IKpiXXX の各種メソッドのうち、呼び出しの頻度が高くないもの
      (IKpiDecoder::Select/Seek は問題なし)
      (IKpiDecoder::Render はそれほど問題ないが UpdateConfig の方が望ましい)
      (IKpiPlayer::GetPosition/GetStatus は非推奨)
*/
};
///////////////////////////////////////////////////////////////////////////////
class IKpiUnkProvider : public IKpiUnknown
{//プラグインの使用側(通常は KbMedia Player 本体)がプラグインに対して
 //IKpiConfig や IKpiDecoder 等のインスタンスを提供
 //引数付 QueryInterface のようなもの
 //本体が実装、プラグイン側は使用するのみ
public:
    virtual DWORD WINAPI CreateInstance(REFIID riid,    //取得したいインターフェースの IID
                                        void *pvParam1, //引数1(riid によって変化)
                                        void *pvParam2, //引数2(riid によって変化)
                                        void *pvParam3, //引数3(riid によって変化)
                                        void *pvParam4, //引数4(riid によって変化)
                                        void **ppvObj)=0;
/*
    kpi_CreateInstance の第3引数に渡される pUnknown を QueryInterface すると
    得られる
    設定の管理を本体に任せる場合や、プラグインから別のプラグインを使用する場合
    等、本体が提供する機能をプラグイン側から使用したい場合に必要

    DWORD WINAPI CreateInstance(REFIID riid,    //取得したいインターフェースの IID
                                void *pvParam1, //引数1(riid によって変化)
                                void *pvParam2, //引数2(riid によって変化)
                                void *pvParam3, //引数3(riid によって変化)
                                void *pvParam4, //引数4(riid によって変化)
                                void **ppvObj);
      本体から IKpiConfig/IKpiDecoder 等を取得する。
      通常は kpi_CreateConfig/kpi_CreateDecoder 等のヘルパー関数内で呼び出される。
      直接は使用しない。
      引数付の QueryInterface のようなものだが、戻り値 0 は成功を意味するとは
      限らない。(HRESULT ではないので)
      通常は *ppvObj が NULL か非 NULL かで判断すること。

      戻り値と引数の意味は riid によって異なる。
      riid が未知の場合は何もしないで 0 を返す。

      kpi_CreateConfig の pdwPlatForm(pvParam2)のように、値の代入を期待される
      ポインタを渡す場合、渡された riid を IKpiUnkProvider が未知だと値が変更
      されない。(未知の場合でも *ppvObj には NULL が代入される)
      そのため、呼び出し前に何らかの値を代入しておくこと。

    以下のヘルパー関数が用意されている(使用する場合は kpi.cpp もプロジェクトに追加すること)

    DWORD WINAPI kpi_CreateConfig(IKpiUnkProvider *pProvider,
                                  const GUID *pGUID, //設定の保存場所を決める識別子
                                  DWORD *pdwPlatform,//本体が直接プラグインを呼び出しているか
                                  IKpiConfig **ppConfig);
      IKpiConfig に対応し、設定画面の自動生成や保存の管理を本体に任せたい場合に呼び出す。
      第1引数が IKpiUnknown 版(kpi_CreateInstance の第3引数をそのまま渡す)
      も提供される。

      IKpiConfig に対応する場合、kpi_CreateConfig による IKpiConfig のインスタンス
      取得は、遅くとも kpi_CreateInstance 内で実行すること。
      kpi_CreateInstance から戻った段階で未だ kpi_CreateConfig が呼ばれていない
      場合、設定の保存を本体に管理させることは出来ない。
      (EnumConfig/ApplyConfig は有効だがバイナリ以外の型は扱えない)

      riid を認識したら 0 以外を返すが、0 以外を返したからといって ppConfig
      の取得に成功したとは限らない。ppConfig の取得に成功したかは *ppConfig が
      NULL かどうかで判断すること。

      pGUID: 設定の保存場所を決める識別子

      渡された GUID に基づいて設定の保存場所が決定される。
      プラグインの置き場所に依存しないため、プラグインのプラットフォームや
      バージョン、ファイル名に関わらず設定が共有される。

      pGUID に NULL (または GUID_NULL)を渡すと、設定の保存場所は基準フォルダ
      から見たプラグインの相対パスによって決定される。
      基準フォルダは、現在の実装では本体実行ファイル(Kbmplay.exe) の親フォルダ
      (readme.txt 等が置いてあるフォルダ)

      pdwPlatform: 本体がプラグインを直接呼び出しているか本体とは別プロセスか

      プラグインをロードしている実行ファイルが本体自身(Kbmplay.exe)の場合は 0
      が入り、本体とは別プロセス(kbrunkpi.exe)経由の場合は 32 か 64 が入る。
      別プロセスの場合、本体のプラットフォームが x64 のときに 64、x86 のときに
      32 が入る。kbrunkpi.exe が x86 でも本体が x86/x64 のどちらなのかは
      判別不可能なため、どちらなのかが必要な場合は pdwPlatform で判断する。
      *pdwPlatform に 0 が入った場合、本体が x86 なのか x64 なのかはプラグイン
      自身と同じなので分かる筈である。
      本体が kpi_CreateConfig を(IID_IKpiConfig の値変更等により)未知の場合は
      *pdwPlatform には (DWORD)-1 が入る(正確には、CreateInstance 呼び出し前に
      -1 を代入、本体が何もしないのでそのままという形となる)

      何らかの事情で kbrunkpi.exe 経由の場合にプラグインの動作や設定を区別
      したい場合は pdwPlatform の戻り値を利用する。
      pdwPlatform が不要な場合は NULL を渡しても良い。

      ppConfig: IKpiConfig のインスタンスを受け取るポインタ

      設定項目が特になく、pdwPlatform だけが目的の場合は NULL を渡しても良い。


      設定の共有について

        A: x86 版本体が x86 版プラグインを使用
        B: x86 版本体が x64 版プラグインを使用
        C: x64 版本体が x86 版プラグインを使用
        D: x64 版本体が x64 版プラグインを使用

      *pGUID != GUID_NULL : A ～ D の設定は全て共有される
      *pGUID == GUID_NULL : A と C、B と D が共有される
      (or pGUID == NULL)   (標準プラグインのフォルダ構成の場合)

      x86 版と x64 版のプラグインで設定を区別しつつ(A と C、B と D が共有)、
      プラグインの置き場所が変わっても設定が引き継がれるようにしたい場合は、
      GUID != GUID_NULL とした上で x64 版用と x86 版用とで異なるセクション名、
      キー名を使用すれば良い。あるいは kpi_CreateConfig に異なる GUID を渡す。

      kbrunkpi 経由で使用時と本体が直接使用時とで設定を区別したい場合も同様。

      kpi_CreateConfig による IKpiConfig の取得は 1 つのプラグインにつき 1 回
      だけ有効。2 回目以降に 1 回目と異なる GUID を渡して複数の設定を使用する
      ことは出来ず、参照カウンタだけ増やして 1 回目と同じインスタンスを返す。

    DWORD WINAPI kpi_CreateDecoder(IKpiUnkProvider *pProvider,
                                   const KPI_MEDIAINFO *cpRequest,//再生周波数等の要求値が含まれた KPI_MEDIAINFO
                                   IKpiFile     *pFile,   //音楽データ
                                   IKpiFolder   *pFolder, //音楽データがあるフォルダ
                                   IKpiDecoder **ppDecoder);
      デコーダプラグインが別のデコーダプラグインを使用したい場合に呼び出す。
      引数と戻り値の意味は IKpiDecoderModule::Open と同じ。
      pFile/pFolder は IKpiDecoderModule::Open で渡された pFile/pFolder をそのまま
      渡しても良いし、OpenFile/OpenFolder して得たものを渡しても良い。
      プラグインが独自に実装した pFile/pFolder を渡しても良い。

      現在未実装
      kbrunkpi.exe 経由での実装が複雑すぎて無理かも…
*/
};
//IKpiUnkProvider のヘルパー関数(使用する場合は kpi.cpp もプロジェクトに追加すること)
extern DWORD WINAPI kpi_CreateConfig(IKpiUnkProvider *pProvider,
                                     const GUID *pGUID,
                                     DWORD *pdwPlatform,
                                     IKpiConfig **ppConfig);
extern DWORD WINAPI kpi_CreateConfig(IKpiUnknown *pUnknown, //kpi_CreateInstance の第3引数から直接取得
                                     const GUID *pGUID,
                                     DWORD *pdwPlatform,
                                     IKpiConfig **ppConfig);
//プラグインがエクスポートする関数
typedef HRESULT (WINAPI *pfn_kpiCreateInstance)(REFIID riid,      //IKpiXXXModule の IID
                                                void **ppvObject, //IKpiXXXModule の戻り値
                                                IKpiUnknown *pUnknown);
//HRESULT WINAPI kpi_CreateInstance(REFIID riid, void **ppvObject, IKpiUnknown *pUnknown);
//↑プラグインが実際にエクスポートする関数
/*
    riid: IID_IKpiDecoderModule (デコーダプラグイン)
          IID_IKpiUnArcModule   (アーカイブプラグイン)
          IID_IKpiPlayerMoodule (演奏プラグイン)
          IID_IKpiOutDeviceModule (デバイスプラグイン)
        のいずれかが渡される。
    デコーダプラグインは riid に IID_IKpiDecoderModule を渡されたら
      *ppvObject に IKpiDecoderModule のインスタンスを渡して S_OK を返す。
    アーカイブプラグインは riid に IID_IKpiUnArcModule を渡されたら
      *ppvObject に IKpiUnArcModule のインスタンスを渡して S_OK を返す。
    演奏プラグインは riid に IID_IKpiPlayerModule を渡されたら
      *ppvObject に IID_IKpiPlayerModule のインスタンスを渡して S_OK を返す。
    デバイスプラグインは riid に IID_IKpiOutDeviceModule を渡されたら
      *ppvObject に IID_IKpiOutDeviceModule のインスタンスを渡して S_OK を返す。
    対応しない riid が渡された場合は *ppvObject に NULL を渡して E_NOINTERFACE を返す。

    IKpiConfig に対応し、設定画面の自動生成と設定の保存管理を本体に任せたい場合は
    kpi_CreateConfig ヘルパー関数を使用する。その場合は kpi.cpp をプロジェクトに
    追加すること。

    IKpiConfig に対応する場合、kpi_CreateInstance から戻るまでに kpi_CreateConfig を
    呼び出すこと。この段階で kpi_CreateConfig が呼ばれていない場合でも
    dwSupportConfig != 0 であれば EnumConfig で設定を列挙することが出来るが、
    バイナリ以外の型の設定は扱うことが出来ない。

    プラグインから別のプラグインを使用する場合 kpi_CreateDecoder を呼び出す。
    プラグインが別のプラグインを使用する場合は kpi_CreateDecoder を介さなければならない。

    pUnknown に直接 IKpiUnkProvider を渡さず一旦 QueryInterface を介するように
    なっているのは、将来の機能の追加や仕様変更に対応するため。

    kpi_CreateInstance は本体によって一度だけ呼ばれる。実際は riid を変えて何度
    か呼び出すが、*ppvObject にインスタンスが最初に返って以降は呼ばない。

    2回目以降の呼び出しの結果は未定義。(2個目以降の *ppModule を使用した結果は未定義)

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
//extern HRESULT WINAPI kpi_CreateDecoderModule(pfn_kpiCreateInstance fnCreateInstance,
//                                              IKpiDecoderModule **ppModule, IKpiUnknown *pUnknown);
//↑プラグイン使用側用のヘルパー関数(プラグイン作成には不要)
//プラグイン使用者(通常は KbMedia Player)が呼び出す場合は kpi.cpp も
//プロジェクトに追加する
///////////////////////////////////////////////////////////////////////////////
#pragma pack(pop) //構造体のアラインメント

#endif

