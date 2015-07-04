#ifndef KMP_PI_H
#define KMP_PI_H

/*

   KbMedia Player Plugin SDK (2015/06/14 版)

   開発者向けです。
   KbMedia Player のプラグインを開発してみたい人だけがお読み下さい。
   手抜きでコメントは C++ 用になってますが、今時のコンパイラなら C でも大丈夫でしょう。

   実際のプラグインのソースコードは

   http://hp.vector.co.jp/authors/VA015058/source/

   にあります。

   必要最小限の機能しかないため、出来ることはかなり制限されますが、
   その分プラグインを作るのは簡単なはずです。プラグインを使うのも簡単でしょう。

   改版履歴

   ・2015/06/14
     ・kmp_GetTestTagInfo を追加
     ・量子化ビット数に 64bit float を追加

*/

#define KMPMODULE_VERSION 100 //KMPMODULE のバージョン
#define SZ_KMP_GETMODULE  "kmp_GetTestModule" //まだ（いつまで？）テスト版...
#define SZ_KMP_CONFIG     "kmp_Config"        //プラグイン設定用（v2.38beta2 以降で対応）
#define SZ_KMP_GETTAGINFO "kmp_GetTestTagInfo"//タグ取得用（v2.51beta3以降で対応）

typedef void* HKMP;//'K'b'M'edia 'P'layer Plugin の Handle

typedef struct
{//オープンしたサウンドデータの情報
    DWORD dwSamplesPerSec;//サンプリング周波数(44100, 22050 など)
    DWORD dwChannels;     //チャンネル数( mono = 1, stereo = 2)
    DWORD dwBitsPerSample;//量子化ビット数( 8 or 16 or 24 or 32 or -32(32bit float) or -64(64bit float) )
                          //本体からのビット数要求に関わらず対応出来る最大のビット数を返すことを推奨
    DWORD dwLength;       //曲の長さ（計算不可能な場合は 0xFFFFFFFF）
                          //１ファイル複数曲（dwReserved2 == 1）のときは、曲数を 1000 倍した値にする。
                          //例えば 10 曲の場合は dwLength == 10000 とする。
                          //１ファイル複数曲の場合、曲毎の演奏時間を取得することは出来ない。
                          //駄目な仕様でスミマセン
    DWORD dwSeekable;     //シークに対応している場合は 1、しない場合は 0
    DWORD dwUnitRender;   //Render 関数の第３引数はこの値が渡される（どんな値でも良い場合は 0）
    DWORD dwReserved1;    //無限ループして演奏終了しない曲の場合に 1, そうでない場合に 0
                          //Render の呼び出しを繰り返しても演奏終了しない場合に 1 にする
                          //dwReserved1 == 1 の場合、
                          //  ・プレイヤー本体の「単曲リピート」が有効になっている場合は無限ループ
                          //  ・dwLength != 0xFFFFFFFF の場合は dwLength だけ再生した後にプレイヤー
                          //    本体によってフェードアウト・演奏終了される
                          //  ・dwLength == 0xFFFFFFFF の場合はプレイヤーの設定の「デフォルトの長さ」
                          //    だけ再生した後にフェードアウト・演奏終了される
                          //dwReserved1 == 0 の場合、
                          //  ・Render の戻り値が第３引数(dwSize)より小さい値が返るまで再生される
    DWORD dwReserved2;    //１ファイル複数曲の場合に 1, １ファイル１曲のときに 0
                          //dwReserved2 == 1 の場合、dwLength には曲数を 1000 倍した値を返すこと。
                          //dwReserved2 == 1 の場合、SetPosition の dwPos に(曲番号 * 1000)が渡される
                          //ようになり、シークは出来なくなるので dwSeek == 0 とすること。
                          //現在の仕様では、１ファイル複数曲の拡張子は KbMedia Player 本体で決め打ち
                          //されており、本体が知らない拡張子に対してこの値を 1 にしても正常には動作
                          //しないし、本体が１ファイル複数曲として扱っている形式に対してこの値を 0 に
                          //しても、やはり正常動作しない（ぉぃ
                          //駄目な仕様でスミマセン
}SOUNDINFO;


