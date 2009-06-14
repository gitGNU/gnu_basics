/*
 * Copyright (c) 2009, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef REFS_H_
#define REFS_H_

/**
 * @file refs.h
 *
 * @brief This file provides the interface for handling with containers
 *
 * Containers are objects that allows building collections of homogeneous
 * data, so-called elements. They are of two main types. Sequential containers
 * stores elements in a raw. Associative containers stores elements in an
 * ordered manner.
 *
 * At glance, sequential containers are fast for adding and removing elements
 * but behave poorly when being searched. Associative containers require
 * elements to be comparable. They are slower for insertion and removal but
 * provide efficient search algorithms.
 *
 * Containers use references to keep track of elements. A reference is a data
 * structure that holds one or several pointer to other references.
 *
 * Elements contains a reference as a member. It is possible to dereference a
 * reference to get the pointer of the element it is bound to using
 * b6_container_of. Here is an example with a simply-linked reference:
 *
 * @code
 * struct element {
 * 	...
 * 	struct b6_sref sref;
 * 	...
 * };
 *
 * struct element *foo(struct b6_sref *ref)
 * {
 *  	return b6_container_of(ref, struct element, sref);
 * }
 * @endcode
 *
 * Note that not every reference is linked to an element: every container has
 * head and tail references that are placed before and after any reference
 * within the container respectively. It is illegal to dereference them.
 *
 * @see b6_sref, b6_dref, b6_tref
 * @see b6_ref_compare_t, b6_ref_examine_t
 */

#include "utils.h"
#include "assert.h"

enum { B6_NEXT, B6_PREV };

typedef int (*b6_ref_compare_t)(const void *ref1, const void *ref2);

/**
 * @brief Type definition for functions to be called back when searching
 * containers
 *
 * It is insured that the function is never called with the head or tail
 * reference of the queue. Thus, it is safe to dereference the reference given
 * as parameter.
 *
 * @param ref reference to check
 * @param arg opaque data specified along with this function pointer
 * @return true or false according to whether the reference matches or not
 */
typedef int (*b6_ref_examine_t)(const void *ref, void *arg);

/**
 * @brief Single reference
 * @see b6_deque
 */
struct b6_sref {
	struct b6_sref *ref; /**< link to another reference */
};

struct b6_dref {
	struct b6_dref *ref[2];
};

struct b6_tref {
	struct b6_tref *ref[2];
	struct b6_tref *top;
	signed char dir;
	signed char balance;
};

/**
 * @brief Doubly-ended queue
 *
 * A doubly-ended queue or deque is a sequence of elements where each element
 * has a reference to the next one. Doubly-ended queues have a small footprint
 * in memory. Insertions and deletions after a reference within the deque are
 * very fast as they execute in a constant time. Adding an element as last
 * element is that fast.
 *
 * The counterpart is that operations that need to get a reference to a
 * previous element are inefficient. They have a linear complexity as the
 * whole deque as possibly to be traveled to find it. For instance, removing
 * the last element is that slow.
 */
struct b6_deque {
	struct b6_sref head; /**< reference before any element in the queue */
	struct b6_sref tail; /**< reference after any element in the queue */
	struct b6_sref *last; /**< reference before b6_deque::tail */
};

/**
 * @brief Initialize a deque statically
 * @param deque name of the variable
 */
#define B6_DEQUE_DEFINE(deque)						\
	struct b6_deque deque = { { &deque.tail }, { NULL }, &deque.head }

/**
 * @brief Initialize or clear a doubly-ended queue
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 */
static inline void b6_deque_initialize(struct b6_deque *deque)
{
	b6_precond(deque != NULL);

	deque->head.ref = &deque->tail;
	deque->tail.ref = NULL;
	deque->last = &deque->head;
}

/**
 * @brief Test if a doubly-ended queue contains elements
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @retval true if the doubly-ended queue does not contains any elements
 * @retval false if the doubly-ended queue contains one element or more
 */
