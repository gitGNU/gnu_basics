/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_SPLAY_H_
#define B6_SPLAY_H_

#include "refs.h"
#include "utils.h"
#include "assert.h"

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

#endif /* B6_SPLAY_H_ */
