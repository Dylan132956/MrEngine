/*
* Viry3D
* Copyright 2014-2019 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "File.h"
#include "Directory.h"
#include "Debug.h"
#include "zlib/unzip.h"
#include <fstream>

#if VR_WINDOWS
#include <Windows.h>
#endif

using namespace std;

namespace moonriver
{
#if VR_UWP
    extern bool FileExist(const std::string& path);
    extern ByteBuffer FileReadAllBytes(const std::string& path);
    extern bool FileWriteAllBytes(const std::string& path, const ByteBuffer& buffer);

    bool File::Exist(const std::string& path)
    {
        return FileExist(path);
    }

    ByteBuffer File::ReadAllBytes(const std::string& path)
    {
        return FileReadAllBytes(path);
    }

    bool File::WriteAllBytes(const std::string& path, const ByteBuffer& buffer)
    {
        return FileWriteAllBytes(path, buffer);
    }
#else
    bool File::Exist(const std::string& path)
    {
        std::ifstream is(path.c_str(), std::ios::binary);

        bool exist = !(!is);

        if (exist)
        {
            is.close();
        }

        return exist;
    }

    ByteBuffer File::ReadAllBytes(const std::string& path)
    {
        ByteBuffer buffer;

        std::ifstream is(path.c_str(), std::ios::binary);
        if (is)
        {
            is.seekg(0, std::ios::end);
            int size = (int) is.tellg();
            is.seekg(0, std::ios::beg);

            buffer = ByteBuffer(size);

            is.read((char*) buffer.Bytes(), size);
            is.close();
        }

        return buffer;
    }

    bool File::WriteAllBytes(const std::string& path, const ByteBuffer& buffer)
    {
        std::ofstream os(path.c_str(), std::ios::binary);
        if (os)
        {
            os.write((const char*) buffer.Bytes(), buffer.Size());
            os.close();

            return true;
        }
        else
        {
            return false;
        }
    }
#endif

    std::string File::ReadAllText(const std::string& path)
	{
		return std::string((const char*)File::ReadAllBytes(path).Bytes());
	}

    bool File::WriteAllText(const std::string& path, const std::string& text)
	{
		ByteBuffer buffer((byte*) text.c_str(), text.size());
        return File::WriteAllBytes(path, buffer);
	}

    void File::Delete(const std::string& path)
    {
#if VR_WINDOWS
        ::DeleteFile(path.c_str());
#endif
    }

	static void UnzipFile(unzFile file, const std::string& path)
	{
		//auto dir = path.substr(0, path.find_last_of("/"));
		//Directory::Create(dir);

		//int result = unzOpenCurrentFilePassword(file, nullptr);

		//ByteBuffer buffer(8192);

		//std::ofstream os(path.c_str(), std::ios::out | std::ios::binary);

		//if (os)
		//{
		//	do
		//	{
		//		result = unzReadCurrentFile(file, buffer.Bytes(), buffer.Size());
		//		if (result > 0)
		//		{
		//			os.write((const char*) buffer.Bytes(), result);
		//		}
		//	} while (result > 0);

		//	os.close();
		//}

		//unzCloseCurrentFile(file);
	}

	void File::Unzip(const string& path, const string& source, const string& dest, bool directory)
	{
		//auto file = unzOpen64(path.c_str());
		//if (file)
		//{
		//	unz_file_info64 file_info;
		//	char filename_inzip[256];

		//	auto result = unzGoToFirstFile(file);
		//	while (result == UNZ_OK)
		//	{
		//		result = unzGetCurrentFileInfo64(file, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);
		//		if (result != UNZ_OK)
		//		{
		//			break;
		//		}

		//		if (directory)
		//		{
		//			string filename(filename_inzip);
		//			if (filename.StartsWith(source))
		//			{
  //                      string dest_filename = dest + filename.Substring(source.Size());
  //                      UnzipFile(file, dest_filename);
		//			}
		//		}
		//		else
		//		{
		//			if (source == filename_inzip)
		//			{
  //                      UnzipFile(file, dest);
		//				break;
		//			}
		//		}

		//		result = unzGoToNextFile(file);
		//	}

		//	unzClose(file);
		//}
		//else
		//{
		//	Log("zip file open failed:%s", path.CString());
		//}
	}
}