static inline int b6_deque_empty(const struct b6_deque *deque)
{
	b6_precond(deque != NULL);

	return deque->head.ref == &deque->tail;
}

/**
 * @brief Travel a doubly-ended queue reference per reference
 * @complexity O(n) for B6_PREV, O(1) for B6_NEXT
 * @param deque pointer to the doubly-ended queue
 * @param curr reference to walk from
 * @param direction B6_PREV or B6_NEXT to get the previous or next reference
 * respectively
 * @returns NULL when going out of range or the next or previous reference in
 * the sequence
 */
static inline struct b6_sref *b6_deque_walk(const struct b6_deque *deque,
                                            const struct b6_sref *curr,
                                            int direction)
{
	const struct b6_sref *prev;

	b6_precond(deque != NULL);
	b6_precond(curr != NULL);
	b6_precond(direction == B6_PREV || direction == B6_NEXT);

	if (direction == B6_NEXT)
		return curr->ref;

	if (curr == &deque->tail)
		return deque->last;

	if (curr == &deque->head)
		return NULL;

	for (prev = &deque->head; prev->ref != curr; prev = prev->ref);

	return (struct b6_sref *)prev;
}

/**
 * @brief Insert a new element after a reference in the doubly-ended queue
 *
 * It is illegal to attempt to insert an element after the tail reference of
 * the doubly-ended queue.
 *
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @param prev reference in the doubly-ended queue
 * @param sref reference of the element to insert
 * @return sref
 */
static inline struct b6_sref *b6_deque_add_after(struct b6_deque *deque,
                                                 struct b6_sref *prev,
                                                 struct b6_sref *sref)
{
	struct b6_sref *next;

	b6_precond(prev != NULL);
	next = prev->ref;

	b6_precond(deque != NULL);
	if (b6_unlikely(prev == deque->last))
		deque->last = sref;

	b6_precond(sref != NULL);
	sref->ref = next;
	prev->ref = sref;

	return sref;
}

/**
 * @brief Remove an element placed after a reference within the doubly-ended
 * queue
 *
 * It is illegal to attempt to remove the tail reference of the doubly-ended
 * queue or after.
 *
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @param prev pointer to the reference in the queue
 * @return pointer to the reference of the element removed
 */
static inline struct b6_sref *b6_deque_del_after(struct b6_deque *deque,
                                                 struct b6_sref *prev)
{
	struct b6_sref *curr;

	b6_precond(prev != NULL);
	curr = prev->ref;

	b6_precond(deque != NULL);
	if (b6_unlikely(curr == deque->last))
		deque->last = prev;

	b6_precond(curr != NULL);
	prev->ref = curr->ref;

	b6_postcond (curr != &deque->tail);

	return curr;
}

/**
 * @brief Search a doubly-ended queue for an element and return its predecessor
 * @complexity O(n)
 * @param deque pointer to the doubly-ended queue
 * @param sref reference in the queue to start the search from
 * @param func pointer to the function to call when watching elements
 * @param arg opaque data to pass to func
 * @param direction either B6_NEXT or B6_PREV
 * @retval pointer to the found element
 * @retval pointer to the head or tail of doubly-ended queue when searching
 * backwards or forwards repectively failed.
 * @note It is safe to specify head or tail as sref parameter as sref is
 * neither dereferenced nor examined through func during the search. As a
 * consequence, sref is to be examined before calling b6_deque_find_before if
 * needed.
 */
static inline struct b6_sref *b6_deque_find_before(const struct b6_deque *deque,
                                                   const struct b6_sref *sref,
                                                   b6_ref_examine_t func,
                                                   void *arg, int direction)
{
	const struct b6_sref *prev, *curr, *temp;

	switch (direction) {
	case B6_PREV:
		temp = prev = &deque->head;
		while ((curr = b6_deque_walk(deque, prev, B6_NEXT)) != sref) {
			if (func(curr, arg))
				temp = prev;
			prev = curr;
		}
		return (struct b6_sref*) temp;

	case B6_NEXT:
		prev = sref;
		while ((curr = b6_deque_walk(deque, prev, B6_NEXT)) != NULL) {
			if (func(curr, arg))
				break;
			prev = curr;
		}
		return (struct b6_sref*) prev;

	default:
		b6_precond(direction == B6_PREV || direction == B6_NEXT);
		return NULL;
	}
}