typedef struct
{
    DWORD dwVersion;
    //モジュールのバージョン。プラグインのバージョンではない。
    //必ず KMPMODULE_VERSION(=100) にすること。
    //この値が KbMedia Player が期待する値と一致しない場合は、KbMedia Player
    //によって直ちに FreeLibrary が呼ばれる。
    //その場合、Init() も Deinit() も呼ばれないことに注意。

    DWORD dwPluginVersion;
    //プラグインのバージョン
    //対応拡張子が競合するプラグインが見つかった場合は、数字が大きいものを優先的に使う

    const char  *pszCopyright;
    //著作権
    //バージョン情報でこの部分の文字列を表示する
    //NULL にしてもよい

    const char  *pszDescription;
    //説明
    //バージョン情報でこの部分の文字列を表示する
    //NULL にしてもよい

    const char  **ppszSupportExts;
    //対応拡張子の文字列の配列(ピリオド含む)
    //NULL で終わるようにする
    //例：ppszSupportExts = {".mp1", ".mp2", ".mp3", "rmp", NULL};
    //
    //ppszSupportExts == {NULL} (ppszSupportExts == NULL は不可）で、かつ Init と Deinit
    //が 非 NULL の場合は常駐プラグインとして扱われ、KbMedia Player が動作中は常にロード
    //されたままになる（非公式の機能）
    //常駐プラグインとしての動作は Init 呼出し時に開始し、Deinit 呼出し時に終了すること。
    //（LoadLibrary 時に動作開始、FreeLibrary 時に動作終了は不可）
    //
    //上記以外で対応拡張子が１つもない場合、不正なプラグインと見なされ、KbMedia Player
    //によって直ちに FreeLibrary される。
    //その場合は Init() も Deinit() も呼ばれないことに注意。

    DWORD dwReentrant;
    //複数ファイルの同時再生が可能な場合は 1, 不可能な場合は 0
    //Open の戻り値の HKMP を２つ以上同時に持てない場合に 0 にする
    //特別な事情がない限り、出来るだけ dwReentrant == 1 となるように実装するべきである
    //この値が 0 の場合、２つ目以降のファイルはプレイヤー本体とは別 EXE を経由して開く
    //ようになる。
    //何らかの理由により、常にプレイヤー本体とは別 EXE でファイルを開くようにしたい場合
    //は dwReentrant == 0xFFFFFFFF とすること
    //例えば、aaa.kpi と bbb.kpi が ccc.dll に依存し、ccc.dll がノンリエントラントである
    //場合は、aaa.kpi も bbb.kpi も dwReentrant == 0xFFFFFFFF になっていた方が望ましい。

    void  (WINAPI *Init)(void);
    //プラグイン初期化。Open 等を呼び出す前に KbMedia Player によって一度だけ呼ばれる。
    //必要ない場合は NULL にしても良い。
    //ただし、常駐プラグイン（非公式の機能）の場合は必ず実装すること。
    //Init の呼び出しで常駐プラグインを動作開始する。
    //LoadLibrary 時に常駐プラグインを動作開始してはいけない。

    void  (WINAPI *Deinit)(void);
    //プラグインの後始末。FreeLibrary の直前に一度だけ呼ばれる。
    //Init() を一度も呼ばずに Deinit() を呼ぶ可能性もあることに注意。
    //必要ない場合は NULL にしても良い。
    //ただし、常駐プラグイン（非公式の機能）の場合は必ず実装すること。
    //Deinit の呼び出しで常駐プラグインを動作終了する
    //FreeLibrary 時に常駐プラグインを動作終了してはいけない。

    HKMP (WINAPI *Open)(const char *cszFileName, SOUNDINFO *pInfo);
    //ファイルを開く。必ず実装すること。
    //エラーの場合は NULL を返す。
    //エラーでない場合は pInfo に適切な情報を入れること。適切な情報が入って
    //いない場合（dwBitsPerSample が 0 など）は KbMedia Player によって直ちに Close
    //が呼ばれる。
    //Open に渡された pInfo->dwSampleRate/dwBitsPerSample/dwChannels の各値が 0 以外の
    //場合、可能ならそれに従った形式（または最も近い形式）でファイルを開くようにすること。
    //（例えば pInfo->dwBitsPerSample == -64(64bit float) になっている場合、可能なら
    //  64bit float で、64bit float が不可能でも 32bit int なら対応しているなら 32bit int
    //  で返すなど）
    //量子化ビット数は 2.51beta15 以降は常に 64bit float で要求が来るようになっている。
    //対応出来る範囲内で最大のものを返すようにすること。可能ならクリッピングの処理が不要な
    //64bit float か 32bit float が望ましい。
    //本体は常に内部で一旦 64bit float に変換する。64bit float で出力出来るのであれば
    //変換の必要がなくなり、最も効率的となる。
    //だからと言って無理して 64bit float での出力に対応する必要はなく、対応出来る最大の
    //ビット数をどれか１つだけに絞り込めば良い
    //複数のビット数に対応するよりもプラグイン開発時の手間も少ない筈である。

    HKMP (WINAPI *OpenFromBuffer)(const BYTE *Buffer, DWORD dwSize, SOUNDINFO *pInfo);
    //未使用。NULL にすること。非 NULL でもエラーにはならないが、使用されない。

    void   (WINAPI *Close)(HKMP hKMP);
    //ハンドルを閉じる。必ず実装すること。

    DWORD  (WINAPI *Render)(HKMP hKMP, BYTE* Buffer, DWORD dwSize);
    //Buffer に PCM を入れる。必ず実装すること。
    //dwSize は Buffer のサイズのバイト数。（サンプル数ではない）
    //戻り値は Buffer に書き込んだバイト数。（サンプル数ではない）
    //dwSize より小さい値を返したら演奏終了。
    //dwSize は SOUNDINFO::dwUnitRender の値が渡される。
    //SOUNDINFO::dwUnitRender == 0 の場合は dwSize には任意の値が渡される。
    //（dwSize はサンプルあたりのバイト数の倍数であることが保証されている）

    DWORD  (WINAPI *SetPosition)(HKMP hKMP, DWORD dwPos);
    //シーク。必ず実装すること。
    //dwPos はシーク先の再生位置。戻り値はシーク後の再生位置。単位はミリ秒。
    //dwPos と戻り値は完全に一致する必要はない。戻り値と本当の再生位置の
    //誤差が大きくなる（歌詞との同期再生時に支障をきたす）場合は Open 時に
    //SOUNDINFO の dwSeekable を 0 にしておくこと。誤差がないか、あっても
    //非常に小さい場合は dwSeekable を 1 にしておくこと。戻り値が正確なら
    //ば、dwPos と戻り値の差が大きくても dwSeekable=1 として良い。
    //ただし、実際のシーク先位置が dwPos より後方になることは避けること。
    //（CueSheet 再生時に問題が起こりやすい）
    //
    //シークに全く対応しない場合は、先頭位置に戻して 0 を返すこと。
    //
    //１ファイル複数曲（SOUNDINFO::dwReserved2 == 1）の場合、この関数で選曲
    //することになる。曲番号は 1 ベース（曲番号 0 は存在しない）で、dwPos
    //には、曲番号を 1000 倍した値が入る。例えば 5 番目の曲に切り替える場合
    //は dwPos == 5000 となる。
    //切り替え後の再生位置は先頭に戻して 0 を返すこと。
    //ただし、dwPos == 0 の場合は曲を切り替えずに再生位置を先頭に戻すこと。
    //SetPosition が一度も呼ばれていない状態では 1 曲目が選択されているように
    //すること。
    //切り替えと同時にシークすることは出来ない。（駄目な仕様でスミマセン）

}KMPMODULE;

