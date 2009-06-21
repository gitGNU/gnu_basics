#include "node.h"

#include "b6/utils.h"

#include <stdio.h>
#include <stdlib.h>

static int splay_cmp(const void *ref1, const void *ref2)
{
	struct node *p1 = b6_container_of(ref1, struct node, dref);
	struct node *p2 = b6_container_of(ref2, struct node, dref);
	return node_cmp(p1, p2);
}

int main(int argc, const char *argv[])
{
	int retval;
	struct node nodes[16];
	struct b6_splay splay;
	struct b6_dref *ref;
	unsigned u;

	b6_splay_initialize(&splay, splay_cmp);

	u = b6_card_of(nodes);
	while (u--) {
		struct node *node;

		node = &nodes[u];
		node->val = u & 1 ? b6_card_of(nodes) - u : u;

		b6_splay_add(&splay, &node->dref);
	}

	b6_splay_search(&splay, NULL, &nodes[8].dref);
	b6_splay_del(&splay);

	ref = b6_splay_walk(&splay, &splay.head, B6_NEXT);
	while (ref != &splay.tail) {
		struct node *node;

		node = b6_container_of(ref, struct node, dref);
		printf("%u ", node->val);
		fflush(stdout);
		ref = b6_splay_walk(&splay, ref, B6_NEXT);
	}
	puts("");

	ref = b6_splay_walk(&splay, &splay.tail, B6_PREV);
	while (ref != &splay.head) {
		struct node *node;

		node = b6_container_of(ref, struct node, dref);
		printf("%u ", node->val);
		fflush(stdout);
		ref = b6_splay_walk(&splay, ref, B6_PREV);
	}
	puts("");

	return retval;
}
