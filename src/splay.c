/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/splay.h"
#include "refs.h"

static int is_splay_ref(const struct b6_dref *ref)
{
	return ref != NULL;
}

static int splay_cmp(const struct b6_splay *splay, const struct b6_dref *ref,
                     b6_ref_examine_t cmp, void *arg)
{
	if (b6_unlikely(ref == &splay->head))
		return -1;

	if (b6_unlikely(ref == &splay->tail))
		return 1;

	return cmp(ref, arg);
}

/*
 * Top-down splay operation based on D. Sleator page:
 * http://www.link.cs.cmu.edu/link/ftp-site/splaying/top-down-splay.c
 */
static int do_splay(struct b6_splay *splay, b6_ref_examine_t cmp, void *arg)
{
	int res;

	struct b6_dref tmp = { { NULL, NULL } };
	struct b6_dref *ref[2] = { &tmp, &tmp };

	b6_precond(splay != NULL);
	b6_precond(cmp != NULL);

	while ((res = splay_cmp(splay, splay->root, cmp, arg))) {
		int opp, dir;

		opp = to_direction(res);
		dir = to_opposite(opp);

		if (!is_splay_ref(splay->root->ref[dir]))
			break;

		if (res == splay_cmp(splay, splay->root->ref[dir], cmp, arg)) {
			struct b6_dref *swp;

			swp = splay->root->ref[dir];
			splay->root->ref[dir] = swp->ref[opp];
			swp->ref[opp] = splay->root;
			splay->root = swp;
			if (!is_splay_ref(splay->root->ref[dir]))
				break;
		}
		ref[opp]->ref[dir] = splay->root;
		ref[opp] = splay->root;
		splay->root = splay->root->ref[dir];
	}

	ref[B6_PREV]->ref[B6_NEXT] = splay->root->ref[B6_PREV];
	ref[B6_NEXT]->ref[B6_PREV] = splay->root->ref[B6_NEXT];
	splay->root->ref[B6_PREV] = tmp.ref[B6_NEXT];
	splay->root->ref[B6_NEXT] = tmp.ref[B6_PREV];

	return res;
}

void b6_splay_initialize(struct b6_splay *splay, b6_ref_compare_t comp)
{
	b6_precond(splay != NULL);

	splay->head.ref[B6_PREV] = NULL;
	splay->head.ref[B6_NEXT] = &splay->tail;
	splay->tail.ref[B6_PREV] = NULL;
	splay->tail.ref[B6_NEXT] = NULL;
	splay->root = &splay->head;
	splay->comp = comp != NULL ? comp : b6_default_compare;
}

/*
 * Splay Tree Insertion
 * --------------------
 *
 * Splay the tree and add the node as new root unless it is already present.
 */
struct b6_dref *b6_splay_add(struct b6_splay *splay, struct b6_dref *ref)
{
	int dir, opp, res;

	b6_precond(splay != NULL);
	b6_precond(ref != NULL);

	res = do_splay(splay, (void*)splay->comp, ref);
	if (res == 0)
		goto bail_out;

	opp = to_direction(res);
	dir = to_opposite(opp);

	ref->ref[dir] = splay->root->ref[dir];
	ref->ref[opp] = splay->root;
	splay->root->ref[dir] = NULL;
	splay->root = ref;

bail_out:
	return splay->root;
}

/*
 * Splay Tree Removal
 * ------------------
 *
 * Due to implementation based on head and tail sentinels, the root of the
 * tree always has a child in the previous direction. If this child has no
 * child in the next direction, we simply replace the root with it. Otherwise,
 * we walk the subtree to find the latest node is the next direction, which
 * actually is the predecessor of the root, and use it as the new root.
 */
struct b6_dref *b6_splay_del(struct b6_splay *splay)
{
	struct b6_dref *top, *cur, *ref;

	b6_precond(splay != NULL);
	b6_precond(!b6_splay_empty(splay));

	ref = splay->root;
	top = splay->root->ref[B6_PREV];
	cur = top->ref[B6_NEXT];
	if (is_splay_ref(cur)) {
		while (is_splay_ref(cur->ref[B6_NEXT])) {
			top = cur;
			cur = cur->ref[B6_NEXT];
		}
		top->ref[B6_NEXT] = cur->ref[B6_PREV];
		cur->ref[B6_PREV] = ref->ref[B6_PREV];
		cur->ref[B6_NEXT] = ref->ref[B6_NEXT];
		splay->root = cur;
	} else {
		top->ref[B6_NEXT] = ref->ref[B6_NEXT];
		splay->root = top;
	}

	return ref;
}

/*
 * Splay Tree Search
 * -----------------
 *
 * Searching a splay tree is first splaying the tree and return the root if it
 * matches.
 */
struct b6_dref *b6_splay_search(const struct b6_splay *splay,
				b6_ref_examine_t examine, void *argument)
{
	if (examine == NULL)
		examine = (b6_ref_examine_t) splay->comp;

	if (do_splay((struct b6_splay *)splay, examine, argument) != 0)
		return NULL;

	return splay->root;
}

/*
 * Splay Tree Travel
 * -----------------
 *
 * Traveling a splay tree is the same operation as traveling a AVL or R/B tree
 * except that this implementation of splay tree does not have a reference to
 * the parent node. Thus, insteand of walking the tree backwards to the root,
 * it is walked from the root to the node, saving the latest parents in each
 * direction.
 */
struct b6_dref *b6_splay_walk(const struct b6_splay *splay,
                              const struct b6_dref *ref, int dir)
{
	struct b6_dref *cur;
	struct b6_dref top;

	b6_precond(splay != NULL);
	b6_precond(ref != NULL);
	b6_precond(is_direction(dir));

	if (is_splay_ref(ref->ref[dir])) {
		int opp;

		opp = to_opposite(dir);
		cur = ref->ref[dir];
		while (is_splay_ref(cur->ref[opp])) cur = cur->ref[opp];

		return cur;
	}

	b6_assert(splay->root != ref);
	cur = splay->root;
	do {
		int res, dir, opp;

		res = splay_cmp(splay, ref, (void *)splay->comp, cur);
		dir = to_direction(res);
		opp = to_opposite(dir);
		top.ref[opp] = cur;
		cur = cur->ref[dir];
	} while (cur != ref);

	return top.ref[dir];
}
