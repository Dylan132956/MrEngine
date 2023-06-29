#pragma once

#include "Macros.h"

class MacroEnumer
{
public:
	MacroEnumer(Macros& macro);
	void Begin();
	void Next();
	bool HasElement();
	void GetCurKeyWords(std::vector<std::string>& outKeys);
private:
	Macros* p_macros;
	size_t m_curIndex;
	size_t m_totalCount;
	size_t m_sysMacCount;
	size_t m_userMacCount;
};