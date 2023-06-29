#pragma once
#include <limits.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdarg.h>
#include <cassert>
#include <Windows.h>

class ByteArrayEncoder
{
public:
	inline static std::string EncodeShader(const std::vector<byte>& _code)
	{
		std::vector<char> buffer(_code.size() << 1);

		for (int i = 0; i < _code.size(); i++)
		{
			int high = _code[i] / 16, low = _code[i] % 16;
			int hexIndex = i << 1;
			buffer[hexIndex] = (high < 10) ? ('0' + high) : ('A' + high - 10);
			buffer[hexIndex + 1] = (low < 10) ? ('0' + low) : ('A' + low - 10);
		}
		return std::string(buffer.data(), buffer.data() + buffer.size());
	}

	inline static std::vector<uint8_t> DecodeShader(const std::string&input)
	{
		assert((input.size() & 1) == 0);
		std::vector<uint8_t> buffer(input.size() >> 1);

		for (int i = 0; i < input.size(); i += 2)
		{
			int highPart = decodeAlpha(input[i]);
			int lowPart = decodeAlpha(input[i + 1]);
			buffer[i >> 1] = (highPart << 4) + lowPart;
		}
		return buffer;
	}

private:
	inline static uint8_t decodeAlpha(char input)
	{
		if (input <= '9' && input >= '0')
		{
			return input - '0';
		}
		else if (input <= 'Z' && input >= 'A')
		{
			return input - 'A' + 10;
		}
		assert(false);
		return '?';
	}
};