typedef KMPMODULE* (WINAPI *pfnGetKMPModule)(void);
//KMPMODULE* WINAPI kmp_GetTestModule(void);//エクスポートする関数名
//
//KMPMODULE 構造体へのポインタを返す。必ずエクスポートすること。
//この関数はプラグインが LoadLibrary されてから FreeLibrary されるまでの間に
//KbMedia Player 本体からは１回だけしか呼ばれない。
//この関数が返した KMPMODULE 構造体のデータメンバは、FreeLibrary されるまで
//変更してはいけない。
//従って、例えば対応拡張子を設定によって変更されるようにしたい場合、対応拡張子
//の変更が KbMedia Player 本体に認識されるのは、プラグイン再ロード時ということ
//になる。

typedef DWORD (WINAPI *pfnKmpConfig)(HWND hWnd, DWORD dwVersion, DWORD dwReserved);
//DWORD WINAPI kmp_Config(HWND hWnd, DWORD dwVersion, DWORD dwReserved);//エクスポートする関数名
//
//プラグインの設定を行う。特に設定の必要がない場合はエクスポートしなくても良い。
//   ・dwVersion/dwReserved ともに 0 が渡される
//   ・戻り値は現状では 0 を返すのみ
//   ・HKMP のインスタンスごとの設定は現状では未対応（対応の必要があるのか？）
//   ・kmp_Config が呼ばれる前に KMPMODULE::Init() が呼ばれる
//   ・kmp_Config から戻っていない（設定ダイアログを表示している）状態では、
//     ・二重に kmp_Config が呼ばれることはない
//     ・KMPMODULE::Deinit() は呼ばれない
//     ・FreeLibrary は呼ばれない
//     ・kmp_Config 呼び出し後、ダイアログを表示したまま直ちにリターンする場合は
//       この限りではない（その場合はプラグイン側で適切に処理すること）
//
//設定はしたいが kmp_Config の実装が面倒、という場合は、プラグインと同じ場所に
//プラグインと同名の INI ファイルを置けば、KbMedia Player 本体のプラグイン設定
//ダイアログから INI ファイルを開いて INI ファイルを直接編集出来るようになっている。

