

#ifndef _SIMPLE_LIST_H
#define _SIMPLE_LIST_H

struct simple_node {
   struct simple_node *next;
   struct simple_node *prev;
};

/**
 * Remove an element from list.
 *
 * \param elem element to remove.
 */
#define remove_from_list(elem)			\
do {						\
   (elem)->next->prev = (elem)->prev;		\
   (elem)->prev->next = (elem)->next;		\
} while (0)

/**
 * Insert an element to the list head.
 *
 * \param list list.
 * \param elem element to insert.
 */
#define insert_at_head(list, elem)		\
do {						\
   (elem)->prev = list;				\
   (elem)->next = (list)->next;			\
   (list)->next->prev = elem;			\
   (list)->next = elem;				\
} while(0)

/**
 * Insert an element to the list tail.
 *
 * \param list list.
 * \param elem element to insert.
 */
#define insert_at_tail(list, elem)		\
do {						\
   (elem)->next = list;				\
   (elem)->prev = (list)->prev;			\
   (list)->prev->next = elem;			\
   (list)->prev = elem;				\
} while(0)

/**
 * Move an element to the list head.
 *
 * \param list list.
 * \param elem element to move.
 */
#define move_to_head(list, elem)		\
do {						\
   remove_from_list(elem);			\
   insert_at_head(list, elem);			\
} while (0)

/**
 * Move an element to the list tail.
 *
 * \param list list.
 * \param elem element to move.
 */
#define move_to_tail(list, elem)		\
do {						\
   remove_from_list(elem);			\
   insert_at_tail(list, elem);			\
} while (0)

/**
 * Make a empty list empty.
 *
 * \param sentinal list (sentinal element).
 */
#define make_empty_list(sentinal)		\
do {						\
   (sentinal)->next = sentinal;			\
   (sentinal)->prev = sentinal;			\
} while (0)

/**
 * Get list first element.
 *
 * \param list list.
 *
 * \return pointer to first element.
 */
#define first_elem(list)       ((list)->next)

/**
 * Get list last element.
 *
 * \param list list.
 *
 * \return pointer to last element.
 */
#define last_elem(list)        ((list)->prev)

/**
 * Get next element.
 *
 * \param elem element.
 *
 * \return pointer to next element.
 */
#define next_elem(elem)        ((elem)->next)

/**
 * Get previous element.
 *
 * \param elem element.
 *
 * \return pointer to previous element.
 */
#define prev_elem(elem)        ((elem)->prev)

/**
 * Test whether element is at end of the list.
 * 
 * \param list list.
 * \param elem element.
 * 
 * \return non-zero if element is at end of list, or zero otherwise.
 */
#define at_end(list, elem)     ((elem) == (list))

/**
 * Test if a list is empty.
 * 
 * \param list list.
 * 
 * \return non-zero if list empty, or zero otherwise.
 */
#define is_empty_list(list)    ((list)->next == (list))

/**
 * Walk through the elements of a list.
 *
 * \param ptr pointer to the current element.
 * \param list list.
 *
 * \note It should be followed by a { } block or a single statement, as in a \c
 * for loop.
 */
#define foreach(ptr, list)     \
        for( ptr=(list)->next ;  ptr!=list ;  ptr=(ptr)->next )

/**
 * Walk through the elements of a list.
 *
 * Same as #foreach but lets you unlink the current value during a list
 * traversal.  Useful for freeing a list, element by element.
 * 
 * \param ptr pointer to the current element.
 * \param t temporary pointer.
 * \param list list.
 *
 * \note It should be followed by a { } block or a single statement, as in a \c
 * for loop.
 */
#define foreach_s(ptr, t, list)   \
        for(ptr=(list)->next,t=(ptr)->next; list != ptr; ptr=t, t=(t)->next)

#endif
