#ifndef dff_types_h
#define dff_types_h

#pragma pack(push, 1)

typedef uint32_t DFFID;

#define MAKE_DFFID(a, b, c, d)		(((((((a) << 8) | (b)) << 8) | (c)) << 8) | (d))
#define MAKE_DFFID_S(s)				(MAKE_DFFID(s[0], s[1], s[2], s[3]))

#define DFFID_FRM8					(MAKE_DFFID('F', 'R', 'M', '8'))
#define DFFID_DSD_					(MAKE_DFFID('D', 'S', 'D', ' '))
#define DFFID_FVER					(MAKE_DFFID('F', 'V', 'E', 'R'))
#define DFFID_PROP					(MAKE_DFFID('P', 'R', 'O', 'P'))
#define DFFID_SND_					(MAKE_DFFID('S', 'N', 'D', ' '))
#define DFFID_FS__					(MAKE_DFFID('F', 'S', ' ', ' '))
#define DFFID_CHNL					(MAKE_DFFID('C', 'H', 'N', 'L'))

// channel ids in CHNL
#define CHID_SLFT					(MAKE_DFFID('S', 'L', 'F', 'T'))
#define CHID_SRGT					(MAKE_DFFID('S', 'R', 'G', 'T'))
#define CHID_MLFT					(MAKE_DFFID('M', 'L', 'F', 'T'))
#define CHID_MRGT					(MAKE_DFFID('M', 'R', 'G', 'T'))
#define CHID_LS__					(MAKE_DFFID('L', 'S', ' ', ' '))
#define CHID_RS__					(MAKE_DFFID('R', 'S', ' ', ' '))
#define CHID_C___					(MAKE_DFFID('C', ' ', ' ', ' '))
#define CHID_LFE_					(MAKE_DFFID('L', 'F', 'E', ' '))

#define DFFID_CMPR					(MAKE_DFFID('C', 'M', 'P', 'R'))
#define CMPR_ID_DSD_				(MAKE_DFFID('D', 'S', 'D', ' '))
#define CMPR_ID_DST_				(MAKE_DFFID('D', 'S', 'T', ' '))
#define CMPR_NAME_DSD				"not compressed"
#define CMPR_NAME_DST				"DST Encoded"

#define DFFID_ABSS					(MAKE_DFFID('A', 'B', 'S', 'S'))
#define DFFID_LSCO					(MAKE_DFFID('L', 'S', 'C', 'O'))

#define LSCFG_STEREO				0
#define LSCFG_5CH_BS775_1			3
#define	LSCFG_6CH_BS775_1_LFE		4
#define LSCFG_UNDEFINED				65535

#define DFFID_DSD_					(MAKE_DFFID('D', 'S', 'D', ' '))
#define DFFID_DST_					(MAKE_DFFID('D', 'S', 'T', ' '))
#define DFFID_FRTE					(MAKE_DFFID('F', 'R', 'T', 'E'))

#define DST_FRAMES_PER_SECOND		75

#define DFFID_DSTF					(MAKE_DFFID('D', 'S', 'T', 'F'))
#define DFFID_DSTC					(MAKE_DFFID('D', 'S', 'T', 'C'))
#define DFFID_DSTI					(MAKE_DFFID('D', 'S', 'T', 'I'))
#define DFFID_COMT					(MAKE_DFFID('C', 'O', 'M', 'T'))

#define CMTTYPE_GENERAL				0
#define CMTTYPE_CHANNEL				1
#define CMTTYPE_SOURCE				2
#define CMTTYPE_HISTORY				3

#define CMTREF_GENERAL				0
#define CMTREF_CHANNEL_ALL			0
#define CMTREF_SOURCE_DSD			0
#define CMTREF_SOURCE_ANALOG		1
#define CMTREF_SOURCE_PCM			2
#define CMTREF_HISTORY_GENERAL		0
#define CMTREF_HISTORY_OPERATOR		1
#define CMTREF_HISTORY_MACHINE		2
#define CMTREF_HISTORY_TIMEZONE		3
#define CMTREF_HISTORY_REVISION		4

#define DFFID_DIIN					(MAKE_DFFID('D', 'I', 'I', 'N'))
#define DFFID_EMID					(MAKE_DFFID('E', 'M', 'I', 'D'))
#define DFFID_MARK					(MAKE_DFFID('M', 'A', 'R', 'K'))

