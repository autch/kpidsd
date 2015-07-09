#include "stdafx.h"
#include "CDFFFile.h"

CDFFFile::CDFFFile()
{

}

CDFFFile::~CDFFFile()
{
	CLargeFile::~CLargeFile();
}

void CDFFFile::Close()
{
	CLargeFile::Close();
}

bool CDFFFile::readChunkHeader(DFFChunkHeader& header, ChunkStep& step)
{
	DWORD dwBytesRead = 0;
	step.headerOffset = Tell();

	if (!Read(&header, sizeof header, &dwBytesRead) || dwBytesRead != sizeof header)
		return false;
	header.ckID = ntohl(header.ckID);
	header.ckDataSize = ntohll(header.ckDataSize);

	step.header = header;
	step.dataOffset = Tell();
	step.dataEndOffset = step.dataOffset + step.header.ckDataSize + (step.header.ckDataSize & 1);

	return true;
}

BOOL CDFFFile::Open(LPCSTR cszFileName)
{
	if (!CLargeFile::Open(cszFileName))
		return FALSE;

	ChunkStep step;
	DFFChunkHeader hdr;

	while (readChunkHeader(hdr, step))
	{
		DFFID parentID = stack.empty() ? 0x0000 : stack.top().header.ckID;
		bool handled = false;

		stack.push(step);

		handled = handled || readFRM8(step, hdr);
		handled = handled || readPROP(step, hdr);
		handled = handled || readDIIN(step, hdr);
		if (!handled)
		{
			// default action -- skip that chunk
			Seek(step.dataEndOffset, NULL, FILE_BEGIN);
			stack.pop();
		}

	}

	stack.pop();

	if (!stack.empty())
	{
		// invalid stack state -- hiearchy mismatch
	}

	return TRUE;
}

bool CDFFFile::readFRM8(ChunkStep& step, DFFChunkHeader& hdr)
{
	switch (hdr.ckID)
	{
	case DFFID_FRM8:
	{
		frm8.header = hdr;
		frm8.offsetToData = step.dataOffset;

		Read(&frm8.data, sizeof frm8.data, NULL);
		frm8.setupData();
		// descend to subchunks
		break;
	}
	case DFFID_FVER:
	{
		Read(&frm8.fver.data, sizeof frm8.fver.data, NULL);
		frm8.fver.setupData();
		stack.pop(); // ascend to parent (FRM8) chunk
		break;
	}
	case DFFID_DSD_:
	{
		frm8.dsd.header = hdr;
		frm8.dsd.offsetToData = step.dataOffset;
		Seek(step.dataEndOffset, NULL, FILE_BEGIN);
		stack.pop(); // ascend to parent (FRM8) chunk
		break;
	}
	case DFFID_DST_:
	case DFFID_DSTI:
	case DFFID_COMT:
	case DFFID_MANF:
	default:
		return false;
	}
	return true;
}

bool CDFFFile::readPROP(ChunkStep& step, DFFChunkHeader& hdr)
{
	switch (hdr.ckID)
	{
	case DFFID_PROP:
	{
		frm8.prop.header = hdr;
		frm8.prop.offsetToData = step.dataOffset;
		Read(&frm8.prop.data, sizeof frm8.prop.data, NULL);
		frm8.prop.setupData();
		// descend to subchunks
		break;
	}
	case DFFID_FS__:
		frm8.prop.fs.header = hdr;
		frm8.prop.fs.offsetToData = step.dataOffset;
		Read(&frm8.prop.fs.data, sizeof frm8.prop.fs.data, NULL);
		frm8.prop.fs.setupData();
		stack.pop();	// ascend to parent (PROP) chunk
		break;
	case DFFID_CHNL:
		frm8.prop.chnl.header = hdr;
		frm8.prop.chnl.offsetToData = step.dataOffset;
		Read(&frm8.prop.chnl.data, sizeof frm8.prop.chnl.data, NULL);
		frm8.prop.chnl.setupData();
		for (int i = 0; i < frm8.prop.chnl.data.numChannels; i++)
		{
			DFFID id;
			Read(&id, sizeof id, NULL);
			id = ntohl(id);
			frm8.prop.chnl.chID.push_back(id);
		}
		stack.pop();	// ascend to parent (PROP) chunk
		break;
	case DFFID_CMPR:
		frm8.prop.cmpr.header = hdr;
		frm8.prop.cmpr.offsetToData = step.dataOffset;
		Read(&frm8.prop.cmpr.data, sizeof frm8.prop.cmpr.data, NULL);
		frm8.prop.cmpr.setupData();
		
		assignBuffer(frm8.prop.cmpr.data.count, frm8.prop.cmpr.compressionName);
		Seek(step.dataEndOffset, NULL, FILE_BEGIN);

		stack.pop();	// ascend to parent (PROP) chunk
		break;
	case DFFID_ABSS:
		frm8.prop.abss.header = hdr;
		frm8.prop.abss.offsetToData = step.dataOffset;
		Read(&frm8.prop.abss.data, sizeof frm8.prop.abss.data, NULL);
		frm8.prop.abss.setupData();
		stack.pop();	// ascend to parent (PROP) chunk
		break;
	case DFFID_LSCO:
		frm8.prop.lsco.header = hdr;
		frm8.prop.lsco.offsetToData = step.dataOffset;
		Read(&frm8.prop.lsco.data, sizeof frm8.prop.lsco.data, NULL);
		frm8.prop.lsco.setupData();
		stack.pop();	// ascend to parent (PROP) chunk
		break;
	default:
		return false;
	}
	return true;
}

bool CDFFFile::readDIIN(ChunkStep& step, DFFChunkHeader& hdr)
{
	switch (hdr.ckID)
	{
	case DFFID_DIIN:
	{
		frm8.diin.header = hdr;
		frm8.diin.offsetToData = step.dataOffset;
		// descend to subchunks
		break;
	}
	case DFFID_DIAR:
	{
		frm8.diin.diar.header = hdr;
		frm8.diin.diar.offsetToData = step.dataOffset;
		Read(&frm8.diin.diar.data, sizeof frm8.diin.diar.data, NULL);
		frm8.diin.diar.setupData();

		assignBuffer(frm8.diin.diar.data.count, frm8.diin.diar.artistText);
		Seek(step.dataEndOffset, NULL, FILE_BEGIN); // may contain padding byte

		stack.pop(); // ascend to parent (DIIN) chunk
		break;
	}
	case DFFID_DITI:
	{
		frm8.diin.diti.header = hdr;
		frm8.diin.diti.offsetToData = step.dataOffset;
		Read(&frm8.diin.diti.data, sizeof frm8.diin.diti.data, NULL);
		frm8.diin.diti.setupData();

		assignBuffer(frm8.diin.diti.data.count, frm8.diin.diti.titleText);
		Seek(step.dataEndOffset, NULL, FILE_BEGIN); // may contain padding byte

		stack.pop(); // ascend to parent (DIIN) chunk
		break;
	}
	default:
		return false;
	}
	return true;
}

void CDFFFile::assignBuffer(uint32_t bytesToRead, std::string& target)
{
	uint8_t* buf = new uint8_t[bytesToRead + 1];
	buf[bytesToRead] = '\0';
	Read(buf, bytesToRead, NULL);
	target.assign((const char*)buf);
	delete[] buf;
}
