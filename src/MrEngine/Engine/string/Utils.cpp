#include "Utils.h"
#include <limits.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdarg.h> 
#include <algorithm>
#include <io.h>
#include <direct.h>

using namespace std;

std::string VFormat(const char* format, va_list ap)
{
	va_list zp;
	va_copy(zp, ap);
	char* buffer = new char[1024 * 1024];
	vsnprintf(buffer, 1024 * 1024, format, zp);
	va_end(zp);
	std::string res = ::string(buffer);
	delete[] buffer;
	return res;
}

std::string Format(const char* format, ...)
{
	va_list va;
	va_start(va, format);
	std::string formatted = VFormat(format, va);
	va_end(va);
	return formatted;
}

size_t FindTokenInText(const std::string& text, const std::string& token, size_t startPos)
{
	int len = text.length();
	int tokenLen = token.length();
	const char* p_start = text.c_str();

	if (tokenLen > 0 && startPos + tokenLen <= len)
	{
		for (int i = startPos; i < len - tokenLen + 1; i++)
		{
			if (p_start[i] == token[0])
			{
				if (memcmp(p_start + i, token.c_str(), tokenLen) == 0)
				{
					return i;
				}
			}
		}
	}

	return std::string::npos; // not found
}

void ReplaceAllString(std::string& str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;
	do
	{
		start_pos = str.find(from, start_pos);
		if (start_pos != std::string::npos)
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.size();
		}
	} while (start_pos != std::string::npos);
}

void StringTrim(std::string& str)
{
	ReplaceAllString(str, " ", "");
	ReplaceAllString(str, "\t", "");
	ReplaceAllString(str, "\n", "");
}

std::string FindPreWord(const std::string& text, size_t startpos)
{
	int len = text.length();
	string res;

	if (startpos < len)
	{
		const unsigned char* p_start = (const unsigned char*)text.c_str();
		int curPos = startpos;
		bool strbegin = false;

		for (; curPos >= 0; curPos--)
		{
			if (!strbegin)
			{
				if (isspace(p_start[curPos]))
				{
					continue;
				}
				else
				{
					strbegin = true;
				}
			}

			if (strbegin)
			{
				unsigned char curChar = p_start[curPos];
				if (isspace(curChar))
				{
					break;
				}
				else
				{
					res.push_back(curChar);
				}
			}
		}
	}

	std::reverse(res.begin(), res.end());

	return res;
}

std::string FindNextWord(const std::string& text, size_t startpos)
{
	int len = text.length();
	string res;
	
	if (startpos < len)
	{
		const char* p_start = text.c_str();
		int curPos = startpos;
		bool strbegin = false;

		for (; curPos < len; curPos++)
		{
			if (!strbegin)
			{
				if (isspace(p_start[curPos]))
				{
					continue;
				}
				else
				{
					strbegin = true;
				}
			}

			if (strbegin)
			{
				char curChar = p_start[curPos];
				if (isspace(curChar))
				{
					break;
				}
				else
				{
					res.push_back(curChar);
				}
			}
		}
	}
	return res;
}

std::string FindStringLine(const std::string& text, size_t startpos)
{
	int len = text.length();
	string res;

	if (startpos < len)
	{
		const char* p_start = text.c_str();
		int curPos = startpos;

		for (; curPos < len; curPos++)
		{
			char curCh = p_start[curPos];
			if (curCh == '\r' 
				|| curCh == '\n')
			{
				break;
			}
			else
			{
				res.push_back(curCh);
			}
		}
	}
	return res;
}

size_t SkipToNextLineBegin(const std::string& text, size_t startpos)
{
	int len = text.length();

	if (startpos < len)
	{
		const char* p_start = text.c_str();
		int curPos = startpos;

		for (; curPos < len; curPos++)
		{
			char curCh = p_start[curPos];
			if (curCh == '\r'
				|| curCh == '\n')
			{
				break;
			}
		}

		for (; curPos < len; curPos++)
		{
			char curCh = p_start[curPos];
			if (curCh == '\r'
				|| curCh == '\n')
			{
				continue;
			}
			else
			{
				break;
			}
		}

		return curPos;
	}
	else
	{
		return string::npos;
	}
}

std::vector<std::string> vStringSplit(const  std::string& s, const std::string& delim)
{
	std::vector<std::string> elems;
	size_t pos = 0;
	size_t len = s.length();
	size_t delim_len = delim.length();
	if (delim_len == 0) return elems;
	while (pos < len)
	{
		int find_pos = s.find(delim, pos);
		if (find_pos < 0)
		{
			elems.push_back(s.substr(pos, len - pos));
			break;
		}
		elems.push_back(s.substr(pos, find_pos - pos));
		pos = find_pos + delim_len;
	}
	return elems;
}

