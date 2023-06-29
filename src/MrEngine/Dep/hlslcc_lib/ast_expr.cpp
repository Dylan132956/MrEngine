
#include "ShaderCompilerCommon.h"
#include "ast.h"

const char * ast_expression::operator_string(enum ast_operators op)
{
	static const char *const operators[] =
	{
		"=",
		"+",
		"-",
		"+",
		"-",
		"*",
		"/",
		"%",
		"<<",
		">>",
		"<",
		">",
		"<=",
		">=",
		"==",
		"!=",
		"&",
		"^",
		"|",
		"~",
		"&&",
		"^^",
		"||",
		"!",

		"*=",
		"/=",
		"%=",
		"+=",
		"-=",
		"<<=",
		">>=",
		"&=",
		"^=",
		"|=",

		"?:",

		"++",
		"--",
		"++",
		"--",
		".",
	};

	check((unsigned int)op < sizeof(operators) / sizeof(operators[0]));

	return operators[op];
}


ast_expression_bin::ast_expression_bin(int oper, ast_expression *ex0,
	ast_expression *ex1) :
	ast_expression(oper, ex0, ex1, NULL)
{
	check((oper >= ast_plus) && (oper <= ast_logic_not));
}


void ast_expression_bin::print(void) const
{
	subexpressions[0]->print();
	printf("%s ", operator_string(oper));
	subexpressions[1]->print();
}
