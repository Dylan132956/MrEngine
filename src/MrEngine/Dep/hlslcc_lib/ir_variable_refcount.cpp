

#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_visitor.h"
#include "ir_variable_refcount.h"
#include "glsl_types.h"


// constructor
ir_variable_refcount_entry::ir_variable_refcount_entry(ir_variable *var)
{
	this->var = var;
	assign = NULL;
	assigned_count = 0;
	declaration = false;
	referenced_count = 0;
}


ir_variable_refcount_entry *
ir_variable_refcount_visitor::get_variable_entry(ir_variable *var)
{
	check(var);
	if (Variables.find(var) == Variables.end())
	{
		ir_variable_refcount_entry *entry = new(mem_ctx)ir_variable_refcount_entry(var);
		check(entry->referenced_count == 0);
		Variables[var] = entry;

	}
	return Variables[var];
}


ir_visitor_status
ir_variable_refcount_visitor::visit(ir_variable *ir)
{
	ir_variable_refcount_entry *entry = this->get_variable_entry(ir);
	if (entry)
	{
		entry->declaration = true;
	}

	return visit_continue;
}


ir_visitor_status
ir_variable_refcount_visitor::visit(ir_dereference_variable *ir)
{
	ir_variable *const var = ir->variable_referenced();
	ir_variable_refcount_entry *entry = this->get_variable_entry(var);

	if (entry)
	{
		entry->referenced_count++;
		entry->last_assign = NULL;
	}

	return visit_continue;
}


ir_visitor_status
ir_variable_refcount_visitor::visit_enter(ir_function_signature *ir)
{
	/* We don't want to descend into the function parameters and
	* dead-code eliminate them, so just accept the body here.
	*/
	visit_list_elements(this, &ir->body);
	return visit_continue_with_parent;
}


ir_visitor_status
ir_variable_refcount_visitor::visit_leave(ir_assignment *ir)
{
	ir_variable_refcount_entry *entry;
	entry = this->get_variable_entry(ir->lhs->variable_referenced());
	if (entry)
	{
		entry->assigned_count++;
		if (entry->assign == NULL)
		{
			entry->assign = ir;
		}
		if (control_flow_depth == 0)
		{
			entry->last_assign = ir;
		}
		else
		{
			entry->last_assign = NULL;
		}
	}

	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_leave(ir_call *ir)
{
	if (ir->return_deref)
	{
		ir_variable_refcount_entry *entry;
		entry = this->get_variable_entry(ir->return_deref->variable_referenced());
		if (entry)
		{
			entry->assigned_count++;
			if (entry->call == NULL)
			{
				entry->call = ir;
			}
		}
	}

	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_enter(ir_if *ir)
{
	control_flow_depth++;
	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_leave(ir_if *ir)
{
	control_flow_depth--;
	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_enter(ir_loop *ir)
{
	control_flow_depth++;
	return visit_continue;
}

ir_visitor_status
ir_variable_refcount_visitor::visit_leave(ir_loop *ir)
{
	control_flow_depth--;
	return visit_continue;
}
