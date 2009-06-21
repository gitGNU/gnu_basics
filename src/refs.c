/*
 * Copyright (c) 2009, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/refs.h"

#define RED 0
#define BLACK 1

static int default_compare(const void *a, const void *b)
{
	return b6_sign_of((int)a - (int)b);
}

static int is_direction(int direction)
{
	return direction == B6_PREV || direction == B6_NEXT;
}

static int to_direction(int weight)
{
	int direction;

	b6_precond(weight == -1 || weight == 1);

	direction = (1 - weight) >> 1;

	b6_postcond(direction == ((weight == -1) ? B6_PREV : B6_NEXT));

	return direction;
}

static int to_opposite(int direction)
{
	int opposite;

	b6_precond(is_direction(direction));

	opposite = direction ^ 1;

	b6_static_assert((B6_PREV ^ 1) == B6_NEXT);
	b6_static_assert(B6_PREV == (B6_NEXT ^ 1));

	return opposite;
}

static int to_weight(int direction)
{
	int weight;

	b6_precond(is_direction(direction));

	weight = (-direction << 1) + 1;

	b6_postcond(weight == (direction == B6_PREV ? -1 : 1));

	return weight;
}

static int has_child(const struct b6_tref *ref, int dir)
{
	b6_precond(ref != NULL);
	b6_precond(is_direction(dir));

	return ref->ref[dir] != NULL;
}

static void rotate(struct b6_tref *r, int dir, int opp)
{
	struct b6_tref *p, *q;

	b6_precond(r != NULL);
	b6_precond(is_direction(opp));
	p = r->ref[opp];

	b6_precond(to_opposite(opp) == dir);
	b6_precond(p != NULL);
	q = p->ref[dir];

	if (has_child(p, dir)) {
		q->top = r;
		q->dir = opp;
	}
	r->ref[opp] = q;
	p->top = r->top;
	r->top->ref[r->dir] = p;
	r->top = p;
	p->ref[dir] = r;
	p->dir = r->dir;
	r->dir = dir;
}

/*
 * AVL Tree Rebalancing
 * --------------------
 *
 * Nodes are written lower case, followed by their balance under parentheses.
 * Trees are written upper case, followed by their height under brackets.
 * Note that the height of the whole tree does not change in case SR2 only.
 *
 * A] single rotations (here, right), that is, when balance of p is not right:
 *
 * SR1:     r(-2)              p(0)
 * ----     /   \             /   \
 *      p(-1)   C[h] ==> A[h+1]   r(0)
 *      /   \                     /  \
 * A[h+1]   B[h]               B[h]  C[h]
 *
 * SR2:     r(-2)              p(1)
 * ----     /   \             /   \
 *      p(0)    C[h] ==> A[h+1]   r(-1)
 *      /  \                      /  \
 * A[h+1]  B[h+1]            B[h+1]  C[h]
 *
 *
 * B] double rotations (here, left on p then right on r):
 *
 * DR1:     r(-2)               r                __q(0)__
 * ----     /   \              / \              /         \
 *       p(1)    C[h] ==>     q   C ==>     p(0)          r(0)
 *       /  \                / \           /   \         /   \
 *    A[h] q(0)             p  D        A[h]   B[h]   D[h]   C[h]
 *         /  \            / \
 *      B[h]  D[h]        A   B
 *
 * DR2:     r(-2)               r                __q(0)__
 * ----     /   \              / \              /        \
 *       p(1)    C[h] ==>     q   C ==>     p(-1)         r(0)
 *       /  \                / \           /    \        /   \
 *    A[h] q(1)             p   D       A[h]    B[h-1] D[h]   C[h]
 *         /  \            / \
 *    B[h-1]  D[h]        A   B
 *
 * DR3:     r(-2)               r                __q(0)__
 * ----     /   \              / \              /        \
 *       p(1)    C[h] ==>     q   C ==>     p(0)          r(1)
 *       /  \                / \           /   \          /  \
 *    A[h]  q(-1)           p   D       A[h]   B[h]  D[h-1]  C[h]
 *          /  \           / \
 *       B[h]  D[h-1]     A   B
 */
