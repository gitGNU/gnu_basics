#ifndef REFS_H_
#define REFS_H_

#include "b6/refs.h"
#include "b6/utils.h"
#include "b6/assert.h"

static inline int is_direction(int direction)
{
	return direction == B6_PREV || direction == B6_NEXT;
}

static inline int to_direction(int weight)
{
	int direction;

	b6_precond(weight == -1 || weight == 1);

	direction = (1 - weight) >> 1;

	b6_postcond(direction == ((weight == -1) ? B6_PREV : B6_NEXT));

	return direction;
}

static inline int to_opposite(int direction)
{
	int opposite;

	b6_precond(is_direction(direction));

	opposite = direction ^ 1;

	b6_static_assert((B6_PREV ^ 1) == B6_NEXT);
	b6_static_assert(B6_PREV == (B6_NEXT ^ 1));

	return opposite;
}

#endif /* REFS_H_ */