/**
 * @brief Insert a new element before a reference in the doubly-ended queue
 * @complexity O(n)
 * @param deque pointer to the doubly-ended queue
 * @param next reference in the doubly-ended queue
 * @param sref reference of the element to insert
 * @return sref
 */
static inline struct b6_sref *b6_deque_add(struct b6_deque *deque,
                                           struct b6_sref *next,
                                           struct b6_sref *sref)
{
	struct b6_sref *prev;

	prev = b6_deque_walk(deque, next, B6_PREV);

	return b6_deque_add_after(deque, prev, sref);
}

/**
 * @brief Remove an element within the doubly-ended queue
 *
 * It is illegal to attempt to remove the head or the tail reference of the
 * doubly-ended queue.
 *
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @param sref pointer to the reference in the queue
 * @return pointer to the reference of the element removed
 */
static inline struct b6_sref *b6_deque_del(struct b6_deque *deque,
                                           struct b6_sref *sref)
{
	struct b6_sref *prev;

	prev = b6_deque_walk(deque, sref, B6_PREV);

	return b6_deque_del_after(deque, prev);
}

/**
 * @brief Search a doubly-ended queue for an element.
 * @complexity O(n)
 * @param deque pointer to the doubly-ended queue
 * @param prev pointer to the element in the queue
 * @param func pointer to the function to call when watching elements
 * @param arg opaque data to pass to func
 * @param direction either B6_NEXT or B6_PREV
 * @return pointer to the found element or a pointer to the head or tail of
 * doubly-ended queue when searching backwards or forwards repectively.
 */
static inline struct b6_sref *b6_deque_find(const struct b6_deque *deque,
                                            const struct b6_sref *prev,
                                            b6_ref_examine_t func, void *arg,
                                            int direction)
{
	const struct b6_sref *curr;

	curr = b6_deque_find_before(deque, prev, func, arg, direction);

	return b6_deque_walk(deque, curr, B6_NEXT);
}

/**
 * @brief Insert an element as first element of the doubly-ended queue
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @param sref pointer to the reference of the element to insert
 * @return sref
 */
static inline struct b6_sref *b6_deque_add_first(struct b6_deque *deque,
                                                 struct b6_sref *sref)
{
	return b6_deque_add_after(deque, &deque->head, sref);
}

/**
 * @brief Insert an element as last element of the doubly-ended queue
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @param sref pointer to the reference of the element to insert
 * @return sref
 */
static inline struct b6_sref *b6_deque_add_last(struct b6_deque *deque,
                                                struct b6_sref *sref)
{
	return b6_deque_add_after(deque, deque->last, sref);
}

/**
 * @brief Remove the first element of a doubly-ended queue
 * @complexity O(1)
 * @pre The doubly-ended queue is not empty
 * @param deque pointer to the doubly-ended queue
 * @return pointer to the reference of the element removed
 */
static inline struct b6_sref *b6_deque_del_first(struct b6_deque *deque)
{
	return b6_deque_del_after(deque, &deque->head);
}

/**
 * @brief Remove the last element of a doubly-ended queue
 * @complexity O(n)
 * @pre The doubly-ended queue is not empty
 * @param deque pointer to the doubly-ended queue
 * @return pointer to the reference of the element removed
 */
static inline struct b6_sref *b6_deque_del_last(struct b6_deque *deque)
{
	return b6_deque_del(deque, deque->last);
}

/**
 * @brief Return the reference of the first element of a doubly-ended queue
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @return pointer to the reference of first element or tail if the
 * doubly-ended queue is empty
 */
