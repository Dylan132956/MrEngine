
/**
* \file opt_noop_swizzle.cpp
*
* If a swizzle doesn't change the order or count of components, then
* remove the swizzle so that other optimization passes see the value
* behind it.
*/

#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_visitor.h"
#include "ir_rvalue_visitor.h"
#include "ir_print_visitor.h"
#include "glsl_types.h"

class ir_noop_swizzle_visitor : public ir_rvalue_visitor
{
public:
	ir_noop_swizzle_visitor()
	{
		this->progress = false;
	}

	void handle_rvalue(ir_rvalue **rvalue);
	bool progress;
};

void ir_noop_swizzle_visitor::handle_rvalue(ir_rvalue **rvalue)
{
	if (!*rvalue)
	{
		return;
	}

	ir_swizzle *swiz = (*rvalue)->as_swizzle();
	if (!swiz || swiz->type != swiz->val->type)
	{
		return;
	}

	int elems = swiz->val->type->vector_elements;
	if (swiz->mask.x != 0)
	{
		return;
	}
	if (elems >= 2 && swiz->mask.y != 1)
	{
		return;
	}
	if (elems >= 3 && swiz->mask.z != 2)
	{
		return;
	}
	if (elems >= 4 && swiz->mask.w != 3)
	{
		return;
	}

	this->progress = true;
	*rvalue = swiz->val;
}

bool do_noop_swizzle(exec_list *instructions)
{
	ir_noop_swizzle_visitor v;
	visit_list_elements(&v, instructions);

	return v.progress;
}
