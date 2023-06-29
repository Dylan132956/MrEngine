
#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "glsl_parser_extras.h"
#include "hash_table.h"

struct call_node : public exec_node
{
	class function *func;
};

class function
{
public:
	function(ir_function_signature *sig)
		: sig(sig)
	{
		/* empty */
	}


	/* Callers of this ralloc-based new need not call delete. It's
	* easier to just ralloc_free 'ctx' (or any of its ancestors). */
	static void* operator new(size_t size, void *ctx)
	{
		void *node;

		node = ralloc_size(ctx, size);
		check(node != NULL);

		return node;
	}

	/* If the user *does* call delete, that's OK, we will just
	* ralloc_free in that case. */
	static void operator delete(void *node)
	{
		ralloc_free(node);
	}

	ir_function_signature *sig;

	/** List of functions called by this function. */
	exec_list callees;

	/** List of functions that call this function. */
	exec_list callers;
};

class has_recursion_visitor : public ir_hierarchical_visitor
{
public:
	has_recursion_visitor()
		: current(NULL)
	{
		this->mem_ctx = ralloc_context(NULL);
		this->function_hash = hash_table_ctor(0, ir_hash_table_pointer_hash, ir_hash_table_pointer_compare);
	}

	~has_recursion_visitor()
	{
		hash_table_dtor(this->function_hash);
		ralloc_free(this->mem_ctx);
	}

	function *get_function(ir_function_signature *sig)
	{
		function *f = (function *)hash_table_find(this->function_hash, sig);
		if (f == NULL)
		{
			f = new(mem_ctx)function(sig);
			hash_table_insert(this->function_hash, f, sig);
		}

		return f;
	}

	virtual ir_visitor_status visit_enter(ir_function_signature *sig)
	{
		this->current = this->get_function(sig);
		return visit_continue;
	}

	virtual ir_visitor_status visit_leave(ir_function_signature *sig)
	{
		(void)sig;
		this->current = NULL;
		return visit_continue;
	}

	virtual ir_visitor_status visit_enter(ir_call *call)
	{
		/* At global scope this->current will be NULL.  Since there is no way to
		* call global scope, it can never be part of a cycle.  Don't bother
		* adding calls from global scope to the graph.
		*/
		if (this->current == NULL)
			return visit_continue;

		function *const target = this->get_function(call->callee);

		/* Create a link from the caller to the callee.
		*/
		call_node *node = new(mem_ctx)call_node;
		node->func = target;
		this->current->callees.push_tail(node);

		/* Create a link from the callee to the caller.
		*/
		node = new(mem_ctx)call_node;
		node->func = this->current;
		target->callers.push_tail(node);
		return visit_continue;
	}

	function *current;
	struct hash_table *function_hash;
	void *mem_ctx;
	bool progress;
};

static void
destroy_links(exec_list *list, function *f)
{
	foreach_list_safe(node, list)
	{
		struct call_node *n = (struct call_node *) node;

		/* If this is the right function, remove it.  Note that the loop cannot
		* terminate now.  There can be multiple links to a function if it is
		* either called multiple times or calls multiple times.
		*/
		if (n->func == f)
			n->remove();
	}
}


/**
* Remove a function if it has either no in or no out links
*/
static void
remove_unlinked_functions(const void *key, void *data, void *closure)
{
	has_recursion_visitor *visitor = (has_recursion_visitor *)closure;
	function *f = (function *)data;

	if (f->callers.is_empty() || f->callees.is_empty())
	{
		while (!f->callers.is_empty())
		{
			struct call_node *n = (struct call_node *) f->callers.pop_head();
			destroy_links(& n->func->callees, f);
		}

		while (!f->callees.is_empty())
		{
			struct call_node *n = (struct call_node *) f->callees.pop_head();
			destroy_links(& n->func->callers, f);
		}

		hash_table_remove(visitor->function_hash, key);
		visitor->progress = true;
	}
}


static void emit_errors_unlinked(const void *key, void *data, void *closure)
{
	struct _mesa_glsl_parse_state *state =
		(struct _mesa_glsl_parse_state *) closure;
	function *f = (function *)data;
	YYLTYPE loc;

	(void)key;

	char *proto = prototype_string(f->sig->return_type,
		f->sig->function_name(),
		&f->sig->parameters);

	memset(&loc, 0, sizeof(loc));
	_mesa_glsl_error(&loc, state,
		"function '%s' has static recursion.",
		proto);
	ralloc_free(proto);
}


void detect_recursion_unlinked(struct _mesa_glsl_parse_state *state, exec_list *instructions)
{
	has_recursion_visitor v;

	/* Collect all of the information about which functions call which other
	* functions.
	*/
	v.run(instructions);

	/* Remove from the set all of the functions that either have no caller or
	* call no other functions.  Repeat until no functions are removed.
	*/
	do
	{
		v.progress = false;
		hash_table_call_foreach(v.function_hash, remove_unlinked_functions, & v);
	}
	while (v.progress);


	/* At this point any functions still in the hash must be part of a cycle.
	*/
	hash_table_call_foreach(v.function_hash, emit_errors_unlinked, state);
}