static inline struct b6_sref *b6_deque_first(const struct b6_deque *deque)
{
	return deque->head.ref;
}

/**
 * @brief Return the reference of the last element of a doubly-ended queue
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @return pointer to the reference of first element or head if the
 * doubly-ended queue is empty
 */
static inline struct b6_sref *b6_deque_last(const struct b6_deque *deque)
{
	return deque->last;
}

struct b6_list {
	struct b6_dref head, tail;
};

#define B6_LIST_DEFINE(list)						\
	struct b6_list list = {{&list.tail, NULL}, {NULL, &list.head}}

static inline void b6_list_initialize(struct b6_list *list)
{
	list->head.ref[B6_NEXT] = &list->tail;
	list->head.ref[B6_PREV] = NULL;
	list->tail.ref[B6_NEXT] = NULL;
	list->tail.ref[B6_PREV] = &list->head;
}

static inline int b6_list_empty(const struct b6_list *list)
{
	b6_precond(list != NULL);

	return list->head.ref[B6_NEXT] == &list->tail;
}

static inline struct b6_dref *b6_list_add(struct b6_list *list,
                                          struct b6_dref *next,
                                          struct b6_dref *node)
{
	struct b6_dref *prev;

	b6_precond(list != NULL);

	b6_precond(next != NULL);
	prev = next->ref[B6_PREV];

	b6_precond(prev != NULL);
	prev->ref[B6_NEXT] = node;
	next->ref[B6_PREV] = node;

	b6_precond(node != NULL);
	node->ref[B6_PREV] = prev;
	node->ref[B6_NEXT] = next;

	return node;
}

static inline struct b6_dref *b6_list_del(struct b6_list *list,
                                          struct b6_dref *node)
{
	struct b6_dref *prev;

	b6_precond(list != NULL);

	b6_precond(node != NULL);
	prev = node->ref[B6_PREV];

	b6_precond(prev != NULL);
	prev->ref[B6_NEXT] = node->ref[B6_NEXT];

	return node;
}

static inline struct b6_dref *b6_list_walk(const struct b6_list *list,
                                           const struct b6_dref *curr,
                                           int direction)
{
	b6_precond((unsigned)direction < b6_card_of(curr->ref));
	b6_precond(list != NULL);
	b6_precond(curr != NULL);

	return (struct b6_dref *)curr->ref[direction];
}

static inline struct b6_dref *b6_list_find(const struct b6_list *list,
                                           const struct b6_dref *prev,
                                           b6_ref_examine_t func, void *arg,
                                           int direction)
{
	struct b6_dref *next;

	b6_precond((unsigned)direction < b6_card_of(prev->ref));

	next = b6_list_walk(list, prev, direction);
	do {
		prev = next;
		next = b6_list_walk(list, prev, direction);
	} while (next != NULL && func(prev, arg));

	return (struct b6_dref *)prev;
}

static inline struct b6_dref *b6_list_add_first(struct b6_list *list,
                                                struct b6_dref *dref)
{
	return b6_list_add(list, list->head.ref[B6_NEXT], dref);
}

static inline struct b6_dref *b6_list_add_last(struct b6_list *list,
                                               struct b6_dref *dref)
{
	return b6_list_add(list, list->tail.ref[B6_PREV], dref);
}

static inline struct b6_dref *b6_list_del_first(struct b6_list *list)
{
	return b6_list_del(list, list->head.ref[B6_NEXT]);
}

static inline struct b6_dref *b6_list_del_last(struct b6_list *list)
{
	return b6_list_del(list, list->tail.ref[B6_PREV]);
}

static inline struct b6_dref *b6_list_first(const struct b6_list *list)
{
	return list->head.ref[B6_NEXT];
}

static inline struct b6_dref *b6_list_last(const struct b6_list *list)
{
	return list->tail.ref[B6_PREV];
}

struct b6_splay {
	struct b6_dref head, tail, *root;
	b6_ref_compare_t comp;
};