static int rebalance_avl(struct b6_tref *r)
{
	int opp, dir, weight, change;
	struct b6_tref *p;

	b6_precond(r != NULL);
	b6_precond(r->balance == -2 || r->balance == 2);

	opp = to_direction(r->balance >> 1);
	dir = to_opposite(opp);
	weight = to_weight(dir);

	b6_precond(has_child(r, opp));
	p = r->ref[opp];

	b6_precond(p != NULL);
	change = p->balance;

	if (change == weight) {
		struct b6_tref *q;

		b6_precond(has_child(p, dir));
		q = p->ref[dir];

		r->balance = -(((q->balance - weight) >> 1) & q->balance);
		p->balance = -(((q->balance + weight) >> 1) & q->balance);
		b6_assert(r->balance == ((q->balance == -weight) ? weight : 0));
		b6_assert(p->balance == ((q->balance == weight) ? -weight : 0));

		q->balance = 0;

		rotate(r->ref[opp], opp, dir);
	} else {
		p->balance += weight;
		r->balance = -p->balance;
	}

	rotate(r, dir, opp);

	return change;
}

/*
 * AVL Tree Insertion
 * ------------------
 *
 * After the node has been inserted, the tree is traveled upwards to adjust
 * possible balances issues. The operation continues until a subtree which
 * height did not change is found. This occur when subtree becomes even after
 * the insertion or when it has to be re-balanced as it will restore its
 * previous height.
 */
static void fix_avl_insert(struct b6_tree *tree, struct b6_tref *ref)
{
	b6_precond(tree != NULL);
	b6_precond(ref != NULL);

	ref->balance = 0;

	for (;;) {
		int balance, weight;

		weight = to_weight(ref->dir);
		ref = ref->top;
		if (ref == &tree->root)
			break;

		balance = ref->balance;
		ref->balance += weight;
		if (ref->balance == 0)
			break;

		if (balance) {
			rebalance_avl(ref);
			break;
		}
	}
}

/*
 * AVL Tree Removal
 * ----------------
 *
 * The tree is traveled upwards to adjust balances until the height of a
 * subtree does not change. This occur either when the subtree was even before
 * the removal or when it had to be re-balanced and that operation did not
 * change its height.
 */
static void fix_avl_remove(struct b6_tree *tree, struct b6_tref *ref, int dir,
                           struct b6_tref *old)
{
	b6_precond(tree != NULL);
	b6_precond(ref != NULL);
	b6_precond(is_direction(dir));

	do {
		struct b6_tref *top;
		int balance, weight;

		balance = ref->balance;
		weight = to_weight(dir);
		ref->balance -= weight;
		dir = ref->dir;
		top = ref->top;

		if (balance == 0)
			break;

		if (ref->balance != 0 && !rebalance_avl(ref))
			break;

		ref = top;
	} while (ref != &tree->root);
}

/*
 * AVL Tree Verification
 * ---------------------
 *
 * The tree is traveled in depth first. For each node we check that left
 * height and right height do not differ of more than 1.
 */
static int verify_avl(const struct b6_tree *tree, struct b6_tref *ref,
                      struct b6_tref **subtree)
{
	int h1, h2;

	b6_precond(tree != NULL);
	b6_precond(ref != NULL);
	b6_precond(subtree != NULL);

	if (has_child(ref, B6_PREV)) {
		h1 = verify_avl(tree, ref->ref[B6_PREV], subtree);
		if (h1 < 0)
			return h1;
	} else
		h1 = 0;

	if (has_child(ref, B6_NEXT)) {
		h2 = verify_avl(tree, ref->ref[B6_NEXT], subtree);
		if (h2 < 0)
			return h2;
	} else
		h2 = 0;

