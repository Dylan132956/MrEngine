

#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_visitor.h"
#include "ir_basic_block.h"
#include "glsl_types.h"

/**
* Calls a user function for every basic block in the instruction stream.
*
* Basic block analysis is pretty easy in our IR thanks to the lack of
* unstructured control flow.  We've got:
*
* ir_loop (for () {}, while () {}, do {} while ())
* ir_loop_jump (
* ir_if () {}
* ir_return
* ir_call()
*
* Note that the basic blocks returned by this don't encompass all
* operations performed by the program -- for example, if conditions
* don't get returned, nor do the assignments that will be generated
* for ir_call parameters.
*/
void call_for_basic_blocks(exec_list *instructions,
	void(*callback)(ir_instruction *first,
	ir_instruction *last,
	void *data),
	void *data)
{
	ir_instruction *leader = NULL;
	ir_instruction *last = NULL;

	foreach_iter(exec_list_iterator, iter, *instructions)
	{
		ir_instruction *ir = (ir_instruction *)iter.get();

		if (!leader)
		{
			leader = ir;
		}

		ir_if *ir_if = ir->as_if();
		ir_loop *ir_loop = ir->as_loop();
		ir_function *ir_function = ir->as_function();
		if (ir_if)
		{
			callback(leader, ir, data);
			leader = NULL;

			call_for_basic_blocks(&ir_if->then_instructions, callback, data);
			call_for_basic_blocks(&ir_if->else_instructions, callback, data);
		}
		else if (ir_loop)
		{
			callback(leader, ir, data);
			leader = NULL;
			call_for_basic_blocks(&ir_loop->body_instructions, callback, data);
		}
		else if (ir->as_return() || ir->as_call())
		{
			callback(leader, ir, data);
			leader = NULL;
		}
		else if (ir_function)
		{
			/* A function definition doesn't interrupt our basic block
			* since execution doesn't go into it.  We should process the
			* bodies of its signatures for BBs, though.
			*
			* Note that we miss an opportunity for producing more
			* maximal BBs between the instructions that precede main()
			* and the body of main().  Perhaps those instructions ought
			* to live inside of main().
			*/
			foreach_iter(exec_list_iterator, fun_iter, *ir_function)
			{
				ir_function_signature *ir_sig;

				ir_sig = (ir_function_signature *)fun_iter.get();

				call_for_basic_blocks(&ir_sig->body, callback, data);
			}
		}
		last = ir;
	}
	if (leader)
	{
		callback(leader, last, data);
	}
}
