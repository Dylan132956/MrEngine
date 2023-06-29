#include "unzipper.h"
#include <zlib.h>
#include <algorithm>
#include <sstream>
#include <cstring>

namespace OrangeFilter {

namespace ziputils
{

class StreamBuffer
{

    public:
        //static const Data Null;
        StreamBuffer() :
            _bytes(nullptr),
            _size(0),
            _index(0),
            _gcnt(0)
        {
            
        }

        StreamBuffer(StreamBuffer&& other) :
            _bytes(nullptr),
            _size(0),
            _index(0),
            _gcnt(0)
        {
            move(other);
        }

        StreamBuffer(const StreamBuffer& other) :
            _bytes(nullptr),
            _size(0),
            _index(0),
            _gcnt(0)
        {
            copy(other._bytes, other._size);
        }

        ~StreamBuffer()
        {
            clear();
        }

        StreamBuffer& operator= (const StreamBuffer& other)
        {
            copy(other._bytes, other._size);
            return *this;
        }

        StreamBuffer& operator= (StreamBuffer&& other)
        {
            move(other);
            return *this;
        }

        void move(StreamBuffer& other)
        {
            clear();

            _bytes = other._bytes;
            _size = other._size;

            other._bytes = nullptr;
            other._size = 0;
            other._index = 0;
            other._gcnt = 0;
        }

        bool isNull() const
        {
            return (_bytes == nullptr || _size == 0);
        }

        unsigned char* getBytes() const
        {
            return _bytes;
        }

        unsigned int getSize() const
        {
            return _size;
        }

        void copy(const unsigned char* bytes, const unsigned int size)
        {
            clear();

            if (size > 0)
            {
                _size = size;
                _bytes = (unsigned char*)malloc(sizeof(unsigned char) * _size);
                memcpy(_bytes, bytes, _size);
                _index = 0;
                _gcnt = 0;
            }
        }

        void fastSet(unsigned char* bytes, const unsigned int size)
        {
            _bytes = bytes;
            _size = size;
            _index = 0;
            _gcnt = 0;
        }

        void clear()
        {
            if (_bytes)
            {
                free(_bytes);
                _bytes = nullptr;
            }
            _size = 0;
            _index = 0;
            _gcnt = 0;
        }

        unsigned char* takeBuffer(unsigned int* size)
        {
            auto buffer = getBytes();
            if (size)
                *size = getSize();
            fastSet(nullptr, 0);
            return buffer;
        }

        bool eof() const
        {
            return _index >= _size;
        }

        bool good() const
        {
            return !eof();
        }

        unsigned int gcount() const
        {
            return _gcnt;
        }

        unsigned int tellg() const
        {
            return _index;
        }
        bool seekg(unsigned int pos, int dir)
        {
            unsigned int tIdx = 0;
            if (dir == 0)
            {
                tIdx = pos;
            }
            else if (dir == 1)
            {
                tIdx = _index + pos;
            }
            else if (dir == 2)
            {
                tIdx = _size + pos;
            }

            /*if (tIdx >= _size)
                return false;*/

            _index = tIdx;
            return true;
        }

        //template<typename T>
        //void read(T& t)
        //{
        //    if (eof())
        //    {
        //        throw std::runtime_error("Data::read failed, Premature end of array!");
        //    }
        //
        //    if ((_index + sizeof(T)) > _size)
        //        throw std::runtime_error("Premature end of array!");
        //
        //    memcpy(reinterpret_cast<void*>(&t), &_bytes[_index], sizeof(T));
        //
        //    //old::swap(t, m_same_type);
        //
        //    _index += sizeof(T);
        //    _gcnt = sizeof(T);
        //}

        void read(char* p, size_t size)
        {
            if (eof())
            {
                throw std::runtime_error("Data::read failed, Premature end of array!");
            }

            if ((_index + size) > _size)
            {
                throw std::runtime_error("Data::read failed, Premature end of array!");
            }

            memcpy((void*)(p), &_bytes[_index], size);

            _index += size;
            _gcnt = size;
        }