	if (h1 > h2)
		if (h1 - h2 > 1) {
			*subtree = ref;
			return -1;
		} else
			return 1 + h1;
	else
		if (h2 - h1 > 1) {
			*subtree = ref;
			return -1;
		} else
			return 1 + h2;
}

/*
 * Red Black Tree Insertion
 * ------------------------
 *
 * This function fixes colors after a node has been inserted in a red-black
 * tree. The recently inserted node is painted red. Then, color are fixed
 * backwards so as not break red-black tree rules. The loop begins at step #2.
 *
 * 1] If the node is the root of the tree, it is colored black and the
 * operation is over as it adds a black node to every path of the tree.
 *
 * 2] The operation is over as well if the parent of the node is black as
 * rules remain satisfied.
 *
 * 3] The node as a red parent, which induces that it also has a grandparent
 * as the root of tree is black. If the node has a red uncle, then the parent
 * and the uncle are turned black while the grandparent is turned red. This
 * does not violate the 4th rule. The operation has to go on with the
 * grandparent as it could still break the other rules.
 *
 * 4] Now, either the node has no uncle or its uncle is painted black, like a
 * Rolling Stone. If the parent is a left (resp. right) child and the node a
 * right (resp. left), then a left (resp. right) rotation will invert their
 * roles, as follows:
 *
 *        _g[b]__                         __g[b]_
 *       /       \                       /       \
 *    p[r]        u[b]     ==>        n[r]        u[b]
 *    /  \        /  \                /  \        /  \
 * A[b]  n[r]  D[?]  E[?]          p[r]  C[b]  D[?]  E[?]
 *       /  \                      /  \
 *    B[b]  C[b]                A[b]  B[b]
 *
 * The rotation is safe as both the node and its parent are red and properties
 * are not violated in any sub-tree.
 *
 * 5] The former parent has to be dealt with, however, as it breaks the 3rd
 * rule. Now, we know that parent and node are either both left (resp. right)
 * children. Thus, a right (resp. left) rotation rooted at the child of the
 * grand parent (e.g. the parent or the former node according to the previous
 * configuration) will make the former grandparent a child of the parent.
 * Then, switching colors of both nodes will restore the rules and the
 * operation is complete.
 *
 *           __g[b]_                      _p[r->b]_
 *          /       \                    /         \
 *       p[r]        u[b]             n[r]          g[b->r]
 *       /  \        /  \             /  \          /     \
 *    n[r]  C[b]  D[?]  E[?]  ==>  A[b]  B[b]    C[b]     u[b]
 *    /  \                                                /  \
 * A[b]  B[b]                                          D[?]  E[?]
 */
static void fix_rb_insert(struct b6_tree *tree, struct b6_tref *ref)
{
	struct b6_tref *top;

	b6_precond(ref != NULL);
	top = ref->top;

	b6_precond(tree != NULL);
	if (top == &tree->root) {
		ref->balance = BLACK;
		return;
	}

	ref->balance = RED;

	while (top->balance == RED) {
		int direction, opposite;
		struct b6_tref *elder;

		elder = top->top;
		direction = top->dir;
		opposite = to_opposite(direction);

		if (has_child(elder, opposite)) {
			struct b6_tref *uncle;
			uncle = elder->ref[opposite];
			if (uncle->balance == RED) {
				top->balance = BLACK;
				uncle->balance = BLACK;
				elder->balance = RED;
				ref = elder;
				top = ref->top;

				if (top != &tree->root)
					continue;

				ref->balance = BLACK;
				break;
			}
		}

		if (top->ref[direction] != ref) {
			struct b6_tref *tmp;

			b6_assert(ref->dir != direction);
			rotate(top, direction, opposite);
			tmp = top;
			top = ref;
			ref = tmp;
		}
		top->balance = BLACK;
		elder->balance = RED;

		rotate(elder, opposite, direction);
		break;
	}
}

