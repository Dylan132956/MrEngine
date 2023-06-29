#pragma once

#include <string>
#include <vector>

typedef std::vector<std::string> SystemMacro;
class SubMacros;

class Macros
{
public:
	static Macros CreateFormShaderSource(std::string& shaderSource);
	void AddSubMacro(SubMacros& subMacro);
public:

	std::vector<std::vector<std::string>> m_userMacros;
	std::vector<std::vector<SystemMacro>> m_systemMacros;
};

