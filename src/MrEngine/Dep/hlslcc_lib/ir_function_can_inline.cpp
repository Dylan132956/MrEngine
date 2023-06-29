

#include "ShaderCompilerCommon.h"
#include "ir.h"

class ir_function_can_inline_visitor : public ir_hierarchical_visitor
{
public:
	ir_function_can_inline_visitor()
	{
		this->num_returns = 0;
	}

	virtual ir_visitor_status visit_enter(ir_return *);

	int num_returns;
};

ir_visitor_status ir_function_can_inline_visitor::visit_enter(ir_return *ir)
{
	(void)ir;
	this->num_returns++;
	return visit_continue;
}

bool can_inline(ir_call *call)
{
	ir_function_can_inline_visitor v;
	const ir_function_signature *callee = call->callee;
	if (!callee->is_defined)
	{
		return false;
	}

	v.run((exec_list *)&callee->body);

	/* If the function is empty (no last instruction) or does not end with a
	* return statement, we need to count the implicit return.
	*/
	ir_instruction *last = (ir_instruction *)callee->body.get_tail();
	if (last == NULL || !last->as_return())
	{
		v.num_returns++;
	}

	return v.num_returns == 1;
}