/*
 * Red Black Tree Removal
 * ----------------------
 *
 * This function fixes colors after a node has been removed from a red-black
 * tree.
 *
 * If the removed node was red, then we are done as no black path has changed
 * at all.
 *
 * Now, we know the removed node was black. It was replaced by one of its
 * children. If this child is red, then, we repaint it black to restore black
 * height.
 *
 * Otherwise, the child is black. If its sibling is red, it is possible to
 * rotate the sub-tree so that it becomes the parent of the parent. Colors
 * have to be exchanged between both of them to avoid breaking rules once
 * more. This insures the sibling is black for the remainder of the
 * discussion. Here, we based the algorithm that it can be proven that a
 * sibling always exists in that case.
 *
 * If both sibling's children are black (or if the sibling is a leaf), then
 * repainting the sibling red will make all paths across it contain the same
 * number of black node as those passing through the node. Now, the parent
 * itself has to be examined as it still breaks the rules since all paths
 * through it have one fewer black node. If it is red, then we repaint it
 * black to add the missing black node in the path and we are done.
 *
 * If sibling's children have different colors, we insure that the sibling has
 * a red child in the same direction as the node. If not, a rotation in the
 * opposite direction rooted at sibling will do it. Colors are exchanged
 * between the former and the new sibling. As a result, number of black nodes
 * are kept unchanged.
 *
 * Now, a rotation rooted at the parent in the direction of the child will
 * make the sibling the parent of the former parent. The new parent gets the
 * former parent's color. Painting its children black will restore red/black
 * properties. Paths passing through the removed node have one additional
 * black node. Other paths either pass through its sibling or its uncle. The
 * first ones are ok since parent and sibling colors have not changed. Thoses
 * passing through the new uncle, have got one new black node as one node was
 * changed from red to black, balancing the node they lost during the
 * rotation.
 */
static void fix_rb_remove(struct b6_tree *tree, struct b6_tref *top, int dir,
                          struct b6_tref *old)
{
	b6_precond(tree != NULL);
	b6_precond(top != NULL);
	b6_precond(is_direction(dir));
	b6_precond(old != NULL);
	/*b6_precond(dir == old->dir);*/

	if (old->balance == RED)
		return;

	if (has_child(top, dir) && top->ref[dir]->balance == RED) {
		top->ref[dir]->balance = BLACK;
		return;
	}

	do {
		int opp;
		struct b6_tref *sibling;
		unsigned char color[2];

		opp = to_opposite(dir);
		sibling = top->ref[opp];
		b6_assert(has_child(top, opp));

		if (sibling->balance == RED) {
			top->balance = RED;
			sibling->balance = BLACK;
			rotate(top, dir, opp);
			sibling = top->ref[opp];
			b6_assert(has_child(top, opp));
		}

		color[B6_PREV] = has_child(sibling, B6_PREV) ?
			sibling->ref[B6_PREV]->balance : BLACK;
		color[B6_NEXT] = has_child(sibling, B6_NEXT) ?
			sibling->ref[B6_NEXT]->balance : BLACK;
		b6_assert(sibling->balance != RED);

		if (color[B6_PREV] == RED || color[B6_NEXT] == RED) {
			if (color[opp] != RED) {
				sibling->ref[dir]->balance = BLACK;
				sibling->balance = RED;
				rotate(sibling, opp, dir);
				sibling = top->ref[opp];
				b6_assert(has_child(top, opp));
			}
			sibling->balance = top->balance;
			top->balance = BLACK;
			sibling->ref[opp]->balance = BLACK;
			rotate(top, dir, opp);
			break;
		}

		sibling->balance = RED;
		if (top->balance == RED) {
			top->balance = BLACK;
			break;
		}
		dir = top->dir;
		top = top->top;
	} while (top != &tree->root);
}

/*
 * Red-Black Tree Verification
 * ---------------------------
 *
 * The tree is traveled in depth first to verify the following rules:
 *
 * 1] A node is either red or black.
 * 2] The root is black.
 * 3] Both children of every red node are black.
 * 4] Every simple path from a node to a descendant leaf contains the same
 *    number of black nodes.
 * 5] Leaves are all considered black nodes.
 */
