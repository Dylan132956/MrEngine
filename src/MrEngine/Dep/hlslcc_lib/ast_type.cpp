
#include "ShaderCompilerCommon.h"
#include "ast.h"
#include "symbol_table.h"

void ast_type_specifier::print(void) const
{
	if (structure)
	{
		structure->print();
	}
	else
	{
		switch (precision)
		{
		case ast_precision_low: printf("lowp "); break;
		case ast_precision_medium: printf("mediumdp "); break;
		case ast_precision_high: printf("highp "); break;
		}

		printf("%s ", type_name);
	}

	if (is_array)
	{
		printf("[ ");

		if (array_size)
		{
			array_size->print();
		}

		printf("] ");
	}
}

bool ast_fully_specified_type::has_qualifiers() const
{
	return this->qualifier.flags.i != 0;
}

bool ast_type_qualifier::has_interpolation() const
{
	return this->flags.q.smooth
		|| this->flags.q.flat
		|| this->flags.q.noperspective;
}

const char* ast_type_qualifier::interpolation_string() const
{
	if (this->flags.q.smooth)
	{
		return "smooth";
	}
	else if (this->flags.q.flat)
	{
		return "flat";
	}
	else if (this->flags.q.noperspective)
	{
		return "noperspective";
	}
	else
	{
		return nullptr;
	}
}
