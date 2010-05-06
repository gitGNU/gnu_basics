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
 * @file splay.h
 *
 * @brief Splay tree container
 *
 * Splay trees are self-balanced binary search trees where recently accessed
 * elements are naturally moved next to the root. As such, there is no need of
 * an extra balance parameter in reference. Thus, they have a small footprint
 * in memory compared to other trees since they only make use of double
 * references.
 *
 * This implementation proposes threaded splay trees so that in-order
 * traversal keeps good performances despite the lack of top reference. This
 * eventually insertion and deletion a bit slower. Note that in-order travesal
 * using b6_splay_walk does not move any element within the tree.xs
 */

/**
 * @ingroup splay
 * @brief splay tree
 */
struct b6_splay {
	struct b6_dref dref; /**< sentinel */
};

static inline int b6_splay_is_thread(const struct b6_dref *dref)
{
	b6_precond(dref);
	return (unsigned long int)dref & 1;
}

static inline struct b6_dref *b6_splay_to_thread(const struct b6_dref *dref)
{
	b6_precond(dref);
	return (struct b6_dref *)((unsigned long int)dref | 1);
}

static inline struct b6_dref *b6_splay_from_thread(const struct b6_dref *dref)
{
	b6_precond(dref);
	return (struct b6_dref *)((unsigned long int)dref & ~1);
}

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
	b6_precond(splay);
	return (struct b6_dref *)&splay->dref;
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
	return b6_splay_head(splay);
}

/**
 * @brief Initialize or clear a splay tree
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @param compare pointer to the function to call for comparing elements
 */
static inline void b6_splay_initialize(struct b6_splay *splay)
{
	struct b6_dref *dref = b6_splay_head(splay);
	dref->ref[0] = b6_splay_to_thread(dref);
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
	struct b6_dref *dref = b6_splay_head(splay);
	return dref->ref[0];
}

/**
 * @brief Test if a splay tree contains elements
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return 0 if the list contains one element or more and another value if it
 * does not contains any elements
 */
static inline int b6_splay_empty(const struct b6_splay *splay)
{
	return b6_splay_is_thread(b6_splay_root(splay));
}

/**
 * @internal
 */
extern struct b6_dref *__b6_splay_dive(struct b6_dref *ref, int dir);

/**
 * @brief In-order traveling of a splay tree
 * @complexity O(ln(n))
 * @param splay pointer to the splay tree
 * @param ref reference to walk from
 * @param direction B6_PREV (B6_NEXT) to get the reference of the greatest
 * smaller elements (smallest greater elements respectively)
 * @return NULL when going out of range or the next or previous reference in
 * the sequence
 */
static inline struct b6_dref *b6_splay_walk(const struct b6_splay *splay,
					    struct b6_dref *dref, int dir)
{
	if (b6_unlikely(dref == b6_splay_head(splay))) {
		struct b6_dref *root = b6_splay_root(splay);
		if (b6_splay_is_thread(root))
			return dref;
		else
			return __b6_splay_dive(root, dir ^ 1);
	}

	if (b6_splay_is_thread(dref->ref[dir]))
		return b6_splay_from_thread(dref->ref[dir]);
	else
		return __b6_splay_dive(dref->ref[dir], dir ^ 1);
}

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
 * @brief Insert a new element in the a splay tree
 * @complexity O(log(n))
 * @param splay pointer to the splay tree
 * @param ref reference of the element to insert
 * @return ref
 */
extern struct b6_dref *b6_splay_add(struct b6_splay *splay, int dir,
				    struct b6_dref *ref);

/**
 * @ingroup splay
 * @brief Remove the element at the root of the splay tree
 * @complexity O(log(n))
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the element removed
 */
extern struct b6_dref *b6_splay_del(struct b6_splay *splay);

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

#define b6_splay_search(splay, dir, cmp, arg)				\
	({								\
		struct b6_splay *_splay = (splay);			\
		void *_arg = (arg);					\
		struct b6_dref bak, *lnk[] = { &bak, &bak };		\
		struct b6_dref *top = b6_splay_root(_splay);		\
		int opp = 1, res = 1;					\
									\
		dir = 0;						\
		if (b6_splay_is_thread(top))				\
			goto bail_out;					\
									\
		while ((res = (cmp)(top, _arg))) {			\
			opp = (res >> 1) & 1;				\
			dir = opp ^ 1;					\
									\
			if (b6_splay_is_thread(top->ref[dir]))		\
				break;					\
									\
			if (res == (cmp)(top->ref[dir], _arg)) {	\
				struct b6_dref *swp = top->ref[dir];	\
				if (b6_splay_is_thread(swp->ref[opp]))	\
					top->ref[dir] =			\
						b6_splay_to_thread(swp); \
				else					\
					top->ref[dir] = swp->ref[opp];	\
				swp->ref[opp] = top;			\
				top = swp;				\
				if (b6_splay_is_thread(top->ref[dir]))	\
					break;				\
			}						\
									\
			lnk[opp]->ref[dir] = top;			\
			lnk[opp] = top;					\
			top = top->ref[dir];				\
		}							\
									\
		if (b6_splay_to_thread(lnk[opp]) != top->ref[opp])	\
			lnk[opp]->ref[dir] = top->ref[opp];		\
		else							\
			lnk[opp]->ref[dir] = b6_splay_to_thread(top);	\
		lnk[dir]->ref[opp] = top->ref[dir];			\
		top->ref[B6_PREV] = bak.ref[B6_NEXT];			\
		top->ref[B6_NEXT] = bak.ref[B6_PREV];			\
									\
		_splay->dref.ref[0] = top;				\
									\
	bail_out:							\
		res;							\
	})

#endif /* B6_SPLAY_H_ */
