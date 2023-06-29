
class ir_rvalue_visitor : public ir_hierarchical_visitor {
public:

   virtual ir_visitor_status visit_leave(ir_assignment *);
   virtual ir_visitor_status visit_leave(ir_call *);
   virtual ir_visitor_status visit_leave(ir_dereference_array *);
   virtual ir_visitor_status visit_leave(ir_dereference_record *);
   virtual ir_visitor_status visit_leave(ir_expression *);
   virtual ir_visitor_status visit_leave(ir_if *);
   virtual ir_visitor_status visit_leave(ir_return *);
   virtual ir_visitor_status visit_leave(ir_swizzle *);
   virtual ir_visitor_status visit_leave(ir_texture *);
   virtual ir_visitor_status visit_leave(ir_atomic *);

   virtual void handle_rvalue(ir_rvalue **rvalue) = 0;
};
