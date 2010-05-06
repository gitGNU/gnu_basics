/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_TREE_H_
#define B6_TREE_H_

#include "refs.h"
#include "utils.h"
#include "assert.h"

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

#endif /* B6_TREE_H_ */
