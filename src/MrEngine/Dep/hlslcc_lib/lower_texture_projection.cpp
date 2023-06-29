
/**
* \file lower_texture_projection.cpp
*
* IR lower pass to perform the division of texture coordinates by the texture
* projector if present.
*
* Many GPUs have a texture sampling opcode that takes the projector
* and does the divide internally, thus the presence of the projector
* in the IR.  For GPUs that don't, this saves the driver needing the
* logic for handling the divide.
*
* \author Eric Anholt <eric@anholt.net>
*/

#include "ShaderCompilerCommon.h"
#include "ir.h"

class lower_texture_projection_visitor : public ir_hierarchical_visitor
{
public:
	lower_texture_projection_visitor()
	{
		progress = false;
	}

	ir_visitor_status visit_leave(ir_texture *ir);

	bool progress;
};

ir_visitor_status
lower_texture_projection_visitor::visit_leave(ir_texture *ir)
{
	if (!ir->projector)
	{
		return visit_continue;
	}

	void *mem_ctx = ralloc_parent(ir);

	ir_variable *var = new(mem_ctx)ir_variable(ir->projector->type,
		"projector", ir_var_auto);
	base_ir->insert_before(var);
	ir_dereference *deref = new(mem_ctx)ir_dereference_variable(var);
	ir_expression *expr = new(mem_ctx)ir_expression(ir_unop_rcp,
		ir->projector->type,
		ir->projector,
		NULL);
	ir_assignment *assign = new(mem_ctx)ir_assignment(deref, expr, NULL);
	base_ir->insert_before(assign);

	deref = new(mem_ctx)ir_dereference_variable(var);
	ir->coordinate = new(mem_ctx)ir_expression(ir_binop_mul,
		ir->coordinate->type,
		ir->coordinate,
		deref);

	if (ir->shadow_comparitor)
	{
		deref = new(mem_ctx)ir_dereference_variable(var);
		ir->shadow_comparitor = new(mem_ctx)ir_expression(ir_binop_mul,
			ir->shadow_comparitor->type,
			ir->shadow_comparitor,
			deref);
	}

	ir->projector = NULL;

	progress = true;
	return visit_continue;
}

bool do_lower_texture_projection(exec_list *instructions)
{
	lower_texture_projection_visitor v;

	visit_list_elements(&v, instructions);

	return v.progress;
}
