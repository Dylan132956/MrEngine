
#include <limits.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdarg.h> 
#include <algorithm>
#include "lz4.h"
#include "lz4hc.h"
#include "zlib.h"
#include <io.h>
#include <direct.h>
#include "shaderpack.h"
#include "utils.h"

using namespace std;

std::vector<unsigned char> CompressMemoryLZ4(const std::string& shaderSource)
{
	size_t sourceSize = shaderSource.size();
	int dstSize = LZ4_compressBound(static_cast<int>(sourceSize));
	bool result = false;
	if (dstSize > 0)
	{
		const char* pSrcData = shaderSource.c_str();
		char* pDstData = new char[dstSize];

		int result = LZ4_compress_HC(pSrcData, pDstData, sourceSize, dstSize, 16);
		if (result > 0)
		{
			std::vector<unsigned char> byteVec((unsigned char*)pDstData, (unsigned char*)pDstData + result);
			delete[] pDstData;
			return byteVec;
		}

		delete[] pDstData;
	}

	return std::vector<unsigned char>();
}

std::vector<unsigned char> CompressMemoryZlib(const std::string& shaderSource)
{
	if (shaderSource.size() <= 0)
	{
		return std::vector<unsigned char>();
	}

	uLong len = (uLong)shaderSource.size();
	uLong comprLen = 2 * len;
	char* pDstData = new char[comprLen];
	uLong dstSize = comprLen;

	compress((Bytef*)pDstData, &dstSize, (const Bytef*)shaderSource.c_str(), len);

	std::vector<unsigned char> outBytes;
	if (dstSize > 0)
	{
		std::vector<unsigned char> byteVec((unsigned char*)pDstData, (unsigned char*)pDstData + dstSize);
		outBytes = std::move(byteVec);
	}

	delete[] pDstData;
	return outBytes;
}

void ShaderPack(const std::string& fileName, ComplierState& gloablState)
{
	const std::vector<std::string>& compressBuffs = gloablState.compressShaders;
	std::string folderName = Format("%s_zip", fileName.c_str());

	// 文件夹不存在则创建文件夹
	if (_access(folderName.c_str(), 0) == -1)
	{
		_mkdir(folderName.c_str());
	}

	int len = compressBuffs.size();

	for (int i = 0; i < len; i++)
	{
		const char* pData = compressBuffs[i].c_str();
		int size = compressBuffs[i].size();

		std::string curName = Format("%s\\%d", folderName.c_str(), i);
		FILE* f = fopen(curName.c_str(), "wb");
		fwrite(pData, size, 1, f);
		fclose(f);
	}

	std::string lz4Path = gloablState.curPath;
	ReplaceString(lz4Path, "CompilerShader", "Lz4ZipCompression");
	string command = Format("%s -p %s -s %s -D", lz4Path.c_str(), fileName.c_str(), folderName.c_str());
	system(command.c_str());


	for (int i = 0; i < len; i++)
	{
		std::string curName = Format("%s\\%d", folderName.c_str(), i);
		remove(curName.c_str());
	}

	_rmdir(folderName.c_str());
}