bool FindInTokensString(const string & str, const std::string& beginToken, const std::string& endToken, size_t beginPos,
	size_t & findBegPos, size_t & findEndPos)
{
	findBegPos = std::string::npos;
	findEndPos = std::string::npos;

	findBegPos = FindTokenInText(str, beginToken, beginPos);
	if (findBegPos != std::string::npos)
	{
		findEndPos = FindTokenInText(str, endToken, findBegPos);
	}

	if (findBegPos != string::npos && findEndPos != string::npos)
	{
		return true;
	}

	return false;
}

void ReplaceString(string& source, string findsubStr, string replaceStr)
{
	size_t pos = source.find(findsubStr);
	while (pos != string::npos)
	{
		source.replace(pos, findsubStr.length(), replaceStr);
		pos += replaceStr.length();
		pos = source.find(findsubStr, pos);
	}
}

void PrintError(const std::string& error)
{
	printf("%s", error.c_str());
}

void PrintLog(const std::string& elem)
{
	printf("%s", elem.c_str());
}

void PrintLog(const std::vector<string>& curVectos)
{
	int vecCount = curVectos.size();
	for (int i = 0; i < vecCount; i++)
	{
		if (i == vecCount - 1)
		{
			printf("%s\n", curVectos[i].c_str());
		}
		else
		{
			printf("%s,", curVectos[i].c_str());
		}
	}
}

void RemoveUTF8BOM(std::string& s)
{
	if (s.size() < 3)
		return;

	if (s[0] == '\xEF' && s[1] == '\xBB' && s[2] == '\xBF')
		s.erase(0, 3);
}

bool ReadFileContent(const string& fileName, string& fileContet)
{
	FILE* file = NULL;
	fopen_s(&file, fileName.c_str(), "rt");
	if (file == NULL)
		return false;

	fseek(file, 0, SEEK_END);
	int rawLength = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (rawLength < 0)
	{
		fclose(file);
		PrintError("read file failed");
		return false;
	}

	fileContet.resize(rawLength);
	int readLength = fread(&*fileContet.begin(), 1, rawLength, file);
	fclose(file);
	fileContet.resize(readLength);

	RemoveUTF8BOM(fileContet);
	return true;
}

void string_replace(std::string &strBig, const std::string &strsrc, const std::string &strdst)
{
	std::string::size_type pos = 0;
	std::string::size_type srclen = strsrc.size();
	std::string::size_type dstlen = strdst.size();

	while ((pos = strBig.find(strsrc, pos)) != std::string::npos)
	{
		strBig.replace(pos, srclen, strdst);
		pos += dstlen;
	}
}

std::string GetPathOrURLShortName(const std::string & strFullName)
{
	if (strFullName.empty())
	{
		return "";
	}
	std::string::size_type iPos = strFullName.find_last_of(PATH_SEP);
	if (iPos == std::string::npos)
	{
		return "." PATH_SEP;
	}
	else
	{
		return strFullName.substr(0, iPos + 1);
	}
}

std::string Vector2String(std::vector<string> inputStrs)
{
	std::string outStr = "";
	for (size_t i = 0; i < inputStrs.size(); i++)
	{
		outStr += Format("\"%s\"", inputStrs[i].c_str());
		if (i != inputStrs.size() - 1)
		{
			outStr += ",";
		}
	}

	return outStr;
}

void WriteText(const std::string& fileName, const std::string& tex)
{
	FILE* f = fopen(fileName.c_str(), "wt");
	if (f != nullptr)
	{
		fwrite(tex.c_str(), tex.length(), 1, f);
		fclose(f);
	}
	else
	{
		printf("WriteText error! fileName : %s open error!", fileName.c_str());
	}
}

size_t SkipWord(const std::string& text, size_t startpos)
{
	int len = text.length();
	size_t newPos = std::string::npos;
	string res;

	if (startpos < len)
	{
		const char* p_start = text.c_str();
		int curPos = startpos;
		bool strbegin = false;

		for (; curPos < len; curPos++)
		{
			if (isspace(p_start[curPos]))
			{
				continue;
			}
			{
				break;
			}
		}

		for (; curPos < len; curPos++)
		{
			if (!isspace(p_start[curPos]))
			{
				continue;
			}
			else
			{
				newPos = curPos;
				break;
			}
		}
	}
	return newPos;
}

std::vector<std::string> split(const std::string & text, const std::string & sep)
{
	std::vector<std::string> splits;
	std::size_t offset = 0;
	auto pos = text.find(sep, offset);
	while (pos != std::string::npos)
	{
		splits.emplace_back(text, offset, pos - offset);
		offset = pos + sep.size();
		pos = text.find(sep, offset);
	}
	if (offset < text.size())
	{
		splits.emplace_back(text, offset);
	}
	return std::move(splits);
}