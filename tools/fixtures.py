#!/usr/bin/env python3
"""kpidop.kpi / kpid2p.kpi のテスト用ファイルを合成し、kpi-render の出力を照合する。

usage: fixtures.py generate DIR
       fixtures.py check-dop DIR --cli KPI_RENDER [--emulator WINE]
       fixtures.py check-tags DIR --cli KPI_RENDER [--emulator WINE]
       fixtures.py seek-check DIR --cli KPI_RENDER [--emulator WINE]
       fixtures.py check-d2p DIR --cli KPI_RENDER [--emulator WINE]
       fixtures.py seek-check-data DATADIR --cli KPI_RENDER [--emulator WINE]
       fixtures.py fuzz-smoke DIR --cli KPI_RENDER [--emulator WINE] [--count N] [--seed N]

generate は DSF / DSDIFF / WSD を DIR に書き、各ファイルの正解 (チャンネルごとの DSD のバイト列、
タグ) を DIR/expect.json と DIR/*.planar に書く。DSD の中身は乱数で、音にはなっていない。
check-dop は kpidop の DoP フレームを正解から組み立てたものと照合する。
check-tags は両方のプラグインのタグと、本体のタグ解析 (GetTagInfo) への渡し方を確かめる。
seek-check は両方のプラグインで kpi-render --seek-check を行う。
check-d2p は kpid2p の出力の周波数を確かめ、ビットを 1 つずつ反転させたファイルから
FIR のインパルス応答を取り出して周波数特性を測る。ΔΣ 変調したサイン波の SINAD も測る。
numpy が要る (なければ終了コード 77 で飛ばす)。
seek-check-data は DATADIR の下の .dsf / .dff / .wsd をすべて seek-check する。
fuzz-smoke は合成したファイルを決まった乱数で壊し、kpi-render が落ちたり止まったりしないことを確かめる。
"""

import argparse
import concurrent.futures
import json
import math
import os
import random
import struct
import subprocess
import sys

DSD64 = 44100 * 64
SILENCE = 0x69
BITREV = bytes(int(f"{i:08b}"[::-1], 2) for i in range(256))

# DSD 1 ビットを 8 サンプル単位で扱う。planar は [チャンネル][バイト] の MSB first


def random_planar(rng, channels, nbytes):
    return [bytes(rng.getrandbits(8) for _ in range(nbytes)) for _ in range(channels)]


# --- DSF (Sony DSF File Format Specification 1.01、リトルエンディアン) ---

def make_dsf(planar, rate, sample_count, lsb_first=True, id3=None, block=4096, id3_pointer=None, fmt_extra=b""):
    channels = len(planar)
    nbytes = len(planar[0])
    blocks = (nbytes + block - 1) // block
    data = bytearray()
    for b in range(blocks):
        for ch in range(channels):
            chunk = planar[ch][b * block:(b + 1) * block]
            if lsb_first:
                chunk = chunk.translate(BITREV)
            data += chunk + bytes(block - len(chunk))  # 最後のブロックは 0 で埋める
    channel_type = {1: 1, 2: 2, 3: 3, 4: 4, 5: 6, 6: 7}[channels]
    fmt = struct.pack("<4sQIIIIIIQII", b"fmt ", 52 + len(fmt_extra), 1, 0, channel_type, channels, rate,
                      1 if lsb_first else 8, sample_count, block, 0) + fmt_extra
    data_chunk = struct.pack("<4sQ", b"data", 12 + len(data)) + data
    size = 28 + len(fmt) + len(data_chunk) + (len(id3) if id3 else 0)
    if id3_pointer is None:
        id3_pointer = 28 + len(fmt) + len(data_chunk) if id3 else 0
    head = struct.pack("<4sQQQ", b"DSD ", 28, size, id3_pointer)
    return head + fmt + data_chunk + (id3 or b"")


# --- DSDIFF (Philips DSDIFF 1.5、ビッグエンディアン) ---

def chunk(ckid, body):
    return struct.pack(">4sQ", ckid, len(body)) + body + (b"\0" if len(body) & 1 else b"")