//タグ取得用
#define SZ_KMP_TAGINFO_NAME_TITLE       "title"    //曲名
#define SZ_KMP_TAGINFO_NAME_ARTIST      "artist"   //アーティスト
#define SZ_KMP_TAGINFO_NAME_ALBUM       "album"    //アルバム
#define SZ_KMP_TAGINFO_NAME_GENRE       "genre"    //ジャンル
#define SZ_KMP_TAGINFO_NAME_DATE        "date"     //日付
#define SZ_KMP_TAGINFO_NAME_COMMENT     "comment"  //コメント
#define SZ_KMP_TAGINFO_NAME_ALBUMARTIST "albumartist" //アルバムアーティスト
#define SZ_KMP_TAGINFO_NAME_BITRATE     "bitrate"     //ビットレート
#define SZ_KMP_TAGINFO_NAME_TRACKNUMBER "tracknumber" //トラック番号
#define SZ_KMP_TAGINFO_NAME_DISCNUMBER  "discnumber"  //ディスク番号
#define SZ_KMP_TAGINFO_NAME_TRACKTOTAL  "tracktotal"  //トラック数
#define SZ_KMP_TAGINFO_NAME_DISCTOTAL   "disctotal"   //ディスク数
#define SZ_KMP_TAGINFO_NAME_REPLAYGAIN_TRACKGAIN "replaygain_track_gain" //リプレイゲイン
#define SZ_KMP_TAGINFO_NAME_REPLAYGAIN_TRACKPEAK "replaygain_track_peak" //リプレイゲイン
#define SZ_KMP_TAGINFO_NAME_REPLAYGAIN_ALBUMGAIN "replaygain_album_gain" //リプレイゲイン
#define SZ_KMP_TAGINFO_NAME_REPLAYGAIN_ALBUMPEAK "replaygain_album_peak" //リプレイゲイン

