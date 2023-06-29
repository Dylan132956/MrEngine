
#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_visitor.h"
#include "ir_rvalue_visitor.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"

class ir_expression_flattening_visitor : public ir_rvalue_visitor
{
public:
	ir_expression_flattening_visitor(bool(*predicate)(ir_instruction *ir))
	{
		this->predicate = predicate;
	}

	virtual ~ir_expression_flattening_visitor()
	{
		/* empty */
	}

	void handle_rvalue(ir_rvalue **rvalue);
	bool(*predicate)(ir_instruction *ir);
};

void do_expression_flattening(exec_list *instructions, bool(*predicate)(ir_instruction *ir))
{
	ir_expression_flattening_visitor v(predicate);

	foreach_iter(exec_list_iterator, iter, *instructions)
	{
		ir_instruction *ir = (ir_instruction *)iter.get();

		ir->accept(&v);
	}
}

void ir_expression_flattening_visitor::handle_rvalue(ir_rvalue **rvalue)
{
	ir_variable *var;
	ir_assignment *assign;
	ir_rvalue *ir = *rvalue;

	if (!ir || !this->predicate(ir))
	{
		return;
	}

	void *ctx = ralloc_parent(ir);

	var = new(ctx)ir_variable(ir->type, "flattening_tmp", ir_var_temporary);
	base_ir->insert_before(var);

	assign = new(ctx)ir_assignment(new(ctx)ir_dereference_variable(var),
		ir,
		NULL);
	base_ir->insert_before(assign);

	*rvalue = new(ctx)ir_dereference_variable(var);
}