def make_dff(planar, rate, compression=b"DSD ", title=None, artist=None, comment=None, id3=None,
             comments=None, version=0x01050000):
    """comments は (cmtType, cmtRef, 文字列) の並び。comment は General Comment 1 つだけのとき用"""
    channels = len(planar)
    nbytes = len(planar[0])
    name = b"not compressed" if compression == b"DSD " else b"DST Encoded"
    ids = [b"SLFT", b"SRGT", b"C   ", b"LFE ", b"LS  ", b"RS  "][:channels] if channels > 1 else [b"C   "]
    prop = b"SND " + chunk(b"FS  ", struct.pack(">I", rate)) \
        + chunk(b"CHNL", struct.pack(">H", channels) + b"".join(ids)) \
        + chunk(b"CMPR", compression + bytes([len(name)]) + name)
    body = b"DSD " + (chunk(b"FVER", struct.pack(">I", version)) if version is not None else b"") + chunk(b"PROP", prop)
    if comment is not None:
        comments = [(0, 0, comment)]
    if comments is not None:
        comt = struct.pack(">H", len(comments))
        for type_, ref, text in comments:
            # 文字列は偶数長になるよう詰め物を付ける (count には含めない)
            comt += struct.pack(">HBBBBHHI", 2026, 11, 10, 12, 0, type_, ref, len(text)) + text + b"\0" * (len(text) & 1)
        body += chunk(b"COMT", comt)
    interleaved = bytes(planar[ch][i] for i in range(nbytes) for ch in range(channels))
    body += chunk(b"DSD ", interleaved)
    if title is not None or artist is not None:
        diin = b""
        if artist is not None:
            diin += chunk(b"DIAR", struct.pack(">I", len(artist)) + artist)
        if title is not None:
            diin += chunk(b"DITI", struct.pack(">I", len(title)) + title)
        body += chunk(b"DIIN", diin)
    if id3 is not None:
        body += chunk(b"ID3 ", id3)
    return chunk(b"FRM8", body)


# --- WSD (1bit オーディオコンソーシアム WSD 1.1) ---

def wsd_field(text, size):
    return text + b" " * (size - len(text))  # 空白で埋める


def make_wsd(planar, rate, text, text_offset=128, data_offset=2048):
    channels = len(planar)
    nbytes = len(planar[0])
    interleaved = bytes(planar[ch][i] for i in range(nbytes) for ch in range(channels))
    size = data_offset + len(interleaved)
    # fileSize (プラグインは使わない) は、上位と下位の 32 ビットを入れ替えて書く
    general = b"1bit" + bytes(4) + bytes([0x11, 0]) + bytes(2) \
        + struct.pack(">II", size & 0xFFFFFFFF, size >> 32) + struct.pack(">III", text_offset, data_offset, 0)
    spec = struct.pack(">III", 0, rate, 0) + bytes([channels]) + bytes(3) + struct.pack(">I", 0) + bytes(12) \
        + struct.pack(">II", 0, 0) + bytes(16) + bytes(40)
    assert len(general) == 32 and len(spec) == 96
    fields = [("title", 128), ("composer", 128), ("songWriter", 128), ("artist", 128), ("album", 128),
              ("genre", 32), ("dateAndTime", 32), ("location", 32), ("comment", 512), ("userSpecific", 512)]
    body = b"".join(wsd_field(text.get(name, b""), n) for name, n in fields) + bytes(160)
    assert len(body) == 1920
    head = general + spec + (body if text_offset else b"")
    head += bytes(max(0, data_offset - len(head)))
    return head[:data_offset] + interleaved


def id3v2(title):
    # ID3v2.3 の TIT2 だけ。本体に渡す範囲の照合に使うので中身は何でもよい
    frame_body = b"\x01" + b"\xff\xfe" + title.encode("utf-16-le")
    frame = b"TIT2" + struct.pack(">I", len(frame_body)) + b"\0\0" + frame_body
    size = len(frame)
    syncsafe = bytes([(size >> 21) & 0x7F, (size >> 14) & 0x7F, (size >> 7) & 0x7F, size & 0x7F])
    return b"ID3\x03\x00\x00" + syncsafe + frame


