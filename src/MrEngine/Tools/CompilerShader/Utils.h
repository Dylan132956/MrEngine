#pragma once

#include <limits.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdarg.h>

#if defined(_WIN32) || defined(_WIN64)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

template<typename Ty, size_t Num>
char(&CountOfRequireArrayArgumentT(const Ty(&)[Num]))[Num];

#define BX_COUNTOF(_x) sizeof(CountOfRequireArrayArgumentT(_x) )

#define BX_ALIGN_MASK(_value, _mask) ( ( (_value)+(_mask) ) & ( (~0)&(~(_mask) ) ) )

std::string VFormat(const char* format, va_list ap);
std::string Format(const char* format, ...);
size_t FindTokenInText(const std::string& text, const std::string& token, size_t startPos);
std::string FindPreWord(const std::string& text, size_t startpos);
std::string FindNextWord(const std::string& text, size_t startpos);
std::string FindStringLine(const std::string& text, size_t startpos);
std::vector<std::string> vStringSplit(const  std::string& s, const std::string& delim = ",");
bool FindInTokensString(const std::string & str, const std::string& beginToken, const std::string& endToken,
	size_t beginPos, size_t & findBegPos, size_t & findEndPos);
void ReplaceString(std::string& source, std::string findsubStr, std::string replaceStr);
void StringTrim(std::string& str);
size_t SkipToNextLineBegin(const std::string& text, size_t startpos);
void PrintError(const std::string& error);
void PrintLog(const std::string& elem);
void PrintLog(const std::vector<std::string>& curVectos);
bool ReadFileContent(const std::string& fileName, std::string& fileContet);
std::string GetPathOrURLShortName(const std::string & strFullName);
std::string Vector2String(std::vector<std::string> inputStrs);
void RemoveUTF8BOM(std::string& s);
void WriteText(const std::string& fileName, const std::string& tex);
void string_replace(std::string &strBig, const std::string &strsrc, const std::string &strdst);
size_t SkipWord(const std::string& text, size_t startpos);
std::vector<std::string> split(const std::string & text, const std::string & sep);