static int verify_rb(const struct b6_tree *tree, struct b6_tref *ref,
                     struct b6_tref **subtree)
{
	int h;

	b6_precond(tree != NULL);
	b6_precond(ref != NULL);
	b6_precond(subtree != NULL);

	if (ref->top == &tree->root && ref->balance == RED) {
		*subtree = ref;
		return -1;
	}

	if (has_child(ref, B6_PREV)) {
		h = verify_rb(tree, ref->ref[B6_PREV], subtree);
		if (h < 0)
			return h;

		if (has_child(ref, B6_NEXT)) {
			int tmp;
			tmp = verify_rb(tree, ref->ref[B6_NEXT],
					subtree);
			if (tmp < 0)
				return tmp;

			if (tmp != h) {
				*subtree = ref;
				return -1;
			}
		}
	} else if (has_child(ref, B6_NEXT)) {
		h = verify_rb(tree, ref->ref[B6_NEXT], subtree);
		if (h < 0)
			return h;
	} else
		h = 0;

	if (ref->balance != RED)
		return 1 + h;
	else if ((!has_child(ref, B6_PREV) ||
		  ref->ref[B6_PREV]->balance != RED) &&
		 (!has_child(ref, B6_NEXT) ||
		  ref->ref[B6_NEXT]->balance != RED))
		return h;
	else {
		*subtree = ref;
		return -2;
	}
}

const struct b6_tree_ops b6_avl_tree = {
	fix_avl_insert,
	fix_avl_remove,
	verify_avl,
};

const struct b6_tree_ops b6_rb_tree = {
	fix_rb_insert,
	fix_rb_remove,
	verify_rb,
};

void b6_tree_initialize(struct b6_tree *tree, b6_ref_compare_t comp,
                        const struct b6_tree_ops *ops)
{
	struct b6_tref *head, *tail, *root;

	head = &tree->head;
	tail = &tree->tail;
	root = &tree->root;

	head->top = root;
	head->ref[B6_PREV] = NULL;
	head->ref[B6_NEXT] = tail;
	head->dir = B6_PREV;
	head->balance = to_weight(B6_NEXT);
	b6_static_assert(BLACK == 1);

	tail->top = head;
	tail->ref[B6_PREV] = NULL;
	tail->ref[B6_NEXT] = NULL;
	tail->dir = B6_NEXT;
	tail->balance = 0; /* red or even */
	b6_static_assert(RED == 0);

	root->top = NULL;
	root->ref[B6_PREV] = head;
	root->ref[B6_NEXT] = NULL;
	root->dir = 0; /* useless */
	root->balance = 0;

	tree->comp = comp != NULL ? comp : default_compare;
	tree->ops = ops;
}

struct b6_tref *b6_tree_search(const struct b6_tree *tree, struct b6_tref **top,
			       int *dir, b6_ref_examine_t cmp, void *arg)
{
	b6_precond(tree != NULL);
	b6_precond(top != NULL);
	b6_precond(dir != NULL);

	*top = (struct b6_tref*)&tree->root;
	*dir = B6_PREV;

	do {
		struct b6_tref *ref;

		ref = (*top)->ref[*dir];

		if (b6_unlikely(ref == &tree->head))
			*dir = B6_NEXT;
		else if (b6_unlikely(ref == &tree->tail))
			*dir = B6_PREV;
		else {
			int result;

			result = cmp(ref, arg);
			if (b6_unlikely(result == 0))
				return ref;

			*dir = to_direction(-result);
		}

		*top = ref;
	} while (has_child(*top, *dir));

	return NULL;
}

