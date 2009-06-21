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
 * @return for sequential containers, boolean value according to whether the
 * reference matches or not
 * @return for associative containers, -1 if the element is smaller than
 * expected, 1 if the element is greater than expected and 0 if the element
 * equals what is expected.
 */
typedef int (*b6_ref_examine_t)(const void *ref, void *arg);

/**
 * @ingroup deque
 * @brief Single reference
 */
struct b6_sref {
	struct b6_sref *ref; /**< pointer to the next reference */
};

/**
 * @ingroup list, splay
 * @brief Double reference
 */
struct b6_dref {
	struct b6_dref *ref[2]; /**< pointers to other references */
};

struct b6_tref {
	struct b6_tref *ref[2];
	struct b6_tref *top;
	signed char dir;
	signed char balance;
};

/**
 * @defgroup deque Doubly-ended queue
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

/**
 * @ingroup deque
 * @brief Doubly-ended queue
 * @see b6_sref
 */
struct b6_deque {
	struct b6_sref head; /**< reference before any element in the queue */
	struct b6_sref tail; /**< reference after any element in the queue */
	struct b6_sref *last; /**< reference before b6_deque::tail */
};

/**
 * @ingroup deque
 * @brief Initialize a deque statically
 * @param deque name of the variable
 */
#define B6_DEQUE_DEFINE(deque)						\
	struct b6_deque deque = { { &deque.tail }, { NULL }, &deque.head }

/**
 * @ingroup deque
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
 * @ingroup deque
 * @brief Return the reference of the head of a doubly-ended queue
 *
 * The tail reference is such that there is no next reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 *
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @return pointer to the reference of the head of the doubly-ended queue
 */
static inline struct b6_sref *b6_deque_head(const struct b6_deque *deque)
{
	b6_precond(deque != NULL);

	return (struct b6_sref *) &deque->head;
}

/**
 * @ingroup deque
 * @brief Return the reference of the tail of a doubly-ended queue
 *
 * The head reference is such that there is no previous reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 *
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @return pointer to the reference of the tail of the doubly-ended queue
 */
static inline struct b6_sref *b6_deque_tail(const struct b6_deque *deque)
{
	b6_precond(deque != NULL);

	return (struct b6_sref *) &deque->tail;
}

/**
 * @ingroup deque
 * @brief Travel a doubly-ended queue reference per reference
 * @complexity O(n) for B6_PREV, O(1) for B6_NEXT
 * @param deque pointer to the doubly-ended queue
 * @param curr reference to walk from
 * @param direction B6_PREV or B6_NEXT to get the previous or next reference
 * respectively
 * @return NULL when going out of range or the next or previous reference in
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

	if (curr == b6_deque_tail(deque))
		return deque->last;

	if (curr == b6_deque_head(deque))
		return NULL;

	for (prev = b6_deque_head(deque); prev->ref != curr; prev = prev->ref);

	return (struct b6_sref *)prev;
}

/**
 * @ingroup deque
 * @brief Return the reference of the first element of a doubly-ended queue
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @return pointer to the reference after the head
 *
 * If the container is empty, then its tail reference is returned
 */
static inline struct b6_sref *b6_deque_first(const struct b6_deque *deque)
{
	return b6_deque_walk(deque, b6_deque_head(deque), B6_NEXT);
}

/**
 * @ingroup deque
 * @brief Return the reference of the last element of a doubly-ended queue
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @return pointer to the reference before the tail
 *
 * If the container is empty, then its tail reference is returned
 */
static inline struct b6_sref *b6_deque_last(const struct b6_deque *deque)
{
	return b6_deque_walk(deque, b6_deque_tail(deque), B6_PREV);
}

/**
 * @ingroup deque
 * @brief Test if a doubly-ended queue contains elements
 * @complexity O(1)
 * @param deque pointer to the doubly-ended queue
 * @return 0 if the deque contains one element or more and another value if it
 * does not contains any elements
 */
static inline int b6_deque_empty(const struct b6_deque *deque)
{
	return b6_deque_first(deque) == b6_deque_tail(deque);
}

