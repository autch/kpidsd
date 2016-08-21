#ifndef dff_types_h
#define dff_types_h

#pragma pack(push, 1)

#include <stdint.h>
#include <vector>

#define DSD_FREQ_64FS	( 2822400)		// = 44100 * 64
#define DSD_FREQ_128FS	( 5644800)		// = 44100 * 128

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

namespace
{
	uint64_t ntohllX(uint64_t be)
	{
		uint8_t* p = (uint8_t*)&be;
		uint64_t r = p[4];

		r = (r << 8) | p[5];
		r = (r << 8) | p[6];
		r = (r << 8) | p[7];
		r = (r << 8) | p[0];
		r = (r << 8) | p[1];
		r = (r << 8) | p[2];
		r = (r << 8) | p[3];

		return r;
	}

	uint64_t ntohll(uint64_t be)
	{
		uint8_t* p = (uint8_t*)&be;
		uint64_t r = p[0];

		r = (r << 8) | p[1];
		r = (r << 8) | p[2];
		r = (r << 8) | p[3];
		r = (r << 8) | p[4];
		r = (r << 8) | p[5];
		r = (r << 8) | p[6];
		r = (r << 8) | p[7];

		return r;
	}

	uint32_t dff_ntohl(uint32_t be)
	{
		uint8_t* p = (uint8_t*)&be;
		uint32_t r = p[0];

		r = (r << 8) | p[1];
		r = (r << 8) | p[2];
		r = (r << 8) | p[3];

		return r;
	}
	
	uint16_t dff_ntohs(uint16_t be)
	{
		uint8_t* p = (uint8_t*)&be;
		return (p[0] << 8) | p[1];
	}
	
}

struct DFFChunkHeader
{
	DFFID		ckID;
	uint64_t	ckDataSize;
};

struct DFFChunk
{
	DFFChunkHeader	header;
	uint64_t		offsetToData;

	void setupHeader()
	{
		header.ckID = dff_ntohl(header.ckID);
		header.ckDataSize = ntohll(header.ckDataSize);
	}

	virtual void setupData() {}
	uint64_t OffsetToData() const { return offsetToData; }
	DFFID ID() const { return header.ckID; }
	uint64_t DataSize() const { return header.ckDataSize; }
};

struct FVERChunk : public DFFChunk
{
	struct
	{
		uint32_t	version;
	} data;

	void setupData()
	{
		data.version = dff_ntohl(data.version);
	}
};

struct FSChunk : public DFFChunk
{
	struct
	{
		uint32_t	sampleRate;
	} data;

	void setupData()
	{
		data.sampleRate = dff_ntohl(data.sampleRate);
	}
};

struct CHNLChunk : public DFFChunk
{
	struct
	{
		uint16_t	numChannels;
	} data;

	std::vector<DFFID> chID;

	void setupData()
	{
		data.numChannels = dff_ntohs(data.numChannels);
	}
};

struct CMPRChunk : public DFFChunk
{
	struct
	{
		DFFID		compressionType;
		uint8_t		count;
	} data;

	std::string	compressionName;

	void setupData()
	{
		data.compressionType = dff_ntohl(data.compressionType);
	}
};

struct ABSSChunk : public DFFChunk
{
	struct
	{
		uint16_t	hours;
		uint8_t		minutes;
		uint8_t		seconds;
		uint32_t	samples;
	} data;

	void setupData()
	{
		data.hours = dff_ntohs(data.hours);
		data.samples = dff_ntohl(data.samples);
	}
};

struct LSCOChunk : public DFFChunk
{
	struct
	{
		uint16_t	lsConfig;
	} data;

	void setupData()
	{
		data.lsConfig = dff_ntohs(data.lsConfig);
	}
};

struct PROPChunk : public DFFChunk
{
	struct
	{
		DFFID		propType;
	} data;

	FSChunk		fs;
	CHNLChunk	chnl;
	CMPRChunk	cmpr;
	ABSSChunk	abss;
	LSCOChunk	lsco;

	void setupData()
	{
		data.propType = dff_ntohl(data.propType);
	}
};

