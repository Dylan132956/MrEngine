#pragma once

#include <vector>
#include <string>

namespace moonriver {
size_t StrLenA(const char* str);
std::vector<std::string> SplitString(const std::string &s, char delim);
std::string& Trim(std::string &s);
int ReplaceString(std::string &str, const std::string &src, const std::string &dest);
int RemoveString(std::string &str, const std::string& src);
std::string Replace(const std::string& input, const std::string& old, const std::string& to);
bool IsVersionGreaterThan(const std::string& strV0, const std::string& strV1);
bool UTF8ToUTF32(const std::string& inUtf8, std::u32string& outUtf32);
bool UTF32ToUTF8(const std::u32string& utf32, std::string& outUtf8);
bool IsValidUTF8(const char* fmtMsg);
std::string& ToUpper(std::string& str);
std::string& ToLower(std::string& str);

bool StartsWiths(const std::string& strOri, const std::string& strCmp);
bool EndsWith(const std::string& strOri, const std::string& strCmp);

double StringToDouble(const char* text, double defaultValue = 0.0, bool* successed = nullptr);
std::string str_format(const char* format, ...);
std::string Gb2312ToUtf8(const std::string& str);

}
