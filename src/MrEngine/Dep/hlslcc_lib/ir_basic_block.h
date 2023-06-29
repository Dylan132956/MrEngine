
void call_for_basic_blocks(exec_list *instructions,
			   void (*callback)(ir_instruction *first,
					    ir_instruction *last,
					    void *data),
			   void *data);