struct DSDChunk : public DFFChunk
{
	// dsd sample data
};

struct FRTEChunk : public DFFChunk
{
	struct
	{
		uint32_t	numFrames;
		uint16_t	frameRate;
	} data;

	void setupData()
	{
		data.numFrames = dff_ntohl(data.numFrames);
		data.frameRate = dff_ntohs(data.frameRate);
	}
};

struct DSTFChunk : public DFFChunk
{
	// dst frame data
};

struct DSTCChunk : public DFFChunk
{
	struct
	{
		uint32_t	crc;
	} data;

	void setupData()
	{
		data.crc = dff_ntohl(data.crc);
	}
};

struct DSTChunk : public DFFChunk
{
	struct 
	{
	} data;

	FRTEChunk	frte;
	DSTFChunk	dstf;
	DSTCChunk	dstc;
};


struct DSTFrameIndex;

struct DSTIChunk : public DFFChunk
{
	struct
	{
	} data;

	std::vector<DSTFrameIndex> indexData;
};

struct DSTFrameIndex
{
	struct
	{
		uint64_t	offset;
		uint32_t	length;
	} data;

	void setupData()
	{
		data.offset = ntohll(data.offset);
		data.length = dff_ntohl(data.length);
	}
};

struct Comment;

struct COMTChunk : public DFFChunk
{
	struct
	{
		uint16_t	numComments;
	} data;

	std::vector<Comment> comments;

	void setupData()
	{
		data.numComments = dff_ntohs(data.numComments);
	}
};

struct Comment
{
	struct
	{
		uint16_t	timeStampYear;
		uint8_t		timeStampMonth;
		uint8_t		timeStampDay;
		uint8_t		timeStampHour;
		uint8_t		timeStampMinutes;
		uint16_t	cmtType;
		uint16_t	cmtRef;
		uint32_t	count;
	} data;

	std::string commentText;

	void setupData()
	{
		data.timeStampYear = dff_ntohs(data.timeStampYear);
		data.cmtType = dff_ntohs(data.cmtType);
		data.cmtRef = dff_ntohs(data.cmtRef);
		data.count = dff_ntohl(data.count);
	}
};

struct EMIDChunk : public DFFChunk
{
	struct
	{
	} data;

	std::string	emid;
};

struct MARKChunk : public DFFChunk
{
	struct
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
	} data;

	std::string	markerText;

	void setupData()
	{
		data.hours = dff_ntohs(data.hours);
		data.samples = dff_ntohl(data.samples);
		data.offset = dff_ntohl(data.offset);
		data.markType = dff_ntohs(data.markType);
		data.markChannel = dff_ntohs(data.markChannel);
		data.trackFlags = dff_ntohs(data.trackFlags);
		data.count = dff_ntohl(data.count);
	}
};

struct DIARChunk : public DFFChunk
{
	struct
	{
		uint32_t	count;
	} data;

	std::string	artistText;

	void setupData()
	{
		data.count = dff_ntohl(data.count);
	}
};

struct DITIChunk : public DFFChunk
{
	struct
	{
		uint32_t	count;
	} data;

	std::string	titleText;

	void setupData()
	{
		data.count = dff_ntohl(data.count);
	}
};

struct DIINChunk : public DFFChunk
{
	struct
	{
	} data;

	EMIDChunk	emid;
	MARKChunk	mark;
	DIARChunk	diar;
	DITIChunk	diti;
};

struct MANFChunk : public DFFChunk
{
	struct
	{
		DFFID		manID;
	} data;

	void setupData()
	{
		data.manID = dff_ntohl(data.manID);
	}
};

struct FRM8Chunk : public DFFChunk
{
	struct
	{
		DFFID		formType;
	} data;

	FVERChunk	fver;
	PROPChunk	prop;
	DSDChunk	dsd;
	DSTChunk	dst;
	DSTIChunk	dsti;
	COMTChunk	comt;
	DIINChunk	diin;
	MANFChunk	manf;

	void setupData()
	{
		data.formType = dff_ntohl(data.formType);
	}
};


#pragma pack(pop)

#endif // !dff_types_h
