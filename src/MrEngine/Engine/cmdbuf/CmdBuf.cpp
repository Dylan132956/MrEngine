#include "CmdBuf.h"
namespace moonriver
{
	CmdBuf CmdBuf::cb;
	CmdBuf::CmdBuf()
	{

	}

	CmdBuf::~CmdBuf()
	{

	}

	CmdBuf* CmdBuf::CmdBufManager()
	{
		return &cb;
	}
}

