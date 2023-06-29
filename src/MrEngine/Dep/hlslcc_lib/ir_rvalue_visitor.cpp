

#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_visitor.h"
#include "ir_rvalue_visitor.h"
#include "ir_print_visitor.h"
#include "glsl_types.h"

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_expression *ir)
{
	unsigned int operand;

	for (operand = 0; operand < ir->get_num_operands(); operand++)
	{
		handle_rvalue(&ir->operands[operand]);
	}

	return visit_continue;
}

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_texture *ir)
{
	ir_rvalue* sampler_rvalue = ir->sampler;
	handle_rvalue(&sampler_rvalue);
	if (sampler_rvalue)
	{
		ir->sampler = sampler_rvalue->as_dereference();
	}
	else
	{
		ir->sampler = NULL;
	}

	handle_rvalue(&ir->coordinate);
	handle_rvalue(&ir->projector);
	handle_rvalue(&ir->shadow_comparitor);
	handle_rvalue(&ir->offset);
	handle_rvalue(&ir->SamplerState);

	switch (ir->op)
	{
	case ir_tex:
	case ir_txm:
	case ir_txg:
		break;
	case ir_txb:
		handle_rvalue(&ir->lod_info.bias);
		break;
	case ir_txf:
	case ir_txl:
	case ir_txs:
		handle_rvalue(&ir->lod_info.lod);
		break;
	case ir_txd:
		handle_rvalue(&ir->lod_info.grad.dPdx);
		handle_rvalue(&ir->lod_info.grad.dPdy);
		break;
	}

	return visit_continue;
}

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_swizzle *ir)
{
	handle_rvalue(&ir->val);
	return visit_continue;
}

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_dereference_array *ir)
{
	/* The array index is not the target of the assignment, so clear the
	* 'in_assignee' flag.  Restore it after returning from the array index.
	*/
	const bool was_in_assignee = this->in_assignee;
	this->in_assignee = false;
	handle_rvalue(&ir->array_index);
	this->in_assignee = was_in_assignee;

	handle_rvalue(&ir->array);
	return visit_continue;
}

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_dereference_record *ir)
{
	handle_rvalue(&ir->record);
	return visit_continue;
}

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_assignment *ir)
{
	handle_rvalue(&ir->rhs);
	handle_rvalue(&ir->condition);

	return visit_continue;
}

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_call *ir)
{
	foreach_iter(exec_list_iterator, iter, *ir)
	{
		ir_rvalue *param = (ir_rvalue *)iter.get();
		ir_rvalue *new_param = param;
		handle_rvalue(&new_param);

		if (new_param != param)
		{
			param->replace_with(new_param);
		}
	}
	return visit_continue;
}

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_atomic *ir)
{
	return ir_hierarchical_visitor::visit_leave(ir);
}

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_return *ir)
{
	handle_rvalue(&ir->value);;
	return visit_continue;
}

ir_visitor_status ir_rvalue_visitor::visit_leave(ir_if *ir)
{
	handle_rvalue(&ir->condition);
	return visit_continue;
}