#define MARKTYPE_TRACKSTART			0
#define MARKTYPE_TRACKSTOP			1
#define MARKTYPE_PROGRAMSTART		2
#define MARKTYPE_OBSOLETE			3
#define MARKTYPE_INDEX				4

#define MARKCH_ALL					0

#define DFFID_DIAR					(MAKE_DFFID('D', 'I', 'A', 'R'))
#define DFFID_DITI					(MAKE_DFFID('D', 'I', 'T', 'I'))
#define DFFID_MANF					(MAKE_DFFID('M', 'A', 'N', 'F'))

struct DFFChunk
{
	DFFID		ckID;
	uint64_t	ckDataSize;

	uint64_t	offsetToData;
};

struct FVERChunk : public DFFChunk
{
	uint32_t	version;
};

struct FSChunk : public DFFChunk
{
	uint32_t	sampleRate;
};

struct CHNLChunk : public DFFChunk
{
	uint16_t	numChannels;
	std::vector<DFFID> chID;
};

struct CMPRChunk : public DFFChunk
{
	DFFID		compressionType;
	uint8_t		count;
	std::string	compressionName;
};

struct ABSSChunk : public DFFChunk
{
	uint16_t	hours;
	uint8_t		minutes;
	uint8_t		seconds;
	uint32_t	samples;
};

struct LSCOChunk : public DFFChunk
{
	uint16_t	lsConfig;
};

struct PROPChunk : public DFFChunk
{
	DFFID		propType;

	FSChunk		fs;
	CHNLChunk	chnl;
	CMPRChunk	cmpr;
	ABSSChunk	abss;
	LSCOChunk	lsco;
};

struct DSDChunk : public DFFChunk
{
	// dsd sample data
};

struct FRTEChunk : public DFFChunk
{
	uint32_t	numFrames;
	uint16_t	frameRate;
};

struct DSTFChunk : public DFFChunk
{
	// dst frame data
};

struct DSTCChunk : public DFFChunk
{
	uint32_t	crc;
};

struct DSTChunk : public DFFChunk
{
	FRTEChunk	frte;
	DSTFChunk	dstf;
	DSTCChunk	dstc;
};


struct DSTFrameIndex;

struct DSTIChunk : public DFFChunk
{
	std::vector<DSTFrameIndex> indexData;
};

struct DSTFrameIndex
{
	uint64_t	offset;
	uint32_t	length;
};

struct Comment;

struct COMTChunk : public DFFChunk
{
	uint16_t	numComments;
	std::vector<Comment> comments;
};

struct Comment
{
	uint16_t	timeStampYear;
	uint8_t		timeStampMonth;
	uint8_t		timeStampDay;
	uint8_t		timeStampHour;
	uint8_t		timeStampMinutes;
	uint16_t	cmtType;
	uint16_t	cmtRef;
	uint32_t	count;
	std::string commentText;
};

struct EMIDChunk : public DFFChunk
{
	std::string	emid;
};

struct MARKChunk : public DFFChunk
{
	uint16_t	hours;
	uint8_t		minutes;
	uint8_t		seconds;
	uint32_t	samples;
	int32_t		offset;
	uint16_t	markType;
	uint16_t	markChannel;
	uint16_t	trackFlags;
	uint32_t	count;
	std::string	markerText;
};

struct DIARChunk : public DFFChunk
{
	uint32_t	count;
	std::string	artistText;
};

struct DITIChunk : public DFFChunk
{
	uint32_t	count;
	std::string	titleText;
};

struct DIINChunk : public DFFChunk
{
	EMIDChunk	emid;
	MARKChunk	mark;
	DIARChunk	diar;
	DITIChunk	diti;
};

struct MANFChunk : public DFFChunk
{
	DFFID		manID;
};

struct FRM8Chunk : public DFFChunk
{
	DFFID		formType;

	FVERChunk	fver;
	PROPChunk	prop;
	DSDChunk	dsd;
	DSTChunk	dst;
	DSTIChunk	dsti;
	COMTChunk	comt;
	DIINChunk	diin;
	MANFChunk	manf;
};


#pragma pack(pop)

#endif // !dff_types_h
