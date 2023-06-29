

/**
* \file opt_dead_code.cpp
*
* Eliminates dead assignments and variable declarations from the code.
*/

#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_visitor.h"
#include "ir_variable_refcount.h"
#include "glsl_types.h"

static bool debug = false;

/**
* Do a dead code pass over instructions and everything that instructions
* references.
*
* Note that this will remove assignments to globals, so it is not suitable
* for usage on an unlinked instruction stream.
*/
bool do_dead_code(exec_list *instructions, bool uniform_locations_assigned)
{
	ir_variable_refcount_visitor v;
	bool progress = false;

	v.run(instructions);

	for (auto& VarListIter : v.Variables)
	{
		auto* entry = VarListIter.second;
		/* Since each assignment is a reference, the referenced count must be
		* greater than or equal to the assignment count.  If they are equal,
		* then all of the references are assignments, and the variable is
		* dead.
		*
		* Note that if the variable is neither assigned nor referenced, both
		* counts will be zero and will be caught by the equality test.
		*/
		check(entry->referenced_count >= entry->assigned_count);

		if (debug)
		{
			printf("%s@%p: %d refs, %d assigns, %sdeclared in our scope\n",
				entry->var->name, (void *)entry->var,
				entry->referenced_count, entry->assigned_count,
				entry->declaration ? "" : "not ");
		}

		if (entry->referenced_count == entry->assigned_count && entry->declaration)
		{
			if (entry->assign)
			{
				/* Remove a single dead assignment to the variable we found.
				* Don't do so if it's a shader output, though.
				*
				* EHartNV - for now, prevent eliminating variables of type image
				*  ToDo - evaluate if there are cases these should be eliminated
				*/
				if (entry->var->mode != ir_var_out && entry->var->mode != ir_var_inout &&
					!(entry->var->type && entry->var->type->base_type == GLSL_TYPE_IMAGE))
				{
					entry->assign->remove();
					progress = true;

					if (debug)
					{
						printf("Removed assignment to %s %d @%p\n",
							entry->var->name, entry->assign->id, (void *)entry->var);
					}
				}
			}
			else if (entry->call)
			{
				if (entry->var->mode != ir_var_out && entry->var->mode != ir_var_inout)
				{
					ir_function_signature* sig = entry->call->callee;
					if (sig->has_output_parameters == false)
					{
						// Since the call doesn't have side effects and the return
						// value is never used, it's safe to remove it.
						entry->call->remove();
						progress = true;
					}
				}
			}
			else
			{
				/* If there are no assignments or references to the variable left,
				* then we can remove its declaration.
				*/

				/* uniform initializers are precious, and could get used by another
				* stage.  Also, once uniform locations have been assigned, the
				* declaration cannot be deleted.
				*/
				if (entry->var->mode == ir_var_uniform && (uniform_locations_assigned || entry->var->constant_value))
				{
					continue;
				}

				entry->var->remove();
				progress = true;

				if (debug)
				{
					printf("Removed declaration of %s %d @%p\n", entry->var->name, entry->var->id, (void *)entry->var);
				}
			}
		}
		else if (entry->declaration && entry->last_assign != NULL &&
			entry->var->mode != ir_var_out && entry->var->mode != ir_var_inout)
		{
			// Make sure this is not a UAV (otherwise single read and write to a UAV get deleted)
			if (entry->var->type && entry->var->type->base_type == GLSL_TYPE_IMAGE)
			{
				// Nothing to do here!
			}
			else
			{
				entry->last_assign->remove();
				progress = true;
			}
		}
	}

	return progress;
}

/**
* Does a dead code pass on the functions present in the instruction stream.
*
* This is suitable for use while the program is not linked, as it will
* ignore variable declarations (and the assignments to them) for variables
* with global scope.
*/
bool do_dead_code_unlinked(exec_list *instructions)
{
	bool progress = false;

	foreach_iter(exec_list_iterator, iter, *instructions)
	{
		ir_instruction *ir = (ir_instruction *)iter.get();
		ir_function *f = ir->as_function();
		if (f)
		{
			foreach_iter(exec_list_iterator, sigiter, *f)
			{
				ir_function_signature *sig =
					(ir_function_signature *)sigiter.get();
				/* The setting of the uniform_locations_assigned flag here is
				* irrelevent.  If there is a uniform declaration encountered
				* inside the body of the function, something has already gone
				* terribly, terribly wrong.
				*/
				if (do_dead_code(&sig->body, false))
				{
					progress = true;
				}
			}
		}
	}

	return progress;
}
