///*
//* Viry3D
//* Copyright 2014-2019 by Stack - stackos@qq.com
//*
//* Licensed under the Apache License, Version 2.0 (the "License");
//* you may not use this file except in compliance with the License.
//* You may obtain a copy of the License at
//*
//*     http://www.apache.org/licenses/LICENSE-2.0
//*
//* Unless required by applicable law or agreed to in writing, software
//* distributed under the License is distributed on an "AS IS" BASIS,
//* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//* See the License for the specific language governing permissions and
//* limitations under the License.
//*/
//
//#include "Directory.h"
//
//#if VR_WINDOWS || VR_UWP
//#include <io.h>
//#include <Windows.h>
//#else
//#include <dirent.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#endif
//
//using namespace std;
//
//namespace Viry3D
//{
//#if VR_WINDOWS || VR_UWP
//	static vector<string> GetDirFiles(const std::string& path, bool recursive, bool* exist, bool get_dirs_only = false)
//	{
//		std::vector<std::string> files;
//		std::vector<std::string> dirs;
//
//		string find_file = path + "/*.*";
//		_finddata_t find_data;
//		intptr_t find_handle = _findfirst(find_file.c_str(), &find_data);
//		intptr_t find = find_handle;
//
//		if (exist != nullptr)
//		{
//			if (find_handle != -1 && (find_data.attrib & _A_SUBDIR) != 0)
//			{
//				*exist = true;
//			}
//			else
//			{
//				*exist = false;
//			}
//
//			if (find_handle != -1)
//			{
//				_findclose(find_handle);
//			}
//			return files;
//		}
//
//		while (find != -1)
//		{
//			std::string name(find_data.name);
//			if (find_data.attrib & _A_SUBDIR)
//			{
//				if (name != "." &&
//					name != "..")
//				{
//					dirs.push_back(name);
//				}
//			}
//			else if (find_data.attrib & _A_ARCH)
//			{
//				files.push_back(name);
//			}
//
//			find = _findnext(find_handle, &find_data);
//		}
//
//		if (find_handle != -1)
//		{
//			_findclose(find_handle);
//		}
//
//		if (get_dirs_only)
//		{
//			return dirs;
//		}
//
//		if (recursive)
//		{
//			for (auto& i : dirs)
//			{
//				auto sub_dir = path + "/" + i;
//				auto sub_files = GetDirFiles(sub_dir, true, nullptr, false);
//
//				for (auto& j : sub_files)
//				{
//					files.push_back(i + "/" + j);
//				}
//			}
//		}
//
//		return files;
//	}
//#else
//	static Vector<String> GetDirFiles(const String& path, bool recursive, bool* exist, bool get_dirs_only = false)
//	{
//		Vector<String> files;
//		Vector<String> dirs;
//
//		DIR *dir = opendir(path.CString());
//
//		if (exist != nullptr)
//		{
//			if (dir != nullptr && (readdir(dir)->d_type & DT_DIR) != 0)
//			{
//				*exist = true;
//			}
//			else
//			{
//				*exist = false;
//			}
//
//			if (dir != nullptr)
//			{
//				closedir(dir);
//			}
//			return files;
//		}
//
//		if (dir != nullptr)
//		{
//			dirent* p;
//			while ((p = readdir(dir)) != nullptr)
//			{
//				String name = p->d_name;
//				if (p->d_type & DT_DIR)
//				{
//					if (name != "." &&
//						name != "..")
//					{
//						dirs.Add(name);
//					}
//				}
//				else if (p->d_type & DT_REG)
//				{
//					files.Add(name);
//				}
//			}
//
//			closedir(dir);
//		}
//
//		if (get_dirs_only)
//		{
//			return dirs;
//		}
//
//		if (recursive)
//		{
//			for (auto& i : dirs)
//			{
//				auto sub_dir = path + "/" + i;
//				auto sub_files = GetDirFiles(sub_dir, true, nullptr);
//
//				for (auto& j : sub_files)
//				{
//					files.Add(i + "/" + j);
//				}
//			}
//		}
//
//		return files;
//	}
//#endif
//
//	bool Directory::Exist(const string& path)
//	{
//		bool exist;
//        GetDirFiles(path, false, &exist);
//		return exist;
//	}
//
//	vector<string> Directory::GetDirectorys(const string& path)
//	{
//		auto dirs = GetDirFiles(path, false, nullptr, true);
//
//		for (auto& i : dirs)
//		{
//			i = path + "/" + i;
//		}
//
//		return dirs;
//	}
//
//    vector<string> string::Split(const string& separator, bool exclude_empty) const
//    {
//        vector<string> result;
//
//        int start = 0;
//        while (true)
//        {
//            //int index = this->IndexOf(separator, start);
//            int index = separator.find()
//            if (index >= 0)
//            {
//                String str = this->Substring(start, index - start);
//                if (!str.Empty() || !exclude_empty)
//                {
//                    result.Add(str);
//                }
//                start = index + separator.Size();
//            }
//            else
//            {
//                break;
//            }
//        }
//
//        String str = this->Substring(start, -1);
//        if (!str.Empty() || !exclude_empty)
//        {
//            result.Add(str);
//        }
//
//        return result;
//    }
//
//    bool String::StartsWith(const String& str) const
//    {
//        if (str.Size() == 0)
//        {
//            return true;
//        }
//        else if (this->Size() < str.Size())
//        {
//            return false;
//        }
//        else
//        {
//            return Memory::Compare(&(*this)[0], &str[0], str.Size()) == 0;
//        }
//    }
//
//    vector<string> Directory::GetFiles(const string& path, bool recursive)
//	{
//		auto files = GetDirFiles(path, recursive, nullptr);
//
//		for (auto& i : files)
//		{
//			i = path + "/" + i;
//		}
//
//		return files;
//	}
//
//	void Directory::Create(const string& path)
//	{
//		auto splits = path.Split("/", true);
//		String folder = splits[0];
//
//		if (path.StartsWith("/"))
//		{
//			folder = "/" + folder;
//		}
//
//		for (int i = 1; i < splits.Size(); ++i)
//		{
//			folder += "/" + splits[i];
//
//#if VR_WINDOWS || VR_UWP
//			CreateDirectoryA(folder.CString(), nullptr);
//#else
//			mkdir(folder.CString(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
//#endif
//		}
//	}
//}
