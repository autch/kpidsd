#ifndef dop_h
#define dop_h

/* Based on DoP open Standard v1.1 */

#define DOP_FREQ_64FS				(176400)	// (44100*64) / 16
#define DOP_FREQ_128FS				(352800)	// (44100*128) / 16

#define DOP_BITS_PER_FRAME			24			// bits per DoP frame
#define DOP_BYTES_PER_FRAME			(DOP_BITS_PER_FRAME >> 3)
#define DOP_DSD_BITS_PER_FRAME		16			// DSD bits in one DoP frame
#define DOP_DSD_BYTES_PER_FRAME		(DOP_DSD_BITS_PER_FRAME >> 3)

#define DOP_MARKER1					0x05		// (0x05 ^ 0xff) == 0xfa
#define DOP_MARKER2					0xfa

#define DOP_MARKER_ALT_1			0x06
#define DOP_MARKER_ALT_2			0xf9

#endif // !dop_h
