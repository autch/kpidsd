# kpidsd

DSD (DSF / DSDIFF / WSD) を再生する [KbMedia Player](https://hwm5.gyao.ne.jp/kobarin/) のデコーダプラグイン。

- **kpidop.kpi** — DSD を [DoP (DSD Audio over PCM Frames)](https://dsd-guide.com/dop-open-standard) v1.1 のフレームにして出す。
  DoP 対応の DAC が要る。ASIO で DSD ネイティブに対応したデバイスでは、本体付属の kbasio.kpi が DoP を DSD ネイティブに変換できる
- **kpid2p.kpi** — DSD を PCM (浮動小数点) に変換して出す

対応する DSD は、44.1kHz 系と 48kHz 系のそれぞれ 64〜1024 倍。DSDIFF の DST 圧縮には対応しない。
利用者向けの説明は `kpidop/kpidop.txt` と `kpid2p/kpid2p.txt` (UTF-8 BOM 付き / CRLF) にある。

以前あった kpidsd.kpi (ASIO の DSD 拡張で DSD を直接出す。本体の対応が要る) はやめた。
kbasio.kpi が kpidop.kpi の DoP を DSD ネイティブに変換するので要らなくなった。

## ファイル

- `libdsd/` — DSF / DSDIFF / WSD のヘッダとチャンクのパーサー (静的ライブラリ)
- `common/` — 両方のプラグインで使う部分
  - `DsdSource.*` — IKpiFile から DSD を読み、どの形式でもチャンネルごとの MSB first のバイト列にそろえる。タグも設定する
  - `KpiFileAdapter.h` — libdsd に IKpiFile を読ませるアダプタ
  - `KpiPartialFile.h` — ID3v2 の範囲を IKpiFile として本体に渡す
  - `kpi_mingw_uuid.h` — MinGW でビルドするときの `__uuidof` 用シム (MSVC では空)
- `kpidop/` — `KpiDopDecoder.*` (DoP のフレーム)、`KpiDopDecoderModule.*`、`kpidop.cpp`、`kpidop.def`
- `kpid2p/` — `DsdDecimator.*` (FIR の設計と間引き)、`KpiD2pDecoder.*`、`KpiD2pDecoderModule.*`、`kpid2p.cpp`、`kpid2p.def`
- `kpisdk/` — KbMedia Player プラグイン SDK のヘッダ (2026/10/31 版、v3.17beta3 用、原本のまま)
- `tools/kpi-render.cpp` — 検証用ホスト。ビルドした .kpi をロードして PCM やタグを書き出す
- `tools/fixtures.py` — テスト用の DSF / DSDIFF / WSD を合成し、kpi-render の出力を照合する

## 形式ごとの扱い

| | DSF | DSDIFF | WSD |
|---|---|---|---|
| データの並び | チャンネルごとのブロック (4096 バイト)、LSB first / MSB first | 1 バイトずつ交互、MSB first | 1 バイトずつ交互、MSB first |
| タグ | ID3v2 の範囲を本体に解析させる | "ID3 " チャンク (FRM8 直下、非公式) があれば本体に解析させる。なければ DIIN の DITI / DIAR と最初の COMT を UTF-8 (正しくなければ Shift_JIS) で読む | 各欄を Shift_JIS で読む |

KbMedia Player 本体は DSD ファイルのタグを自前では解析しない。ID3v2 がないときも
`GetTagInfo(KPI_TAGTYPE_NONE)` を呼び、本体が拡張子を元にタグを探さないようにしている。
KORG AudioGate は DSDIFF / WSD のタグに非 ASCII の文字があるとタグ自体を書かないので、
DSDIFF の ID3 チャンクと非 ASCII のタグは合成したファイルでしか確かめていない。

2ch を超えるファイルは、チャンネル数をそのまま本体に伝える (並べ替えない)。
本体がマルチチャンネルのチャンネル配置の仕様を持っていないため。

## kpid2p の変換

1 段の FIR (カイザー窓の sinc) で、DSD の周波数の 1/D (D = 8〜128 の 2 のべき乗) に間引く。
出力の周波数は本体の要求に一番近いものを 44.1kHz〜384kHz から選ぶ (要求がなければ 88.2kHz)。
48kHz 系への変換はしない (DSD が 48kHz 系なら 48kHz 系になる)。

- 遮断周波数は出力のナイキスト周波数、通過域の端 fp = min(0.4535 fout, 30kHz)、阻止域の端は fout − fp、
  減衰量 120dB を目標にタップ数を決める
- DSD の 1 バイトごとに、係数 8 個の符号の組み合わせ 256 通りの和を表にしておき (係数が左右対称なので半分だけ)、
  出力するサンプルだけを表引きと足し算で計算する
- 出力 k は DSD の k·D サンプル目を中心にした窓から計算する。曲の前後は無音パターン (0x69) とみなす。
  このため遅延がなく、シークしても先頭から再生したのと同じ結果になる
- ゲインは 1 (DSD のビットがすべて 1 で +1.0)。e-onkyo の DSF と同じ曲の 96kHz flac を比べると、flac は
  +6.02dB (SOUVENIR、QUARTET) や +3.02dB (WHITE ALBUM2) 大きく、制作元ごとに違った。どれかに合わせると
  0dBFS を超える曲があるので、ゲインは変えない。時間のずれはどれも 0 サンプルだった

`check-d2p` で測った値 (取り出したインパルス応答から計算):

| | タップ数 | 通過域 | リップル | fout/2 | 阻止域 | 減衰 |
|---|---|---|---|---|---|---|
| DSD64 → 44.1kHz | 5376 | 〜20kHz | 0.00002dB | −6.02dB | 24.1kHz〜 | 119.4dB |
| DSD64 → 88.2kHz | 784 | 〜30kHz | 0.00002dB | −6.02dB | 58.2kHz〜 | 119.0dB |
| DSD64 → 176.4kHz | 192 | 〜30kHz | 0.00002dB | −6.02dB | 146.4kHz〜 | 119.8dB |
| DSD128 → 44.1kHz | 10752 | 〜20kHz | 0.00002dB | −6.02dB | 24.1kHz〜 | 119.4dB |
| DSD256 → 88.2kHz | 3136 | 〜30kHz | 0.00002dB | −6.02dB | 58.2kHz〜 | 118.9dB |

いちばん重い DSD128 → 44.1kHz でも、手元 (x64 Release) でステレオを実時間の約 30 倍で変換できる。

## ビルド

リリースビルドは Windows の MSVC (Visual Studio 2026) で行う。cmake は Visual Studio 付属のものを使う
(`vcvars64.bat` などを通した環境で実行する)。

```
cmake -S . -B build-x64 -G "Visual Studio 18 2026" -A x64
cmake --build build-x64 --config Release
cmake -S . -B build-x86 -G "Visual Studio 18 2026" -A Win32
cmake --build build-x86 --config Release
```

`build-*/bin/Release/` に `kpidop.kpi`、`kpid2p.kpi`、`kpi-render.exe` ができる。KbMedia Player 本体付属の
プラグインと同じく、VC++ ランタイムは動的リンク (`/MD`) なので、Visual Studio 2026 に対応した
VC++ 再頒布可能パッケージが要る。外部のライブラリは要らない。

Visual Studio で `kpidsd.sln` を開いてビルドすることもできる。各 `.vcxproj` は CMakeLists.txt と同じ設定にしてあり、
`bin\<Platform>\<Configuration>\` に `kpidop.kpi`、`kpid2p.kpi`、`kpi-render.exe` ができる。
設定を変えるときは CMakeLists.txt と両方を直すこと。テストは CMake でビルドしたときだけ使える。

MinGW (gcc / clang) でのクロスビルドも CMakeLists.txt には書いてあるが、確かめていない。

## 検証

```
cd build-x64
ctest -C Release -j 16
```

Python 3 が要る。fixtures.py は kpi-render を CPU のスレッド数だけ同時に走らせる (`-j N` で変えられる)。`check-d2p` は numpy も要る (なければ飛ばす)。

- `make-fixtures` — DSF (LSB first / MSB first、ブロックの途中で終わるもの、ID3v2 あり、5.1ch、DSD128)、
  DSDIFF (UTF-8 / Shift_JIS のタグ、ID3 チャンク、奇数バイト、48kHz 系)、WSD (Shift_JIS のタグ) を合成する。
  開けてはいけないもの (DST 圧縮、対応しない周波数) も作る
- `check-dop` — kpidop の DoP フレームを、合成したデータから組み立てた正解とバイト単位で照合する。
  KPI_MEDIAINFO (周波数、ビット数、長さ、シーク) も確かめる
- `check-tags` — 両方のプラグインのタグ (タグ取得用に開いたときも) と、本体のタグ解析 (`GetTagInfo`) に
  渡した種類と範囲を確かめる。ACP に頼る `wSetValueA` などを使っていないことも確かめる
- `seek-check` — いろいろな位置にシークしてから半端な長さで Render した結果が、先頭から続けて Render した
  結果と一致するかを確かめる。DoP はマーカーのバイトを除いて比べ、マーカーがシークをまたいでも
  0x05 / 0xFA の交互に続くことを確かめる
- `check-d2p` — kpid2p の出力の周波数とビット数の選び方を確かめる。無音のファイルと、ビットを 1 つずつ
  反転させたファイルの差から FIR のインパルス応答を取り出し、周波数特性 (直流、通過域のリップル、
  阻止域の減衰) を測る。2 次の ΔΣ 変調で作った 997Hz のサイン波の SINAD を、同じ区間を理想的な
  フィルタで切った場合と比べる (変調器の雑音で決まるので、同じになればよい)
- `seek-check-data` — `-DKPIDSD_TEST_DATA=<ディレクトリ>` を指定したときだけ、その下にある
  DSF / DSDIFF / WSD をすべて seek-check する

kpi-render は単体でも使える。

```
kpi-render [-k PLUGIN] [-r RATE] [-b BITS] [-n MAXFRAMES] file > out.raw   # プラグインが出したままのバイト列
kpi-render [-k PLUGIN] [-r RATE] [-b BITS] [--tag-only] --info file        # KPI_MEDIAINFO、タグ、GetTagInfo の呼ばれ方
kpi-render [-k PLUGIN] [-r RATE] [-b BITS] --seek-check file...
```

`PLUGIN` の既定値は kpi-render と同じディレクトリの `kpidop.kpi`。`-r` / `-b` は本体からの要求
(`cpRequest` の `dwSampleRate` / `nBitsPerSample`、既定は 0 / -64)。

KbMedia Player 本体での確認 (認識、再生、シーク、タグの表示、DoP の DAC での再生) は別に行う。

## 配布

GitHub Actions (`.github/workflows/build.yml`) が x64 / x86 を MSVC でビルドしてテストし、
プラグインごとに `.kpi`、説明書、`COPYING` をまとめた zip を作る。`v*` のタグを push すると、
その zip を添付したドラフトのリリースができる。

## ライセンス

MIT ライセンス。`COPYING`、`kpidop/COPYING`、`kpid2p/COPYING`、`libdsd/COPYING` を参照。
