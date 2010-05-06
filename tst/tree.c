#include "node.h"

#include "b6/deque.h"
#include "b6/tree.h"

#include <stdio.h>
#include <stdlib.h>

static int tree_cmp(const void *ref1, const void *ref2)
{
	struct node *p1 = b6_container_of(ref1, struct node, tref);
	struct node *p2 = b6_container_of(ref2, struct node, tref);
	return node_cmp(p1, p2);
}

/*
static int deque_cmp(const void *sref, void *arg)
{
	struct node *p1 = b6_container_of(sref, struct node, sref);
	struct node *p2 = (struct node*)arg;
	return p1->val < p2->val;
}
*/

static void b6_tree_debug_helper(struct b6_tree *tree, struct b6_tref *ref)
{
	struct b6_tref *head, *tail;

	b6_precond(tree != NULL);
	b6_precond(ref != NULL);

	head = &tree->head;
	tail = &tree->tail;

	printf("<node id='%u'", (unsigned) ref);

	if (ref == head)
		printf(" label='HEAD'");
	else if (ref == tail)
		printf(" label='TAIL'");
	else {
		struct node *t = b6_container_of(ref, struct node, tref);
		printf(" label='%d'", t->val);
	}

	printf(" balance='%d'>", ref->balance);

	if (ref->ref[0])
		b6_tree_debug_helper(tree, ref->ref[0]);
	else
		printf("<node id='%u' label='NULL' balance='-1' />",
		       ((unsigned) ref) + 1);

	if (ref->ref[1])
		b6_tree_debug_helper(tree, ref->ref[1]);
	else
		printf("<node id='%u' label='NULL' balance='-1' />",
		       ((unsigned) ref) + 3);

	printf("</node>");

}

void b6_tree_debug(struct b6_tree *tree)
{
	printf("<tree>");
	b6_tree_debug_helper(tree, tree->root.ref[0]);
	printf("</tree>");
}

static int do_tree_test(const struct b6_tree_ops *ops)
{
	int retval;
	struct node nodes[256];
	struct b6_tree tree;
	struct b6_deque deque;
	unsigned u;

	b6_tree_initialize(&tree, tree_cmp, ops);
	b6_deque_initialize(&deque);

	u = rand() % b6_card_of(nodes);
	while (u--) {
		struct node *node;
		struct b6_tref *tref;

		node = &nodes[u];
		node->val = rand() & 65535;

		tref = b6_tree_add(&tree, &node->tref);
		if (tref == &node->tref) {
			b6_deque_add_last(&deque, &node->sref);
			retval = b6_tree_check(&tree, &tref);
			if (retval < 0)
				goto bail_out;
		}
	}

	while (b6_deque_walk(&deque, &deque.head, B6_NEXT) != &deque.tail) {
		struct node *node;
		struct b6_sref *sref;
		struct b6_tref *tref;

		sref = b6_deque_del_after(&deque, &deque.head);
		node = b6_container_of(sref, struct node, sref);
		tref = b6_tree_del(&tree, &node->tref);
		b6_assert(tref == &node->tref);
		retval = b6_tree_check(&tree, &tref);
		if (retval < 0)
			goto bail_out;
	}

	retval = 0;
bail_out:
	return retval;
}

int main(int argc, const char *argv[])
{
	int retval;

	for (;;) {
		retval = do_tree_test(&b6_avl_tree);
		if (retval < 0) {
			printf("avl: %d\n", retval);
			break;
		}
		retval = do_tree_test(&b6_rb_tree);
		if (retval < 0) {
			printf("r-b: %d\n", retval);
			break;
		}
	}

	return retval;
}