        void read(std::string& str, const unsigned int size)
        {
            if (eof())
            {
                throw std::runtime_error("Data::read failed, Premature end of array!");
            }

            if ((_index + size) > _size)
            {
                throw std::runtime_error("Data::read failed, Premature end of array!");
            }

            str.assign(_bytes[_index], size);

            _index += str.size();
            _gcnt = str.size();
        }

        void readLine(char* str, size_t maxSize)
        {
            if (eof())
            {
                throw std::runtime_error("Data::readLine failed, Premature end of array!");
            }

            int tIdx = _index;
            while (tIdx < _size && tIdx - _index < maxSize - 1 && _bytes[tIdx] != '\n')
            {
                tIdx += 1;
            }

            memcpy(reinterpret_cast<void*>(str), &_bytes[_index], tIdx - _index);
            str[tIdx - _index + 1] = '\0';

            _index += tIdx - _index;
            _gcnt = tIdx - _index;

            if (!eof() && _bytes[_index] == '\n')
            {
                _index += 1;
            }
        }

    private:
        unsigned char* _bytes;
        unsigned int _size;
        size_t _index;
        size_t _gcnt;
 };

 // -------------- zlib reader --------------
 voidpf ZCALLBACK unzipper_fopen_file_func(voidpf opaque, const char* filename, int mode)
 {
     /*FILE* file = NULL;
     const char* mode_fopen = NULL;
     if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ)
         mode_fopen = "rb";
     else
         if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
             mode_fopen = "r+b";
         else
             if (mode & ZLIB_FILEFUNC_MODE_CREATE)
                 mode_fopen = "wb";

     if ((filename != NULL) && (mode_fopen != NULL))
         file = fopen(filename, mode_fopen);*/
     return opaque;
 }


 uLong ZCALLBACK unzipper_fread_file_func( voidpf opaque, voidpf stream, void* buf, uLong size)
 {
     /*uLong ret;
     ret = (uLong)fread(buf, 1, (size_t)size, (FILE *)stream);
     return ret;*/

     StreamBuffer* streamBuffer = static_cast<StreamBuffer*>(opaque);
     streamBuffer->read((char*)buf, size);
     //printf("read - need size: %d, real size:%d\n", size, streamBuffer->gcount());
     return streamBuffer->gcount();
 }


 uLong ZCALLBACK unzipper_fwrite_file_func( voidpf opaque, voidpf stream, const void* buf, uLong size)
 {
     /*uLong ret;
     ret = (uLong)fwrite(buf, 1, (size_t)size, (FILE *)stream);
     return ret;*/
     printf("unzipper_fwrite_file_func Error: unzipper not support write mode\n");
     return 0;
 }

 long ZCALLBACK unzipper_ftell_file_func( voidpf opaque, voidpf stream)
 {
     /*long ret;
     ret = ftell((FILE *)stream);
     return ret;*/
     StreamBuffer* streamBuffer = static_cast<StreamBuffer*>(opaque);
     return streamBuffer->tellg();
 }

 long ZCALLBACK unzipper_fseek_file_func(voidpf opaque, voidpf stream, uLong offset, int origin)
 {
     int fseek_origin = 0;
     switch (origin)
     {
     case ZLIB_FILEFUNC_SEEK_CUR:
         fseek_origin = SEEK_CUR;
         break;
     case ZLIB_FILEFUNC_SEEK_END:
         fseek_origin = SEEK_END;
         break;
     case ZLIB_FILEFUNC_SEEK_SET:
         fseek_origin = SEEK_SET;
         break;
     default: return -1;
     }
     //fseek((FILE *)stream, offset, fseek_origin);
     StreamBuffer* streamBuffer = static_cast<StreamBuffer*>(opaque);
     bool r = streamBuffer->seekg(offset, fseek_origin);
     //printf("seek - offset:%d, dir:%d, result:%d\n", offset, fseek_origin, r);
     return 0;
 }

