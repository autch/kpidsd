#include "stdafx.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "DsdDecimator.h"

namespace
{
	constexpr double DefaultRate = 88200;
	constexpr double MinRate = 44100;
	constexpr double MaxRate = 384000;
	constexpr uint32_t MinDecimation = 8;
	constexpr uint32_t MaxDecimation = 128;

	constexpr double PassbandRatio = 0.4535;	// 44.1kHz で 20kHz
	constexpr double MaxPassband = 30000;
	constexpr double Attenuation = 120;		// dB

	// 第 1 種変形ベッセル関数 I0
	double besselI0(double x)
	{
		double sum = 1, term = 1;
		const double q = x * x / 4;
		for (int k = 1; k < 500; k++)
		{
			term *= q / (static_cast<double>(k) * k);
			sum += term;
			if (term < sum * 1e-17)
				break;
		}
		return sum;
	}
}

uint32_t DsdDecimator::ChooseDecimation(uint32_t dsdRate, uint32_t requestedRate)
{
	const double request = requestedRate != 0 ? requestedRate : DefaultRate;
	uint32_t best = 0;
	double bestDistance = 0;
	// D の小さい (出力の周波数が高い) ほうから見るので、等距離なら高いほうが残る
	for (uint32_t d = MinDecimation; d <= MaxDecimation; d *= 2)
	{
		if (dsdRate % d != 0)
			continue;
		const double rate = static_cast<double>(dsdRate / d);
		if (rate < MinRate || rate > MaxRate)
			continue;
		const double distance = std::fabs(std::log(rate / request));
		if (best == 0 || distance < bestDistance - 1e-9)
		{
			best = d;
			bestDistance = distance;
		}
	}
	return best;
}

void DsdDecimator::Design(uint32_t dsdRate, uint32_t decimation)
{
	const double fs = dsdRate;
	const double fout = fs / decimation;
	passband = std::min(fout * PassbandRatio, MaxPassband);
	stopband = fout - passband;

	// カイザー窓の設計式
	const double beta = 0.1102 * (Attenuation - 8.7);
	const double transition = 2 * std::numbers::pi * (stopband - passband) / fs;
	size_t taps = static_cast<size_t>(std::ceil((Attenuation - 7.95) / (2.285 * transition))) + 1;
	taps = (taps + 15) / 16 * 16;	// 8 サンプル単位の表を左右対称に組めるように

	const double cutoff = 0.5 / decimation;	// fout / 2 を fs で割ったもの
	const double center = (static_cast<double>(taps) - 1) / 2;
	const double i0beta = besselI0(beta);
	std::vector<double> h(taps);
	double sum = 0;
	for (size_t n = 0; n < taps; n++)
	{
		const double t = n - center;
		const double x = 2 * cutoff * t;
		const double sinc = std::sin(std::numbers::pi * x) / (std::numbers::pi * x);	// t は整数にならない
		const double r = t / center;
		const double window = besselI0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / i0beta;
		h[n] = 2 * cutoff * sinc * window;
		sum += h[n];
	}
	for (double& c : h)
		c /= sum;	// 直流のゲインを 1 にする

	windowBytes = taps / 8;
	stepBytes = decimation / 8;

	// 表 j は係数 h[8j .. 8j+7] を受け持つ。バイトの MSB が h[8j] に掛かる。ビットが 1 なら +1、0 なら -1
	const size_t half = windowBytes / 2;
	tables.assign(half * 256, 0.0f);
	for (size_t j = 0; j < half; j++)
	{
		for (int byte = 0; byte < 256; byte++)
		{
			double acc = 0;
			for (int bit = 0; bit < 8; bit++)
				acc += ((byte >> (7 - bit)) & 1 ? 1.0 : -1.0) * h[j * 8 + bit];
			tables[j * 256 + byte] = static_cast<float>(acc);
		}
	}
	// 後ろ半分の係数は前半分を逆順にしたものなので、ビットを反転したバイトで前半分の表を引く
	for (int i = 0; i < 256; i++)
	{
		int r = 0;
		for (int bit = 0; bit < 8; bit++)
			if (i & (1 << bit))
				r |= 0x80 >> bit;
		bitReverse[i] = static_cast<uint8_t>(r);
	}
}