def generate(directory):
    os.makedirs(directory, exist_ok=True)
    rng = random.Random(20261110)
    expect = {}

    def save(name, data, planar, rate, tags, gettaginfo):
        with open(os.path.join(directory, name), "wb") as f:
            f.write(data)
        with open(os.path.join(directory, name + ".planar"), "wb") as f:
            for p in planar:
                f.write(p)
        expect[name] = {"rate": rate, "channels": len(planar), "bytes": len(planar[0]),
                        "tags": tags, "gettaginfo": gettaginfo}

    common_tags = lambda rate: {"Bitrate": f"{rate // 1000000}.{rate // 1000 % 1000:03d}Mbps",
                                "DSD_SamplesPerSec": f"{rate // 1000000}.{rate // 1000 % 1000:03d}MHz"}

    # DSF: LSB first、ステレオ、ブロックの途中で終わる (サンプル数も 8 の倍数でない)。ID3v2 あり
    nbytes = 4096 * 3 + 1000
    planar = random_planar(rng, 2, nbytes)
    tag = id3v2("DSF のタイトル")
    save("lsb_stereo.dsf", make_dsf(planar, DSD64, nbytes * 8 - 3, lsb_first=True, id3=tag), planar, DSD64,
         common_tags(DSD64), {"type": 1, "data": tag.hex()})

    # DSF: MSB first、モノラル、ID3v2 なし。DSD128
    nbytes = 4096 * 2
    planar = random_planar(rng, 1, nbytes)
    save("msb_mono_dsd128.dsf", make_dsf(planar, DSD64 * 2, nbytes * 8, lsb_first=False), planar, DSD64 * 2,
         common_tags(DSD64 * 2), {"type": 0})

    # DSF: 5.1ch。チャンネル数はそのまま報告する
    nbytes = 4096 + 17
    planar = random_planar(rng, 6, nbytes)
    save("six_channels.dsf", make_dsf(planar, DSD64, nbytes * 8), planar, DSD64,
         common_tags(DSD64), {"type": 0})

    # DSDIFF: UTF-8 のタイトル・アーティスト・コメント。チャンネルあたりのバイト数が奇数
    nbytes = 20001
    planar = random_planar(rng, 2, nbytes)
    save("utf8_tags.dff", make_dff(planar, DSD64, title="タイトル Ünïcødé".encode(), artist="アーティスト".encode(),
                                   comment="コメント".encode()), planar, DSD64,
         dict(common_tags(DSD64), Title="タイトル Ünïcødé", Artist="アーティスト", Comment="コメント"), {"type": 0})

    # DSDIFF: Shift_JIS のタイトル・アーティスト。48kHz 系の DSD64 (3.072MHz)
    rate48 = 48000 * 64
    nbytes = 9000
    planar = random_planar(rng, 2, nbytes)
    save("cp932_tags_48k.dff", make_dff(planar, rate48, title="曲名ソ表".encode("cp932"), artist="演奏者".encode("cp932")),
         planar, rate48, dict(common_tags(rate48), Title="曲名ソ表", Artist="演奏者"), {"type": 0})

    # DSDIFF: ID3 チャンクがあれば DIIN は使わず、ID3 チャンクの中身を本体に渡す
    nbytes = 5000
    planar = random_planar(rng, 1, nbytes)
    tag = id3v2("DFF の ID3")
    save("id3_chunk.dff", make_dff(planar, DSD64, title=b"not used", artist=b"not used", id3=tag), planar, DSD64,
         common_tags(DSD64), {"type": 1, "data": tag.hex()})

    # WSD: Shift_JIS の各欄 (空白で埋める)
    nbytes = 12345
    planar = random_planar(rng, 2, nbytes)
    text = {"title": "ＷＳＤの曲".encode("cp932"), "artist": b"Artist", "album": "アルバム".encode("cp932"),
            "composer": "作曲者".encode("cp932"), "genre": b"Classical", "dateAndTime": b"2026-11-10",
            "comment": "コメント欄".encode("cp932")}
    save("cp932.wsd", make_wsd(planar, DSD64, text), planar, DSD64,
         dict(common_tags(DSD64), Title="ＷＳＤの曲", Artist="Artist", Album="アルバム", Composer="作曲者",
              Genre="Classical", Date="2026-11-10", Comment="コメント欄"), {"type": 0})

    # DSF: ID3v2 のポインタが音のデータの途中を指す。本体には渡さない
    nbytes = 4096 * 2
    planar = random_planar(rng, 2, nbytes)
    save("id3_inside_data.dsf", make_dsf(planar, DSD64, nbytes * 8, id3=id3v2("x"), id3_pointer=28 + 52 + 12 + 100),
         planar, DSD64, common_tags(DSD64), {"type": 0})

    # DSF: ID3v2 のポインタの先が "ID3" で始まらない。本体には渡さない
    save("id3_no_magic.dsf", make_dsf(planar, DSD64, nbytes * 8, id3=b"XYZ" + bytes(20)),
         planar, DSD64, common_tags(DSD64), {"type": 0})

    # DSF: 3 ブロック目の途中で切れたファイル (ヘッダの file_size は元のまま)。読めた 2 ブロック分だけ使う
    nbytes = 4096 * 3
    planar = random_planar(rng, 2, nbytes)
    data = make_dsf(planar, DSD64, nbytes * 8)
    data = data[:28 + 52 + 12 + 4096 * 2 * 2 + 3000]
    save("truncated.dsf", data, [p[:4096 * 2] for p in planar], DSD64, common_tags(DSD64), {"type": 0})

    # WSD: テキストがない (textOffset = 0)
    nbytes = 3000
    planar = random_planar(rng, 1, nbytes)
    save("no_text.wsd", make_wsd(planar, DSD64, {}, text_offset=0, data_offset=128), planar, DSD64,
         common_tags(DSD64), {"type": 0})

    # DSDIFF: 仕様にない場所にあるチャンクは使わず、同じチャンクは最初のものを使う
    #   FRM8 直下の FS (別の周波数)、PROP の中の DSD (偽のデータ)、2 つ目の PROP と DSD と DIIN
    nbytes = 4001
    planar = random_planar(rng, 2, nbytes)
    fake = random_planar(rng, 2, 500)
    name_ = b"not compressed"
    prop = b"SND " + chunk(b"FS  ", struct.pack(">I", DSD64))         + chunk(b"CHNL", struct.pack(">H", 2) + b"SLFTSRGT")         + chunk(b"CMPR", b"DSD " + bytes([len(name_)]) + name_)         + chunk(b"DSD ", bytes(fake[0]))
    prop2 = b"SND " + chunk(b"FS  ", struct.pack(">I", DSD64 * 2)) + chunk(b"CHNL", struct.pack(">H", 1) + b"C   ")
    interleave = lambda pl: bytes(pl[ch][i] for i in range(len(pl[0])) for ch in range(len(pl)))
    body = b"DSD " + chunk(b"FVER", struct.pack(">I", 0x01050000)) + chunk(b"FS  ", struct.pack(">I", DSD64 * 4))         + chunk(b"PROP", prop) + chunk(b"PROP", prop2)         + chunk(b"DIIN", chunk(b"DITI", struct.pack(">I", 5) + b"first") + chunk(b"DSD ", interleave(fake)))         + chunk(b"DSD ", interleave(planar)) + chunk(b"DSD ", interleave(fake))         + chunk(b"DIIN", chunk(b"DITI", struct.pack(">I", 6) + b"second"))
    save("misplaced_chunks.dff", chunk(b"FRM8", body), planar, DSD64,
         dict(common_tags(DSD64), Title="first"), {"type": 0})

    # DSF: fmt チャンクが 52 バイトより長い (仕様では "Usually 52bytes")。後ろの 8 バイトは飛ばす
    nbytes = 5000
    planar = random_planar(rng, 2, nbytes)
    save("long_fmt.dsf", make_dsf(planar, DSD64, nbytes * 8, fmt_extra=b"\xAA" * 8), planar, DSD64,
         common_tags(DSD64), {"type": 0})

    # DSDIFF: コメントが File History、General の順に並ぶ。General を使う (奇数長の文字列の詰め物も飛ばす)
    planar = random_planar(rng, 2, 3001)
    save("comments.dff", make_dff(planar, DSD64, comments=[(3, 2, b"User Identification"), (3, 1, b"Program X"),
                                                          (0, 0, b"album comment"), (0, 0, b"second")]),
         planar, DSD64, dict(common_tags(DSD64), Comment="album comment"), {"type": 0})

    # DSDIFF: General Comment がない。最初のコメントを使う
    save("history_only.dff", make_dff(planar, DSD64, comments=[(3, 2, b"operator"), (2, 0, b"DSD recording")]),
         planar, DSD64, dict(common_tags(DSD64), Comment="operator"), {"type": 0})

    # DSDIFF: FVER がない。仕様では必須だが、なくても開く
    save("no_fver.dff", make_dff(planar, DSD64, version=None), planar, DSD64, common_tags(DSD64), {"type": 0})

    # 開けてはいけないもの: DST 圧縮、対応しない周波数、WSD のデータとテキストが重なる
    planar = random_planar(rng, 2, 1000)
    with open(os.path.join(directory, "dst.dff"), "wb") as f:
        f.write(make_dff(planar, DSD64, compression=b"DST "))
    with open(os.path.join(directory, "bad_rate.dsf"), "wb") as f:
        f.write(make_dsf(planar, 1000000, 8000))
    with open(os.path.join(directory, "text_overlaps_data.wsd"), "wb") as f:
        f.write(make_wsd(planar, DSD64, {"title": b"t"}, text_offset=128, data_offset=1024))
    # DSDIFF の主バージョンが 2 (1.x と同じ形とは限らない)
    with open(os.path.join(directory, "version2.dff"), "wb") as f:
        f.write(make_dff(planar, DSD64, version=0x02000000))
    expect["_invalid"] = ["dst.dff", "bad_rate.dsf", "text_overlaps_data.wsd", "version2.dff"]

    with open(os.path.join(directory, "expect.json"), "w", encoding="utf-8") as f:
        json.dump(expect, f, ensure_ascii=False, indent=1)
    print(f"wrote {len(expect) - 1} files to {directory}")


