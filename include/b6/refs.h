/*
 * Copyright (c) 2009, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef REFS_H_
#define REFS_H_

#include "utils.h"
#include "assert.h"

enum { B6_NEXT, B6_PREV };

typedef int (*b6_ref_compare_t)(const void *ref1, const void *ref2);
typedef int (*b6_ref_examine_t)(const void *ref, void *arg);

struct b6_sref {
	struct b6_sref *next;
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

struct b6_deque {
	struct b6_sref head;
	struct b6_sref tail;
	struct b6_sref *last;
};

#define B6_DEQUE_DEFINE(deque)						\
	struct b6_deque deque = { { &deque.tail }, { NULL }, &deque.head }

static inline void b6_deque_initialize(struct b6_deque *deque)
{
	b6_precond(deque != NULL);

	deque->head.next = &deque->tail;
	deque->tail.next = NULL;
	deque->last = &deque->head;
}

static inline int b6_deque_empty(const struct b6_deque *deque)
{
	b6_precond(deque != NULL);

	return deque->head.next == &deque->tail;
}

static inline struct b6_sref *b6_deque_walk(const struct b6_deque *deque,
                                            const struct b6_sref *curr,
                                            int direction)
{
	const struct b6_sref *prev;

	b6_precond(deque != NULL);
	b6_precond(curr != NULL);
	b6_precond(direction == B6_PREV || direction == B6_NEXT);

	if (direction == B6_NEXT)
		return curr->next;

	if (curr == &deque->tail)
		return deque->last;

	if (curr == &deque->head)
		return NULL;

	for (prev = &deque->head; prev->next != curr; prev = prev->next);

	return (struct b6_sref *)prev;
}

static inline struct b6_sref *b6_deque_add_after(struct b6_deque *deque,
                                                 struct b6_sref *prev,
                                                 struct b6_sref *sref)
{
	struct b6_sref *next;

	b6_precond(prev != NULL);
	next = prev->next;

	b6_precond(deque != NULL);
	if (b6_unlikely(prev == deque->last))
		deque->last = sref;

	b6_precond(sref != NULL);
	sref->next = next;
	prev->next = sref;

	return sref;
}

static inline struct b6_sref *b6_deque_del_after(struct b6_deque *deque,
                                                 struct b6_sref *prev)
{
	struct b6_sref *curr;

	b6_precond(prev != NULL);
	curr = prev->next;

	b6_precond(deque != NULL);
	if (b6_unlikely(curr == deque->last))
		deque->last = prev;

	b6_precond(curr != NULL);
	prev->next = curr->next;

	return curr;
}

static inline struct b6_sref *b6_deque_find_before(const struct b6_deque *deque,
                                                   const struct b6_sref *prev,
                                                   b6_ref_examine_t func,
                                                   void *arg, int direction)
{
	struct b6_sref *curr, *next;

	curr = b6_deque_walk(deque, prev, direction);
	next = b6_deque_walk(deque, curr, direction);

	while (next != NULL && func(curr, arg)) {
		prev = curr;
		curr = next;
		next = b6_deque_walk(deque, curr, direction);
	}

	return (struct b6_sref *)prev;
}

static inline struct b6_sref *b6_deque_add(struct b6_deque *deque,
                                           struct b6_sref *next,
                                           struct b6_sref *node)
{
	struct b6_sref *prev;

	prev = b6_deque_walk(deque, next, B6_PREV);

	return b6_deque_add_after(deque, prev, node);
}

static inline struct b6_sref *b6_deque_del(struct b6_deque *deque,
                                           struct b6_sref *node)
{
	struct b6_sref *prev;

	prev = b6_deque_walk(deque, node, B6_PREV);

	return b6_deque_del_after(deque, prev);
}

static inline struct b6_sref *b6_deque_find(const struct b6_deque *deque,
                                            const struct b6_sref *prev,
                                            b6_ref_examine_t func, void *arg,
                                            int direction)
{
	const struct b6_sref *curr;

	curr = b6_deque_find_before(deque, prev, func, arg, direction);

	return b6_deque_walk(deque, curr, B6_NEXT);
}

static inline struct b6_sref *b6_deque_add_first(struct b6_deque *deque,
                                                 struct b6_sref *sref)
{
	return b6_deque_add_after(deque, &deque->head, sref);
}

static inline struct b6_sref *b6_deque_add_last(struct b6_deque *deque,
                                                struct b6_sref *sref)
{
	return b6_deque_add_after(deque, deque->last, sref);
}

static inline struct b6_sref *b6_deque_del_first(struct b6_deque *deque)
{
	return b6_deque_del_after(deque, &deque->head);
}

static inline struct b6_sref *b6_deque_del_last(struct b6_deque *deque)
{
	return b6_deque_del(deque, deque->last);
}

static inline struct b6_sref *b6_deque_first(const struct b6_deque *deque)
{
	return deque->head.next;
}

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
