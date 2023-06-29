
#include "MacroEnumer.h"

using namespace std;

MacroEnumer::MacroEnumer(Macros& macro)
{
	p_macros = &macro;
}

void MacroEnumer::Begin()
{
	std::vector<std::vector<string>>& userMacros = p_macros->m_userMacros;
	std::vector<std::vector<SystemMacro>>& systemMacros = p_macros->m_systemMacros;

	int userMacroCount = 1;
	int systemMacroCount = 1;

	for (int i = 0; i < userMacros.size(); i++)
	{
		userMacroCount *= userMacros[i].size();
	}

	for (int i = 0; i < systemMacros.size(); i++)
	{
		systemMacroCount *= systemMacros[i].size();
	}

	m_userMacCount = userMacroCount;
	m_sysMacCount = systemMacroCount;

	m_totalCount = userMacroCount * m_sysMacCount;
	m_curIndex = 0;
}

bool MacroEnumer::HasElement()
{
	return m_curIndex < m_totalCount;
}

void MacroEnumer::Next()
{
	m_curIndex++;
}

void MacroEnumer::GetCurKeyWords(std::vector<std::string>& outKeys)
{
	std::vector<std::vector<SystemMacro>>& systemMacros = p_macros->m_systemMacros;
	std::vector<std::vector<string>>& userMacros = p_macros->m_userMacros;
	size_t sysMacIndex;
	size_t userMacIndex;

	if (m_sysMacCount > 0)
	{
		sysMacIndex = m_curIndex % m_sysMacCount;
		userMacIndex = m_curIndex / m_sysMacCount;

		for (int i = 0; i < systemMacros.size(); i++)
		{
			int curCount = systemMacros[i].size();
			int curMacIndex = sysMacIndex % curCount;

			SystemMacro& systemMac = systemMacros[i][curMacIndex];
			for (int j = 0; j < systemMac.size(); j++)
			{
				outKeys.push_back(systemMac[j]);
			}
			
			sysMacIndex /= curCount;
		}
	}
	else
	{
		userMacIndex = m_curIndex;
	}
	
	for (int i = 0; i < userMacros.size(); i++)
	{
		int curCount = userMacros[i].size();
		int curMacIndex = userMacIndex % curCount;
		outKeys.push_back(userMacros[i][curMacIndex]);
		userMacIndex /= curCount;
	}
}