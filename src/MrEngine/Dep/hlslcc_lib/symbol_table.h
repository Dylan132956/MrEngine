
#ifndef MESA_SYMBOL_TABLE_H
#define MESA_SYMBOL_TABLE_H

struct _mesa_symbol_table;
struct _mesa_symbol_table_iterator;

extern void _mesa_symbol_table_push_scope(struct _mesa_symbol_table *table);

extern void _mesa_symbol_table_pop_scope(struct _mesa_symbol_table *table);

extern int _mesa_symbol_table_add_symbol(struct _mesa_symbol_table *symtab,
    int name_space, const char *name, void *declaration);

extern int _mesa_symbol_table_add_global_symbol(
    struct _mesa_symbol_table *symtab, int name_space, const char *name,
    void *declaration);

extern int _mesa_symbol_table_symbol_scope(struct _mesa_symbol_table *table,
    int name_space, const char *name);

extern void *_mesa_symbol_table_find_symbol(
    struct _mesa_symbol_table *symtab, int name_space, const char *name);

extern struct _mesa_symbol_table *_mesa_symbol_table_ctor(void);

extern void _mesa_symbol_table_dtor(struct _mesa_symbol_table *);

extern struct _mesa_symbol_table_iterator *_mesa_symbol_table_iterator_ctor(
    struct _mesa_symbol_table *table, int name_space, const char *name);

extern void _mesa_symbol_table_iterator_dtor(
    struct _mesa_symbol_table_iterator *);

extern void *_mesa_symbol_table_iterator_get(
    struct _mesa_symbol_table_iterator *iter);

extern int _mesa_symbol_table_iterator_next(
    struct _mesa_symbol_table_iterator *iter);

#endif /* MESA_SYMBOL_TABLE_H */
