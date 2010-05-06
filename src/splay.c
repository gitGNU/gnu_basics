/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */
#include "b6/splay.h"

struct b6_dref *__b6_splay_dive(struct b6_dref *ref, int dir)
{
	struct b6_dref *tmp;

	while (!b6_splay_is_thread(tmp = ref->ref[dir]))
		ref = tmp;

	return ref;
}

struct b6_dref *b6_splay_add(struct b6_splay *splay, int dir,
			     struct b6_dref *ref)
{
	struct b6_dref *top = b6_splay_root(splay);

	if (!b6_splay_is_thread(top)) {
		int opp = dir ^ 1;
		struct b6_dref *tmp = top->ref[dir];
		ref->ref[opp] = top;
		ref->ref[dir] = tmp;
		top->ref[dir] = b6_splay_to_thread(ref);
		if (!b6_splay_is_thread(tmp)) {
			tmp = __b6_splay_dive(tmp, opp);
			tmp->ref[opp] = b6_splay_to_thread(ref);
		}
	} else
		ref->ref[B6_NEXT] = ref->ref[B6_PREV] = b6_splay_to_thread(top);

	b6_splay_head(splay)->ref[0] = ref;

	return ref;
}

struct b6_dref *b6_splay_del(struct b6_splay *splay)
{
	struct b6_dref bak = { { NULL, NULL } };
	struct b6_dref *lnk[] = { &bak, &bak };
	struct b6_dref *top = b6_splay_root(splay);
	struct b6_dref *ref, *tmp;

	if (b6_splay_is_thread(top->ref[B6_PREV])) {
		ref = top->ref[B6_NEXT];
		if (!b6_splay_is_thread(ref)) {
			tmp = __b6_splay_dive(ref, B6_PREV);
			tmp->ref[B6_PREV] = top->ref[B6_PREV];
		}
		goto fix_root;
	}

	if (b6_splay_is_thread(top->ref[B6_NEXT])) {
		ref = top->ref[B6_PREV];
		tmp = __b6_splay_dive(ref, B6_NEXT);
		tmp->ref[B6_NEXT] = top->ref[B6_NEXT];
		goto fix_root;
	}

	for (ref = top->ref[B6_NEXT]; !b6_splay_is_thread(ref->ref[B6_PREV]);
	     ref = ref->ref[B6_PREV]) {
	 	tmp = ref->ref[B6_PREV];
		if (b6_splay_is_thread(tmp->ref[B6_NEXT]))
			ref->ref[B6_PREV] = b6_splay_to_thread(tmp);
		else
			ref->ref[B6_PREV] = tmp->ref[B6_NEXT];
		tmp->ref[B6_NEXT] = ref;
		ref = tmp;
		if (b6_splay_is_thread(ref->ref[B6_PREV]))
			break;
		lnk[B6_NEXT]->ref[B6_PREV] = ref;
		lnk[B6_NEXT] = ref;
	}

	if (b6_splay_to_thread(lnk[B6_NEXT]) != ref->ref[B6_NEXT])
		lnk[B6_NEXT]->ref[B6_PREV] = ref->ref[B6_NEXT];
	else
		lnk[B6_NEXT]->ref[B6_PREV] = b6_splay_to_thread(ref);
	lnk[B6_PREV]->ref[B6_NEXT] = ref->ref[B6_PREV];
	ref->ref[B6_PREV] = bak.ref[B6_NEXT];
	ref->ref[B6_NEXT] = bak.ref[B6_PREV];

	ref->ref[B6_PREV] = top->ref[B6_PREV];
	tmp = __b6_splay_dive(ref->ref[B6_PREV], B6_NEXT);
	tmp->ref[B6_NEXT] = b6_splay_to_thread(ref);

fix_root:
	tmp = b6_splay_head(splay);
	tmp->ref[0] = ref;

	return top;
}
