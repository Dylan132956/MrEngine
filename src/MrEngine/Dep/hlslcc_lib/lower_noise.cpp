
/**
* \file lower_noise.cpp
* IR lower pass to remove noise opcodes.
*
* \author Ian Romanick <ian.d.romanick@intel.com>
*/

#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_rvalue_visitor.h"

class lower_noise_visitor : public ir_rvalue_visitor
{
public:
	lower_noise_visitor() : progress(false)
	{
		/* empty */
	}

	void handle_rvalue(ir_rvalue **rvalue)
	{
		if (!*rvalue)
		{
			return;
		}

		ir_expression *expr = (*rvalue)->as_expression();
		if (!expr)
		{
			return;
		}

		/* In the future, ir_unop_noise may be replaced by a call to a function
		* that implements noise.  No hardware has a noise instruction.
		*/
		if (expr->operation == ir_unop_noise)
		{
			*rvalue = ir_constant::zero(ralloc_parent(expr), expr->type);
			this->progress = true;
		}
	}

	bool progress;
};


bool lower_noise(exec_list *instructions)
{
	lower_noise_visitor v;

	visit_list_elements(&v, instructions);

	return v.progress;
}
