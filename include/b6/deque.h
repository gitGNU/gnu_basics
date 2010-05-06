/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_DEQUE_H_
#define B6_DEQUE_H_

#include "refs.h"
#include "utils.h"
#include "assert.h"

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

#endif /* B6_DEQUE_H_ */
