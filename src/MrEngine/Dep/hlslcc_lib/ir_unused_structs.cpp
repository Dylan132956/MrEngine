
#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_visitor.h"
#include "ir_unused_structs.h"
#include "glsl_types.h"

struct struct_entry : public exec_node
{
	struct_entry(const glsl_type *type_) : type(type_) { }
	const glsl_type *type;
};


void 
ir_struct_usage_visitor::add_type(const glsl_type *t)
{
	// Resolve the actual type of arrays.
	t = t->is_array() ? t->fields.array : t;

	if (t->base_type == GLSL_TYPE_STRUCT && !has_struct_entry(t))
	{
		struct_entry *entry = new(mem_ctx) struct_entry(t);
		this->struct_list.push_tail (entry);

		// Iterate over the fields of a struct to add nested types.
		for (unsigned i = 0; i < t->length; ++i)
		{
			this->add_type(t->fields.structure[i].type);
		}
	}
}


bool
ir_struct_usage_visitor::has_struct_entry(const glsl_type *t) const
{
	check(t);
	foreach_iter(exec_list_iterator, iter, this->struct_list)
	{
		struct_entry *entry = (struct_entry *)iter.get();
		if (entry->type == t)
		{
			return true;
		}
	}
	return false;
}


ir_visitor_status
ir_struct_usage_visitor::visit(ir_dereference_variable *ir)
{
	const glsl_type* t = ir->type;
	this->add_type(t);
	return visit_continue;
}

static void visit_variable (ir_instruction* ir, void* data)
{
	ir_variable* var = ir->as_variable();
	if (!var)
		return;
	ir_struct_usage_visitor* self = reinterpret_cast<ir_struct_usage_visitor*>(data);
	const glsl_type* t = var->type;
	self->add_type(t);
}

ir_struct_usage_visitor::ir_struct_usage_visitor()
{
	this->mem_ctx = ralloc_context(NULL);
	this->struct_list.make_empty();
	this->callback = visit_variable;
	this->data = this;
}

ir_struct_usage_visitor::~ir_struct_usage_visitor(void)
{
	ralloc_free(mem_ctx);
}
