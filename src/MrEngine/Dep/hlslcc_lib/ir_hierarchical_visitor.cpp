

#include "ShaderCompilerCommon.h"
#include "ir.h"
#include "ir_hierarchical_visitor.h"

ir_hierarchical_visitor::ir_hierarchical_visitor()
{
   this->base_ir = NULL;
   this->callback = NULL;
   this->data = NULL;
   this->in_assignee = false;
}

ir_visitor_status
ir_hierarchical_visitor::visit(ir_rvalue *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit(ir_variable *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit(ir_constant *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit(ir_loop_jump *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit(ir_dereference_variable *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_enter(ir_loop *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_leave(ir_loop *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_enter(ir_function_signature *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_leave(ir_function_signature *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_enter(ir_function *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_leave(ir_function *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_enter(ir_expression *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_leave(ir_expression *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_enter(ir_texture *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_leave(ir_texture *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_enter(ir_swizzle *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_leave(ir_swizzle *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_enter(ir_dereference_array *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_leave(ir_dereference_array *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_enter(ir_dereference_image *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_leave(ir_dereference_image *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_enter(ir_dereference_record *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_hierarchical_visitor::visit_leave(ir_dereference_record *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_enter(ir_assignment *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_leave(ir_assignment *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_enter(ir_call *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_leave(ir_call *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_enter(ir_return *ir)
{
   if (this->callback != NULL)
      this->callback(ir, this->data);

   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_leave(ir_return *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_enter(ir_discard *ir)
{
   if (this->callback != NULL)
   {
	   this->callback(ir, this->data);
   }

   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_leave(ir_discard *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_enter(ir_if *ir)
{
   if (this->callback != NULL)
   {
	   this->callback(ir, this->data);
   }

   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_leave(ir_if *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_enter(ir_atomic *ir)
{
   if (this->callback != NULL)
   {
	   this->callback(ir, this->data);
   }

   return visit_continue;
}

ir_visitor_status ir_hierarchical_visitor::visit_leave(ir_atomic *ir)
{
   (void) ir;
   return visit_continue;
}

void ir_hierarchical_visitor::run(exec_list *instructions)
{
   visit_list_elements(this, instructions);
}


void visit_tree(ir_instruction *ir,
	   void (*callback)(class ir_instruction *ir, void *data),
	   void *data)
{
   ir_hierarchical_visitor v;

   v.callback = callback;
   v.data = data;

   ir->accept(&v);
}