# --- kpi-render ---

class Cli:
    def __init__(self, args):
        self.cmd = ([args.emulator] if args.emulator else []) + [args.cli]

    def run(self, plugin, *args, check=True, timeout=None):
        result = subprocess.run(self.cmd + ["-k", plugin] + list(args), capture_output=True, timeout=timeout)
        if check and result.returncode != 0:
            raise RuntimeError(f"kpi-render {' '.join(args)} failed: {result.stderr.decode(errors='replace')}")
        return result

    def info(self, plugin, path, *args):
        out = self.run(plugin, *args, "--info", path).stdout.decode("utf-8")
        return dict(line.split("=", 1) for line in out.splitlines())


def load(directory):
    with open(os.path.join(directory, "expect.json"), encoding="utf-8") as f:
        expect = json.load(f)
    invalid = expect.pop("_invalid")
    return expect, invalid


def read_planar(directory, name, e):
    with open(os.path.join(directory, name + ".planar"), "rb") as f:
        data = f.read()
    n = e["bytes"]
    return [data[ch * n:(ch + 1) * n] for ch in range(e["channels"])]


def dop_reference(planar, word):
    nbytes = len(planar[0])
    frames = (nbytes + 1) // 2
    padded = [p + bytes([SILENCE]) * (frames * 2 - nbytes) for p in planar]
    out = bytearray()
    marker = 0x05
    for i in range(frames):
        for p in padded:
            if word == 4:
                out.append(0)
            out += bytes([p[2 * i + 1], p[2 * i], marker])
        marker ^= 0xFF
    return bytes(out)


