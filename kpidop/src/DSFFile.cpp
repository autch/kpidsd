#include "stdafx.h"
#include "DSFFile.h"
#include "dsf_types.h"

namespace TagLib
{
	DSFFile::DSFFile(FileName file) : File(file), pTag(NULL)
	{
		if (isOpen())
			read();
	}

	DSFFile::DSFFile(IOStream* stream) : File(stream), pTag(NULL)
	{
		if (isOpen())
			read();
	}

	DSFFile::~DSFFile()
	{
	}

	Tag* DSFFile::tag() const
	{
		return pTag;
	}

	AudioProperties* DSFFile::audioProperties() const
	{
		return NULL;
	}

	bool DSFFile::save()
	{
		return false;
	}


	void DSFFile::read()
	{
		TagLib::ByteVector DSD_;						// 'D', 'S', 'D', ' '
		TagLib::ulonglong size;						// 28
		TagLib::ulonglong file_size;					// overall file size
		TagLib::ulonglong id3v2_pointer;				// offset to id3v2 metadata chunk

		DSD_ = readBlock(4);
		size = readBlock(8).toLongLong(false);
		file_size = readBlock(8).toLongLong(false);
		id3v2_pointer = readBlock(8).toLongLong(false);

		if (id3v2_pointer == 0) {
			pTag = new ID3v2::Tag;
		}
		else {
			pTag = new ID3v2::Tag(this, (long)id3v2_pointer);
		}
	}
}