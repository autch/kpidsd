#ifndef dsf_types_h
#define dsf_types_h

/* Based on "DSF File Format Specification, Version 1.01" from Sony Corporation */

#include <stdint.h>

#pragma pack(push, 1)

#define DSF_SIGNATURE_DSD_	0x20445344	// 'DSD '
#define DSF_HEADER_SIZE		28

typedef struct {
	uint32_t DSD_;						// 'D', 'S', 'D', ' '
	uint64_t size;						// 28
	uint64_t file_size;					// overall file size
	uint64_t id3v2_pointer;				// offset to id3v2 metadata chunk
} DSF_HEADER;

#define DSF_fmt_SIZE		52

#define DSF_FORMAT_VERSION	1

#define DSF_FMTID_RAW		0			// DSD raw

#define DSF_CHTYPE_MONO		1			// 1 : Center
#define DSF_CHTYPE_STEREO	2			// 1 : Front Left 2 : Front Right
#define DSF_CHTYPE_3CHS		3			// 1 : Front Left 2 : Front Right 3 : Center
#define DSF_CHTYPE_QUAD		4			// 1 : Front Left 2 : Front Right 3 : Back Left 4 : Back Right
#define DSF_CHTYPE_4CHS		5			// 1 : Front Left 2 : Front Right 3 : Center 4 : Low Frequency
#define DSF_CHTYPE_5CHS		6			// 1 : Front Left 2 : Front Right 3 : Center 4 : Back Left 5 : Back Right
#define DSF_CHTYPE_5_1CHS	7			// 1 : Front Left 2 : Front Right 3 : Center 4 : Low Frequency 5 : Back Left 6 : Back Right

#define DSD_FREQ_64FS	( 2822400)		// = 44100 * 64
#define DSD_FREQ_128FS	( 5644800)		// = 44100 * 128
#define DSD_FREQ_256FS	(11289600)		// = 44100 * 256

#define DSF_BPS_LSB		1				// LSb first
#define DSF_BPS_MSB		8				// MSb first

#define DSF_BLOCK_SIZE_PER_CHANNEL	4096
#define DSF_RESERVED	0

#define DSF_SIGNATURE_fmt_	0x20746d66	// 'fmt '

typedef struct {
	uint32_t fmt_;						// 'f', 'm', 't', ' '
	uint64_t size;						// 52
	uint32_t format_version;			// 1, DSF_FORMAT_VERSION
	uint32_t format_id;					// DSF_FMTID_*
	uint32_t channel_type;				// layout of speakers and interleaved samples, DSF_CHTYPE_*
	uint32_t channel_num;				// # of channels
	uint32_t sampling_frequency;		// 2822400 (DSD64), 5644800 (DSD128)
	uint32_t bits_per_sample;			// 1 or 8, DSF_BPS_*
	uint64_t sample_count;				// sampling_frequency * seconds
	uint32_t block_size_per_channel;	// 4096
	uint32_t reserved;					// 0
} DSF_fmt_HEADER;

#define DSF_SIGNATURE_data	0x61746164	// 'data'
#define DSF_data_SIZE		12

typedef struct {
	uint32_t data;						// 'd', 'a', 't', 'a'
	uint64_t size;
	// here comes data
} DSF_data_HEADER;

#pragma pack(pop)

#endif // !dsf_types_h
