
#include "ComplierParam.h"
#include "utils.h"

using namespace std;

ComplierParam ComplierParam::CreateFromString(string& shaderSource)
{
	static std::string token = "#pragma";
	static std::string vsToken = "vertex";
	static std::string psToken = "fragment";
	static std::string csToken = "compute";
	ComplierParam param;
	size_t pos = FindTokenInText(shaderSource, token, 0);

	while (pos != string::npos)
	{
		string line = FindStringLine(shaderSource, pos);
		size_t linePos = token.length() + 1;
		string word = FindNextWord(line, linePos);

		if (word == vsToken)
		{
			linePos += word.length();
			string vsName = FindNextWord(line, linePos);
			param.vsName = vsName;
		}
		else if (word == psToken)
		{
			linePos += word.length();
			string psName = FindNextWord(line, linePos);
			param.psName = psName;
		}
		else if (word == csToken)
		{
			linePos += word.length();
			string csName = FindNextWord(line, linePos);
			param.csName = csName;
		}

		pos += line.length();
		pos = FindTokenInText(shaderSource, token, pos);
	}

	return param;
}

bool ComplierParam::HasProgram()
{
	return vsName != "" || psName != "" || csName != "";
}
