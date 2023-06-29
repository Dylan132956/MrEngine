

#include "ir.h"
#include "ir_visitor.h"

class ir_struct_usage_visitor : public ir_hierarchical_visitor {
public:
	ir_struct_usage_visitor();
	~ir_struct_usage_visitor(void);

	virtual ir_visitor_status visit(ir_dereference_variable *);

	void add_type(const glsl_type *t);
	bool has_struct_entry(const glsl_type *t) const;

	exec_list struct_list;
	void *mem_ctx;
};
