#pragma once

#include <cstdint>
#include <vector>

// DSD を 1 段の FIR で間引いて PCM にする。
//
// - 間引き率 D は 8 以上の 2 のべき乗 (出力は DSD の周波数 / D)。出力するサンプルだけを計算する
// - FIR はカイザー窓をかけた sinc を起動時に作る (直線位相、タップ数は 16 の倍数)
//   - 遮断周波数は出力のナイキスト周波数 (fout / 2)
//   - 通過域の端 fp = min(0.4535 * fout, 30kHz)、阻止域の端は fout - fp。
//     fout / 2 から fout - fp の間の成分は fp から fout / 2 の間に折り返すが、通過域には入らない
//   - 阻止域の減衰量は 120dB を目標にする
// - DSD の 1 バイト (8 サンプル) ごとに、係数 8 個の符号の組み合わせ 256 通りの和を表にしておき、
//   表を引いて足すだけで畳み込む。係数が左右対称なので、表は半分だけ持つ
class DsdDecimator
{
public:
	// 出力の周波数を選ぶ。requestedRate (0 なら 88.2kHz) に一番近いものを、
	// 44.1kHz 以上 384kHz 以下、D = 8〜128 の中から選び、D を返す。選べなければ 0
	static uint32_t ChooseDecimation(uint32_t dsdRate, uint32_t requestedRate);

	// フィルタを作る。decimation は 8 の倍数
	void Design(uint32_t dsdRate, uint32_t decimation);

	// 1 サンプルの計算に使う、チャンネルあたりの入力バイト数 (窓の長さ)
	size_t WindowBytes() const { return windowBytes; }
	// 1 サンプルごとに進む入力バイト数 (D / 8)
	size_t StepBytes() const { return stepBytes; }
	size_t Taps() const { return windowBytes * 8; }
	double PassbandEdge() const { return passband; }
	double StopbandEdge() const { return stopband; }

	// window[0] から WindowBytes() バイト (MSB first、古い順) を畳み込んだ値を返す。
	// 窓の中央 (WindowBytes() / 2 バイト目の先頭) のサンプルに対応する
	double Apply(const uint8_t* window) const
	{
		const float* table = tables.data();
		const size_t half = windowBytes / 2;
		const uint8_t* tail = window + windowBytes - 1;
		double acc = 0;
		for (size_t j = 0; j < half; j++, table += 256)
			acc += static_cast<double>(table[window[j]]) + table[bitReverse[tail[-static_cast<ptrdiff_t>(j)]]];
		return acc;
	}

private:
	size_t windowBytes = 0;
	size_t stepBytes = 0;
	double passband = 0;
	double stopband = 0;
	std::vector<float> tables;	// [windowBytes / 2][256]
	uint8_t bitReverse[256] = {};
};
