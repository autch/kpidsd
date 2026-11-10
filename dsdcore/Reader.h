#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "kpi.h"

// ファイルの決まった位置から決まった長さを読む。パーサーと DsdSource はこれだけを使う
class Reader
{
public:
	virtual ~Reader() = default;
	virtual uint64_t Size() const = 0;
	// offset から size バイトを読む。全部読めなければ false
	virtual bool ReadAt(uint64_t offset, void* dst, size_t size) = 0;
};

// IKpiFile を読む。IKpiFile の参照は持たない (持ち主が AddRef している)
class KpiFileReader : public Reader
{
public:
	explicit KpiFileReader(IKpiFile* file) : file_(file), size_(file->GetSize()) {}

	uint64_t Size() const override { return size_; }

	bool ReadAt(uint64_t offset, void* dst, size_t size) override
	{
		if (offset > size_ || size > size_ - offset)
			return false;
		if (position_ != offset)
		{
			if (file_->Seek(static_cast<INT64>(offset), FILE_BEGIN) != offset)
			{
				position_ = UINT64_MAX;
				return false;
			}
			position_ = offset;
		}
		auto* out = static_cast<uint8_t*>(dst);
		while (size > 0)
		{
			const DWORD request = static_cast<DWORD>(std::min<size_t>(size, 1 << 20));
			const DWORD got = file_->Read(out, request);
			position_ += got;
			if (got < request)
				return false;
			out += got;
			size -= got;
		}
		return true;
	}

	// 他の誰か (本体のタグ解析など) が IKpiFile の位置を動かしたときに呼ぶ
	void Invalidate() { position_ = UINT64_MAX; }

private:
	IKpiFile* file_;
	uint64_t size_;
	uint64_t position_ = UINT64_MAX;
};

// メモリ上のデータを読む (テストと fuzz 用)
class MemoryReader : public Reader
{
public:
	MemoryReader(const void* data, size_t size) : data_(static_cast<const uint8_t*>(data)), size_(size) {}

	uint64_t Size() const override { return size_; }

	bool ReadAt(uint64_t offset, void* dst, size_t size) override
	{
		if (offset > size_ || size > size_ - offset)
			return false;
		std::memcpy(dst, data_ + offset, size);
		return true;
	}

private:
	const uint8_t* data_;
	size_t size_;
};

// バイト列から整数を読む
inline uint16_t ReadBE16(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }
inline uint32_t ReadBE32(const uint8_t* p)
{
	return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | p[3];
}
inline uint64_t ReadBE64(const uint8_t* p) { return (static_cast<uint64_t>(ReadBE32(p)) << 32) | ReadBE32(p + 4); }
inline uint32_t ReadLE32(const uint8_t* p)
{
	return (static_cast<uint32_t>(p[3]) << 24) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[1]) << 8) | p[0];
}
inline uint64_t ReadLE64(const uint8_t* p) { return (static_cast<uint64_t>(ReadLE32(p + 4)) << 32) | ReadLE32(p); }
