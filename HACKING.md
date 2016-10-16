
# ビルドガイド

## 用意するもの

- Visual Studio 2015
  - 開発は Community エディションで行っている
- SoX Resampler のソースコード
  - https://sourceforge.net/p/soxr/wiki/Home/
  - `soxr-0.1.2-Source.tar.xz` をダウンロードして、`kpidsd` ディレクトリと同じ階層に展開する。以下のような階層になるはず。
    - `kpidsd/`
      - `kpidsd.sln`
      - `kpidop/`
      - `kpid2p/`
      - `libdsd/`
      - ...
    - `soxr-0.1.2-Source/`
      - `README`
      - `msvc/`
        - `libsoxr.vcproj`
      - ...
- KbMedia Player 2.80beta30 以降

## ソースの入手

```
git clone https://github.com/autch/kpidsd.git
```

## clone してから

- `kpidsd.sln` を開く
- 「表示」メニュー→「その他のウィンドウ」→「プロパティ マネージャ」を開く
- 各プロジェクトの構成を一つ開くと `KMPPlugin` という項目があるので、ダブルクリックして開く
- ツリーの「共通プロパティ」/「ユーザーマクロ」を開く
- `KMPPluginsPath` の値を KbMedia Player の配布ファイルを展開したディレクトリに修正する。
  - `SDK/` ディレクトリではない。名前と一致してない。
  - 各プロジェクトで一回ずつ行えば十分である。構成ごとにやり直す必要はない。
- ビルドできるはず。