 int ZCALLBACK unzipper_fclose_file_func(voidpf opaque, voidpf stream)
 {
     /*int ret;
     ret = fclose((FILE *)stream);
     return ret;*/

     return 0;
 }

 int ZCALLBACK unzipper_ferror_file_func(voidpf opaque, voidpf stream)
 {
     /*int ret;
     ret = ferror((FILE *)stream);
     return ret;*/
     /*StreamBuffer* streamBuffer = static_cast<StreamBuffer*>(opaque);
     return streamBuffer->eof() ? 1 : 0;*/
     return 0;
 }

 void unzipper_fill_fopen_filefunc(zlib_filefunc_def* pzlib_filefunc_def, voidpf opaque)
 {
     pzlib_filefunc_def->zopen_file = unzipper_fopen_file_func;
     pzlib_filefunc_def->zread_file = unzipper_fread_file_func;
     pzlib_filefunc_def->zwrite_file = unzipper_fwrite_file_func;
     pzlib_filefunc_def->ztell_file = unzipper_ftell_file_func;
     pzlib_filefunc_def->zseek_file = unzipper_fseek_file_func;
     pzlib_filefunc_def->zclose_file = unzipper_fclose_file_func;
     pzlib_filefunc_def->zerror_file = unzipper_ferror_file_func;
     pzlib_filefunc_def->opaque = opaque;
 }


	// Default constructor
	unzipper::unzipper() :
		zipFile_( 0 ), 
		entryOpen_( false ),
        streamBuffer_(nullptr)
	{
        zlib_reader_.opaque = nullptr;
	}

	// Default destructor
	unzipper::~unzipper(void)
	{
		close();
	}

	// open a zip file.
	// param:
	// 		filename	path and the filename of the zip file to open
	//
	// return:
	// 		true if open, false otherwise
	bool unzipper::open( const char* filename ) 
	{
		close();
		//zipFile_ = unzOpen64( filename );
		zipFile_ = unzOpen(filename);
		if ( zipFile_ )
		{
			readEntries();
		}

		return isOpen();
	}

    bool unzipper::openMemory(const char* stream, unsigned int size)
    {
        close();

        streamBuffer_ = new StreamBuffer();
        streamBuffer_->copy((const unsigned char*)stream, size);
        if (streamBuffer_->isNull())
        {
            return false;
        }

        unzipper_fill_fopen_filefunc(&zlib_reader_, streamBuffer_);

        zipFile_ = unzOpen2("", &zlib_reader_);
        if (zipFile_)
        {
            readEntries();
        }

        return isOpen();
    }

	// Close the zip file
	void unzipper::close()
	{
		if ( zipFile_ )
		{
			files_.clear();
			folders_.clear();

			closeEntry();
			unzClose( zipFile_ );
			zipFile_ = 0;

            cacheDatas_.clear();
		}

        if (streamBuffer_)
        {
            delete streamBuffer_;
            streamBuffer_ = nullptr;
            zlib_reader_.opaque = nullptr;
        }
	}

	bool unzipper::isEncrypted()
	{
		for (auto&& s : files_)
		{
			if (s == OF_ENCRYPT_FILE_NAME)
			{
				return true;
			}
		}

		return false;
	}

    bool unzipper::isMemory()
	{
		return streamBuffer_ != nullptr;
	}

	// Check if a zipfile is open.
	// return:
	//		true if open, false otherwise
	bool unzipper::isOpen()
	{
		return zipFile_ != 0;
	}

	// Get the list of file zip entires contained in the zip file.
	const std::vector<std::string>& unzipper::getFilenames()
	{
		return files_;
	}

	// Get the list of folders zip entires contained in the zip file.
	const std::vector<std::string>& unzipper::getFolders()
	{
		return folders_;
	}