class IKmpTagInfo
{//実装するのは本体側
 //プラグイン側は呼び出すだけ
public:
    virtual void WINAPI SetValueA(const char *cszName, const char *cszValue) = 0;   //Ansi
    virtual void WINAPI SetValueW(const char *cszName, const WCHAR *cwszValue) = 0; //Unicode
    virtual void WINAPI SetValueU8(const char *cszName, const char *cszValueU8) = 0;//Utf8
};
typedef BOOL (WINAPI *pfnGetTagInfo)(const char *cszFileName, IKmpTagInfo *pInfo);
//DWORD WINAPI kmp_GetTestTagInfo(const char *cszFileName, IKmpTagInfo *pInfo);//エクスポートする関数名
//
//タイトル等のタグ情報やビットレート等の情報を IKmpTagInfo に通知する
//エクスポートされていない場合は本体のタグ取得ルーチンでタグ情報を取得する
//
//次のように実装する
//BOOL WINAPI kmp_GetTestTagInfo(const char *cszFileName, IKmpTagInfo *pInfo)
//{
//    char  *szTitle;   //Ansi
//    WCHAR *wszArtist; //Unicode
//    char  *szU8Album; //Utf8
//    ...
//    pInfo->SetValueA(SZ_KMP_TAGINFO_NAME_TITLE, szTitle);
//    pInfo->SetValueW(SZ_KMP_TAGINFO_NAME_ARTIST, szArtist);
//    pInfo->SetValueU8(SZ_KMP_TAGINFO_NAME_ALBUM, szU8Album);
//    pInfo->SetValueA(SZ_KMP_TAGINFO_NAME_BITRATE, "128kbps");
//    ...
//    return TRUE;
//}
//
//SetValueA/W/U8 はそれぞれ Ansi/Unicode/Utf8
//
//cszName は大文字と小文字を区別しない
//cszName で扱えるのは KMP_TAGINFO_NAME_XXXX
//SZ_KMP_TAGINFO_NAME_XXXX で未定義の文字列を渡しても本体によって無視される。
//定義済みでも本体のバージョンによっては無視されることがある。
//
//SetValueA(ANSI) と SetValueW(UNICODE) と SetValueU8(UTF8) は混在して使用可能
//3 種類用意したのはプラグイン側での文字コードの変換の手間を省くため
//同じ Value 値をわざわざ 3 種類の文字コードで指定して呼ぶ必要はない
//    pInfo->SetValueA(SZ_KMP_TAGINFO_NAME_TITLE, "たいとる");
//    pInfo->SetValueW(SZ_KMP_TAGINFO_NAME_TITLE, L"たいとる");
//    pInfo->SetValueU8(SZ_KMP_TAGINFO_NAME_TITLE, (utf8の)"たいとる");
//のような呼び方は不要
//cszName は常に ANSI であることに注意(マルチバイト文字は使えない)
//
//KbMedia Player は、以下の順序でタグ情報を取得する（一部未実装）
//（将来的には以下のようにする予定だが未実装）
//
//1.実際の再生に使うプラグインから取得
//2.同じ形式に対応する別のプラグインから取得(使用優先度が高い順)
//3.KbMedia Player 自身が取得
//1 > 2 > 3 の順に優先度が高い(1が最も優先度が高い)
//
//例えばビットレート以外の情報取得は他のプラグインなり KbMedia Player 自身に
//任せたければ、ビットレート情報だけを通知すれば良い。
//
//逆に全ての情報取得を自身で行い、2, 3 の取得を省略させたい場合は、
//一番最初に SetValueA(NULL, NULL) を呼び出す(SetValueW/SetValueU8 でも可)
//例：
//    pInfo->SetValueA(NULL, NULL);//SetValueW(NULL, NULL)/SetValueU8(NULL, NULL)も OK
//    pInfo->SetValueA("bitrate", "128kbps");
//    pInfo->SetValueA("title", "タイトル");
//この場合、2,3 でアーティスト情報等を取得出来ても、1 で取得した情報しか表示しない。
//
//SetValue(NULL, NULL) がない場合、cszName が指定されてないものに関しては 2,3 で取得
//出来れば表示する。上記の例では、アーティスト情報を 2,3 で得た場合はアーティスト情報
//も表示する。
//
//1,2,3 で同じ cszName に対して異なる cszValue が得られた場合、優先度が高いものを採用する
//従って、
//    pInfo->SetValueA("title", "");
//のように呼び出すと、2,3 で異なるタイトルを得てもタイトルは "" となる。
//
#endif
