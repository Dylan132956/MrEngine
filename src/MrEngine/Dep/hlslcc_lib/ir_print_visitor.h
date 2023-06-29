
#pragma once
#ifndef IR_PRINT_VISITOR_H
#define IR_PRINT_VISITOR_H

#include "ir.h"
#include "ir_visitor.h"

extern void _mesa_print_ir(exec_list *instructions,
			   struct _mesa_glsl_parse_state *state);

/**
 * Abstract base class of visitors of IR instruction trees
 */
class ir_print_visitor : public ir_visitor {
public:
   ir_print_visitor();
   virtual ~ir_print_visitor();

   void indent(void);

   /**
    * \name Visit methods
    *
    * As typical for the visitor pattern, there must be one \c visit method for
    * each concrete subclass of \c ir_instruction.  Virtual base classes within
    * the hierarchy should not have \c visit methods.
    */
   /*@{*/
   virtual void visit(ir_rvalue *);
   virtual void visit(ir_variable *);
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_texture *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference_variable *);
   virtual void visit(ir_dereference_array *);
   virtual void visit(ir_dereference_image *);
   virtual void visit(ir_dereference_record *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_discard *);
   virtual void visit(ir_if *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_loop_jump *);
   virtual void visit(ir_atomic *);
   /*@}*/

private:
   /**
    * Fetch/generate a unique name for ir_variable.
    *
    * GLSL IR permits multiple ir_variables to share the same name.  This works
    * fine until we try to print it, when we really need a unique one.
    */
   const char *unique_name(ir_variable *var);

   /** A mapping from ir_variable * -> unique printable names. */
   hash_table *printable_names;
   _mesa_symbol_table *symbols;

   void *mem_ctx;

   int indentation;
};

#endif /* IR_PRINT_VISITOR_H */
