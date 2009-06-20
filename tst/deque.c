#include "b6/refs.h"
#include "test.h"

static int always_fails()
{
	return 0;
}

static int static_init()
{
	B6_DEQUE_DEFINE(deque);

	return b6_deque_empty(&deque) && deque.last == &deque.head;
}

static int runtime_init()
{
	struct b6_deque deque;

	b6_deque_initialize(&deque);

	return b6_deque_empty(&deque) && deque.last == &deque.head;
}

static int first_is_tail_when_empty()
{
	B6_DEQUE_DEFINE(deque);
	return b6_deque_first(&deque) == &deque.tail;
}

static int last_is_head_when_empty()
{
	B6_DEQUE_DEFINE(deque);
	return b6_deque_last(&deque) == &deque.head;
}

static int add_null_after()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_add_after(&deque, &sref, NULL);
	}

	return retval;
}

static int add_after_null()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_add_after(&deque, NULL, &sref);
	}

	return retval;
}

static int add_before_head()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_add(&deque, &deque.head, &sref);
	}

	return retval;
}

static int add_after_tail()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_add_after(&deque, &deque.tail, &sref);
	}

	return retval;
}

static int add_after()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;

	if (&sref != b6_deque_add_after(&deque, &deque.head, &sref))
		return 0;

	if (b6_deque_empty(&deque))
		return 0;

	return 1;
}

static int add_after_last()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;

	if (&sref != b6_deque_add_after(&deque, deque.last, &sref))
		return 0;

	if (&sref != b6_deque_last(&deque))
		return 0;

	if (b6_deque_empty(&deque))
		return 0;

	return 1;
}

static int add()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;

	if (&sref != b6_deque_add(&deque, &deque.tail, &sref))
		return 0;

	if (b6_deque_empty(&deque))
		return 0;

	return 1;
}

static int add_first()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;

	if (&sref != b6_deque_add_first(&deque, &sref))
		return 0;

	if (&sref != b6_deque_first(&deque))
		return 0;

	if (b6_deque_empty(&deque))
		return 0;

	return 1;
}

static int add_last()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;

	if (&sref != b6_deque_add_last(&deque, &sref))
		return 0;

	if (&sref != b6_deque_last(&deque))
		return 0;

	if (b6_deque_empty(&deque))
		return 0;

	return 1;
}

static int del_after_tail()
{
	B6_DEQUE_DEFINE(deque);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_del_after(&deque, &deque.tail);
	}

	return retval;
}

static int del_after_last()
{
	B6_DEQUE_DEFINE(deque);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_del_after(&deque, deque.last);
	}

	return retval;
}

static int del_after_null()
{
	B6_DEQUE_DEFINE(deque);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_del_after(&deque, NULL);
	}

	return retval;
}

static int del_head()
{
	B6_DEQUE_DEFINE(deque);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_del(&deque, &deque.head);
	}

	return retval;
}

static int del_tail()
{
	B6_DEQUE_DEFINE(deque);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_del(&deque, &deque.tail);
	}

	return retval;
}

static int del_first_when_empty()
{
	B6_DEQUE_DEFINE(deque);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_del_first(&deque);
	}

	return retval;
}

static int del_last_when_empty()
{
	B6_DEQUE_DEFINE(deque);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_deque_del_last(&deque);
	}

	return retval;
}

static int del_after()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref[3];
	int i;

	for (i = 0; i < b6_card_of(sref); i += 1)
		if (&sref[i] != b6_deque_add_last(&deque, &sref[i]))
			return 0;

	for (i = 0; i < b6_card_of(sref); i += 1) {
		if (&sref[b6_card_of(sref) - 1] != b6_deque_last(&deque))
			return 0;

		if (&sref[i] != b6_deque_del_after(&deque, &deque.head))
			return 0;
	}

	if (&deque.head != b6_deque_last(&deque))
		return 0;

	if (!b6_deque_empty(&deque))
		return 0;

	return 1;
}

static int del()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;

	if (&sref != b6_deque_add(&deque, &deque.tail, &sref))
		return 0;

	if (&sref != b6_deque_del(&deque, &sref))
		return 0;

	if (!b6_deque_empty(&deque))
		return 0;

	return 1;
}

static int walk_on_bounds()
{
	B6_DEQUE_DEFINE(deque);

	if (b6_deque_walk(&deque, &deque.head, B6_NEXT) != &deque.tail)
		return 0;

	if (b6_deque_walk(&deque, &deque.head, B6_PREV) != NULL)
		return 0;

	if (b6_deque_walk(&deque, &deque.tail, B6_NEXT) != NULL)
		return 0;

	if (b6_deque_walk(&deque, &deque.tail, B6_PREV) != &deque.head)
		return 0;

	return 1;
}

static int walk()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref[16];
	struct b6_sref *iter;
	int i;

	for (i = 0; i < b6_card_of(sref); i += 1)
		if (&sref[i] != b6_deque_add_last(&deque, &sref[i]))
			return 0;

	for (i = 0, iter = b6_deque_first(&deque); iter != &deque.tail;
	     iter = b6_deque_walk(&deque, iter, B6_NEXT), i += 1)
		if (iter != &sref[i])
			return 0;

	if (i != b6_card_of(sref))
		return 0;

	for (i = 0, iter = b6_deque_last(&deque); iter != &deque.head;
	     iter = b6_deque_walk(&deque, iter, B6_PREV), i += 1)
		if (iter != &sref[b6_card_of(sref) - 1 - i])
			return 0;

	if (i != b6_card_of(sref))
		return 0;

	return 1;
}

static int del_last()
{
	B6_DEQUE_DEFINE(deque);
	struct b6_sref sref;

	if (&sref != b6_deque_add(&deque, &deque.tail, &sref))
		return 0;

	if (&sref != b6_deque_last(&deque))
		return 0;

	if (&sref != b6_deque_del_last(&deque))
		return 0;

	if (!b6_deque_empty(&deque))
		return 0;

	return 1;
}

static int find_todo()
{
	return 0;
}

static int find_before_todo()
{
	return 0;
}

/*
 * generate with:
 egrep "^static int.*()" deque.c | sed -e 's/(/,/g' -e 's/static int /\ttest(/g' -e 's/$/;/g'
 */

int main(int argc, const char *argv[])
{
	test_init();

	test_exec(always_fails,);
	test_exec(static_init,);
	test_exec(runtime_init,);
	test_exec(first_is_tail_when_empty,);
	test_exec(last_is_head_when_empty,);
	test_exec(add_null_after,);
	test_exec(add_after_null,);
	test_exec(add_before_head,);
	test_exec(add_after_tail,);
	test_exec(add_after,);
	test_exec(add_after_last,);
	test_exec(add,);
	test_exec(add_first,);
	test_exec(add_last,);
	test_exec(del_after_tail,);
	test_exec(del_after_last,);
	test_exec(del_after_null,);
	test_exec(del_head,);
	test_exec(del_tail,);
	test_exec(del_first_when_empty,);
	test_exec(del_last_when_empty,);
	test_exec(del_after,);
	test_exec(del,);
	test_exec(walk_on_bounds,);
	test_exec(walk,);
	test_exec(del_last,);
	test_exec(find_todo,);
	test_exec(find_before_todo,);

	test_exit();

	return 0;
}
