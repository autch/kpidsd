#pragma once

#include <stdint.h>

#pragma pack(push, 1)

#define ID3V2_HEADER_ID	"ID3"
#define ID3V2_FOOTER_ID	"3DI"

typedef uint32_t ID3V2_ID;		// four-character IDs
typedef uint32_t syncsafe_t; // syncsafe integer

#define MAKE_ID3V2_ID(a, b, c, d)		(((((((a) << 8) | (b)) << 8) | (c)) << 8) | (d))
#define MAKE_ID3V2_ID_S(s)				(MAKE_ID3V2_ID((s)[0], (s)[1], (s)[2], (s)[3]))

#define ID3V2_VERSION_3	0x03
#define ID3V2_VERSION_4	0x04

#define ID3V23_HEADER_UNSYNC(f)	((f) & 0x80)
#define ID3V23_HEADER_EXT_HEADER(f)	((f) & 0x40)
#define ID3V23_HEADER_EXPERIMENTAL(f)	((f) & 0x20)
#define ID3V24_HEADER_HAS_FOOTER(f)	((f) & 0x10)

#define ID3V24_EXT_UPDATE(f)	((f) & 0x40)
#define ID3V24_EXT_CRC(f)	((f) & 0x20)
#define ID3V24_EXT_RESTRICTION(f)	((f) & 0x10)

#define ID3V24_RESTRICT_128F_1MB(f)	(((f) & 0c0) == 0x00)
#define ID3V24_RESTRICT_64F_128KB(f) (((f) & 0c0) == 0x40)
#define ID3V24_RESTRICT_32F_40KB(f) (((f) & 0c0) == 0x80)
#define ID3V24_RESTRICT_32F_4KB(f) (((f) & 0c0) == 0xc0)

#define ID3V24_RESTRICT_ENC_8BIT(f)	((f) & 0x20)

#define ID3V24_RESTRICT_SIZE_NO_LIMIT(f)	(((f) & 0x18) == 0x00)
#define ID3V24_RESTRICT_SIZE_1024(f)	(((f) & 0x18) == 0x08)
#define ID3V24_RESTRICT_SIZE_128(f)	(((f) & 0x18) == 0x10)
#define ID3V24_RESTRICT_SIZE_30(f)	(((f) & 0x18) == 0x18)

#define ID3V24_RESTRICT_IMG_PNG_AND_JPEG(f)	((f) & 0x04)

#define ID3V24_RESTRICT_IMGSIZE_NO_LIMIT(f)	(((f) & 0x03) == 0x00)
#define ID3V24_RESTRICT_IMGSIZE_256X256(f)	(((f) & 0x03) == 0x01)
#define ID3V24_RESTRICT_IMGSIZE_64X64(f)	(((f) & 0x03) == 0x02)
#define ID3V24_RESTRICT_IMGSIZE_EXACTLY_64X64(f)	(((f) & 0x03) == 0x03)

#define ID3V2_ENCODING_ISO8859_1	0x00	// 0x00 terminated
#define ID3V2_ENCODING_UTF16BOM		0x01	// w/ BOM, 0x00 0x00 terminated
#define ID3V2_ENCODING_UTF16BE		0x02	// w/o BOM, 0x00 0x00 terminated
#define ID3V2_ENCODING_UTF8			0x03	// 0x00 terminated


struct ID3V2Header
{
	uint8_t id[3];
	uint8_t version;
	uint8_t revision;
	uint8_t flags;	// see ID3V2_HEADER_*() macro
	syncsafe_t size;
};

struct ID3V24Footer
{
	uint8_t id[3];
	uint8_t version;
	uint8_t revision;
	uint8_t flags;	// see ID3V2_HEADER_*() macro
	syncsafe_t size;
};

struct ID3V23ExtHeader
{
	syncsafe_t size;
	uint8_t flags1;
	uint8_t flags2;
	syncsafe_t padding_size;
};

struct ID3V24ExtHeader
{
	syncsafe_t size;
	uint8_t flag_size;
	uint8_t flags;
};

struct ID3V2FrameHeader
{
	ID3V2_ID id;
	uint32_t size;
	uint8_t flags1;
	uint8_t flags2;
};

#pragma pack(pop)
