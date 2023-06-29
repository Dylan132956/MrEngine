
#include "Macros.h"
#include "Utils.h"

#include <cassert>

using namespace std;

class SubMacros
{
public:
	SubMacros();
	~SubMacros();

	static SubMacros CreateFrom(const string& macroString);
	static void ProcessSystemMacros(const string& macroString, SubMacros& submacro);

	void SetSystemMacro(const std::vector<SystemMacro>& systemmacro)
	{
		m_systemMacro = systemmacro;
	}

	inline const std::vector<SystemMacro> GetSystemMacro() const
	{
		return m_systemMacro;
	}

	inline const std::vector<string>& GetUserMacro() const
	{
		return m_userMacros;
	}

private:
	std::vector<string> m_userMacros;
	std::vector<SystemMacro> m_systemMacro;
};

SubMacros::SubMacros()
{
}

SubMacros::~SubMacros()
{
}

void SubMacros::ProcessSystemMacros(const string& macroString, SubMacros& submacro)
{
#define sysMacCount 3
	string systemStrings[sysMacCount];
	string systemContext[sysMacCount];

	// always pass
	int index = 0;
	systemStrings[index].assign("#pragma multi_compile_skin");
	systemContext[index].assign(
		"_SKIN_ON;\
        ;"
	);

	// gpu instancing
	index++;
	systemStrings[index].assign("#pragma multi_compile_gpuinstacing");
	systemContext[index].assign(
		"GPUINSTANCING;\
        ;"
	);

	// const buffer
	index++;
	systemStrings[index].assign("#pragma multi_compile_constbuffer");
	systemContext[index].assign(
		"CONST_BUFFER;\
        ;"
	);

	for (int i = 0; i <= index; i++)
	{
		string curToken = systemStrings[i];
		string curExtract = systemContext[i];
		size_t pos = FindTokenInText(macroString, curToken, 0);

		if (pos != string::npos)
		{
			std::vector<std::vector<string>> result;
			std::vector<string> splitStr = vStringSplit(curExtract, ";");
			int count = splitStr.size();

			for (int j = 0; j < count; j++)
			{
				string curStr = splitStr[j];

				StringTrim(curStr);
				result.push_back(vStringSplit(curStr, ","));
			}

			submacro.SetSystemMacro(result);

			break;
		}
	}
}

SubMacros SubMacros::CreateFrom(const string& macroString)
{
	static std::string token = "#pragma multi_compile ";
    size_t pos = FindTokenInText(macroString, token, 0);
	SubMacros subMacros;

	if (pos != string::npos)
	{
		pos += token.length();
		string word = FindNextWord(macroString, pos);

		while (word != "")
		{
			subMacros.m_userMacros.push_back(word);
			pos += word.length() + 1;
			word = FindNextWord(macroString, pos);
		}
	}
	else
	{
		ProcessSystemMacros(macroString, subMacros);
	}

	return subMacros;
}

void Macros::AddSubMacro(SubMacros& subMacro)
{
	const std::vector<string>& userMacros = subMacro.GetUserMacro();
	if (userMacros.size() > 0)
	{
		this->m_userMacros.push_back(subMacro.GetUserMacro());
	}
	
    std::vector<SystemMacro> systemMacro = subMacro.GetSystemMacro();
	if (systemMacro.size() > 0)
	{
		this->m_systemMacros.push_back(systemMacro);
	}
}

Macros Macros::CreateFormShaderSource(string& shaderSource)
{
	static std::string token = "#pragma multi_compile";
	size_t pos = FindTokenInText(shaderSource, token, 0);
	Macros macros;

	while (pos != string::npos)
	{
		string lineStr = FindStringLine(shaderSource, pos);
		SubMacros subMac = SubMacros::CreateFrom(lineStr);
		macros.AddSubMacro(subMac);
		pos += lineStr.length();
		pos = FindTokenInText(shaderSource, token, pos);
	}

	return macros;
}