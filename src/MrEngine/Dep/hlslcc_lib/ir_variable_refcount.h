

#include "ir.h"
#include "ir_visitor.h"
#include "glsl_types.h"

#include <map>

class ir_variable_refcount_entry : public exec_node
{
public:
	ir_variable_refcount_entry(ir_variable *var);

	ir_variable *var; /* The key: the variable's pointer. */
	ir_assignment *assign; /* An assignment to the variable, if any */
	ir_assignment *last_assign; /* The last assignment to the variable. */
	ir_call *call; /* The function call that assigns to this variable, if any */

	/** Number of times the variable is referenced, including assignments. */
	unsigned referenced_count;

	/** Number of times the variable is assigned. */
	unsigned assigned_count;

	bool declaration; /* If the variable had a decl in the instruction stream */
};

class ir_variable_refcount_visitor : public ir_hierarchical_visitor
{
public:
	ir_variable_refcount_visitor(void)
	{
		this->mem_ctx = ralloc_context(NULL);
		this->control_flow_depth = 0;
	}

	~ir_variable_refcount_visitor(void)
	{
		ralloc_free(this->mem_ctx);
	}

	virtual ir_visitor_status visit(ir_variable *);
	virtual ir_visitor_status visit(ir_dereference_variable *);

	virtual ir_visitor_status visit_enter(ir_function_signature *);
	virtual ir_visitor_status visit_leave(ir_assignment *);
	virtual ir_visitor_status visit_leave(ir_call *);

	virtual ir_visitor_status visit_enter(ir_if *);
	virtual ir_visitor_status visit_leave(ir_if *);
	virtual ir_visitor_status visit_enter(ir_loop *);
	virtual ir_visitor_status visit_leave(ir_loop *);

	ir_variable_refcount_entry *get_variable_entry(ir_variable *var);

	/* List of ir_variable_refcount_entry */
	std::map<ir_variable*, ir_variable_refcount_entry*, ir_variable_compare> Variables;
	unsigned control_flow_depth;
	void *mem_ctx;
};
