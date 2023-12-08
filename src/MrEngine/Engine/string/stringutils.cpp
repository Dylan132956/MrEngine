#include "stringutils.h"
#include <stdlib.h>
#include <sstream>
#include <locale>
#include <stdarg.h>
#include <cstring>
#include <algorithm>
#include <memory/Memory.h>

namespace moonriver {

size_t StrLenA(const char* str)
{
    return strlen(str);
}

std::vector<std::string> &SplitString(const std::string &s, char delim, std::vector<std::string> &elems) 
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> SplitString(const std::string &s, char delim) 
{
    std::vector<std::string> elems;
    SplitString(s, delim, elems);
    return elems;
}

std::string& Trim(std::string &s)
{
    if (s.empty())
    {
        return s;
    }

    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

std::string& ToUpper(std::string& str)
{
    if (str.empty())
    {
        return str;
    }
    std::transform(str.begin(), str.end(), str.begin(), (int(*)(int))::toupper);
    return str;
}

std::string& ToLower(std::string& str)
{
    if (str.empty())
    {
        return str;
    }
    std::transform(str.begin(), str.end(), str.begin(), (int(*)(int))::tolower);
    return str;
}

int ReplaceString(std::string &str, const std::string &src, const std::string &dest)
{
    int counter = 0;
    std::string::size_type pos = 0;
    while ((pos = str.find(src, pos)) != std::string::npos)
    {
        str.replace(pos, src.size(), dest);
        ++counter;
        pos += dest.size();
    }
    return counter;
}

int RemoveString(std::string &str, const std::string& src)
{
    int counter = 0;
    std::string::size_type pos = 0;
    while ((pos = str.find(src, pos)) != std::string::npos)
    {
        str.erase(pos, src.size());
        ++counter;
    }
    return counter;
}

bool IsVersionGreaterThan(const std::string& strV0, const std::string& strV1)
{
    std::vector<std::string> v0List = SplitString(strV0, '.');
    std::vector<std::string> v1List = SplitString(strV1, '.');
    for (int i = 0; i < 2; ++i)
    {
        int v0 = atoi(v0List[ i ].c_str());
        int v1 = atoi(v1List[ i ].c_str());
        if (v0 > v1)
        {
            return true;
        }
        else if (v0 < v1)
        {
            return false;
        }
    }

    return false;
}

bool IsValidUTF8(const char* fmtMsg)
{
    size_t size = strlen(fmtMsg);
    int   encodingBytesCount = 0;
    bool   allTextsAreASCIIChars = true;
    for (int i = 0; i < size; i++)
    {
        unsigned  char  current = fmtMsg[i];

        if ((current & 0x80) == 0x80)
        {
            allTextsAreASCIIChars = false;
        }
        // First byte
        if (encodingBytesCount == 0)
        {
            if ((current & 0x80) == 0)
            {
                // ASCII chars, from 0x00-0x7F
                continue;
            }

            if ((current & 0xC0) == 0xC0)
            {
                encodingBytesCount = 1;
                current <<= 2;

                // More than two bytes used to encoding a unicode char.
                // Calculate the real length.
                while ((current & 0x80) == 0x80)
                {
                    current <<= 1;
                    encodingBytesCount++;
                }
            }
            else
            {
                // Invalid bits structure for UTF8 encoding rule.
                return   false;
            }
        }
        else
        {
            // Following bytes, must start with 10.
            if ((current & 0xC0) == 0x80)
            {
                encodingBytesCount--;
            }
            else
            {
                // Invalid bits structure for UTF8 encoding rule.
                return   false;
            }
        }
    }

    if (encodingBytesCount != 0)
    {
        // Invalid bits structure for UTF8 encoding rule.
        // Wrong following bytes count.
        return   false;
    }

    //UTF8 supports encoding for ASCII chars, we regard as a input stream, whose contents are all ASCII as default encoding.
    return   true;
}

bool StartsWiths(const std::string& strOri, const std::string& strCmp) {
    if (strOri.size() < strCmp.size()) { return false; }
    return std::equal(strOri.begin(), strOri.begin() + strCmp.size(), strCmp.begin());
}

bool EndsWith(const std::string& strOri, const std::string& strCmp) {
    if (strOri.size() < strCmp.size()) { return false; }
    return std::equal(strOri.begin() + (strOri.size() - strCmp.size()), strOri.end(), strCmp.begin());
}

double StringToDouble(const char* text, double defaultValue, bool* successed) {
    if (text == nullptr) { if (successed) { *successed = false; } return defaultValue; }
    char* end; auto result = strtod(text, &end);
    if (end == text || *end != '\0') { if (successed) { *successed = false; } return defaultValue; }
    return result;
}

}