/**
 * @ingroup deque
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

	b6_precond(next != NULL);
	b6_precond(deque != NULL);
	if (b6_unlikely(prev == deque->last))
		deque->last = sref;

	b6_precond(sref != NULL);
	sref->ref = next;
	prev->ref = sref;

	return sref;
}

/**
 * @ingroup deque
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

	b6_precond(curr != &deque->tail);
	b6_precond(curr != NULL);
	prev->ref = curr->ref;

	return curr;
}

/**
 * @ingroup deque
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
 * @ingroup deque
 * @brief Remove an element within the doubly-ended queue
 *
 * It is illegal to attempt to remove the head or the tail reference of the
 * doubly-ended queue.
 *
 * @complexity O(n)
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
 * @ingroup deque
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
 * @ingroup deque
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
 * @ingroup deque
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
 * @ingroup deque
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
 * @defgroup list Doubly-linked list
 *
 * Doubly-linked list is a deque where each element is linked to its successor
 * and its predecessor in the sequence. As a result, there are no "after"
 * operations anymore as inserting or removing an element before a reference
 * offer the same performance.
 *
 * Fast operations on deque are faster than their equivalent on lists, even if
 * they have the same overall complexity. Moreover, lists have a slightly
 * bigger memeory footprint.
 */

/**
 * @brief Doubly-linked list
 * @ingroup list
 * @see b6_dref
 */
struct b6_list {
	struct b6_dref head; /**< reference before any element in the list */
	struct b6_dref tail; /**< reference after any element in the list */
};

/**
 * @brief Initialize a list statically
 * @ingroup list
 * @param list name of the variable
 */
#define B6_LIST_DEFINE(list)						\
	struct b6_list list = {{{&list.tail, NULL}}, {{NULL, &list.head}}}

/**
 * @brief Initialize or clear a doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 */
static inline void b6_list_initialize(struct b6_list *list)
{
	b6_precond(list != NULL);

	list->head.ref[B6_NEXT] = &list->tail;
	list->head.ref[B6_PREV] = NULL;
	list->tail.ref[B6_NEXT] = NULL;
	list->tail.ref[B6_PREV] = &list->head;
}

/**
 * @ingroup list
 * @brief Return the reference of the head of a doubly-linked list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return pointer to the reference of the head of the doubly-linked list
 *
 * The tail reference is such that there is no next reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 *
 */
static inline struct b6_dref *b6_list_head(const struct b6_list *list)
{
	b6_precond(list != NULL);

	return (struct b6_dref *) &list->head;
}

/**
 * @ingroup list
 * @brief Return the reference of the tail of a doubly-linked list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return pointer to the reference of the tail of the doubly-linked list
 *
 * The head reference is such that there is no previous reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 *
 */
static inline struct b6_dref *b6_list_tail(const struct b6_list *list)
{
	b6_precond(list != NULL);

	return (struct b6_dref *) &list->tail;
}

/**
 * @brief Travel a doubly-linked list reference per reference
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @param dref reference to walk from
 * @param direction B6_PREV or B6_NEXT to get the previous or next reference
 * respectively
 * @return NULL when going out of range or the next or previous reference in
 * the sequence
 */
static inline struct b6_dref *b6_list_walk(const struct b6_list *list,
                                           const struct b6_dref *dref,
                                           int direction)
{
	b6_precond((unsigned)direction < b6_card_of(dref->ref));
	b6_precond(list != NULL);
	b6_precond(dref != NULL);

	return (struct b6_dref *)dref->ref[direction];
}

/**
 * @brief Return the reference of the first element of a doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return pointer to the reference of the first element or tail if the
 * list is empty
 */
static inline struct b6_dref *b6_list_first(const struct b6_list *list)
{
	return b6_list_walk(list, b6_list_head(list), B6_NEXT);
}

/**
 * @brief Return the reference of the last element of a doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return pointer to the reference of the last element or head if the
 * list is empty
 */
static inline struct b6_dref *b6_list_last(const struct b6_list *list)
{
	return b6_list_walk(list, b6_list_tail(list), B6_PREV);
}

/**
 * @brief Test if a doubly-linked list contains elements
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return 0 if the list contains one element or more and another value if it
 * does not contains any elements
 */
static inline int b6_list_empty(const struct b6_list *list)
{
	return b6_list_first(list) == b6_list_tail(list);
}

/**
 * @brief Insert a new element before a reference in the doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @param next reference in the list
 * @param dref reference of the element to insert
 * @return dref
 */
static inline struct b6_dref *b6_list_add(struct b6_list *list,
                                          struct b6_dref *next,
                                          struct b6_dref *dref)
{
	struct b6_dref *prev;

	b6_precond(list != NULL);

	b6_precond(next != NULL);
	prev = next->ref[B6_PREV];

	b6_precond(prev != NULL);
	b6_precond(next != &list->head);
	prev->ref[B6_NEXT] = dref;
	next->ref[B6_PREV] = dref;

	b6_precond(dref != NULL);
	dref->ref[B6_PREV] = prev;
	dref->ref[B6_NEXT] = next;

	return dref;
}

/**
 * @ingroup list
 * @brief Remove an element within the doubly-linked list
 *
 * It is illegal to attempt to remove the head or the tail reference of the
 * doubly-linked list.
 *
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @param dref pointer to the reference in the list
 * @return dref
 */