def parallel(args, func, items):
    """items をそれぞれ func で調べる。kpi-render を別プロセスで同時に args.jobs 個まで走らせる。
    func は (表示する行のリスト, 失敗の数) を返す。結果は items の順に表示する"""
    fails = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for lines, n in pool.map(func, items):
            for line in lines:
                print(line, flush=True)
            fails += n
    return 1 if fails else 0


def check_dop(args):
    expect, invalid = load(args.dir)
    cli = Cli(args)

    def check(item):
        name, e = item
        lines, fails = [], 0
        path = os.path.join(args.dir, name)
        planar = read_planar(args.dir, name, e)
        frames = (e["bytes"] + 1) // 2
        for bits, word in (("24", 3), ("-64", 4), ("0", 3)):
            info = cli.info("kpidop.kpi", path, "-b", bits)
            want = {"format": "1", "rate": str(e["rate"] // 16), "channels": str(e["channels"]),
                    "bits": str(word * 8), "length": str(frames * 10000000 // (e["rate"] // 16)),
                    "loop": "0", "seekable": "7", "unit": "0"}
            for key, value in want.items():
                if info.get(key) != value:
                    lines.append(f"{name} -b {bits}: {key}={info.get(key)}, expected {value}")
                    fails += 1
            pcm = cli.run("kpidop.kpi", "-b", bits, path).stdout
            ref = dop_reference(planar, word)
            if pcm != ref:
                diff = next((i for i in range(min(len(pcm), len(ref))) if pcm[i] != ref[i]), min(len(pcm), len(ref)))
                lines.append(f"{name} -b {bits}: DoP differs at byte {diff} (got {len(pcm)} bytes, expected {len(ref)})")
                fails += 1
        lines.append(f"{name}: {'NG' if fails else 'ok'}")
        return lines, fails

    def refuse(item):
        name, plugin = item
        if cli.run(plugin, "--info", os.path.join(args.dir, name), check=False).returncode == 0:
            return [f"{name}: {plugin} opened a file it should refuse"], 1
        return [], 0

    ret = parallel(args, check, list(expect.items()))
    return parallel(args, refuse, [(n, p) for n in invalid for p in ("kpidop.kpi", "kpid2p.kpi")]) | ret


def check_tags(args):
    expect, _ = load(args.dir)
    cli = Cli(args)

    def check(item):
        name, e = item
        lines, fails = [], 0
        path = os.path.join(args.dir, name)
        for plugin in ("kpidop.kpi", "kpid2p.kpi"):
            for mode in ([], ["--tag-only"]):
                info = cli.info(plugin, path, *mode)
                tags = {k[4:]: v for k, v in info.items() if k.startswith("tag.")}
                label = f"{name} {plugin} {' '.join(mode)}"
                if tags != e["tags"]:
                    lines.append(f"{label}: tags {tags}, expected {e['tags']}")
                    fails += 1
                calls = sorted({k.split(".")[1] for k in info if k.startswith("gettaginfo.")})
                g = e["gettaginfo"]
                if calls != ["0"] or info.get("gettaginfo.0.type") != str(g["type"])                         or info.get("gettaginfo.0.data") != g.get("data"):
                    lines.append(f"{label}: GetTagInfo was called as {[(k, v[:40]) for k, v in info.items() if k.startswith('gettaginfo.')]}")
                    fails += 1
        lines.append(f"{name}: {'NG' if fails else 'ok'}")
        return lines, fails

    return parallel(args, check, list(expect.items()))


SEEK_RUNS = [("kpidop.kpi", ["-b", "24"]), ("kpidop.kpi", ["-b", "-64"]),
             ("kpid2p.kpi", []), ("kpid2p.kpi", ["-r", "44100", "-b", "-32"]), ("kpid2p.kpi", ["-r", "352800"])]


def seek_check_files(args, files):
    cli = Cli(args)

    # ファイルとプラグインの設定の組ごとに kpi-render を 1 つ走らせる
    def check(item):
        path, (plugin, opts) = item
        result = cli.run(plugin, *opts, "--seek-check", path, check=False)
        lines = result.stderr.decode("utf-8", errors="replace").splitlines()
        label = f"[{plugin} {' '.join(opts)}]"
        lines = [f"{label} {line}" for line in lines if line]
        if result.returncode != 0:
            lines.append(f"{label} {path}: NG")
            return lines, 1
        return lines, 0

    return parallel(args, check, [(f, run) for f in files for run in SEEK_RUNS])


def seek_check(args):
    expect, _ = load(args.dir)
    return seek_check_files(args, [os.path.join(args.dir, name) for name in expect])


def seek_check_data(args):
    files = []
    for root, _, names in os.walk(args.dir):
        files += [os.path.join(root, n) for n in sorted(names) if n.lower().endswith((".dsf", ".dff", ".wsd"))]
    if not files:
        print(f"no DSD files under {args.dir}")
        return 1
    return seek_check_files(args, files)


# --- D2P ---

def d2p_rule(dsd_rate, request):
    """kpid2p の出力周波数の選び方 (DsdDecimator::ChooseDecimation と同じ規則)"""
    request = request or 88200
    best = None
    d = 8
    while d <= 128:
        rate = dsd_rate // d
        if dsd_rate % d == 0 and 44100 <= rate <= 384000:
            distance = abs(math.log(rate / request))
            if best is None or distance < best[0] - 1e-9:
                best = (distance, d)
        d *= 2
    return best[1]


def write_dsf_planar(path, planar, rate):
    with open(path, "wb") as f:
        f.write(make_dsf(planar, rate, len(planar[0]) * 8, lsb_first=False))


def render_float(np, cli, path, *opts):
    out = cli.run("kpid2p.kpi", "-b", "-64", *opts, path).stdout
    return np.frombuffer(out, dtype="<f8")


def check_d2p(args):
    try:
        import numpy as np
    except ImportError:
        print("numpy is not installed; skipped")
        return 77
    cli = Cli(args)
    fails = 0
    work = os.path.join(args.dir, "d2p")
    os.makedirs(work, exist_ok=True)

    # 出力の周波数とビット数の選び方
    planar = [bytes([SILENCE]) * 4096] * 2
    for multiple in (64, 128, 256, 512):
        write_dsf_planar(os.path.join(work, f"silence_{multiple}.dsf"), planar, 44100 * multiple)

    def check_rate(item):
        multiple, request = item
        rate = 44100 * multiple
        d = d2p_rule(rate, request)
        info = cli.info("kpid2p.kpi", os.path.join(work, f"silence_{multiple}.dsf"), "-r", str(request))
        step = d // 8
        want = {"format": "0", "rate": str(rate // d), "bits": "-64", "seekable": "7", "unit": "0",
                "fadeout": "-1", "length": str(((4096 + step - 1) // step) * 10000000 // (rate // d))}
        lines = [f"DSD{multiple} -r {request}: {key}={info.get(key)}, expected {value}"
                 for key, value in want.items() if info.get(key) != value]
        return lines, len(lines)

    fails += parallel(args, check_rate, [(m, r) for m in (64, 128, 256, 512)
                                          for r in (0, 22050, 44100, 48000, 96000, 176400, 192000, 352800, 1000000)])
    for bits, want in (("-32", "-32"), ("16", "-64"), ("0", "-64")):
        got = cli.info("kpid2p.kpi", os.path.join(work, "silence_64.dsf"), "-b", bits).get("bits")
        if got != want:
            print(f"-b {bits}: bits={got}, expected {want}")
            fails += 1

    # 無音パターンだけのファイルはほぼ 0 になる
    for request in (44100, 88200, 352800):
        y = render_float(np, cli, os.path.join(work, "silence_64.dsf"), "-r", str(request))
        if np.max(np.abs(y)) > 1e-4:
            print(f"silence -r {request}: peak {np.max(np.abs(y)):.2e}")
            fails += 1

    # インパルス応答: 無音パターンのファイルと、ビットを 1 つずつ反転させたファイルの差から FIR を取り出す
    spacing = 32768  # 反転させるビットの間隔 (タップ数より長く)

    def check_impulse(item):
        multiple, request = item
        rate = 44100 * multiple
        d = d2p_rule(rate, request)
        fout = rate // d
        fp = min(fout * 0.4535, 30000)
        fstop = fout - fp
        flips = list(range(d))  # 反転させるビットの位相 0..D-1 (D の剰余がすべて出るように)
        nbits = spacing * (len(flips) + 2)
        base = np.tile(np.unpackbits(np.array([SILENCE], dtype=np.uint8)), nbits // 8)
        flipped = base.copy()
        positions = [spacing * (i + 1) + phase for i, phase in enumerate(flips)]
        for q in positions:
            flipped[q] ^= 1
        pa = os.path.join(work, f"impulse_{multiple}_{fout}_a.dsf")
        pb = os.path.join(work, f"impulse_{multiple}_{fout}_b.dsf")
        write_dsf_planar(pa, [np.packbits(base).tobytes()], rate)
        write_dsf_planar(pb, [np.packbits(flipped).tobytes()], rate)
        ya = render_float(np, cli, pa, "-r", str(request))
        yb = render_float(np, cli, pb, "-r", str(request))
        diff = yb - ya
        # 反転させたビット q の寄与は、出力 k について h(k*D - q) * 2 * (新しい値)。t = k*D - q で並べる
        half = spacing // 2
        g = np.zeros(2 * half)
        for q in positions:
            sign = 1.0 if flipped[q] else -1.0
            k0 = (q - half + d - 1) // d
            k1 = (q + half) // d
            for k in range(k0, k1):
                t = k * d - q + half
                if 0 <= t < 2 * half:
                    g[t] += diff[k] / (2 * sign)
        taps = np.count_nonzero(np.abs(g) > 1e-12)
        n = 1 << 20
        spectrum = np.abs(np.fft.rfft(g, n))
        freqs = np.fft.rfftfreq(n, 1 / rate)
        db = 20 * np.log10(np.maximum(spectrum, 1e-20))
        dc = db[0]
        passband = db[freqs <= fp]
        stopband = db[freqs >= fstop]
        ripple = passband.max() - passband.min()
        rejection = -stopband.max()
        nyquist = db[np.argmin(np.abs(freqs - fout / 2))]
        ok = abs(dc) < 1e-3 and ripple < 1e-3 and rejection > 110
        line = (f"DSD{multiple} -> {fout}Hz: {taps} taps, DC {dc:+.5f}dB, passband 0-{fp / 1000:.2f}kHz ripple {ripple:.6f}dB, "
                f"{fout / 2000:.2f}kHz {nyquist:+.2f}dB, stopband {fstop / 1000:.2f}kHz- {rejection:.1f}dB: {'ok' if ok else 'NG'}")
        return [line], 0 if ok else 1

    fails += parallel(args, check_impulse, [(64, 44100), (64, 88200), (64, 176400), (64, 352800), (128, 44100),
                                            (128, 88200), (256, 88200), (256, 352800)])

    # ΔΣ 変調 (2 次) した 997Hz、-6dBFS のサイン波。SINAD は変調器の量子化雑音で決まる
    rate = DSD64
    seconds = 0.3
    count = int(rate * seconds)
    x = 0.5 * np.sin(2 * np.pi * 997 * np.arange(count) / rate)
    bits = bytearray(count)
    i1 = i2 = 0.0
    y = 1.0
    for i, v in enumerate(x.tolist()):
        i1 += v - y
        i2 += i1 - y
        y = 1.0 if i2 >= 0 else -1.0
        bits[i] = 1 if y > 0 else 0
    path = os.path.join(work, "sine_997.dsf")
    write_dsf_planar(path, [np.packbits(np.frombuffer(bytes(bits), dtype=np.uint8)).tobytes()], rate)
    def measure(seg, fs):
        # 4 項の Blackman-Harris 窓 (サイドローブ -92dB)
        t = 2 * np.pi * np.arange(len(seg)) / len(seg)
        window = 0.35875 - 0.48829 * np.cos(t) + 0.14128 * np.cos(2 * t) - 0.01168 * np.cos(3 * t)
        spec = np.abs(np.fft.rfft(seg * window)) ** 2
        freqs = np.fft.rfftfreq(len(seg), 1 / fs)
        band = (freqs >= 20) & (freqs <= 20000)
        peak = np.argmax(np.where(band, spec, 0))
        signal = spec[peak - 6:peak + 7].sum()
        noise = spec[band].sum() - signal
        # 片側スペクトルのサイン波の電力は (N / 2) * (A^2 / 2) * sum(w^2)
        level = 10 * np.log10(4 * signal / (len(seg) * np.sum(window ** 2)))
        return level, 10 * np.log10(signal / noise), freqs[peak]

    pm1 = np.frombuffer(bytes(bits), dtype=np.uint8).astype(np.float64) * 2 - 1
    for request in (44100, 88200):
        out = render_float(np, cli, path, "-r", str(request))
        d = d2p_rule(rate, request)
        fout = rate // d
        start, length = len(out) // 4, 8192
        level, sinad, freq = measure(out[start:start + length], fout)
        # 同じ時間の DSD そのものを 20kHz で切った場合 (理想的なフィルタ) の SINAD。変調器の雑音で決まる
        _, ideal, _ = measure(pm1[start * d:(start + length) * d], rate)
        ok = abs(level + 6.02) < 0.1 and abs(freq - 997) < 10 and sinad > ideal - 0.5
        print(f"997Hz -6dBFS DSD64 -> {fout}Hz: level {level:+.2f}dBFS, SINAD (20Hz-20kHz) {sinad:.1f}dB "
              f"(ideal filter {ideal:.1f}dB): {'ok' if ok else 'NG'}")
        if not ok:
            fails += 1
    return 1 if fails else 0


# --- 壊れたファイル ---

def mutate(rng, data):
    """ヘッダの近くを中心に壊す。長さの欄によく出る値も入れる"""
    if len(data) < 8:
        return data + bytes(rng.getrandbits(8) for _ in range(rng.randint(0, 64)))
    data = bytearray(data)
    head = min(len(data), 4096)
    kind = rng.randrange(6)
    if kind == 0:  # ビットを反転させる
        for _ in range(rng.randint(1, 8)):
            i = rng.randrange(head)
            data[i] ^= 1 << rng.randrange(8)
    elif kind == 1:  # 4 / 8 バイトの欄を極端な値にする
        i = rng.randrange(max(1, head - 8))
        width = rng.choice((2, 4, 8))
        value = rng.choice((0, 1, 0x7F, 0xFF, 0x7FFFFFFF, 0xFFFFFFFF, (1 << 63) - 1, (1 << 64) - 1,
                            len(data), len(data) + 1, len(data) - 1, rng.getrandbits(64)))
        data[i:i + width] = (value & ((1 << (8 * width)) - 1)).to_bytes(width, rng.choice(("little", "big")))
    elif kind == 2:  # 途中で切る
        del data[rng.randrange(len(data) + 1):]
    elif kind == 3:  # 一部を乱数にする
        i = rng.randrange(head)
        n = rng.randint(1, 64)
        data[i:i + n] = bytes(rng.getrandbits(8) for _ in range(n))
    elif kind == 4:  # チャンクを複製・挿入する
        i, j = sorted(rng.randrange(len(data) + 1) for _ in range(2))
        k = rng.randrange(len(data) + 1)
        data[k:k] = data[i:min(j, i + 4096)]
    else:  # 先頭の 4 バイト以外をすべて同じ値にする
        fill = rng.choice((0, 0xFF))
        data[4:head] = bytes([fill]) * (head - 4)
    return bytes(data)


def fuzz_smoke(args):
    """合成したファイルを決まった乱数で壊して kpi-render に渡し、落ちたり止まったりしないことを確かめる。
    kpi-render が返してよいのは 0 (開けた) か 1 (開けなかった) だけ"""
    expect, invalid = load(args.dir)
    cli = Cli(args)
    work = os.path.join(args.dir, "broken")
    os.makedirs(work, exist_ok=True)
    rng = random.Random(args.seed)
    sources = sorted(list(expect) + invalid)
    cases = []
    for i in range(args.count):
        name = sources[i % len(sources)]
        with open(os.path.join(args.dir, name), "rb") as f:
            data = f.read()
        for _ in range(rng.randint(1, 3)):
            data = mutate(rng, data)
        path = os.path.join(work, f"{i:04d}_{name}")
        with open(path, "wb") as f:
            f.write(data)
        cases.append(path)

    runs = [("kpidop.kpi", ["--info"]), ("kpidop.kpi", ["--tag-only", "--info"]), ("kpidop.kpi", ["-n", "50000"]),
            ("kpid2p.kpi", ["-n", "20000"]), ("kpid2p.kpi", ["-r", "352800", "-b", "-32", "-n", "20000"]),
            ("kpidop.kpi", ["--seek-check"])]

    def check(path):
        lines = []
        for plugin, opts in runs:
            try:
                result = cli.run(plugin, *opts, path, check=False, timeout=60)
            except subprocess.TimeoutExpired:
                lines.append(f"{os.path.basename(path)}: {plugin} {' '.join(opts)} timed out")
                continue
            # --seek-check は内容が食い違うと 1 を返す (壊れたファイルでは食い違ってもよい)
            if result.returncode not in (0, 1):
                lines.append(f"{os.path.basename(path)}: {plugin} {' '.join(opts)} exited with {result.returncode:#x}")
        return lines, len(lines)

    ret = parallel(args, check, cases)
    print(f"{len(cases)} broken files x {len(runs)} runs: {'NG' if ret else 'ok'}")
    return ret


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["generate", "check-dop", "check-tags", "seek-check", "check-d2p", "seek-check-data", "fuzz-smoke"])
    parser.add_argument("dir")
    parser.add_argument("--cli")
    parser.add_argument("--emulator")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 1,
                        help="kpi-render を同時に走らせる数 (既定は CPU のスレッド数)")
    parser.add_argument("--count", type=int, default=600, help="fuzz-smoke で作る壊れたファイルの数")
    parser.add_argument("--seed", type=int, default=20261110, help="fuzz-smoke の乱数の種")
    args = parser.parse_args()
    if args.command == "generate":
        generate(args.dir)
        return 0
    if not args.cli:
        parser.error("--cli is required")
    return {"check-dop": check_dop, "check-tags": check_tags, "seek-check": seek_check,
            "check-d2p": check_d2p, "seek-check-data": seek_check_data, "fuzz-smoke": fuzz_smoke}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
