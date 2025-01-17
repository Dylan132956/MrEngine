#pragma once
#include "ioapi.h"
#include "zip.h"
#include <time.h>
#include <istream>

namespace OrangeFilter 
{

namespace ziputils
{
	class zipper
	{
	public:
		zipper();
		~zipper(void);

		bool open( const char* filename, bool append = false );
		void close();
		bool isOpen();

		bool addEntry( const char* filename );
		void closeEntry();
		bool isOpenEntry();

		zipper& operator<<( std::istream& is );

	private:
		void getTime(tm_zip& tmZip);

	private:
		zipFile			zipFile_;
		bool			entryOpen_;
	};

}
};

