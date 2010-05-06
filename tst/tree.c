#include "node.h"

#include "b6/deque.h"
#include "b6/tree.h"

#include <stdio.h>
#include <stdlib.h>

static void b6_tree_debug_helper(struct b6_tree *tree, struct b6_tref *ref)
{
	struct node *node;

	printf("<node id='%u'", (unsigned) ref);

	node = b6_container_of(ref, struct node, tref);
	printf(" label='%d'", node->val);

	printf(" tag='%ld'>", ((long int)(ref->top) & 3) - 1);

	if (ref->ref[B6_PREV])
		b6_tree_debug_helper(tree, ref->ref[B6_PREV]);
	else
		printf("<node id='%u' label='NULL' tag='-1' />",
		       ((unsigned) ref) + 1);

	if (ref->ref[B6_NEXT])
		b6_tree_debug_helper(tree, ref->ref[B6_NEXT]);
	else
		printf("<node id='%u' label='NULL' tag='-1' />",
		       ((unsigned) ref) + 3);

	printf("</node>");

}

void b6_tree_debug(struct b6_tree *tree)
{
	printf("<tree>");
	b6_tree_debug_helper(tree, tree->tref.ref[0]);
	printf("</tree>");
	puts("");
}

typedef struct b6_tref *(*add_t)(struct b6_tref*, int, struct b6_tref*);
typedef struct b6_tref *(*del_t)(struct b6_tref*, int);
typedef int(*chk_t)(const struct b6_tree *, struct b6_tref**);

static struct b6_tref *do_add(struct b6_tree *tree, struct b6_tref *tref)
{
	struct b6_tref *top, *ref;
	int dir;
	struct node *n1 = b6_cast_of(tref, struct node, tref);

	b6_tree_search(tree, ref, top, dir) {
		struct node *n2 = b6_cast_of(ref, struct node, tref);
		int result = node_cmp(n1, n2);

		if (b6_unlikely(!result))
			return ref;

		dir = result == -1 ? B6_NEXT : B6_PREV;
	}

	return b6_tree_add(tree, top, dir, tref);
}

static struct b6_tref *do_del(struct b6_tree *tree, struct b6_tref *tref)
{
	struct b6_tref *top, *ref;
	int dir;
	struct node *n1 = b6_cast_of(tref, struct node, tref);

	b6_tree_search(tree, ref, top, dir) {
		struct node *n2 = b6_cast_of(ref, struct node, tref);
		int result = node_cmp(n1, n2);

		if (b6_unlikely(!result))
			return b6_tree_del(tree, top, dir);

		dir = result == -1 ? B6_NEXT : B6_PREV;
	}

	return NULL;
}

static int do_tree_test(const struct b6_tree_ops *ops)
{
	int retval;
	struct node nodes[256];
	struct b6_tree tree;
	struct b6_deque deque;
	unsigned u;

	b6_tree_initialize(&tree, ops);
	b6_deque_initialize(&deque);

	u = rand() % b6_card_of(nodes);
	while (u--) {
		struct node *node;
		struct b6_tref *tref;

		node = &nodes[u];
		node->val = rand() & 65535;

		tref = do_add(&tree, &node->tref);
		if (tref == &node->tref) {
			b6_deque_add_last(&deque, &node->sref);
			retval = b6_tree_check(&tree, &tref);
			if (retval < 0)
				goto bail_out;
		}
	}

	while (!b6_deque_empty(&deque)) {
		struct node *node;
		struct b6_sref *sref;
		struct b6_tref *tref;

		sref = b6_deque_del_first(&deque);
		node = b6_container_of(sref, struct node, sref);
		tref = do_del(&tree, &node->tref);
		b6_assert(tref == &node->tref);
		retval = b6_tree_check(&tree, &tref);
		if (retval < 0)
			goto bail_out;
	}

	retval = 0;
bail_out:
	return retval;
}

static inline int do_walk_test(void)
{
	struct b6_tree tree;
	struct b6_tref *tref;
	struct node nodes[3];
	int i, dir;

	b6_tree_initialize(&tree, &b6_tree_avl_ops);


	for (tref = b6_tree_first(&tree); tref != b6_tree_tail(&tree);
	     tref = b6_tree_walk(tref, B6_NEXT))
		printf("%p\n", tref);

	for (tref = b6_tree_last(&tree); tref != b6_tree_head(&tree);
	     tref = b6_tree_walk(tref, B6_PREV))
		printf("%p\n", tref);

	for (i = 0; i < b6_card_of(nodes); i += 1)
		nodes[i].val = i;

	b6_tree_top(&tree, &tref, &dir);
	tref = b6_tree_add(&tree, tref, dir, &nodes[0].tref);
	tref = b6_tree_add(&tree, tref, B6_NEXT, &nodes[1].tref);
	tref = b6_tree_add(&tree, tref, B6_NEXT, &nodes[2].tref);

	for (tref = b6_tree_first(&tree); tref != b6_tree_tail(&tree);
	     tref = b6_tree_walk(tref, B6_NEXT)) {
		struct node *node = b6_cast_of(tref, struct node, tref);
		printf("%d ", node->val);
	}
	puts("");

	for (tref = b6_tree_last(&tree); tref != b6_tree_head(&tree);
	     tref = b6_tree_walk(tref, B6_PREV)) {
		struct node *node = b6_cast_of(tref, struct node, tref);
		printf("%d ", node->val);
	}
	puts("");

	return 0;
}

int main(int argc, const char *argv[])
{
	int retval;

	(void)do_tree_test;

	retval = do_walk_test();
	if (retval < 0)
		goto bail_out;

	for (;;) {
		retval = do_tree_test(&b6_tree_avl_ops);
		if (retval < 0) {
			printf("avl: %d\n", retval);
			break;
		}
		retval = do_tree_test(&b6_tree_rb_ops);
		if (retval < 0) {
			printf("r-b: %d\n", retval);
			break;
		}
	}

bail_out:
	return retval;
}