void b6_splay_initialize(struct b6_splay *splay, b6_ref_compare_t compare);

static inline int b6_splay_empty(const struct b6_splay *splay)
{
	b6_precond(splay != NULL);

	return &splay->tail == splay->head.ref[B6_NEXT] ||
	       &splay->head == splay->tail.ref[B6_PREV];
}

struct b6_dref *b6_splay_add(struct b6_splay *splay, struct b6_dref *ref);
struct b6_dref *b6_splay_del(struct b6_splay *splay, struct b6_dref *ref);
struct b6_dref *b6_splay_find(const struct b6_splay *splay,
                              b6_ref_examine_t examine, void *argument);
struct b6_dref *b6_splay_walk(const struct b6_splay *splay,
                              const struct b6_dref *ref, int direction);

static inline struct b6_dref *b6_splay_first(const struct b6_splay *splay)
{
	return b6_splay_walk(splay, &splay->head, B6_NEXT);
}

static inline struct b6_dref *b6_splay_last(const struct b6_splay *splay)
{
	return b6_splay_walk(splay, &splay->tail, B6_PREV);
}

struct b6_tree {
	const struct b6_tree_ops *ops;
	struct b6_tref head, tail, root;
	b6_ref_compare_t comp;
};

struct b6_tree_ops {
	void (*add)(struct b6_tree*, struct b6_tref*);
	void (*del)(struct b6_tree*, struct b6_tref*, int, struct b6_tref*);
	int (*chk)(const struct b6_tree*, struct b6_tref*, struct b6_tref**);
};

extern const struct b6_tree_ops b6_avl_tree;
extern const struct b6_tree_ops b6_rb_tree;

void b6_tree_initialize(struct b6_tree *tree, b6_ref_compare_t compare,
                        const struct b6_tree_ops *ops);

static inline int b6_tree_empty(const struct b6_tree *tree)
{
	b6_precond(tree != NULL);

	return &tree->tail == tree->head.ref[B6_NEXT] ||
	       &tree->head == tree->tail.ref[B6_PREV];
}

struct b6_tref *b6_tree_find(const struct b6_tree *tree,
                             struct b6_tref **top, int *dir,
                             b6_ref_examine_t examine, void *argument);

struct b6_tref *b6_tree_insert(struct b6_tree *tree, struct b6_tref *top,
                               int dir, struct b6_tref *ref);

static inline struct b6_tref *b6_tree_add(struct b6_tree *tree,
                                          struct b6_tref *ref)
{
	struct b6_tref *top, *tmp;
	int dir;

	tmp = b6_tree_find(tree, &top, &dir, (void*)tree->comp, ref);
	if (tmp != NULL)
		return tmp;

	return b6_tree_insert(tree, top, dir, ref);
}

struct b6_tref *b6_tree_del(struct b6_tree *tree, struct b6_tref *ref);

static inline struct b6_tref *b6_tree_remove(struct b6_tree *tree,
                                             b6_ref_examine_t examine,
                                             void *argument)
{
	struct b6_tref *top, *ref;
	int dir;

	ref = b6_tree_find(tree, &top, &dir, examine, argument);
	if (ref != NULL)
		b6_tree_del(tree, ref);

	return ref;
}

struct b6_tref *b6_tree_walk(const struct b6_tree *tree,
                             const struct b6_tref *ref, int direction);

static inline struct b6_tref *b6_tree_first(const struct b6_tree *tree)
{
	return b6_tree_walk(tree, &tree->head, B6_NEXT);
}

static inline struct b6_tref *b6_tree_last(const struct b6_tree *tree)
{
	return b6_tree_walk(tree, &tree->tail, B6_PREV);
}

static inline int b6_tree_check(const struct b6_tree *tree,
                                struct b6_tref **subtree)
{
	return tree->ops->chk(tree, tree->root.ref[B6_PREV], subtree);
}

#endif /* REFS_H_ */
