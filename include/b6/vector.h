/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_VECTOR_H_
#define B6_VECTOR_H_

#include "assert.h"
#include "allocator.h"

struct b6_vector {
	const struct b6_vector_ops *ops;
	struct b6_allocator *allocator;
	unsigned long int itemsize;
	unsigned long int capacity; /* mutable */
	unsigned long int length;
	char *buffer; /* mutable */
};

struct b6_vector_ops {
	void (*move)(struct b6_vector*, void*, const void*, unsigned long int);
};

static inline void b6_vector_initialize(struct b6_vector *self,
					const struct b6_vector_ops *ops,
					struct b6_allocator *allocator,
					unsigned long int itemsize)
{
	b6_precond(self);
	b6_precond(ops);
	b6_precond(allocator);
	b6_precond(itemsize);

	self->ops = ops;
	self->allocator = allocator;
	self->itemsize = itemsize;
	self->capacity = 0;
	self->length = 0;
	self->buffer = 0;
}

static inline void b6_vector_finalize(struct b6_vector *self)
{
	b6_deallocate(self->allocator, self->buffer);
}

static inline int b6_vector_length(const struct b6_vector *self)
{
	b6_precond(self);

	return self->length;
}

static inline const void *b6_vector_get_const(const struct b6_vector *self,
					      unsigned long int index,
					      unsigned long int n)
{
	unsigned long long int offset;

	b6_precond(self);
	b6_precond(index < self->length);

	if (n > self->length - index)
		return NULL;

	offset = (unsigned long long int)self->itemsize * index;
	if (offset > ~0UL)
		return NULL;

	return &self->buffer[offset];
}

static inline void *b6_vector_get(struct b6_vector *self,
				  unsigned long int index,
				  unsigned long int n)
{
	return (void*)b6_vector_get_const(self, index, n);
}

extern void *__b6_vector_add(struct b6_vector *self, unsigned long int index,
			     unsigned long int n);

static inline void *b6_vector_add(struct b6_vector *self,
				  unsigned long int index,
				  unsigned long int n)
{
	b6_precond(self);
	return __b6_vector_add(self, index, n);
}

extern unsigned long int __b6_vector_del(struct b6_vector *self,
					 unsigned long int index,
					 unsigned long int n);

static inline unsigned long int b6_vector_del(struct b6_vector *self,
					      unsigned long int index,
					      unsigned long int n)
{
	b6_precond(self);
	return __b6_vector_del(self, index, n);
}

static inline void *b6_vector_add_first(struct b6_vector *self,
					unsigned long int n)
{
	return b6_vector_add(self, 0, n);
}

static inline void *b6_vector_add_last(struct b6_vector *self,
				       unsigned long int n)
{
	return b6_vector_add(self, self->length, n);
}

static inline int b6_vector_del_first(struct b6_vector *self,
				      unsigned long int n)
{
	return b6_vector_del(self, 0, n);
}

static inline int b6_vector_del_last(struct b6_vector *self,
				     unsigned long int n)
{
	return b6_vector_del(self, self->length > n ? self->length - n : 0, n);
}

#endif /* B6_VECTOR_H_ */
