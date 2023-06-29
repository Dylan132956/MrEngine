
/**
* \file opt_redundant_jumps.cpp
* Remove certain types of redundant jumps
*/

#include "ShaderCompilerCommon.h"
#include "ir.h"

class redundant_jumps_visitor : public ir_hierarchical_visitor
{
public:
	redundant_jumps_visitor()
	{
		this->progress = false;
	}

	virtual ir_visitor_status visit_leave(ir_if *);
	virtual ir_visitor_status visit_leave(ir_loop *);
	virtual ir_visitor_status visit_enter(ir_assignment *);

	bool progress;
};

/* We only care about the top level instructions, so don't descend
* into expressions.
*/
ir_visitor_status redundant_jumps_visitor::visit_enter(ir_assignment *ir)
{
	return visit_continue_with_parent;
}

ir_visitor_status redundant_jumps_visitor::visit_leave(ir_if *ir)
{
	/* If the last instruction in both branches is a 'break' or a 'continue',
	* pull it out of the branches and insert it after the if-statment.  Note
	* that both must be the same type (either 'break' or 'continue').
	*/
	ir_instruction *const last_then =
		(ir_instruction *)ir->then_instructions.get_tail();
	ir_instruction *const last_else =
		(ir_instruction *)ir->else_instructions.get_tail();

	if ((last_then == NULL) || (last_else == NULL))
	{
		return visit_continue;
	}

	if ((last_then->ir_type != ir_type_loop_jump)
		|| (last_else->ir_type != ir_type_loop_jump))
	{
		return visit_continue;
	}

	ir_loop_jump *const then_jump = (ir_loop_jump *)last_then;
	ir_loop_jump *const else_jump = (ir_loop_jump *)last_else;

	if (then_jump->mode != else_jump->mode)
	{
		return visit_continue;
	}

	then_jump->remove();
	else_jump->remove();
	this->progress = true;

	ir->insert_after(then_jump);

	/* If both branchs of the if-statement are now empty, remove the
	* if-statement.
	*/
	if (ir->then_instructions.is_empty() && ir->else_instructions.is_empty())
	{
		ir->remove();
	}

	return visit_continue;
}


ir_visitor_status
redundant_jumps_visitor::visit_leave(ir_loop *ir)
{
	/* If the last instruction of a loop body is a 'continue', remove it.
	*/
	ir_instruction *const last =
		(ir_instruction *)ir->body_instructions.get_tail();

	if (last && (last->ir_type == ir_type_loop_jump)
		&& (((ir_loop_jump *)last)->mode == ir_loop_jump::jump_continue))
	{
		last->remove();
		this->progress = true;
	}

	return visit_continue;
}


bool
optimize_redundant_jumps(exec_list *instructions)
{
	redundant_jumps_visitor v;

	v.run(instructions);
	return v.progress;
}
