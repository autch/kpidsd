#ifndef wsd_types_h
#define wsd_types_h

#include <stdint.h>

#pragma pack(push, 1)

#define WSD_FILEID			"1bit"
#define WSD_FORMAT_VERSION	0x11

#define DSD_FREQ_64FS	( 2822400)		// = 44100 * 64
#define DSD_FREQ_128FS	( 5644800)		// = 44100 * 128

typedef struct {
	uint8_t		fileID[4];
	uint32_t	reserved1_1;
	uint8_t		version;
	uint8_t		reserved1_2;
	uint16_t	reserved1_3;
	uint64_t	fileSize;
	uint32_t	textOffset;
	uint32_t	dataOffset;
	uint32_t	reserved1_4;
} WSD_GENERAL_INFO;

typedef struct {
	uint32_t	playbackTime;
	uint32_t	samplingFrequency;
	uint32_t	reserved1_1;
	uint8_t		channels;
	uint8_t		reserved1_2[3];
	uint32_t	channelAssignment;
	uint8_t		reserved1_3[12];
	uint32_t	emphasis;
	uint32_t	reserved1_4;
	uint64_t	timeReferenceHi;
	uint64_t	timeReferenceLo;
	uint8_t		reserved1_5[40];
} WSD_DATA_SPEC;

typedef struct {
	uint8_t		title[128];
	uint8_t		composer[128];
	uint8_t		songWriter[128];
	uint8_t		artist[128];
	uint8_t		album[128];
	uint8_t		genre[32];
	uint8_t		dateAndTime[32];
	uint8_t		location[32];
	uint8_t		comment[512];
	uint8_t		userSpecific[512];
	uint8_t		reserved2[160];
} WSD_TEXT;

#pragma pack(pop)

#endif // !wsd_types_h
