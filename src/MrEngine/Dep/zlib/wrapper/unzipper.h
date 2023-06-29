#pragma once
#include "ioapi.h"
#include "unzip.h"
#include <string>
#include <vector>
#include <iostream>
#include <map>

#define OF_ENCRYPT_FILE_NAME "__OF_ENCRYPT_INFO__"

namespace OrangeFilter 
{

namespace ziputils
{
    class StreamBuffer;

    struct CacheData 
    {
        char* data;
        int size;

        CacheData() :
            data(NULL),
            size(0)
        {

        }
        ~CacheData()
        {
            if (data)
            {
                delete[] data;
                data = NULL;
            }
            size = 0;
        }
    };

	class unzipper
	{
	public:
		unzipper();
		~unzipper(void);

		bool open( const char* filename ); // open zip package.
        bool openMemory(const char* stream, unsigned int size);
		void close();
		bool isOpen();

		bool openEntry( const char* filename ); // open file inside the zip package.
		void closeEntry();
		bool isOpenEntry();
		unsigned int getEntrySize();

		const std::vector<std::string>& getFilenames();
		const std::vector<std::string>& getFolders();

		unzipper& operator>>( std::ostream& os );
		bool getEntryData(char* data, unsigned int size);

        bool isMemory();
		bool isEncrypted();

	private:
		void readEntries();

	private:
		unzFile			zipFile_;
		bool			entryOpen_;
		std::string		entryOpenPath_;

        StreamBuffer*   streamBuffer_;
        zlib_filefunc_def zlib_reader_;

		std::vector<std::string> files_;
		std::vector<std::string> folders_;

        std::map<std::string, CacheData> cacheDatas_;
	};
}
};