static inline struct b6_dref *b6_list_del(struct b6_list *list,
                                          struct b6_dref *dref)
{
	struct b6_dref *prev, *next;

	b6_precond(list != NULL);

	b6_precond(dref != NULL);
	prev = dref->ref[B6_PREV];
	next = dref->ref[B6_NEXT];

	b6_precond(prev != NULL);
	b6_precond(next != NULL);
	b6_precond(dref != &list->head);
	b6_precond(dref != &list->tail);

	prev->ref[B6_NEXT] = next;
	next->ref[B6_PREV] = prev;

	return dref;
}

/**
 * @brief Insert an element as first element of the doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @param dref pointer to the reference of the element to insert
 * @return dref
 */
static inline struct b6_dref *b6_list_add_first(struct b6_list *list,
                                                struct b6_dref *dref)
{
	return b6_list_add(list, list->head.ref[B6_NEXT], dref);
}

/**
 * @brief Insert an element as last element of the doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @param dref pointer to the reference of the element to insert
 * @return dref
 */
static inline struct b6_dref *b6_list_add_last(struct b6_list *list,
                                               struct b6_dref *dref)
{
	return b6_list_add(list, &list->tail, dref);
}

/**
 * @brief Remove the first element of a doubly-linked list
 * @complexity O(1)
 * @pre The doubly-linked list is not empty
 * @param list pointer to the list
 * @return pointer to the reference of the element removed
 */
static inline struct b6_dref *b6_list_del_first(struct b6_list *list)
{
	return b6_list_del(list, list->head.ref[B6_NEXT]);
}

/**
 * @brief Remove the last element of a doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @pre The doubly-linked list is not empty
 * @param list pointer to the doubly-linked list
 * @return pointer to the reference of the element removed
 */
static inline struct b6_dref *b6_list_del_last(struct b6_list *list)
{
	return b6_list_del(list, list->tail.ref[B6_PREV]);
}

/**
 * @defgroup splay Splay trees
 *
 * Splay trees are self-balanced binary search trees where recently accessed
 * elements are naturally moved near the root. As such, there is no need of an
 * extra balance parameter in reference. Thus, they have a small footprint in
 * memory compared to other trees since they only make use of double
 * references.
 *
 * The drawback is that in-order traversal is slower with splay trees than
 * with AVL or red/black trees as moving from one reference to the next one
 * sometimes require finding its parent, starting from the root.
 */

/**
 * @ingroup splay
 * @brief splay tree
 */
struct b6_splay {
	struct b6_dref head; /**< reference before any element in the tree */
	struct b6_dref tail; /**< reference after any element in the tree */
	struct b6_dref *root; /**< reference that has no parent in the tree */
	b6_ref_compare_t comp; /**< how elements compare to each other */
};

/**
 * @ingroup splay
 * @brief Initialize or clear a splay tree
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @param compare pointer to the function to call for comparing elements
 */
void b6_splay_initialize(struct b6_splay *splay, b6_ref_compare_t compare);

/**
 * @ingroup splay
 * @brief Return the reference of the head of a splay tree
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the head of the container
 *
 * The tail reference is such that there is no next reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 *
 */
static inline struct b6_dref *b6_splay_head(const struct b6_splay *splay)
{
	b6_precond(splay != NULL);

	return (struct b6_dref *) &splay->head;
}

/**
 * @ingroup splay
 * @brief Return the reference of the tail of a splay tree
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the tail of the container
 *
 * The head reference is such that there is no previous reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 *
 */
static inline struct b6_dref *b6_splay_tail(const struct b6_splay *splay)
{
	b6_precond(splay != NULL);

	return (struct b6_dref *) &splay->tail;
}

/**
 * @ingroup splay
 * @brief Return the reference of the most recently accessed reference
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return pointer to the root reference of the splay tree
 */
static inline struct b6_dref *b6_splay_root(const struct b6_splay *splay)
{
	b6_precond(splay != NULL);

	return splay->root;
}

/**
 * @ingroup splay
 * @brief In-order traveling of a splay tree
 * @complexity O(ln(n))
 * @param splay pointer to the splay tree
 * @param dref reference to walk from
 * @param direction B6_PREV (B6_NEXT) to get the reference of the greatest
 * smaller elements (smallest greater elements respectively)
 * @return NULL when going out of range or the next or previous reference in
 * the sequence
 */
struct b6_dref *b6_splay_walk(const struct b6_splay *splay,
                              const struct b6_dref *dref, int direction);

/**
 * @ingroup splay
 * @brief Return the reference of the smallest element in the splay tree
 * according to how elements compare within it
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the smallest element or tail if the
 * tree is empty
 */