struct b6_tref *b6_tree_insert(struct b6_tree *tree, struct b6_tref *top,
                               int dir, struct b6_tref *ref)
{
	int opp;

	b6_precond(tree != NULL);
	b6_precond(top != NULL);
	b6_precond(is_direction(dir));
	b6_precond(!has_child(top, dir));

	opp = to_opposite(dir);

	ref->top = top;
	ref->ref[dir] = NULL;
	ref->ref[opp] = NULL;
	ref->dir = dir;

	top->ref[dir] = ref;

	tree->ops->add(tree, ref);

	return ref;
}

struct b6_tref *b6_tree_del(struct b6_tree *tree, struct b6_tref *ref)
{
	struct b6_tref *top;
	int dir;

	b6_precond(tree != NULL);
	b6_precond(ref != NULL);

	dir = ref->dir;
	top = ref->top;

	if (!has_child(ref, B6_PREV)) {
		if (has_child(ref, B6_NEXT)) {
			struct b6_tref *tmp;

			tmp = ref->ref[B6_NEXT];
			tmp->dir = dir;
			tmp->top = top;
			top->ref[dir] = tmp;
		} else
			top->ref[dir] = NULL;
		tree->ops->del(tree, top, dir, ref);
	} else if (!has_child(ref, B6_NEXT)) {
		struct b6_tref *tmp;

		tmp = ref->ref[B6_PREV];
		tmp->dir = dir;
		tmp->top = top;
		top->ref[dir] = tmp;
		tree->ops->del(tree, top, dir, ref);
	} else {
		struct b6_tref *aux, *tmp;
		int direction, opposite, balance;

		direction = (ref->balance + 1) >> 1;
		b6_assert(direction == (ref->balance == to_weight(B6_NEXT)) ?
			  B6_PREV : B6_NEXT);
		opposite = to_opposite(direction);

		aux = ref->ref[opposite];
		if (has_child(aux, direction)) {
			do
				aux = aux->ref[direction];
			while (has_child(aux, direction));
			tmp = aux->top;

			tmp->ref[direction] = aux->ref[opposite];
			if (has_child(tmp, direction)) {
				tmp->ref[direction]->top = tmp;
				tmp->ref[direction]->dir = direction;
			}

			top->ref[dir] = aux;
			aux->top = top;
			aux->ref[opposite] = ref->ref[opposite];
			aux->ref[direction] = ref->ref[direction];
			aux->ref[opposite]->top = aux;
			aux->ref[direction]->top = aux;
			aux->dir = dir;
			balance = aux->balance;
			aux->balance = ref->balance;
			ref->balance = balance;

			/*ref->dir = direction; */ /* FIXME */
			tree->ops->del(tree, tmp, direction, ref);
		} else {
			top->ref[dir] = aux;
			aux->top = top;
			aux->dir = dir;
			aux->ref[direction] = ref->ref[direction];
			aux->ref[direction]->top = aux;
			balance = aux->balance;
			aux->balance = ref->balance;
			ref->balance = balance;

			/*ref->dir = opposite; */ /* FIXME */
			tree->ops->del(tree, aux, opposite, ref);
		}
	}

	return ref;
}

/*
 * AVL/Red-Black Tree Traveling
 * ----------------------------
 *
 * If the node has a child in the traveling direction, the node to find It is
 * the latest node in the opposite direction of the subtree of the
 * child. Otherwise, the next node is the first elder when walking the tree
 * backwards to the root, which child is in the opposite direction.
 */
struct b6_tref *b6_tree_walk(const struct b6_tree *tree,
                             const struct b6_tref *ref, int dir)
{
	b6_precond(tree != NULL);
	b6_precond(ref != NULL);
	b6_precond(is_direction(dir));

	if (has_child(ref, dir)) {
		int opp;

		opp = to_opposite(dir);
		ref = ref->ref[dir];
		while (has_child(ref, opp))
			ref = ref->ref[opp];
	} else {
		while (ref->dir == dir && ref != &tree->root)
			ref = ref->top;
		ref = ref->top;
	}

	return (struct b6_tref*)ref;
}



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
	splay->comp = comp != NULL ? comp : default_compare;
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
