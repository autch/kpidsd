#include "taglib.h"
#include "tag.h"
#include "tbytevector.h"
#include "tiostream.h"
#include "tfile.h"
#include "id3v2tag.h"

namespace TagLib
{
	// "Fake" DSF File class impl. for TagLib 
	class DSFFile : public TagLib::File
	{
	public:
		virtual ~DSFFile();

		virtual Tag* tag() const;
		virtual AudioProperties* audioProperties() const;
		virtual bool save();

		DSFFile(FileName file);
		DSFFile(IOStream* stream);

	protected:
		ID3v2::Tag* pTag;

		void read();
	};
}