static inline struct b6_dref *b6_splay_first(const struct b6_splay *splay)
{
	return b6_splay_walk(splay, b6_splay_head(splay), B6_NEXT);
}

/**
 * @ingroup splay
 * @brief Return the reference of the greatest element in the splay tree
 * according to how elements compare within it
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the greatest element or tail if the
 * tree is empty
 */
static inline struct b6_dref *b6_splay_last(const struct b6_splay *splay)
{
	return b6_splay_walk(splay, b6_splay_tail(splay), B6_PREV);
}

/**
 * @ingroup splay
 * @brief Test if a splay tree contains elements
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return 0 if the list contains one element or more and another value if it
 * does not contains any elements
 */
static inline int b6_splay_empty(const struct b6_splay *splay)
{
	return b6_splay_tail(splay) == b6_splay_head(splay)->ref[B6_NEXT] ||
	       b6_splay_head(splay) == b6_splay_tail(splay)->ref[B6_PREV];
}

/**
 * @ingroup splay
 * @brief Insert a new element in the a splay tree
 * @complexity O(log(n))
 * @param splay pointer to the splay tree
 * @param dref reference of the element to insert
 * @return dref if insertion succeded or the pointer to the element in the
 * tree that equals dref.
 */
struct b6_dref *b6_splay_add(struct b6_splay *splay, struct b6_dref *dref);

/**
 * @ingroup splay
 * @brief Remove the element at the root of the splay tree
 * @complexity O(log(n))
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the element removed
 */
struct b6_dref *b6_splay_del(struct b6_splay *splay);

/**
 * @ingroup splay
 * @brief Search a splay tree
 * @param splay pointer to the splay tree
 * @param examine pointer to the function to call when evaluation the elements
 * or NULL for using the default comparison
 * @param argument opaque data to pass to examine if not NULL or a pointer to
 * the reference of an element to find an equal in splay.
 * @return pointer to the element found that has become the root of the splay
 * tree or NULL if no element was found.
 */
struct b6_dref *b6_splay_search(const struct b6_splay *splay,
				b6_ref_examine_t examine, void *argument);

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

/**
 * @ingroup tree
 * @brief Return the reference of the head of a tree
 * @complexity O(1)
 * @param tree pointer to the tree
 * @return pointer to the reference of the head of the container
 *
 * The tail reference is such that there is no next reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 *
 */
static inline struct b6_tref *b6_tree_head(const struct b6_tree *tree)
{
	b6_precond(tree != NULL);

	return (struct b6_tref *) &tree->head;
}

/**
 * @ingroup tree
 * @brief Return the reference of the tail of a tree
 * @complexity O(1)
 * @param tree pointer to the tree
 * @return pointer to the reference of the tail of the container
 *
 * The head reference is such that there is no previous reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 *
 */
static inline struct b6_tref *b6_tree_tail(const struct b6_tree *tree)
{
	b6_precond(tree != NULL);

	return (struct b6_tref *) &tree->tail;
}

static inline int b6_tree_empty(const struct b6_tree *tree)
{
	b6_precond(tree != NULL);

	return b6_tree_tail(tree) == b6_tree_head(tree)->ref[B6_NEXT] ||
	       b6_tree_head(tree) == b6_tree_tail(tree)->ref[B6_PREV];
}

struct b6_tref *b6_tree_search(const struct b6_tree *tree,
			       struct b6_tref **top, int *dir,
			       b6_ref_examine_t examine, void *argument);

struct b6_tref *b6_tree_insert(struct b6_tree *tree, struct b6_tref *top,
                               int dir, struct b6_tref *ref);

static inline struct b6_tref *b6_tree_add(struct b6_tree *tree,
                                          struct b6_tref *ref)
{
	struct b6_tref *top, *tmp;
	int dir;

	tmp = b6_tree_search(tree, &top, &dir, (void*)tree->comp, ref);
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

	ref = b6_tree_search(tree, &top, &dir, examine, argument);
	if (ref != NULL)
		b6_tree_del(tree, ref);

	return ref;
}

struct b6_tref *b6_tree_walk(const struct b6_tree *tree,
                             const struct b6_tref *ref, int direction);

static inline struct b6_tref *b6_tree_first(const struct b6_tree *tree)
{
	return b6_tree_walk(tree, b6_tree_head(tree), B6_NEXT);
}

static inline struct b6_tref *b6_tree_last(const struct b6_tree *tree)
{
	return b6_tree_walk(tree, b6_tree_tail(tree), B6_PREV);
}

static inline int b6_tree_check(const struct b6_tree *tree,
                                struct b6_tref **subtree)
{
	return tree->ops->chk(tree, tree->root.ref[B6_PREV], subtree);
}

#endif /* REFS_H_ */