	// open an existing zip entry.
	// return:
	//		true if open, false otherwise
	bool unzipper::openEntry( const char* filename )
	{
		if ( isOpen() && entryOpenPath_ != filename)
		{
			closeEntry();
			
			int err = unzLocateFile( zipFile_, filename, 0 );
			if ( err == UNZ_OK )
			{
				err = unzOpenCurrentFile( zipFile_ );
				entryOpen_ = (err == UNZ_OK);
				entryOpenPath_ = filename;
			}
		}
		return entryOpen_;
	}

	// Close the currently open zip entry.
	void unzipper::closeEntry()
	{
		if ( entryOpen_ )
		{
			unzCloseCurrentFile( zipFile_ );
			entryOpen_ = false;
			entryOpenPath_.clear();
		}
	}

	// Check if there is a currently open zip entry.
	// return:
	//		true if open, false otherwise
	bool unzipper::isOpenEntry()
	{
		return entryOpen_;
	}

	// Get the zip entry uncompressed size.
	// return:
	//		zip entry uncompressed size
	unsigned int unzipper::getEntrySize()
	{
		if ( entryOpen_ )
		{
			//unz_file_info64 oFileInfo;
			//int err = unzGetCurrentFileInfo64( zipFile_, &oFileInfo, 0, 0, 0, 0, 0, 0);

			unz_file_info oFileInfo;

			int err = unzGetCurrentFileInfo(zipFile_, &oFileInfo, 0, 0, 0, 0, 0, 0);

			if ( err == UNZ_OK )
			{
				return (unsigned int)oFileInfo.uncompressed_size;
			}

		}
		return 0;
	}

	bool unzipper::getEntryData(char* data, unsigned int size)
	{
		if (size < getEntrySize())
		{
			return false;
		}

		if (!isOpenEntry())
		{
			return false;
		}

        size = getEntrySize();

        if (cacheDatas_.find(entryOpenPath_) != cacheDatas_.end())
        {
            CacheData& cacheData = cacheDatas_[entryOpenPath_];
            memcpy(data, cacheData.data, cacheData.size);
            return true;
        }

		size = unzReadCurrentFile(zipFile_, data, size);
		if (size == 0)
		{
			return false;
		}

        cacheDatas_[entryOpenPath_].data = new char[size];
        memcpy(cacheDatas_[entryOpenPath_].data, data, size);
        cacheDatas_[entryOpenPath_].size = size;

		return true;
	}

	// Private method used to build a list of files and folders.
	void unzipper::readEntries()
	{
		files_.clear();
		folders_.clear();

		if ( isOpen() )
		{
			unz_global_info oGlobalInfo;
			int err = unzGetGlobalInfo( zipFile_, &oGlobalInfo );
			for ( unsigned long i=0; 
				i < oGlobalInfo.number_entry && err == UNZ_OK; i++ )
			{
				char filename[FILENAME_MAX];
				unz_file_info oFileInfo;

				err = unzGetCurrentFileInfo( zipFile_, &oFileInfo, filename, 
					sizeof(filename), NULL, 0, NULL, 0);
				if ( err == UNZ_OK )
				{
					char nLast = filename[oFileInfo.size_filename-1];
					if ( nLast =='/' || nLast == '\\' )
					{
						folders_.push_back(filename);
					}
					else
					{
						files_.push_back(filename);
					}

					err = unzGoToNextFile(zipFile_);
				}
			}
		}
	}

	// Dump the currently open entry to the output stream
	unzipper& unzipper::operator>>( std::ostream& os )
	{
		if ( isOpenEntry() )
		{
			unsigned int size = getEntrySize();
			char* buf = new char[size];
			size = unzReadCurrentFile( zipFile_, buf, size );
			if ( size > 0 )
			{
				os.write( buf, size );
				os.flush();
			}
			delete [] buf;
		}
		return *this;
	}

}
}