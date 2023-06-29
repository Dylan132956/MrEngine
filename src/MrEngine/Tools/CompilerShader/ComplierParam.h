
#pragma once

#include <limits.h>
#include <stdio.h>
#include <string>
#include <vector>

class ComplierParam
{
public:
	static ComplierParam CreateFromString(std::string& shaderSource);
	bool HasProgram();

public:
	std::string vsName;
	std::string psName;
	std::string csName;
	std::string m_shaderSource;
	std::vector<std::string> m_includePaths;
};



